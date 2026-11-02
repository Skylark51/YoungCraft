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

QWidget *StyleEditorDialog::makeCommonTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *lightingGroup = new QGroupBox(QStringLiteral("조명"));
    auto *lightingLayout = new QFormLayout(lightingGroup);
    m_lighting = new QCheckBox(QStringLiteral("조명 켜기")); m_lighting->setChecked(m_style.lighting);
    m_twoLights = new QCheckBox(QStringLiteral("2개 조명 사용")); m_twoLights->setChecked(m_style.twoLights);
    m_cameraRelativeLights = new QCheckBox(QStringLiteral("카메라를 따라 조명 방향 유지"));
    m_cameraRelativeLights->setObjectName(QStringLiteral("CameraRelativeLights"));
    m_cameraRelativeLights->setChecked(m_style.cameraRelativeLights);
    m_lightX = new QDoubleSpinBox; m_lightY = new QDoubleSpinBox; m_lightZ = new QDoubleSpinBox;
    for (QDoubleSpinBox *spin : {m_lightX, m_lightY, m_lightZ}) { spin->setRange(-10, 10); spin->setSingleStep(0.1); }
    m_lightX->setValue(m_style.lightDirection.x()); m_lightY->setValue(m_style.lightDirection.y()); m_lightZ->setValue(m_style.lightDirection.z());
    m_secondLightX = new QDoubleSpinBox; m_secondLightY = new QDoubleSpinBox; m_secondLightZ = new QDoubleSpinBox;
    for (QDoubleSpinBox *spin : {m_secondLightX, m_secondLightY, m_secondLightZ}) { spin->setRange(-10, 10); spin->setSingleStep(0.1); }
    m_secondLightX->setValue(m_style.secondLightDirection.x()); m_secondLightY->setValue(m_style.secondLightDirection.y()); m_secondLightZ->setValue(m_style.secondLightDirection.z());
    m_ambient = new QDoubleSpinBox; m_diffuse = new QDoubleSpinBox; m_specular = new QDoubleSpinBox;
    for (QDoubleSpinBox *spin : {m_ambient, m_diffuse, m_specular}) { spin->setRange(0, 2); spin->setSingleStep(0.05); }
    m_ambient->setValue(m_style.ambientIntensity); m_diffuse->setValue(m_style.diffuseIntensity); m_specular->setValue(m_style.specularIntensity);
    m_saturation = new QDoubleSpinBox;
    m_saturation->setRange(0.0, 1.5); m_saturation->setSingleStep(0.05); m_saturation->setValue(m_style.colorSaturation);
    m_shininess = new QDoubleSpinBox; m_shininess->setRange(1, 256); m_shininess->setValue(m_style.shininess);
    m_shininessSlider = new QSlider(Qt::Horizontal); m_shininessSlider->setRange(1, 256); m_shininessSlider->setValue(qRound(m_style.shininess));
    auto *shininessControl = new QWidget; auto *shininessLayout = new QHBoxLayout(shininessControl); shininessLayout->setContentsMargins(0, 0, 0, 0);
    shininessLayout->addWidget(m_shininessSlider, 1); shininessLayout->addWidget(m_shininess);
    connect(m_shininessSlider, &QSlider::valueChanged, m_shininess, [this](int value) { QSignalBlocker blocker(m_shininess); m_shininess->setValue(value); });
    connect(m_shininess, qOverload<double>(&QDoubleSpinBox::valueChanged), m_shininessSlider, [this](double value) { QSignalBlocker blocker(m_shininessSlider); m_shininessSlider->setValue(qRound(value)); });
    m_lightColorButton = new QPushButton(QStringLiteral("주 조명 색"));
    m_secondLightColorButton = new QPushButton(QStringLiteral("보조 조명 색"));
    connect(m_lightColorButton, &QPushButton::clicked, this, &StyleEditorDialog::chooseLightColor);
    connect(m_secondLightColorButton, &QPushButton::clicked, this, &StyleEditorDialog::chooseSecondLightColor);
    lightingLayout->addRow(QStringLiteral("광택"), m_specular);
    lightingLayout->addRow(QStringLiteral("표면 부드러움"), shininessControl);
    lightingLayout->addRow(QStringLiteral("입체감"), m_diffuse);
    lightingLayout->addRow(QStringLiteral("색상 채도"), m_saturation);
    lightingLayout->addRow(m_lighting); lightingLayout->addRow(m_twoLights);
    lightingLayout->addRow(m_cameraRelativeLights);
    auto *direction = new QWidget; auto *directionLayout = new QHBoxLayout(direction); directionLayout->setContentsMargins(0, 0, 0, 0);
    directionLayout->addWidget(new QLabel(QStringLiteral("X"))); directionLayout->addWidget(m_lightX);
    directionLayout->addWidget(new QLabel(QStringLiteral("Y"))); directionLayout->addWidget(m_lightY);
    directionLayout->addWidget(new QLabel(QStringLiteral("Z"))); directionLayout->addWidget(m_lightZ);
    lightingLayout->addRow(QStringLiteral("주 조명 방향"), direction);
    auto *secondDirection = new QWidget; auto *secondLayout = new QHBoxLayout(secondDirection); secondLayout->setContentsMargins(0, 0, 0, 0);
    secondLayout->addWidget(m_secondLightX); secondLayout->addWidget(m_secondLightY); secondLayout->addWidget(m_secondLightZ);
    lightingLayout->addRow(QStringLiteral("보조 조명 방향"), secondDirection);
    lightingLayout->addRow(m_lightColorButton); lightingLayout->addRow(m_secondLightColorButton);
    lightingLayout->addRow(QStringLiteral("주변광 강도"), m_ambient);
    layout->addWidget(lightingGroup);
    layout->addStretch();
    return page;
}

