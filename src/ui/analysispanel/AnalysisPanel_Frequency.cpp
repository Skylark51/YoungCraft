#include "ui/AnalysisPanel.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"

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

QWidget *AnalysisPanel::makeFrequencyTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_frequencyList = new QListWidget;
    m_frequencyList->setObjectName(QStringLiteral("FrequencyList"));
    m_frequencyList->setMinimumWidth(0);
    m_frequencyList->setMinimumHeight(90);
    layout->addWidget(m_frequencyList, 1);
    auto *right = new QVBoxLayout;
    m_frequencyInfo = new QLabel(QStringLiteral("진동 모드를 선택하세요"));
    m_frequencyInfo->setWordWrap(true);
    right->addWidget(m_frequencyInfo);
    auto *form = new QFormLayout;
    m_amplitude = new QDoubleSpinBox;
    m_amplitude->setObjectName(QStringLiteral("FrequencyAmplitude"));
    m_amplitude->setRange(0.0, 3.0); m_amplitude->setSingleStep(0.05); m_amplitude->setValue(0.5);
    m_cycleTime = new QDoubleSpinBox;
    m_cycleTime->setObjectName(QStringLiteral("FrequencyCycleTime"));
    m_cycleTime->setRange(0.2, 20.0); m_cycleTime->setSingleStep(0.1); m_cycleTime->setValue(2.0);
    m_cycleTime->setSuffix(QStringLiteral(" 초"));
    form->addRow(QStringLiteral("진동 스케일"), m_amplitude);
    form->addRow(QStringLiteral("주기"), m_cycleTime);
    m_recountBonds = new QCheckBox(QStringLiteral("결합 다시 계산"));
    m_recountBonds->setChecked(true);
    m_vectors = new QCheckBox(QStringLiteral("변위 벡터 표시"));
    m_scaledVectors = new QCheckBox(QStringLiteral("스케일 벡터"));
    m_scaledVectors->setEnabled(false);
    form->addRow(m_recountBonds);
    form->addRow(m_vectors);
    form->addRow(m_scaledVectors);
    right->addLayout(form);
    auto *play = new QPushButton(QStringLiteral("진동 재생 / 일시정지"));
    right->addWidget(play);
    auto *restore = new QPushButton(QStringLiteral("원래 구조"));
    right->addWidget(restore);
    auto *copyAnimation = m_copyFrequencyAnimation = new QPushButton(QStringLiteral("TS 애니메이션 복사"));
    copyAnimation->hide();
    copyAnimation->setObjectName(QStringLiteral("CopyFrequencyAnimation"));
    copyAnimation->setToolTip(QStringLiteral("선택한 진동 모드를 투명 배경 GIF 애니메이션으로 복사합니다."));
    right->addWidget(copyAnimation);
    connect(copyAnimation, &QPushButton::clicked, this, &AnalysisPanel::animationCopyRequested);
    auto *saveAnimation = new QPushButton(QStringLiteral("애니메이션 저장"));
    right->addWidget(saveAnimation);
    m_frequencyPlot = new PlotWidget;
    right->addWidget(m_frequencyPlot, 1);
    layout->addLayout(right, 2);
    connect(m_frequencyList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        m_frequencySelectionChanged = true;
        emit frequencyRequested(row);
        QTimer::singleShot(0, this, [this] { m_frequencySelectionChanged = false; });
    });
    connect(m_frequencyList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!m_frequencySelectionChanged) emit frequencyRequested(m_frequencyList->row(item));
    });
    connect(play, &QPushButton::clicked, this, &AnalysisPanel::frequencyPlayRequested);
    connect(restore, &QPushButton::clicked, this, &AnalysisPanel::frequencyRestoreRequested);
    connect(m_amplitude, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnalysisPanel::frequencyAmplitudeChanged);
    connect(m_cycleTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnalysisPanel::frequencyCycleTimeChanged);
    connect(m_recountBonds, &QCheckBox::toggled, this, &AnalysisPanel::recountBondsChanged);
    const auto vectorChanged = [this] { emit vibrationVectorsChanged(m_vectors->isChecked(), m_scaledVectors->isChecked()); };
    connect(m_vectors, &QCheckBox::toggled, this, [this, vectorChanged](bool enabled) {
        m_scaledVectors->setEnabled(enabled);
        vectorChanged();
    });
    connect(m_scaledVectors, &QCheckBox::toggled, this, vectorChanged);
    connect(saveAnimation, &QPushButton::clicked, this, &AnalysisPanel::animationSaveRequested);
    return page;
}

double AnalysisPanel::frequencyAmplitude() const { return m_amplitude ? m_amplitude->value() : 0.5; }

double AnalysisPanel::frequencyCycleTime() const { return m_cycleTime ? m_cycleTime->value() : 2.0; }

bool AnalysisPanel::recountBondsEnabled() const { return m_recountBonds && m_recountBonds->isChecked(); }

bool AnalysisPanel::scaledVectorsEnabled() const { return m_scaledVectors && m_scaledVectors->isChecked(); }

void AnalysisPanel::fillFrequencyList() {
    bool hasImaginaryMode = false;
    for (const auto &mode : m_data.frequencies) if (mode.imaginary || mode.wavenumber < 0) hasImaginaryMode = true;
    m_copyFrequencyAnimation->setVisible(hasImaginaryMode);
    m_frequencyList->clear();
    QVector<QPointF> bars;
    for (const FrequencyMode &mode : m_data.frequencies) {
        m_frequencyList->addItem(QStringLiteral("%1: %2 cm⁻¹%3")
                                     .arg(mode.number).arg(numberOrDash(mode.wavenumber, 3))
                                     .arg(mode.imaginary ? QStringLiteral("  [허수]") : QString()));
        bars.push_back({double(mode.number), mode.wavenumber});
    }
    m_frequencyPlot->setSeries(bars, QStringLiteral("Normal mode"), QStringLiteral("Wavenumber (cm⁻¹)"));
    if (!m_data.frequencies.isEmpty()) setCurrentFrequency(0);
}

void AnalysisPanel::setCurrentFrequency(int index) {
    if (!m_frequencyList || index < 0 || index >= m_frequencyList->count()) return;
    const QSignalBlocker blocker(m_frequencyList);
    m_frequencyList->setCurrentRow(index);
    const FrequencyMode &mode = m_data.frequencies[index];
    m_frequencyInfo->setText(QStringLiteral("모드 %1\n파수: %2 cm⁻¹\nIR 세기: %3\nRaman 활성도: %4\n상태: %5")
                                 .arg(mode.number).arg(numberOrDash(mode.wavenumber, 4)).arg(numberOrDash(mode.irIntensity, 4))
                                 .arg(numberOrDash(mode.ramanActivity, 4)).arg(mode.imaginary ? QStringLiteral("허수 진동") : QStringLiteral("정상 진동")));
}

} // namespace youngcraft
