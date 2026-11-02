#include "ui/AnalysisPanel.h"
#include "ui/ScanResultsDialog.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"

#include <QDialog>
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

QWidget *AnalysisPanel::makeGeometryTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_openOptimizationGraph = new QPushButton(QStringLiteral("최적화 그래프..."),page);
    m_openOptimizationGraph->setObjectName("OpenOptimizationGraph");
    layout->addWidget(m_openOptimizationGraph);
    m_optimizationResults=new QDialog(this);m_optimizationResults->setObjectName("OptimizationResultsDialog");
    m_optimizationResults->setWindowTitle(QStringLiteral("최적화 에너지 그래프"));m_optimizationResults->resize(900,600);
    auto *graphLayout=new QVBoxLayout(m_optimizationResults);
    m_framePlot = new PlotWidget(m_optimizationResults);
    graphLayout->addWidget(m_framePlot);
    connect(m_openOptimizationGraph,&QPushButton::clicked,this,[this]{m_optimizationResults->show();m_optimizationResults->raise();m_optimizationResults->activateWindow();});
    m_framePlot->setObjectName(QStringLiteral("OptimizationEnergyPlot"));
    m_framePlot->setEmptyMessage(QString());
    m_framePlot->hide();

    m_frameList = new QListWidget;
    m_frameList->setObjectName(QStringLiteral("FrameList"));
    layout->addWidget(m_frameList, 1);
    auto *right = new QVBoxLayout;
    m_frameInfo = new QLabel;
    m_frameInfo->setWordWrap(true);
    right->addWidget(m_frameInfo);
    m_frameSlider = new QSlider(Qt::Horizontal);
    m_frameSlider->setObjectName(QStringLiteral("FrameSlider"));
    m_frameSlider->setMinimum(0);
    m_frameSlider->setMaximum(0);
    right->addWidget(m_frameSlider);
    auto *buttons = new QHBoxLayout;
    auto *play = new QPushButton(QStringLiteral("재생 / 일시정지"));
    auto *last = new QPushButton(QStringLiteral("최종 구조"));
    buttons->addWidget(play); buttons->addWidget(last);
    right->addLayout(buttons);
    layout->addLayout(right);
    connect(m_frameList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) { setCurrentFrame(row); emit frameRequested(row); }
    });
    connect(m_frameSlider, &QSlider::valueChanged, this, [this](int value) {
        if (value >= 0 && value < m_frameList->count()) { setCurrentFrame(value); emit frameRequested(value); }
    });
    connect(play, &QPushButton::clicked, this, &AnalysisPanel::framePlayRequested);
    connect(last, &QPushButton::clicked, this, [this]() {
        if (m_frameList->count()) { const int frame = m_frameList->count() - 1; setCurrentFrame(frame); emit frameRequested(frame); }
    });
    connect(m_framePlot, &PlotWidget::pointClicked, this, [this](int point) {
        if (point >= 0 && point < m_energyFrames.size()) {
            const int frame = m_energyFrames[point]; setCurrentFrame(frame); emit frameRequested(frame);
        }
    });
    return page;
}

void AnalysisPanel::fillFrameList() {
    const QSignalBlocker listBlocker(m_frameList), sliderBlocker(m_frameSlider);
    m_frameList->clear();
    m_energyFrames.clear();
    QVector<QPointF> series;
    double minimum = std::numeric_limits<double>::infinity();
    for (const auto &frame : m_data.frames)
        if (std::isfinite(frame.metrics.energyHartree)) minimum = std::min(minimum, frame.metrics.energyHartree);
    m_frameSlider->setMaximum(qMax(0, m_data.frames.size() - 1));
    for (int i = 0; i < m_data.frames.size(); ++i) {
        const FrameMetrics &m = m_data.frames[i].metrics;
        if (std::isfinite(m.energyHartree)) {
            m_energyFrames.push_back(i);
            series.push_back({double(i + 1), (m.energyHartree - minimum) * 627.509474});
        }
        m_frameList->addItem(QStringLiteral("프레임 %1  |  step %2  |  %3")
                                 .arg(i + 1).arg(m.step).arg(AnalysisEngine::formatEnergy(m.energyHartree)));
    }
    m_framePlot->setSeries(series, QStringLiteral("Optimization step"), QStringLiteral("ΔE (kcal mol⁻¹)"));
    m_openOptimizationGraph->setEnabled(!series.isEmpty());
    if(series.isEmpty())m_optimizationResults->hide();
    m_framePlot->setVisible(!series.isEmpty());
    m_framePlot->setMarkers(series);
    m_frameInfo->clear();
    if (!m_data.frames.isEmpty()) setCurrentFrame(m_data.frames.size() - 1);
}

void AnalysisPanel::setCurrentFrame(int index) {
    if(m_scanResults)m_scanResults->setSelectedFrame(index);
    if (!m_frameList || index < 0 || index >= m_frameList->count()) return;
    const QSignalBlocker listBlocker(m_frameList), sliderBlocker(m_frameSlider);
    m_frameList->setCurrentRow(index);
    m_frameSlider->setValue(index);
    int point = m_energyFrames.indexOf(index);
    if (point < 0 && index == m_data.finalFrameIndex() && !m_energyFrames.isEmpty()) point = m_energyFrames.size() - 1;
    m_framePlot->setSelectedPoint(point);
    m_framePlot->setToolTip(point >= 0 ? QStringLiteral("프레임 %1 | E = %2 Hartree")
        .arg(m_energyFrames[point] + 1).arg(m_data.frames[m_energyFrames[point]].metrics.energyHartree, 0, 'f', 9) : QString());
    const FrameMetrics &metrics = m_data.frames[index].metrics;
    m_frameInfo->setText(QStringLiteral("프레임 %1 / %2\n에너지: %3\n최대/RMS force: %4 / %5\n최대/RMS displacement: %6 / %7")
                             .arg(index + 1).arg(m_data.frames.size()).arg(AnalysisEngine::formatEnergy(metrics.energyHartree))
                             .arg(numberOrDash(metrics.maxForce, 6)).arg(numberOrDash(metrics.rmsForce, 6))
                             .arg(numberOrDash(metrics.maxDisplacement, 6)).arg(numberOrDash(metrics.rmsDisplacement, 6)));
}

} // namespace youngcraft
