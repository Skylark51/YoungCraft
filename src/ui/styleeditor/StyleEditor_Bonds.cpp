#include "ui/StyleEditorDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace youngcraft {

QWidget *StyleEditorDialog::makeBondTab() {
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    m_showBondOrders = new QCheckBox(QStringLiteral("다중 결합 차수 표시"));
    m_showBondOrders->setObjectName(QStringLiteral("ShowBondOrders"));
    m_showBondOrders->setChecked(m_style.showBondOrders);
    layout->addRow(m_showBondOrders);
    m_bondRadius = new QDoubleSpinBox; m_bondRadius->setRange(0.01, 0.8); m_bondRadius->setSingleStep(0.01); m_bondRadius->setValue(m_style.bondRadius);
    m_bondRadiusSlider = new QSlider(Qt::Horizontal);
    m_bondRadiusSlider->setObjectName(QStringLiteral("BondRadiusSlider"));
    m_bondRadiusSlider->setRange(1, 80);
    m_bondRadiusSlider->setValue(qRound(m_style.bondRadius * 100.0f));
    auto *bondRadiusControl = new QWidget;
    auto *bondRadiusLayout = new QHBoxLayout(bondRadiusControl); bondRadiusLayout->setContentsMargins(0, 0, 0, 0);
    bondRadiusLayout->addWidget(m_bondRadiusSlider, 1); bondRadiusLayout->addWidget(m_bondRadius);
    connect(m_bondRadiusSlider, &QSlider::valueChanged, this, [this](int value) {
        QSignalBlocker blocker(m_bondRadius);
        m_bondRadius->setValue(value / 100.0);
    });
    connect(m_bondRadius, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        QSignalBlocker blocker(m_bondRadiusSlider);
        m_bondRadiusSlider->setValue(qRound(value * 100.0));
    });
    m_opacity = new QDoubleSpinBox; m_opacity->setRange(0.1, 1.0); m_opacity->setSingleStep(0.05); m_opacity->setValue(m_style.opacity);
    m_bondSpacing = new QDoubleSpinBox; m_bondSpacing->setRange(1.2, 6); m_bondSpacing->setValue(m_style.doubleBondSpacing);
    m_bondOpacity = new QDoubleSpinBox; m_bondOpacity->setRange(0.05, 1); m_bondOpacity->setValue(m_style.bondOpacity);
    m_dashLength = new QDoubleSpinBox; m_dashLength->setRange(0.02, 2); m_dashLength->setValue(m_style.dashLength);
    m_dashGap = new QDoubleSpinBox; m_dashGap->setRange(0.01, 2); m_dashGap->setValue(m_style.dashGap);
    m_bondColorMode = new QComboBox;
    m_bondColorMode->addItem(QStringLiteral("원자 색상 절반씩"), int(BondColorMode::EndpointAtoms));
    m_bondColorMode->addItem(QStringLiteral("단일 결합 색"), int(BondColorMode::Uniform));
    m_bondColorMode->addItem(QStringLiteral("원자 색상"), int(BondColorMode::MixedAtoms));
    m_bondColorMode->setCurrentIndex(m_bondColorMode->findData(int(m_style.bondColorMode)));
    layout->addRow(QStringLiteral("결합 굵기"), bondRadiusControl);
    layout->addRow(QStringLiteral("테두리 두께"), new QDoubleSpinBox);
    layout->addRow(QStringLiteral("불투명도"), m_opacity);
    layout->addRow(QStringLiteral("이중/삼중 결합 간격"), m_bondSpacing);
    layout->addRow(QStringLiteral("결합 불투명도"), m_bondOpacity);
    layout->addRow(QStringLiteral("점선 길이"), m_dashLength);
    layout->addRow(QStringLiteral("점선 간격"), m_dashGap);
    layout->addRow(QStringLiteral("결합 색상 방식"), m_bondColorMode);
    m_bondColorButton = new QPushButton(QStringLiteral("결합 색상"));
    m_bondColorButton->setStyleSheet(QStringLiteral("background: %1").arg(m_style.bondColor.name()));
    connect(m_bondColorButton, &QPushButton::clicked, this, &StyleEditorDialog::chooseBondColor);
    layout->addRow(QStringLiteral("단일 결합 색상"), m_bondColorButton);
    return page;
}

void StyleEditorDialog::chooseBondColor() {
    const QColor color = QColorDialog::getColor(m_style.bondColor, this, QStringLiteral("결합 색상"));
    if (!color.isValid()) return;
    m_style.bondColor = color;
    m_bondColorButton->setStyleSheet(QStringLiteral("background: %1").arg(color.name()));
    apply();
}

} // namespace youngcraft
