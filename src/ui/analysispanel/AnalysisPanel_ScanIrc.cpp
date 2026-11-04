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
    openGraph->setObjectName(QStringLiteral("OpenScanGraph"));
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
    m_scanList->installEventFilter(this);
    connect(m_scanList, &QListWidget::currentRowChanged, this, [this](int row) { if (row >= 0) { setCurrentScanPoint(row); emit scanPointRequested(row); } });
    return page;
}

QWidget *AnalysisPanel::makeIrcTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_ircList = new QListWidget;
    layout->addWidget(m_ircList, 1);
    m_ircPlot = new PlotWidget;
    layout->addWidget(m_ircPlot, 3);
    m_ircList->installEventFilter(this);
    m_ircPlot->installEventFilter(this);
    connect(m_ircList, &QListWidget::currentRowChanged, this, [this](int row) { if (row >= 0) emit ircPointRequested(row); });
    connect(m_ircPlot, &PlotWidget::pointClicked, this, &AnalysisPanel::ircPointRequested);
    return page;
}

void AnalysisPanel::fillScanPlot() {
    const QSignalBlocker blocker(m_scanList);
    m_scanList->clear();
    for (int i = 0; i < m_data.scanPoints.size(); ++i) {
        const ScanPoint &point = m_data.scanPoints[i];
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
    if (m_scanResults) m_scanResults->setData(m_data, m_scanList->currentRow());
}

void AnalysisPanel::setCurrentScanPoint(int index) {
    if (index < 0 || index >= m_data.scanPoints.size()) return;
    const QSignalBlocker blocker(m_scanList);
    m_scanList->setCurrentRow(index);
    if (m_scanResults) m_scanResults->setSelectedPoint(index);
}

void AnalysisPanel::fillIrcPlot() {
    m_ircList->clear();
    QVector<QPointF> series;
    for (const IRCPoint &point : m_data.ircPoints) {
        m_ircList->addItem(QStringLiteral("점 %1  |  %2  |  ΔE %3 kcal/mol")
                               .arg(point.index).arg(point.direction).arg(numberOrDash(point.relativeKcal, 3)));
        series.push_back({point.coordinate, point.relativeKcal});
    }
    m_ircPlot->setSeries(series, QStringLiteral("Reaction coordinate"), QStringLiteral("ΔE (kcal mol⁻¹)"));
    m_ircPlot->setMarkers(series);
}

} // namespace youngcraft
