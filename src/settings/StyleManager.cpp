#include "settings/StyleManager.h"
#include "model/Molecule.h"

#include <QSettings>

namespace youngcraft {

QStringList StyleManager::presetNames() {
    return {QStringLiteral("Youngcraft Studio"), QStringLiteral("GaussView 유사 스타일"),
            QStringLiteral("Ball-and-Stick"), QStringLiteral("CPK / Space-Filling"),
            QStringLiteral("Stick"), QStringLiteral("Publication"), QStringLiteral("Dark Studio"),
            QStringLiteral("Chemcraft-like"), QStringLiteral("Flat Illustration"), QStringLiteral("사용자 지정")};
}

QString StyleManager::canonicalPresetName(const QString &name) {
    // Old names remain readable without presenting indistinguishable menu entries.
    static const QMap<QString, QString> aliases = {
        {QStringLiteral("Chemcraft"), QStringLiteral("Chemcraft-like")},
        {QStringLiteral("Chemcraft 참조"), QStringLiteral("Chemcraft-like")},
        {QStringLiteral("Classic CPK"), QStringLiteral("CPK / Space-Filling")},
        {QStringLiteral("CPK 2 (흰 배경)"), QStringLiteral("CPK / Space-Filling")},
        {QStringLiteral("큰 원자"), QStringLiteral("CPK / Space-Filling")},
        {QStringLiteral("Spartan 유사 스타일 2"), QStringLiteral("CPK / Space-Filling")},
        {QStringLiteral("Dark"), QStringLiteral("Dark Studio")},
        {QStringLiteral("기본 1"), QStringLiteral("Dark Studio")},
        {QStringLiteral("기본 2"), QStringLiteral("Dark Studio")},
        {QStringLiteral("CPK 1 (어두운 배경)"), QStringLiteral("Dark Studio")},
        {QStringLiteral("HyperChem 유사 스타일"), QStringLiteral("Dark Studio")},
        {QStringLiteral("likehugo 유사 스타일"), QStringLiteral("Dark Studio")},
        {QStringLiteral("혼합 스타일"), QStringLiteral("Dark Studio")},
        {QStringLiteral("Spartan 유사 스타일 1"), QStringLiteral("Ball-and-Stick")},
        {QStringLiteral("스틱"), QStringLiteral("Stick")},
        {QStringLiteral("얇은 스틱"), QStringLiteral("Stick")},
        {QStringLiteral("단순"), QStringLiteral("Flat Illustration")},
        {QStringLiteral("빠른 표시"), QStringLiteral("Flat Illustration")}
    };
    for (int i = 1; i <= 3; ++i)
        if (name == QStringLiteral("출판용 %1").arg(i)) return QStringLiteral("Publication");
    for (int i = 1; i <= 7; ++i)
        if (name == QStringLiteral("2D 스타일 %1").arg(i)) return QStringLiteral("Flat Illustration");
    return aliases.value(name, name);
}

RenderStyle StyleManager::preset(const QString &requestedName) {
    const QString name = canonicalPresetName(requestedName);
    if (name == QStringLiteral("사용자 지정")) return load();
    if (name == QStringLiteral("Dark Studio")) {
        auto style = preset(QStringLiteral("Youngcraft Studio"));
        style.name = name;
        style.background = Qt::black;
        return style;
    }
    RenderStyle style;
    style.name = name;
    // Familiar element hues, lifted toward white for the requested pastel palette.
    for (int z = 1; z <= 118; ++z) {
        const QColor c = elementColor(z);
        style.elementColors.insert(z, QColor(qRound(c.red() * 0.65 + 255 * 0.35),
                                            qRound(c.green() * 0.65 + 255 * 0.35),
                                            qRound(c.blue() * 0.65 + 255 * 0.35)));
    }
    const QMap<int, QColor> pastel = {{1, QColor(246,248,252)}, {6, QColor(164,181,201)},
        {7, QColor(130,169,234)}, {8, QColor(240,145,158)}, {9, QColor(146,213,176)},
        {15, QColor(243,187,145)}, {16, QColor(245,220,145)}, {17, QColor(151,213,171)},
        {25, QColor(202,164,216)}, {26, QColor(225,171,136)}, {27, QColor(149,182,222)},
        {29, QColor(219,177,145)}, {30, QColor(167,196,211)}, {35, QColor(209,157,145)},
        {53, QColor(193,158,221)}};
    for (auto it = pastel.cbegin(); it != pastel.cend(); ++it) style.elementColors.insert(it.key(), it.value());
    if (name == QStringLiteral("Youngcraft Studio")) {
        style.elementColors.insert(1, QColor(250,250,250));
        style.elementColors.insert(6, QColor(12,12,12));
        style.elementColors.insert(7, QColor(0,0,255));
        style.elementColors.insert(8, QColor(255,0,0));
        style.elementColors.insert(9, QColor(145,220,175));
        style.elementColors.insert(16, QColor(255,220,25));
        style.elementColors.insert(17, QColor(155,220,165));
        style.elementColors.insert(25, QColor(185,55,245));
        style.elementColors.insert(26, QColor(255,160,30));
        style.elementColors.insert(27, QColor(140,65,225));
        style.elementRadiusScales.insert(1, 0.76f);
        style.elementRadiusScales.insert(6, 0.75f);
        style.elementRadiusScales.insert(7, 1.08f);
        style.elementRadiusScales.insert(8, 0.98f);
        style.background = Qt::white;
        style.atomScale = 0.28f;
        style.bondRadius = 0.032f;
        style.bondColorMode = BondColorMode::EndpointAtoms;
        style.ambientIntensity = 0.40f * 1.35f;
        style.diffuseIntensity = 0.90f * 1.35f;
        style.specularIntensity = 0.95f;
        style.shininess = 120.0f;
        style.atomSheenStrength = 0.08f;
        style.twoLights = false;
        style.cameraRelativeLights = true;
        style.lightColor = Qt::white;
        style.secondLightColor = Qt::white;
        style.lightDirection = QVector3D(-1.0f,1.6f,2.4f);
        style.secondLightDirection = QVector3D(1.6f,-0.8f,2.0f);
        style.shadows = true;
        style.shadowStrength = 0.24f;
        style.perspective = false;
    } else if (name == QStringLiteral("GaussView 유사 스타일")) {
        // Inspired by conventional Gaussian visualization, not an exact reproduction.
        style.background = QColor(237,240,246);
        style.elementColors.insert(6, QColor(150,162,181));
        style.bondColorMode = BondColorMode::Uniform;
        style.bondColor = QColor(157,167,184);
        style.atomScale = 0.38f;
        style.bondRadius = 0.09f;
        style.doubleBondSpacing = 2.8f;
        style.ambientIntensity = 0.26f;
        style.diffuseIntensity = 0.86f;
        style.specularIntensity = 0.30f;
        style.shininess = 72.0f;
        style.twoLights = false;
        style.perspective = false;
    } else if (name == QStringLiteral("Ball-and-Stick")) {
        style.background = QColor(252,253,255);
        style.atomScale = 0.33f;
        style.bondRadius = 0.105f;
        style.ambientIntensity = 0.28f;
        style.diffuseIntensity = 0.76f;
        style.specularIntensity = 0.13f;
        style.shininess = 28.0f;
    } else if (name == QStringLiteral("CPK / Space-Filling")) {
        style.displayMode = DisplayMode::SpaceFilling;
        style.spaceFillingScale = 1.0f;
        style.background = QColor(245,248,253);
        style.ambientIntensity = 0.26f;
        style.specularIntensity = 0.20f;
        style.shininess = 36.0f;
        style.shadows = true;
        style.shadowStrength = 0.30f;
    } else if (name == QStringLiteral("Stick")) {
        style.displayMode = DisplayMode::Stick;
        style.showBondOrders = false;
        style.background = QColor(241,247,252);
        style.atomScale = style.bondRadius = 0.16f;
        style.specularIntensity = 0.22f;
        style.shininess = 42.0f;
    } else if (name == QStringLiteral("Publication")) {
        style.background = Qt::white;
        style.bondColorMode = BondColorMode::Uniform;
        style.bondColor = QColor(133,147,165);
        style.atomScale = 0.32f;
        style.bondRadius = 0.07f;
        style.ambientIntensity = 0.40f;
        style.diffuseIntensity = 0.62f;
        style.specularIntensity = 0.08f;
        style.shininess = 18.0f;
        style.perspective = false;
    } else if (name == QStringLiteral("Chemcraft-like")) {
        style.background = QColor(250,247,241);
        style.atomScale = 0.50f;
        style.bondRadius = 0.10f;
        style.bondColorMode = BondColorMode::MixedAtoms;
        style.ambientIntensity = 0.20f;
        style.specularIntensity = 0.25f;
        style.shininess = 64.0f;
        style.shadows = true;
        style.shadowStrength = 0.22f;
        style.perspectiveFov = 30.0f;
    } else if (name == QStringLiteral("Flat Illustration")) {
        style.displayMode = DisplayMode::Stick;
        style.background = Qt::white;
        style.atomScale = 0.12f;
        style.bondRadius = 0.055f;
        style.bondColorMode = BondColorMode::Uniform;
        style.bondColor = QColor(113,128,154);
        style.lighting = false;
        style.perspective = false;
        style.showLabels = true;
    }
    return style;
}

void StyleManager::save(const RenderStyle &style) {
    QSettings settings;
    settings.beginGroup(QStringLiteral("style"));
    settings.setValue(QStringLiteral("studioRevision"), 3);
    settings.setValue(QStringLiteral("darkStudioRevision"), 1);
    settings.setValue(QStringLiteral("name"), style.name);
    settings.setValue(QStringLiteral("background"), style.background);
    settings.setValue(QStringLiteral("bondColor"), style.bondColor);
    settings.setValue(QStringLiteral("labelColor"), style.labelColor);
    settings.setValue(QStringLiteral("displayMode"), int(style.displayMode));
    settings.setValue(QStringLiteral("bondColorMode"), int(style.bondColorMode));
    settings.setValue(QStringLiteral("atomScale"), style.atomScale);
    settings.setValue(QStringLiteral("spaceFillingScale"), style.spaceFillingScale);
    settings.setValue(QStringLiteral("bondRadius"), style.bondRadius);
    settings.setValue(QStringLiteral("labelSize"), style.labelSize);
    settings.setValue(QStringLiteral("opacity"), style.opacity);
    settings.setValue(QStringLiteral("bondOpacity"), style.bondOpacity);
    settings.setValue(QStringLiteral("doubleBondSpacing"), style.doubleBondSpacing);
    settings.setValue(QStringLiteral("dashLength"), style.dashLength);
    settings.setValue(QStringLiteral("dashGap"), style.dashGap);
    settings.setValue(QStringLiteral("ambientIntensity"), style.ambientIntensity);
    settings.setValue(QStringLiteral("diffuseIntensity"), style.diffuseIntensity);
    settings.setValue(QStringLiteral("specularIntensity"), style.specularIntensity);
    settings.setValue(QStringLiteral("shininess"), style.shininess);
    settings.setValue(QStringLiteral("atomSheenStrength"), style.atomSheenStrength);
    settings.setValue(QStringLiteral("colorSaturation"), style.colorSaturation);
    settings.setValue(QStringLiteral("lightDirection"), style.lightDirection);
    settings.setValue(QStringLiteral("secondLightDirection"), style.secondLightDirection);
    settings.setValue(QStringLiteral("lightColor"), style.lightColor);
    settings.setValue(QStringLiteral("secondLightColor"), style.secondLightColor);
    settings.setValue(QStringLiteral("shadowStrength"), style.shadowStrength);
    settings.setValue(QStringLiteral("depthCueStrength"), style.depthCueStrength);
    settings.setValue(QStringLiteral("lighting"), style.lighting);
    settings.setValue(QStringLiteral("twoLights"), style.twoLights);
    settings.setValue(QStringLiteral("cameraRelativeLights"), style.cameraRelativeLights);
    settings.setValue(QStringLiteral("shadows"), style.shadows);
    settings.setValue(QStringLiteral("depthCueing"), style.depthCueing);
    settings.setValue(QStringLiteral("antiAliasing"), style.antiAliasing);
    settings.setValue(QStringLiteral("monochrome"), style.monochrome);
    settings.setValue(QStringLiteral("showHydrogen"), style.showHydrogen);
    settings.setValue(QStringLiteral("showBondOrders"), style.showBondOrders);
    settings.setValue(QStringLiteral("showLabels"), style.showLabels);
    settings.setValue(QStringLiteral("showProperties"), style.showProperties);
    settings.setValue(QStringLiteral("showAxes"), style.showAxes);
    settings.setValue(QStringLiteral("perspective"), style.perspective);
    settings.setValue(QStringLiteral("showVdwSpheres"), style.showVdwSpheres);
    settings.setValue(QStringLiteral("showPositiveSurface"), style.showPositiveSurface);
    settings.setValue(QStringLiteral("showNegativeSurface"), style.showNegativeSurface);
    settings.setValue(QStringLiteral("isoValue"), style.isoValue);
    settings.setValue(QStringLiteral("isoOpacity"), style.isoOpacity);
    settings.setValue(QStringLiteral("surfacePattern"), int(style.surfacePattern));
    settings.setValue(QStringLiteral("positiveSurfaceColor"), style.positiveSurfaceColor);
    settings.setValue(QStringLiteral("negativeSurfaceColor"), style.negativeSurfaceColor);
    settings.setValue(QStringLiteral("showVibrationVectors"), style.showVibrationVectors);
    settings.setValue(QStringLiteral("vectorScale"), style.vectorScale);
    settings.setValue(QStringLiteral("vectorWidth"), style.vectorWidth);
    settings.setValue(QStringLiteral("vectorThreshold"), style.vectorThreshold);
    settings.setValue(QStringLiteral("vectorColor"), style.vectorColor);
    settings.setValue(QStringLiteral("recountCutoffScale"), style.recountCutoffScale);
    settings.setValue(QStringLiteral("recountHydrogen"), style.recountHydrogen);
    settings.setValue(QStringLiteral("recountMetalCoordination"), style.recountMetalCoordination);
    settings.setValue(QStringLiteral("perspectiveFov"), style.perspectiveFov);
    settings.beginGroup(QStringLiteral("elementColors"));
    settings.remove(QString());
    for (auto it = style.elementColors.cbegin(); it != style.elementColors.cend(); ++it)
        settings.setValue(QString::number(it.key()), it.value());
    settings.endGroup();
    settings.beginGroup(QStringLiteral("elementRadiusScales"));
    settings.remove(QString());
    for (auto it = style.elementRadiusScales.cbegin(); it != style.elementRadiusScales.cend(); ++it)
        settings.setValue(QString::number(it.key()), it.value());
    settings.endGroup();
    settings.endGroup();
}

RenderStyle StyleManager::load() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("style"));
    const QString storedName = settings.value(QStringLiteral("name"), QStringLiteral("Youngcraft Studio")).toString();
    const bool hasSavedStyle = settings.contains(QStringLiteral("name"));
    const auto presetName = canonicalPresetName(storedName);
    // Replace previous named Studio presets once; subsequent edits round-trip.
    if (hasSavedStyle &&
        ((presetName == QStringLiteral("Youngcraft Studio") &&
          settings.value(QStringLiteral("studioRevision"),0).toInt() < 3) ||
         (presetName == QStringLiteral("Dark Studio") &&
          settings.value(QStringLiteral("darkStudioRevision"),0).toInt() < 1))) {
        settings.endGroup();
        auto updated = preset(presetName);
        save(updated);
        return updated;
    }
    RenderStyle style = preset(storedName == QStringLiteral("사용자 지정")
                                   ? QStringLiteral("Youngcraft Studio") : storedName);
    style.name = canonicalPresetName(storedName);
    // Saved colors (including an empty override map) belong to the user's snapshot.
    if (hasSavedStyle) { style.elementColors.clear(); style.elementRadiusScales.clear(); }
    style.background = settings.value(QStringLiteral("background"), style.background).value<QColor>();
    style.bondColor = settings.value(QStringLiteral("bondColor"), style.bondColor).value<QColor>();
    style.labelColor = settings.value(QStringLiteral("labelColor"), style.labelColor).value<QColor>();
    style.displayMode = DisplayMode(settings.value(QStringLiteral("displayMode"), int(style.displayMode)).toInt());
    style.bondColorMode = BondColorMode(settings.value(QStringLiteral("bondColorMode"), int(style.bondColorMode)).toInt());
    style.atomScale = settings.value(QStringLiteral("atomScale"), style.atomScale).toFloat();
    style.spaceFillingScale = settings.value(QStringLiteral("spaceFillingScale"),
        hasSavedStyle ? 0.72f : style.spaceFillingScale).toFloat();
    style.bondRadius = settings.value(QStringLiteral("bondRadius"), style.bondRadius).toFloat();
    style.labelSize = settings.value(QStringLiteral("labelSize"), style.labelSize).toFloat();
    style.opacity = settings.value(QStringLiteral("opacity"), style.opacity).toFloat();
    style.bondOpacity = settings.value(QStringLiteral("bondOpacity"), style.bondOpacity).toFloat();
    style.doubleBondSpacing = settings.value(QStringLiteral("doubleBondSpacing"), style.doubleBondSpacing).toFloat();
    style.dashLength = settings.value(QStringLiteral("dashLength"), style.dashLength).toFloat();
    style.dashGap = settings.value(QStringLiteral("dashGap"), style.dashGap).toFloat();
    style.ambientIntensity = settings.value(QStringLiteral("ambientIntensity"), style.ambientIntensity).toFloat();
    style.diffuseIntensity = settings.value(QStringLiteral("diffuseIntensity"), style.diffuseIntensity).toFloat();
    style.specularIntensity = settings.value(QStringLiteral("specularIntensity"), style.specularIntensity).toFloat();
    style.shininess = settings.value(QStringLiteral("shininess"), style.shininess).toFloat();
    style.atomSheenStrength = settings.value(QStringLiteral("atomSheenStrength"),
        hasSavedStyle ? 0.0f : style.atomSheenStrength).toFloat();
    style.colorSaturation = settings.value(QStringLiteral("colorSaturation"), style.colorSaturation).toFloat();
    style.lightDirection = settings.value(QStringLiteral("lightDirection"), style.lightDirection).value<QVector3D>();
    style.secondLightDirection = settings.value(QStringLiteral("secondLightDirection"), style.secondLightDirection).value<QVector3D>();
    style.lightColor = settings.value(QStringLiteral("lightColor"), style.lightColor).value<QColor>();
    style.secondLightColor = settings.value(QStringLiteral("secondLightColor"), style.secondLightColor).value<QColor>();
    style.shadowStrength = settings.value(QStringLiteral("shadowStrength"), style.shadowStrength).toFloat();
    style.depthCueStrength = settings.value(QStringLiteral("depthCueStrength"), style.depthCueStrength).toFloat();
    style.lighting = settings.value(QStringLiteral("lighting"), style.lighting).toBool();
    style.twoLights = settings.value(QStringLiteral("twoLights"), style.twoLights).toBool();
    style.cameraRelativeLights = settings.value(QStringLiteral("cameraRelativeLights"),
        hasSavedStyle ? false : style.cameraRelativeLights).toBool();
    style.shadows = settings.value(QStringLiteral("shadows"), style.shadows).toBool();
    style.depthCueing = settings.value(QStringLiteral("depthCueing"), style.depthCueing).toBool();
    style.antiAliasing = settings.value(QStringLiteral("antiAliasing"), style.antiAliasing).toBool();
    style.monochrome = settings.value(QStringLiteral("monochrome"), style.monochrome).toBool();
    style.showHydrogen = settings.value(QStringLiteral("showHydrogen"), style.showHydrogen).toBool();
    style.showBondOrders = settings.value(QStringLiteral("showBondOrders"),
        hasSavedStyle ? true : style.showBondOrders).toBool();
    style.showLabels = settings.value(QStringLiteral("showLabels"), style.showLabels).toBool();
    style.showProperties = settings.value(QStringLiteral("showProperties"), style.showProperties).toBool();
    style.showAxes = settings.value(QStringLiteral("showAxes"), style.showAxes).toBool();
    style.perspective = settings.value(QStringLiteral("perspective"), style.perspective).toBool();
    style.showVdwSpheres = settings.value(QStringLiteral("showVdwSpheres"), style.showVdwSpheres).toBool();
    style.showPositiveSurface = settings.value(QStringLiteral("showPositiveSurface"), style.showPositiveSurface).toBool();
    style.showNegativeSurface = settings.value(QStringLiteral("showNegativeSurface"), style.showNegativeSurface).toBool();
    style.isoValue = settings.value(QStringLiteral("isoValue"), style.isoValue).toFloat();
    style.isoOpacity = settings.value(QStringLiteral("isoOpacity"), style.isoOpacity).toFloat();
    style.surfacePattern = SurfacePattern(qBound(0, settings.value(QStringLiteral("surfacePattern"), 0).toInt(), 4));
    style.positiveSurfaceColor = settings.value(QStringLiteral("positiveSurfaceColor"), style.positiveSurfaceColor).value<QColor>();
    style.negativeSurfaceColor = settings.value(QStringLiteral("negativeSurfaceColor"), style.negativeSurfaceColor).value<QColor>();
    style.showVibrationVectors = settings.value(QStringLiteral("showVibrationVectors"), style.showVibrationVectors).toBool();
    style.vectorScale = settings.value(QStringLiteral("vectorScale"), style.vectorScale).toFloat();
    style.vectorWidth = settings.value(QStringLiteral("vectorWidth"), style.vectorWidth).toFloat();
    style.vectorThreshold = settings.value(QStringLiteral("vectorThreshold"), style.vectorThreshold).toFloat();
    style.vectorColor = settings.value(QStringLiteral("vectorColor"), style.vectorColor).value<QColor>();
    style.recountCutoffScale = settings.value(QStringLiteral("recountCutoffScale"), style.recountCutoffScale).toFloat();
    style.recountHydrogen = settings.value(QStringLiteral("recountHydrogen"), style.recountHydrogen).toBool();
    style.recountMetalCoordination = settings.value(QStringLiteral("recountMetalCoordination"), style.recountMetalCoordination).toBool();
    style.perspectiveFov = settings.value(QStringLiteral("perspectiveFov"), style.perspectiveFov).toFloat();
    settings.beginGroup(QStringLiteral("elementColors"));
    for (const QString &key : settings.childKeys()) style.elementColors.insert(key.toInt(), settings.value(key).value<QColor>());
    settings.endGroup();
    settings.beginGroup(QStringLiteral("elementRadiusScales"));
    for (const QString &key : settings.childKeys()) style.elementRadiusScales.insert(key.toInt(), settings.value(key).toFloat());
    settings.endGroup();
    settings.endGroup();
    return style;
}

} // namespace youngcraft
