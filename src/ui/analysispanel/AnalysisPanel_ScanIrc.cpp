#include "ui/SidebarButton.h"
#include "ui/AnalysisPanel.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"
#include "ui/ScanResultsDialog.h"

#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

namespace youngcraft {

namespace {

QString numberOrDash(double value, int decimals = 4) {
    return std::isfinite(value) ? QString::number(value, 'f', decimals) : QStringLiteral("—");
}

QTableWidgetItem *item(const QString &text) { return new QTableWidgetItem(text); }

} // namespace

QWidget *AnalysisPanel::makeScanTab() {
    auto *page = new QWidget;
    auto *outer = new QVBoxLayout(page);
    m_scanStatus = new QLabel;
    m_scanStatus->setWordWrap(true);
    outer->addWidget(m_scanStatus);
    auto *openGraph = new QPushButton(QStringLiteral("스캔 그래프..."));
    makeSidebarIconButton(openGraph, UiIcon::Scan);
    openGraph->setObjectName(QStringLiteral("OpenScanGraph"));
    openGraph->setEnabled(false);
    outer->addWidget(openGraph);
    connect(openGraph, &QPushButton::clicked, this, [this] {
        if (!m_scanResults) {
            m_scanResults = new ScanResultsDialog(this);
            m_scanResults->setData(m_data, m_scanList->currentRow());
            connect(m_scanResults,&ScanResultsDialog::frameSelected,this,&AnalysisPanel::frameRequested);
            connect(m_scanResults, &ScanResultsDialog::pointSelected, this, [this](int index) {
                setCurrentScanPoint(index);
                emit scanPointRequested(index);
            });
        }
        m_scanResults->setSelectedPoint(m_scanList->currentRow());
        m_scanResults->show();
        m_scanResults->raise();
        m_scanResults->activateWindow();
    });
    auto *layout = new QVBoxLayout;
    outer->addLayout(layout);
    m_scanList = new QListWidget;
    m_scanList->setObjectName(QStringLiteral("ScanList"));
    layout->addWidget(m_scanList, 1);
    m_scanPlot = new PlotWidget(page);
    m_scanPlot->setObjectName(QStringLiteral("ScanEnergyPreview"));
    m_scanPlot->setMinimumHeight(150);
    m_scanPlot->setMaximumHeight(220);
    m_scanPlot->hide();
    m_scanPlot->setToolTip(QStringLiteral("스캔 지점을 선택하면 해당 구조를 표시합니다. 전체 그래프에서 최적화 이력과 내보내기를 확인할 수 있습니다."));
    layout->addWidget(m_scanPlot);
    m_scanList->installEventFilter(this);
    m_scanPlot->installEventFilter(this);
    connect(m_scanList, &QListWidget::currentRowChanged, this, [this](int row) { if (row >= 0) { setCurrentScanPoint(row); emit scanPointRequested(row); } });
    connect(m_scanPlot, &PlotWidget::pointClicked, this, [this](int row) {
        if (row >= 0 && row < m_data.scanPoints.size()) { setCurrentScanPoint(row); emit scanPointRequested(row); }
    });
    return page;
}

QWidget *AnalysisPanel::makeIrcTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_ircList = new QListWidget;
    m_ircList->setObjectName(QStringLiteral("IrcPointList"));
    layout->addWidget(m_ircList, 1);
    m_ircPlot = new PlotWidget;
    m_ircPlot->setObjectName(QStringLiteral("IrcEnergyPlot"));
    layout->addWidget(m_ircPlot, 3);
    m_ircList->installEventFilter(this);
    m_ircPlot->installEventFilter(this);
    connect(m_ircList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) { setCurrentIrcPoint(row); emit ircPointRequested(row); }
    });
    connect(m_ircPlot, &PlotWidget::pointClicked, this, [this](int row) {
        if (row >= 0 && row < m_data.ircPoints.size()) { setCurrentIrcPoint(row); emit ircPointRequested(row); }
    });
    return page;
}

