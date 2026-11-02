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

StyleEditorDialog::StyleEditorDialog(const RenderStyle &style, const Molecule *molecule,
                                     int selectedAtom, QWidget *parent)
    : QDialog(parent), m_style(style), m_molecule(molecule), m_selectedAtom(selectedAtom) {
    setWindowTitle(QStringLiteral("표시 스타일 편집기"));
    resize(720, 520);
    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget;
    tabs->addTab(makePresetTab(), QStringLiteral("프리셋"));
    tabs->addTab(makeAtomTab(), QStringLiteral("원자"));
    tabs->addTab(makeBondTab(), QStringLiteral("결합"));
    tabs->addTab(makeCommonTab(), QStringLiteral("재질/조명"));
    tabs->addTab(makeScreenTab(), QStringLiteral("화면"));
    tabs->addTab(makeIsosurfaceTab(), QStringLiteral("등가면(Isosurface)"));
    tabs->addTab(makeSimpleTab(QStringLiteral("원자·결합 라벨의 내용과 글꼴을 설정합니다.")), QStringLiteral("원자/결합 라벨"));
    tabs->addTab(makeSimpleTab(QStringLiteral("거리·각도·이면각 표기의 단위와 정밀도를 설정합니다.")), QStringLiteral("구조 파라미터"));
    auto *vectorTab = new QWidget;
    auto *vectorForm = new QFormLayout(vectorTab);
    m_vectorScale = new QDoubleSpinBox; m_vectorScale->setRange(0.05, 20); m_vectorScale->setValue(m_style.vectorScale);
    m_vectorWidth = new QDoubleSpinBox; m_vectorWidth->setRange(0.5, 12); m_vectorWidth->setValue(m_style.vectorWidth);
    m_vectorThreshold = new QDoubleSpinBox; m_vectorThreshold->setRange(0, 1); m_vectorThreshold->setDecimals(4); m_vectorThreshold->setValue(m_style.vectorThreshold);
    vectorForm->addRow(QStringLiteral("벡터 크기"), m_vectorScale);
    vectorForm->addRow(QStringLiteral("벡터 두께"), m_vectorWidth);
    vectorForm->addRow(QStringLiteral("숨김 threshold"), m_vectorThreshold);
    tabs->addTab(vectorTab, QStringLiteral("벡터"));
    tabs->addTab(makeSimpleTab(QStringLiteral("2D 구조 표시의 선 굵기와 라벨 규칙을 설정합니다.")), QStringLiteral("2D 스타일"));
    layout->addWidget(tabs);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    auto *ok = new QPushButton(QStringLiteral("확인"));
    auto *applyButton = new QPushButton(QStringLiteral("적용"));
    auto *saveButton = new QPushButton(QStringLiteral("저장"));
    auto *cancel = new QPushButton(QStringLiteral("취소"));
    buttons->addWidget(ok); buttons->addWidget(applyButton); buttons->addWidget(saveButton); buttons->addWidget(cancel);
    layout->addLayout(buttons);
    connect(ok, &QPushButton::clicked, this, [this]() { apply(); accept(); });
    connect(applyButton, &QPushButton::clicked, this, &StyleEditorDialog::apply);
    connect(saveButton, &QPushButton::clicked, this, &StyleEditorDialog::save);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    for (QDoubleSpinBox *spin : findChildren<QDoubleSpinBox *>())
        connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] { apply(); });
    for (QSlider *slider : findChildren<QSlider *>())
        connect(slider, &QSlider::valueChanged, this, [this] { apply(); });
    for (QCheckBox *check : findChildren<QCheckBox *>())
        connect(check, &QCheckBox::toggled, this, [this] { apply(); });
    for (QComboBox *combo : findChildren<QComboBox *>())
        if (combo != m_presetCombo)
            connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { apply(); });
}

QWidget *StyleEditorDialog::makePresetTab() {
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    m_presetCombo = new QComboBox(page);
    m_presetCombo->setObjectName(QStringLiteral("StylePreset"));
    m_presetCombo->addItems(StyleManager::presetNames());
    const QString canonical = StyleManager::canonicalPresetName(m_style.name);
    m_presetCombo->setCurrentText(m_presetCombo->findText(canonical) >= 0 ? canonical : QStringLiteral("사용자 지정"));
    layout->addRow(QStringLiteral("스타일"), m_presetCombo);
    connect(m_presetCombo, &QComboBox::currentTextChanged, this, [this](const QString &name) {
        if (name == QStringLiteral("사용자 지정")) m_style.name = name;
        else m_style = StyleManager::preset(name);
        writeControls();
        emit styleApplied(m_style);
    });
    return page;
}

