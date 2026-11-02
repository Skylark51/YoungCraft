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

QWidget *StyleEditorDialog::makeAtomTab() {
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    m_atomScale = new QDoubleSpinBox; m_atomScale->setRange(0.03, 2.0); m_atomScale->setSingleStep(0.02); m_atomScale->setValue(m_style.atomScale);
    m_atomScaleSlider = new QSlider(Qt::Horizontal);
    m_atomScaleSlider->setObjectName(QStringLiteral("AtomScaleSlider"));
    m_atomScaleSlider->setRange(3, 200);
    m_atomScaleSlider->setValue(qRound(m_style.atomScale * 100.0f));
    auto *atomScaleControl = new QWidget;
    auto *atomScaleLayout = new QHBoxLayout(atomScaleControl); atomScaleLayout->setContentsMargins(0, 0, 0, 0);
    atomScaleLayout->addWidget(m_atomScaleSlider, 1); atomScaleLayout->addWidget(m_atomScale);
    connect(m_atomScaleSlider, &QSlider::valueChanged, this, [this](int value) {
        QSignalBlocker blocker(m_atomScale);
        m_atomScale->setValue(value / 100.0);
    });
    connect(m_atomScale, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        QSignalBlocker blocker(m_atomScaleSlider);
        m_atomScaleSlider->setValue(qRound(value * 100.0));
    });
    m_labelSize = new QDoubleSpinBox; m_labelSize->setRange(6, 40); m_labelSize->setValue(m_style.labelSize);
    layout->addRow(QStringLiteral("전체 원자 크기"), atomScaleControl);
    m_spaceFillingScale = new QDoubleSpinBox;
    m_spaceFillingScale->setObjectName(QStringLiteral("SpaceFillingScale"));
    m_spaceFillingScale->setRange(0.25, 2.0); m_spaceFillingScale->setSingleStep(0.05);
    m_spaceFillingScale->setValue(m_style.spaceFillingScale);
    m_spaceFillingScale->setToolTip(QStringLiteral("CPK / Space-Filling에서 1.0은 원소의 반데르발스 반지름입니다."));
    layout->addRow(QStringLiteral("공간 채움 크기 (vdW 배율)"), m_spaceFillingScale);
    const bool spaceFilling = m_style.displayMode == DisplayMode::SpaceFilling;
    m_spaceFillingScale->setEnabled(spaceFilling);
    m_atomScale->setEnabled(!spaceFilling); m_atomScaleSlider->setEnabled(!spaceFilling);
    layout->addRow(QStringLiteral("라벨 크기"), m_labelSize);
    m_element = new QComboBox;
    m_element->setObjectName(QStringLiteral("ElementSelector"));
    for (int number = 1; number <= 118; ++number)
        m_element->addItem(QStringLiteral("%1 (%2)").arg(elementSymbol(number)).arg(number), number);
    const int selectedElement = m_molecule && m_selectedAtom >= 0 && m_selectedAtom < m_molecule->atoms.size()
        ? m_molecule->atoms[m_selectedAtom].atomicNumber : 6;
    m_element->setCurrentIndex(m_element->findData(selectedElement));
    m_elementColorButton = new QPushButton(QStringLiteral("원소색 선택"));
    connect(m_elementColorButton, &QPushButton::clicked, this, &StyleEditorDialog::chooseElementColor);
    connect(m_element, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &StyleEditorDialog::refreshElementColorButton);
    refreshElementColorButton();
    layout->addRow(QStringLiteral("원소"), m_element);
    m_elementRadiusSlider = new QSlider(Qt::Horizontal);
    m_elementRadiusSlider->setObjectName(QStringLiteral("ElementRadiusSlider"));
    m_elementRadiusSlider->setRange(25, 300);
    m_elementRadiusSlider->setSingleStep(5);
    m_elementRadiusSlider->setPageStep(10);
    m_elementRadiusValue = new QLabel;
    auto *elementRadiusControl = new QWidget;
    auto *elementRadiusLayout = new QHBoxLayout(elementRadiusControl); elementRadiusLayout->setContentsMargins(0, 0, 0, 0);
    elementRadiusLayout->addWidget(m_elementRadiusSlider, 1); elementRadiusLayout->addWidget(m_elementRadiusValue);
    auto loadElementRadius = [this](int index) {
        const bool valid = m_element && index >= 0;
        m_elementRadiusSlider->setEnabled(valid);
        const int value = valid ? qRound(m_style.elementRadiusScales.value(m_element->itemData(index).toInt(), 1.0f) * 100.0f) : 100;
        QSignalBlocker blocker(m_elementRadiusSlider);
        m_elementRadiusSlider->setValue(value);
        m_elementRadiusValue->setText(QStringLiteral("%1%").arg(value));
    };
    connect(m_element, qOverload<int>(&QComboBox::currentIndexChanged), this, loadElementRadius);
    connect(m_elementRadiusSlider, &QSlider::valueChanged, this, [this](int value) {
        m_elementRadiusValue->setText(QStringLiteral("%1%").arg(value));
        if (m_element && m_element->currentIndex() >= 0)
            m_style.elementRadiusScales.insert(m_element->currentData().toInt(), value / 100.0f);
    });
    loadElementRadius(m_element->currentIndex());
    layout->addRow(QStringLiteral("선택 원소 크기"), elementRadiusControl);
    auto *resetElementRadius = new QPushButton(QStringLiteral("선택 원소 크기 100% 복원"));
    connect(resetElementRadius, &QPushButton::clicked, this, [this] {
        if (!m_element || m_element->currentIndex() < 0) return;
        m_style.elementRadiusScales.remove(m_element->currentData().toInt());
        m_elementRadiusSlider->setValue(100);
        m_elementRadiusValue->setText(QStringLiteral("100%"));
        apply();
    });
    layout->addRow(resetElementRadius);
    layout->addRow(m_elementColorButton);
    auto *resetElement = new QPushButton(QStringLiteral("선택 원소 기본값 복원"));
    connect(resetElement, &QPushButton::clicked, this, [this] {
        if (!m_element) return;
        const int number = m_element->currentData().toInt();
        m_style.elementColors.remove(number);
        m_style.elementRadiusScales.remove(number);
        m_elementRadiusSlider->setValue(100);
        m_elementRadiusValue->setText(QStringLiteral("100%"));
        refreshElementColorButton();
        apply();
    });
    layout->addRow(resetElement);
    auto *resetAll = new QPushButton(QStringLiteral("모든 원소 CPK 초기화"));
    connect(resetAll, &QPushButton::clicked, this, [this] {
        m_style.elementColors.clear();
        m_style.elementRadiusScales.clear();
        m_elementRadiusSlider->setValue(100);
        m_elementRadiusValue->setText(QStringLiteral("100%"));
        refreshElementColorButton();
        apply();
    });
    layout->addRow(resetAll);
    auto *selectedColor = new QPushButton(QStringLiteral("선택된 원자에만 색상 적용"));
    selectedColor->setEnabled(m_selectedAtom >= 0);
    connect(selectedColor, &QPushButton::clicked, this, [this] {
        if (m_selectedAtom < 0) return;
        const QColor initial = m_molecule && m_selectedAtom < m_molecule->atoms.size() && m_molecule->atoms[m_selectedAtom].colorOverride.isValid()
                                   ? m_molecule->atoms[m_selectedAtom].colorOverride : Qt::white;
        const QColor color = QColorDialog::getColor(initial, this, QStringLiteral("선택 원자 색"));
        if (color.isValid()) emit atomColorApplied(m_selectedAtom, color);
    });
    layout->addRow(selectedColor);
    return page;
}

void StyleEditorDialog::chooseElementColor() {
    if (!m_element || m_element->currentIndex() < 0) return;
    const int number = m_element->currentData().toInt();
    const QColor color = QColorDialog::getColor(m_style.elementColors.value(number, elementColor(number)), this,
                                                 QStringLiteral("원소 색"));
    if (color.isValid()) {
        m_style.elementColors.insert(number, color);
        refreshElementColorButton();
        apply();
    }
}

void StyleEditorDialog::refreshElementColorButton() {
    if (!m_element || !m_elementColorButton || m_element->currentIndex() < 0) return;
    const int number = m_element->currentData().toInt();
    const QColor color = m_style.elementColors.value(number, elementColor(number));
    m_elementColorButton->setStyleSheet(QStringLiteral("background: %1").arg(color.name()));
}

} // namespace youngcraft
