#pragma once

#include <QColor>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector3D>

namespace youngcraft {

enum class DisplayMode {
    BallAndStick,
    Stick,
    SpaceFilling
};

enum class BondColorMode {
    EndpointAtoms,
    Uniform,
    MixedAtoms
};

struct RenderStyle {
    QString name = QStringLiteral("Youngcraft Studio");
    DisplayMode displayMode = DisplayMode::BallAndStick;
    BondColorMode bondColorMode = BondColorMode::EndpointAtoms;
    QColor background = QColor(249, 250, 253);
    QColor bondColor = QColor(160, 166, 178);
    QColor labelColor = QColor(35, 38, 44);
    QColor selectionColor = QColor(255, 255, 255);
    QColor axisColor = QColor(210, 210, 210);
    float atomScale = 0.46f;
    float bondRadius = 0.135f;
    float atomBorderWidth = 0.0f;
    float labelSize = 12.0f;
    float opacity = 1.0f;
    float ambientIntensity = 0.23f;
    float diffuseIntensity = 0.80f;
    float specularIntensity = 0.24f;
    float shininess = 48.0f;
    float atomSheenStrength = 0.0f;
    float colorSaturation = 1.0f;
    float spaceFillingScale = 0.72f;
    float doubleBondSpacing = 2.35f;
    float bondOpacity = 1.0f;
    float dashLength = 0.22f;
    float dashGap = 0.14f;
    float shadowStrength = 0.32f;
    float depthCueStrength = 0.35f;
    float vectorScale = 1.0f;
    float vectorWidth = 2.0f;
    float vectorThreshold = 0.01f;
    float recountCutoffScale = 1.25f;
    float perspectiveFov = 33.0f;
    QVector3D lightDirection = QVector3D(-1.0f, 1.3f, 2.0f);
    QVector3D secondLightDirection = QVector3D(1.5f, 0.3f, 1.0f);
    QColor lightColor = Qt::white;
    QColor secondLightColor = QColor(235, 242, 255);
    QColor vectorColor = QColor(255, 195, 45);
    QMap<int, QColor> elementColors;
    QMap<int, float> elementRadiusScales;
    bool lighting = true;
    bool twoLights = true;
    bool cameraRelativeLights = true;
    bool shadows = false;
    bool monochrome = false;
    bool depthCueing = false;
    bool gridOnAtoms = false;
    bool fixedBorderWidth = false;
    bool showHydrogen = true;
    bool showBondOrders = true;
    bool showLabels = false;
    bool showProperties = false;
    bool showAxes = false;
    bool centerAxes = true;
    bool perspective = true;
    bool showVdwSpheres = false;
    bool antiAliasing = true;
    bool showVibrationVectors = false;
    bool recountHydrogen = true;
    bool recountMetalCoordination = true;
    bool showPositiveSurface = true;
    bool showNegativeSurface = true;
    float isoValue = 0.03f;
    float isoOpacity = 0.55f;
    QColor positiveSurfaceColor = QColor(70, 130, 240);
    QColor negativeSurfaceColor = QColor(235, 80, 90);
};

class StyleManager {
public:
    static QStringList presetNames();
    static QString canonicalPresetName(const QString &name);
    static RenderStyle preset(const QString &name);
    static void save(const RenderStyle &style);
    static RenderStyle load();
};

} // namespace youngcraft
