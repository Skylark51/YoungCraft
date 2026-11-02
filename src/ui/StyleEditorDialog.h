#pragma once

#include "settings/StyleManager.h"
#include "model/Molecule.h"

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;
class QPushButton;
class QComboBox;
class QSlider;
class QLabel;

namespace youngcraft {

class StyleEditorDialog final : public QDialog {
    Q_OBJECT
public:
    explicit StyleEditorDialog(const RenderStyle &style, const Molecule *molecule = nullptr,
                               int selectedAtom = -1, QWidget *parent = nullptr);
    RenderStyle style() const { return m_style; }

signals:
    void styleApplied(const RenderStyle &style);
    void styleSaved(const RenderStyle &style);
    void atomColorApplied(int atomIndex, const QColor &color);

private slots:
    void apply();
    void save();
    void chooseBackground();
    void chooseLightColor();
    void chooseSecondLightColor();
    void chooseElementColor();
    void chooseBondColor();

private:
    QWidget *makeCommonTab();
    QWidget *makeScreenTab();
    QWidget *makePresetTab();
    QWidget *makeAtomTab();
    QWidget *makeBondTab();
    QWidget *makeIsosurfaceTab();
    QWidget *makeSimpleTab(const QString &message);
    void readControls();
    void writeControls();
    void refreshElementColorButton();

    RenderStyle m_style;
    const Molecule *m_molecule = nullptr;
    int m_selectedAtom = -1;
    QPushButton *m_backgroundButton = nullptr;
    QPushButton *m_lightColorButton = nullptr;
    QPushButton *m_secondLightColorButton = nullptr;
    QPushButton *m_elementColorButton = nullptr;
    QPushButton *m_bondColorButton = nullptr;
    QComboBox *m_presetCombo = nullptr;
    QComboBox *m_element = nullptr;
    QComboBox *m_bondColorMode = nullptr;
    QCheckBox *m_lighting = nullptr;
    QCheckBox *m_twoLights = nullptr;
    QCheckBox *m_showBondOrders = nullptr;
    QCheckBox *m_cameraRelativeLights = nullptr;
    QCheckBox *m_shadows = nullptr;
    QCheckBox *m_monochrome = nullptr;
    QCheckBox *m_depthCueing = nullptr;
    QCheckBox *m_grid = nullptr;
    QCheckBox *m_fixedWidth = nullptr;
    QCheckBox *m_showAxes = nullptr;
    QDoubleSpinBox *m_atomScale = nullptr;
    QDoubleSpinBox *m_spaceFillingScale = nullptr;
    QDoubleSpinBox *m_bondRadius = nullptr;
    QSlider *m_atomScaleSlider = nullptr;
    QSlider *m_elementRadiusSlider = nullptr;
    QLabel *m_elementRadiusValue = nullptr;
    QSlider *m_bondRadiusSlider = nullptr;
    QDoubleSpinBox *m_labelSize = nullptr;
    QDoubleSpinBox *m_opacity = nullptr;
    QDoubleSpinBox *m_lightX = nullptr;
    QDoubleSpinBox *m_lightY = nullptr;
    QDoubleSpinBox *m_lightZ = nullptr;
    QDoubleSpinBox *m_secondLightX = nullptr;
    QDoubleSpinBox *m_secondLightY = nullptr;
    QDoubleSpinBox *m_secondLightZ = nullptr;
    QDoubleSpinBox *m_ambient = nullptr;
    QDoubleSpinBox *m_diffuse = nullptr;
    QDoubleSpinBox *m_specular = nullptr;
    QDoubleSpinBox *m_saturation = nullptr;
    QDoubleSpinBox *m_shininess = nullptr;
    QDoubleSpinBox *m_shadowStrength = nullptr;
    QSlider *m_shininessSlider = nullptr;
    QSlider *m_shadowStrengthSlider = nullptr;
    QDoubleSpinBox *m_depthCueStrength = nullptr;
    QDoubleSpinBox *m_bondSpacing = nullptr;
    QDoubleSpinBox *m_bondOpacity = nullptr;
    QDoubleSpinBox *m_dashLength = nullptr;
    QDoubleSpinBox *m_dashGap = nullptr;
    QDoubleSpinBox *m_vectorScale = nullptr;
    QDoubleSpinBox *m_vectorWidth = nullptr;
    QDoubleSpinBox *m_vectorThreshold = nullptr;
    QDoubleSpinBox *m_perspectiveFov = nullptr;
    QCheckBox *m_perspective = nullptr;
    QCheckBox *m_antiAliasing = nullptr;
    QCheckBox *m_showPositiveSurface = nullptr;
    QCheckBox *m_showNegativeSurface = nullptr;
    QDoubleSpinBox *m_isoValue = nullptr;
    QDoubleSpinBox *m_isoOpacity = nullptr;
};

} // namespace youngcraft