void AnalysisPanel::fillScanPlot() {
    const QSignalBlocker blocker(m_scanList);
    m_scanList->clear();
    QVector<QPointF> series;
    QVector<PlotWidget::HeatmapPoint> heatmap;
    for (int i = 0; i < m_data.scanPoints.size(); ++i) {
        const ScanPoint &point = m_data.scanPoints[i];
        series.push_back({point.coordinate, point.relativeKcal});
        if (m_data.scanDimensions == 2)
            heatmap.push_back({point.coordinate, point.coordinate2, point.relativeKcal, i, point.energyHartree});
        if (m_data.format == QStringLiteral("GSM")) {
            m_scanList->addItem(QStringLiteral("노드 %1  |  ΔE %2 kcal/mol")
                                    .arg(point.index).arg(numberOrDash(point.relativeKcal, 2)));
        } else if (m_data.scanDimensions == 2) {
            m_scanList->addItem(QStringLiteral("점 %1  |  %2 / %3  |  ΔE %4 kcal/mol")
                                    .arg(point.index).arg(numberOrDash(point.coordinate, 2))
                                    .arg(numberOrDash(point.coordinate2, 2)).arg(numberOrDash(point.relativeKcal, 2)));
        } else {
            m_scanList->addItem(QStringLiteral("점 %1  |  좌표 %2  |  ΔE %3 kcal/mol")
                                    .arg(point.index).arg(numberOrDash(point.coordinate, 2)).arg(numberOrDash(point.relativeKcal, 2)));
        }
    }
    for (int i = 0; i < m_data.scanPoints.size(); ++i) {
        const auto &point = m_data.scanPoints[i];
        if (point.inProgress) m_scanList->item(i)->setText(m_scanList->item(i)->text() + QStringLiteral(" · 진행 중"));
        m_scanList->item(i)->setToolTip(m_data.format == QStringLiteral("GSM")
            ? QStringLiteral("ΔE = %1 kcal/mol").arg(numberOrDash(point.relativeKcal, 3))
            : QStringLiteral("E = %1 Hartree").arg(point.energyHartree, 0, 'f', 2));
    }
    m_scanStatus->setText(m_data.scanStatusText());
    if (m_data.scanDimensions == 2)
        m_scanPlot->setHeatmapSeries(heatmap, m_data.scanAxisLabel, m_data.scanAxisLabel2, QStringLiteral("ΔE\n(kcal mol⁻¹)"));
    else
        m_scanPlot->setScanSeries(series, m_data.scanAxisLabel, QStringLiteral("ΔE (kcal mol⁻¹)"));
    m_scanPlot->setVisible(!series.isEmpty());
    if (auto *button = findChild<QPushButton*>(QStringLiteral("OpenScanGraph")))
        button->setEnabled(!series.isEmpty());
    if (m_scanResults) m_scanResults->setData(m_data, m_scanList->currentRow());
}

void AnalysisPanel::setCurrentScanPoint(int index) {
    if (index < 0 || index >= m_data.scanPoints.size()) return;
    const QSignalBlocker blocker(m_scanList);
    m_scanList->setCurrentRow(index);
    m_scanPlot->setSelectedPoint(index);
    if (m_scanResults) m_scanResults->setSelectedPoint(index);
}

void AnalysisPanel::setCurrentIrcPoint(int index) {
    if (!m_ircList || !m_ircPlot) return;
    if (index < -1 || index >= m_data.ircPoints.size()) return;
    const QSignalBlocker blocker(m_ircList);
    m_ircList->setCurrentRow(index);
    m_ircPlot->setSelectedPoint(index);
}

void AnalysisPanel::fillIrcPlot() {
    m_ircList->clear();
    QVector<QPointF> series;
    for (const IRCPoint &point : m_data.ircPoints) {
        m_ircList->addItem(QStringLiteral("점 %1  |  %2  |  ΔE %3 kcal/mol")
                               .arg(point.index).arg(point.direction).arg(numberOrDash(point.relativeKcal, 3)));
        if (point.frameIndex < 0 || point.frameIndex >= m_data.frames.size())
            m_ircList->item(m_ircList->count() - 1)->setText(m_ircList->item(m_ircList->count() - 1)->text() + QStringLiteral(" | 대응 구조 없음"));
        series.push_back({point.coordinate, point.relativeKcal});
    }
    m_ircPlot->setSeries(series, m_data.format == QStringLiteral("ORCA") ? QStringLiteral("IRC path step (relative to TS)") : QStringLiteral("Reaction coordinate"), QStringLiteral("ΔE (kcal mol⁻¹)"));
    m_ircPlot->setMarkers(series);
}

} // namespace youngcraft