QWidget *StyleEditorDialog::makeScreenTab() {
    auto *appearance = new QGroupBox(QStringLiteral("화면"));
    auto *appearanceLayout = new QFormLayout(appearance);
    m_backgroundButton = new QPushButton(QStringLiteral("배경색 선택"));
    m_backgroundButton->setStyleSheet(QStringLiteral("background: %1").arg(m_style.background.name()));
    connect(m_backgroundButton, &QPushButton::clicked, this, &StyleEditorDialog::chooseBackground);
    m_shadows = new QCheckBox(QStringLiteral("그림자")); m_shadows->setChecked(m_style.shadows);
    m_monochrome = new QCheckBox(QStringLiteral("흑백")); m_monochrome->setChecked(m_style.monochrome);
    m_depthCueing = new QCheckBox(QStringLiteral("깊이 단서(depth cueing)")); m_depthCueing->setChecked(m_style.depthCueing);
    m_shadowStrength = new QDoubleSpinBox; m_shadowStrength->setRange(0, 1); m_shadowStrength->setValue(m_style.shadowStrength);
    m_shadowStrengthSlider = new QSlider(Qt::Horizontal); m_shadowStrengthSlider->setRange(0, 100); m_shadowStrengthSlider->setValue(qRound(m_style.shadowStrength * 100.0f));
    auto *shadowControl = new QWidget; auto *shadowLayout = new QHBoxLayout(shadowControl); shadowLayout->setContentsMargins(0, 0, 0, 0);
    shadowLayout->addWidget(m_shadowStrengthSlider, 1); shadowLayout->addWidget(m_shadowStrength);
    connect(m_shadowStrengthSlider, &QSlider::valueChanged, m_shadowStrength, [this](int value) { QSignalBlocker blocker(m_shadowStrength); m_shadowStrength->setValue(value / 100.0); });
    connect(m_shadowStrength, qOverload<double>(&QDoubleSpinBox::valueChanged), m_shadowStrengthSlider, [this](double value) { QSignalBlocker blocker(m_shadowStrengthSlider); m_shadowStrengthSlider->setValue(qRound(value * 100.0)); });
    m_depthCueStrength = new QDoubleSpinBox; m_depthCueStrength->setRange(0, 1); m_depthCueStrength->setValue(m_style.depthCueStrength);
    m_antiAliasing = new QCheckBox(QStringLiteral("슈퍼샘플링 (가로·세로 2배) + MSAA")); m_antiAliasing->setChecked(m_style.antiAliasing);
    m_perspective = new QCheckBox(QStringLiteral("원근 보기")); m_perspective->setChecked(m_style.perspective);
    m_perspectiveFov = new QDoubleSpinBox; m_perspectiveFov->setRange(20, 70); m_perspectiveFov->setSuffix(QStringLiteral("°")); m_perspectiveFov->setValue(m_style.perspectiveFov);
    m_grid = new QCheckBox(QStringLiteral("원자 격자 표시")); m_grid->setChecked(m_style.gridOnAtoms);
    m_fixedWidth = new QCheckBox(QStringLiteral("테두리 고정 두께")); m_fixedWidth->setChecked(m_style.fixedBorderWidth);
    appearanceLayout->addRow(QStringLiteral("배경색"), m_backgroundButton);
    appearanceLayout->addRow(m_shadows); appearanceLayout->addRow(m_monochrome); appearanceLayout->addRow(m_depthCueing);
    appearanceLayout->addRow(QStringLiteral("그림자 강도"), shadowControl);
    appearanceLayout->addRow(QStringLiteral("깊이 단서 강도"), m_depthCueStrength);
    appearanceLayout->addRow(m_antiAliasing);
    appearanceLayout->addRow(m_perspective);
    appearanceLayout->addRow(QStringLiteral("원근 시야각(FOV)"), m_perspectiveFov);
    appearanceLayout->addRow(m_grid); appearanceLayout->addRow(m_fixedWidth);
    return appearance;
}

void StyleEditorDialog::chooseLightColor() {
    const QColor color = QColorDialog::getColor(m_style.lightColor, this, QStringLiteral("주 조명 색"));
    if (color.isValid()) { m_style.lightColor = color; apply(); }
}

void StyleEditorDialog::chooseSecondLightColor() {
    const QColor color = QColorDialog::getColor(m_style.secondLightColor, this, QStringLiteral("보조 조명 색"));
    if (color.isValid()) { m_style.secondLightColor = color; apply(); }
}

void StyleEditorDialog::chooseBackground() {
    const QColor color = QColorDialog::getColor(m_style.background, this, QStringLiteral("배경색"));
    if (color.isValid()) {
        m_style.background = color;
        m_backgroundButton->setStyleSheet(QStringLiteral("background: %1").arg(color.name()));
        apply();
    }
}

} // namespace youngcraft