void StyleEditorDialog::readControls() {
    if (m_lighting) m_style.lighting = m_lighting->isChecked();
    if (m_twoLights) m_style.twoLights = m_twoLights->isChecked();
    if (m_showBondOrders) m_style.showBondOrders = m_showBondOrders->isChecked();
    if (m_cameraRelativeLights) m_style.cameraRelativeLights = m_cameraRelativeLights->isChecked();
    if (m_shadows) m_style.shadows = m_shadows->isChecked();
    if (m_monochrome) m_style.monochrome = m_monochrome->isChecked();
    if (m_depthCueing) m_style.depthCueing = m_depthCueing->isChecked();
    if (m_grid) m_style.gridOnAtoms = m_grid->isChecked();
    if (m_fixedWidth) m_style.fixedBorderWidth = m_fixedWidth->isChecked();
    if (m_atomScale) m_style.atomScale = float(m_atomScale->value());
    if (m_spaceFillingScale) m_style.spaceFillingScale = float(m_spaceFillingScale->value());
    if (m_bondRadius) m_style.bondRadius = float(m_bondRadius->value());
    if (m_labelSize) m_style.labelSize = float(m_labelSize->value());
    if (m_opacity) m_style.opacity = float(m_opacity->value());
    if (m_lightX) m_style.lightDirection = QVector3D(float(m_lightX->value()), float(m_lightY->value()), float(m_lightZ->value()));
    if (m_secondLightX) m_style.secondLightDirection = QVector3D(float(m_secondLightX->value()), float(m_secondLightY->value()), float(m_secondLightZ->value()));
    if (m_ambient) m_style.ambientIntensity = float(m_ambient->value());
    if (m_diffuse) m_style.diffuseIntensity = float(m_diffuse->value());
    if (m_specular) m_style.specularIntensity = float(m_specular->value());
    if (m_saturation) m_style.colorSaturation = float(m_saturation->value());
    if (m_shininess) m_style.shininess = float(m_shininess->value());
    if (m_shadowStrength) m_style.shadowStrength = float(m_shadowStrength->value());
    if (m_depthCueStrength) m_style.depthCueStrength = float(m_depthCueStrength->value());
    if (m_antiAliasing) m_style.antiAliasing = m_antiAliasing->isChecked();
    if (m_bondSpacing) m_style.doubleBondSpacing = float(m_bondSpacing->value());
    if (m_bondOpacity) m_style.bondOpacity = float(m_bondOpacity->value());
    if (m_dashLength) m_style.dashLength = float(m_dashLength->value());
    if (m_dashGap) m_style.dashGap = float(m_dashGap->value());
    if (m_vectorScale) m_style.vectorScale = float(m_vectorScale->value());
    if (m_vectorWidth) m_style.vectorWidth = float(m_vectorWidth->value());
    if (m_vectorThreshold) m_style.vectorThreshold = float(m_vectorThreshold->value());
    if (m_perspectiveFov) m_style.perspectiveFov = float(m_perspectiveFov->value());
    if (m_perspective) m_style.perspective = m_perspective->isChecked();
    if (m_bondColorMode) m_style.bondColorMode = BondColorMode(m_bondColorMode->currentData().toInt());
    if (m_showPositiveSurface) m_style.showPositiveSurface = m_showPositiveSurface->isChecked();
    if (m_showNegativeSurface) m_style.showNegativeSurface = m_showNegativeSurface->isChecked();
    if (m_isoValue) m_style.isoValue = float(m_isoValue->value());
    if (m_isoOpacity) m_style.isoOpacity = float(m_isoOpacity->value());
}

void StyleEditorDialog::writeControls() {
    const auto children = findChildren<QObject *>();
    QVector<bool> previous;
    previous.reserve(children.size());
    for (QObject *child : children) previous.push_back(child->blockSignals(true));
    const auto spin = [](QDoubleSpinBox *widget, float value) {
        if (widget) widget->setValue(value);
    };
    spin(m_atomScale, m_style.atomScale);
    spin(m_spaceFillingScale, m_style.spaceFillingScale);
    const bool spaceFilling = m_style.displayMode == DisplayMode::SpaceFilling;
    if (m_spaceFillingScale) m_spaceFillingScale->setEnabled(spaceFilling);
    if (m_atomScale) m_atomScale->setEnabled(!spaceFilling);
    if (m_atomScaleSlider) m_atomScaleSlider->setEnabled(!spaceFilling);
    spin(m_bondRadius, m_style.bondRadius);
    spin(m_labelSize, m_style.labelSize);
    spin(m_opacity, m_style.opacity);
    spin(m_bondSpacing, m_style.doubleBondSpacing);
    spin(m_bondOpacity, m_style.bondOpacity);
    spin(m_dashLength, m_style.dashLength);
    spin(m_dashGap, m_style.dashGap);
    spin(m_ambient, m_style.ambientIntensity);
    spin(m_diffuse, m_style.diffuseIntensity);
    spin(m_specular, m_style.specularIntensity);
    spin(m_saturation, m_style.colorSaturation);
    spin(m_shininess, m_style.shininess);
    spin(m_shadowStrength, m_style.shadowStrength);
    spin(m_depthCueStrength, m_style.depthCueStrength);
    spin(m_lightX, m_style.lightDirection.x());
    spin(m_lightY, m_style.lightDirection.y());
    spin(m_lightZ, m_style.lightDirection.z());
    spin(m_secondLightX, m_style.secondLightDirection.x());
    spin(m_secondLightY, m_style.secondLightDirection.y());
    spin(m_secondLightZ, m_style.secondLightDirection.z());
    spin(m_perspectiveFov, m_style.perspectiveFov);
    spin(m_vectorScale, m_style.vectorScale);
    spin(m_vectorWidth, m_style.vectorWidth);
    spin(m_vectorThreshold, m_style.vectorThreshold);
    spin(m_isoValue, m_style.isoValue);
    spin(m_isoOpacity, m_style.isoOpacity);
    if (m_atomScaleSlider) m_atomScaleSlider->setValue(qRound(m_style.atomScale * 100));
    if (m_bondRadiusSlider) m_bondRadiusSlider->setValue(qRound(m_style.bondRadius * 100));
    if (m_shininessSlider) m_shininessSlider->setValue(qRound(m_style.shininess));
    if (m_shadowStrengthSlider) m_shadowStrengthSlider->setValue(qRound(m_style.shadowStrength * 100));
    if (m_elementRadiusSlider && m_element && m_element->currentIndex() >= 0)
        m_elementRadiusSlider->setValue(qRound(m_style.elementRadiusScales.value(
            m_element->currentData().toInt(), 1.0f) * 100));
    if (m_elementRadiusValue && m_elementRadiusSlider)
        m_elementRadiusValue->setText(QStringLiteral("%1%").arg(m_elementRadiusSlider->value()));
    if (m_bondColorMode) m_bondColorMode->setCurrentIndex(
        m_bondColorMode->findData(int(m_style.bondColorMode)));
    if (m_lighting) m_lighting->setChecked(m_style.lighting);
    if (m_twoLights) m_twoLights->setChecked(m_style.twoLights);
    if (m_showBondOrders) m_showBondOrders->setChecked(m_style.showBondOrders);
    if (m_cameraRelativeLights) m_cameraRelativeLights->setChecked(m_style.cameraRelativeLights);
    if (m_shadows) m_shadows->setChecked(m_style.shadows);
    if (m_monochrome) m_monochrome->setChecked(m_style.monochrome);
    if (m_depthCueing) m_depthCueing->setChecked(m_style.depthCueing);
    if (m_grid) m_grid->setChecked(m_style.gridOnAtoms);
    if (m_fixedWidth) m_fixedWidth->setChecked(m_style.fixedBorderWidth);
    if (m_antiAliasing) m_antiAliasing->setChecked(m_style.antiAliasing);
    if (m_perspective) m_perspective->setChecked(m_style.perspective);
    if (m_showPositiveSurface) m_showPositiveSurface->setChecked(m_style.showPositiveSurface);
    if (m_showNegativeSurface) m_showNegativeSurface->setChecked(m_style.showNegativeSurface);
    if (m_backgroundButton) m_backgroundButton->setStyleSheet(
        QStringLiteral("background: %1").arg(m_style.background.name()));
    if (m_bondColorButton) m_bondColorButton->setStyleSheet(
        QStringLiteral("background: %1").arg(m_style.bondColor.name()));
    refreshElementColorButton();
    for (int i = 0; i < children.size(); ++i) children[i]->blockSignals(previous[i]);
}

void StyleEditorDialog::apply() {
    readControls();
    emit styleApplied(m_style);
}

void StyleEditorDialog::save() {
    readControls();
    StyleManager::save(m_style);
    emit styleApplied(m_style);
    emit styleSaved(m_style);
}

} // namespace youngcraft
