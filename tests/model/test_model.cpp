#include "export/ImageExporter.h"
#include "export/ReactionCapture.h"
#include "io/CubeParser.h"
#include "io/FileDispatcher.h"
#include "model/Molecule.h"
#include "model/GeometryEditor.h"
#include "render/IsosurfaceBuilder.h"
#include "animation/AnimationController.h"
#include "settings/StyleManager.h"

#include <QFile>
#include <QImageReader>
#include <QQuaternion>
#include <QDataStream>
#include <QSet>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

using namespace youngcraft;

class ModelTest final : public QObject {
    Q_OBJECT
private slots:
    void reactionCapturePathsAlignmentAndPanels();
    void infersBondsAndElements();
    void buildsPositiveAndNegativeIsosurfaces();
    void parsesCubeAndSharesGrid();
    void writesAnimatedGif();
    void decodesGifAcrossDictionaryGrowth();
    void preservesManualBondOrders();
    void recountsBondsAndPreservesOverrides();
    void supportsAtomAndElementColorOverrides();
    void frequencyCycleIsContinuous();
    void savesTransparentGif();
    void cropsTransparentContentAndAnimation();
    void persistsRenderOptions();
    void distinctPresetsPreserveLegacySettings();
    void studioPaletteAndMigration();
    void editsInternalCoordinates();
    void periodicTableMappingAndAtomDeletion();
    void autoHydrogensFollowValenceWithoutRemovingManualHydrogen();
};

void ModelTest::autoHydrogensFollowValenceWithoutRemovingManualHydrogen() {
    for (const auto [atomicNumber, valence] : {QPair{7, 3}, QPair{8, 2}, QPair{9, 1},
                                               QPair{14, 4}, QPair{15, 3}, QPair{16, 2},
                                               QPair{17, 1}, QPair{35, 1}, QPair{53, 1}}) {
        Molecule isolated;
        isolated.atoms = {{1, atomicNumber, elementSymbol(atomicNumber), {0, 0, 0}}};
        isolated.reconcileAutoHydrogens();
        QCOMPARE(isolated.atoms.size(), valence + 1);
    }
    Molecule metal;
    metal.atoms = {{1, 26, "Fe", {0, 0, 0}}};
    metal.reconcileAutoHydrogens();
    QCOMPARE(metal.atoms.size(), 1);
    Molecule molecule;
    molecule.atoms = {{1, 6, "C", {0, 0, 0}}};
    molecule.reconcileAutoHydrogens();
    QCOMPARE(molecule.atoms.size(), 5); // CH4
    for (int i = 1; i < 5; ++i) QVERIFY(molecule.atoms[i].autoHydrogen);

    molecule.atoms.push_back({6, 6, "C", {1.5f, 0, 0}});
    molecule.setBond(0, 5, 1.0);
    molecule.reconcileAutoHydrogens();
    QCOMPARE(molecule.atoms.size(), 8); // CH3-CH3
    QCOMPARE(molecule.bonds.size(), 7);
    molecule.setBond(0, 1, 2.0);
    molecule.reconcileAutoHydrogens();
    QCOMPARE(molecule.atoms.size(), 6); // CH2=CH2
    molecule.setBond(0, 1, 3.0);
    molecule.reconcileAutoHydrogens();
    QCOMPARE(molecule.atoms.size(), 4); // HC#CH
    molecule.removeBond(0, 1);
    molecule.reconcileAutoHydrogens();
    QCOMPARE(molecule.atoms.size(), 10); // Two isolated CH4 fragments.

    Molecule oxygen;
    oxygen.atoms = {{1, 8, "O", {0, 0, 0}}, {2, 1, "H", {0.9f, 0, 0}}};
    oxygen.setBond(0, 1, 1.0);
    oxygen.reconcileAutoHydrogens();
    QCOMPARE(oxygen.atoms.size(), 3);
    QVERIFY(!oxygen.atoms[1].autoHydrogen);
    QVERIFY(oxygen.atoms[2].autoHydrogen);
    oxygen.atoms.push_back({4, 6, "C", {1.3f, 0, 0}});
    oxygen.setBond(0, 3, 1.0);
    oxygen.reconcileAutoHydrogens();
    QCOMPARE(oxygen.atoms[1].atomicNumber, 1);
    QVERIFY(!oxygen.atoms[1].autoHydrogen);
    QCOMPARE(oxygen.bondIndex(0, 1), 0);
    for (const Atom &atom : oxygen.atoms)
        QVERIFY(!(atom.atomicNumber == 1 && atom.autoHydrogen && atom.index == 3));
}

void ModelTest::periodicTableMappingAndAtomDeletion() {
    for (int z = 1; z <= 118; ++z) {
        QVERIFY2(!elementSymbol(z).isEmpty(), qPrintable(QString::number(z)));
        QCOMPARE(atomicNumberFor(elementSymbol(z)), z);
    }
    QCOMPARE(atomicNumberFor(QStringLiteral("Te")), 52);
    QCOMPARE(atomicNumberFor(QStringLiteral("I")), 53);
    QCOMPARE(atomicNumberFor(QStringLiteral("Pt")), 78);
    QCOMPARE(atomicNumberFor(QStringLiteral("Au")), 79);
    QCOMPARE(atomicNumberFor(QStringLiteral("Hg")), 80);
    QCOMPARE(elementSymbol(52), QStringLiteral("Te"));
    QCOMPARE(elementSymbol(53), QStringLiteral("I"));
    QCOMPARE(elementSymbol(78), QStringLiteral("Pt"));
    QCOMPARE(elementSymbol(79), QStringLiteral("Au"));
    QCOMPARE(elementSymbol(80), QStringLiteral("Hg"));
    Molecule molecule;
    molecule.atoms = {{1, 6, "C", {}}, {2, 7, "N", {}}, {3, 8, "O", {}}};
    for (double order : {1.0, 2.0, 3.0, 1.5}) {
        molecule.setBond(0, 1, order);
        QCOMPARE(molecule.bonds[molecule.bondIndex(0, 1)].order, order);
        QVERIFY(molecule.bonds[molecule.bondIndex(0, 1)].manual);
    }
    molecule.setBond(1, 2, 1.5, false, BondStyle::Solid, true);
    molecule.setBond(0, 2, 1.0);
    QVERIFY(molecule.removeAtom(1));
    QCOMPARE(molecule.atoms.size(), 2);
    QCOMPARE(molecule.atoms[1].index, 2);
    QCOMPARE(molecule.bonds.size(), 1);
    QCOMPARE(molecule.bondIndex(0, 1), 0);
    QVERIFY(molecule.bonds[0].manual);
}

void ModelTest::editsInternalCoordinates() {
    auto chain=[] {Molecule m;m.atoms={{1,6,"C",{0,0,0}},{2,6,"C",{1,0,0}},{3,6,"C",{1,1,0}},{4,7,"N",{2,1,1}},{5,1,"H",{3,1,1}}};m.setBond(0,1,1);m.setBond(1,2,1);m.setBond(2,3,1);m.setBond(3,4,1);return m;};
    Molecule measured=chain();QVERIFY(qAbs(GeometryEditor::distance(measured,0,1)-1.0)<1e-9);QVERIFY(qAbs(GeometryEditor::angleDegrees(measured,0,1,2)-90.0)<1e-6);QVERIFY(std::isfinite(GeometryEditor::dihedralDegrees(measured,0,1,2,3)));
    QString warning;Molecule d1=chain();const double rigidBefore=GeometryEditor::distance(d1,3,4);QVERIFY(GeometryEditor::setDistance(d1,0,1,1.5,GeometryMoveMode::OneSide,&warning));QVERIFY(qAbs(GeometryEditor::distance(d1,0,1)-1.5)<1e-4);QVERIFY(qAbs(GeometryEditor::distance(d1,3,4)-rigidBefore)<1e-5);
    Molecule d2=chain();const QVector3D a0=d2.atoms[0].position,b0=d2.atoms[1].position;QVERIFY(GeometryEditor::setDistance(d2,0,1,2.0,GeometryMoveMode::BothSides));QVERIFY(qAbs(GeometryEditor::distance(d2,0,1)-2.0)<1e-4);QVERIFY(qAbs((d2.atoms[0].position-a0).length()-0.5)<1e-5);QVERIFY(qAbs((d2.atoms[1].position-b0).length()-0.5)<1e-5);
    Molecule a1=chain();QVERIFY(GeometryEditor::setAngle(a1,0,1,2,120.0,GeometryMoveMode::OneSide));QVERIFY(qAbs(GeometryEditor::angleDegrees(a1,0,1,2)-120.0)<1e-3);
    Molecule a2=chain();QVERIFY(GeometryEditor::setAngle(a2,0,1,2,60.0,GeometryMoveMode::BothSides));QVERIFY(qAbs(GeometryEditor::angleDegrees(a2,0,1,2)-60.0)<1e-3);
    auto periodic=[](double x,double y){double d=x-y;while(d<=-180)d+=360;while(d>180)d-=360;return qAbs(d);};
    Molecule t1=chain();QVERIFY(GeometryEditor::setDihedral(t1,0,1,2,3,75.0,GeometryMoveMode::OneSide));QVERIFY(periodic(GeometryEditor::dihedralDegrees(t1,0,1,2,3),75.0)<1e-3);
    Molecule t2=chain();QVERIFY(GeometryEditor::setDihedral(t2,0,1,2,3,-110.0,GeometryMoveMode::BothSides));QVERIFY(periodic(GeometryEditor::dihedralDegrees(t2,0,1,2,3),-110.0)<1e-3);
    Molecule wrap=chain();QVERIFY(GeometryEditor::setDihedral(wrap,0,1,2,3,179.0,GeometryMoveMode::OneSide));QVERIFY(GeometryEditor::setDihedral(wrap,0,1,2,3,-179.0,GeometryMoveMode::OneSide));QVERIFY(periodic(GeometryEditor::dihedralDegrees(wrap,0,1,2,3),-179.0)<1e-3);
    Molecule ring=chain();ring.setBond(0,3,1);QVERIFY(GeometryEditor::setDistance(ring,0,1,1.3,GeometryMoveMode::BothSides,&warning));QVERIFY(!warning.isEmpty());for(const Atom&atom:ring.atoms){QVERIFY(std::isfinite(atom.position.x()));QVERIFY(std::isfinite(atom.position.y()));QVERIFY(std::isfinite(atom.position.z()));}
}

void ModelTest::infersBondsAndElements() {
    Molecule molecule;
    molecule.atoms = {{1, 8, QStringLiteral("O"), {0, 0, 0}},
                      {2, 1, QStringLiteral("H"), {0, 0, 0.96f}},
                      {3, 1, QStringLiteral("H"), {0.92f, 0, -0.24f}}};
    molecule.rebuildBonds();
    QCOMPARE(elementSymbol(6), QStringLiteral("C"));
    QCOMPARE(atomicNumberFor(QStringLiteral("cl")), 17);
    QCOMPARE(molecule.bonds.size(), 2);
    QVERIFY(vanDerWaalsRadius(8) > 1.0);
}

void ModelTest::buildsPositiveAndNegativeIsosurfaces() {
    CubeGrid grid;
    grid.valid = true;
    grid.nx = 2;
    grid.ny = 2;
    grid.nz = 2;
    grid.axisX = {1, 0, 0};
    grid.axisY = {0, 1, 0};
    grid.axisZ = {0, 0, 1};
    grid.values = {-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f};
    const SurfaceMesh positive = IsosurfaceBuilder::build(grid, 0.25f, true);
    const SurfaceMesh negative = IsosurfaceBuilder::build(grid, 0.25f, false);
    QVERIFY(!positive.isEmpty());
    QVERIFY(!negative.isEmpty());
    QCOMPARE(positive.positions.size(), positive.normals.size());
    QCOMPARE(negative.positions.size(), negative.normals.size());
}

void ModelTest::writesAnimatedGif() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("animation.gif"));
    QImage first(12, 12, QImage::Format_ARGB32);
    first.fill(Qt::red);
    QImage second(12, 12, QImage::Format_ARGB32);
    second.fill(Qt::blue);
    QString error;
    QVERIFY2(ImageExporter::saveGif({first, second}, path, 40, false, true, &error), qPrintable(error));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    QCOMPARE(bytes.left(6), QByteArrayLiteral("GIF89a"));
    QCOMPARE(bytes.count(char(0x2c)), 2);
}

void ModelTest::decodesGifAcrossDictionaryGrowth() {
    QTemporaryDir directory;
    const QString path = directory.filePath("dictionary-growth.gif");
    QImage frame(400, 300, QImage::Format_ARGB32);
    quint32 random = 0x12345678;
    for (int y=0;y<frame.height();++y) for (int x=0;x<frame.width();++x) {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        const int code = random % 216;
        frame.setPixelColor(x,y,QColor((code/36)*51,((code/6)%6)*51,(code%6)*51));
    }
    QString error;
    QVERIFY2(ImageExporter::saveGif({frame,frame.mirrored()},path,50,true,true,&error),qPrintable(error));
    QImageReader reader(path, "gif");
    const auto first = reader.read(); QVERIFY2(!first.isNull(),qPrintable(reader.errorString()));
    QCOMPARE(first.convertToFormat(QImage::Format_ARGB32),frame);
    const auto second = reader.read(); QVERIFY2(!second.isNull(),qPrintable(reader.errorString()));
    QCOMPARE(second.convertToFormat(QImage::Format_ARGB32),frame.mirrored());
}

void ModelTest::preservesManualBondOrders() {
    Molecule molecule;
    molecule.atoms = {{1, 6, QStringLiteral("C"), {0, 0, 0}},
                      {2, 6, QStringLiteral("C"), {1.3f, 0, 0}},
                      {3, 8, QStringLiteral("O"), {2.5f, 0, 0}}};
    molecule.setBond(0, 1, 2.0);
    molecule.setBond(1, 2, 3.0);
    Molecule displayed = molecule;
    QCOMPARE(displayed.bonds[0].order, 2.0);
    QCOMPARE(displayed.bonds[1].order, 3.0);
    displayed.removeBond(0, 1);
    QCOMPARE(displayed.bondIndex(0, 1), -1);
}

void ModelTest::recountsBondsAndPreservesOverrides() {
    Molecule molecule;
    molecule.atoms = {{1, 6, QStringLiteral("C"), {0, 0, 0}},
                      {2, 6, QStringLiteral("C"), {1.3f, 0, 0}},
                      {3, 1, QStringLiteral("H"), {0, 1.0f, 0}}};
    molecule.setBond(0, 1, 2.0);
    molecule.rebuildBonds(1.25, true, true, true);
    QVERIFY(molecule.bondIndex(0, 1) >= 0);
    QCOMPARE(molecule.bonds[molecule.bondIndex(0, 1)].order, 2.0);
    molecule.rebuildBonds(1.25, false, true, false);
    QCOMPARE(molecule.bondIndex(0, 2), -1);
}

void ModelTest::supportsAtomAndElementColorOverrides() {
    Atom atom; atom.atomicNumber = 26; atom.colorOverride = QColor(120, 40, 190);
    RenderStyle style; style.elementColors.insert(26, QColor(255, 120, 30));
    QVERIFY(atom.colorOverride.isValid());
    QCOMPARE(style.elementColors.value(26), QColor(255, 120, 30));
    QCOMPARE(atom.colorOverride, QColor(120, 40, 190));
}

void ModelTest::frequencyCycleIsContinuous() {
    const QVector<QVector3D> base{{0, 0, 0}, {1, 0, 0}};
    FrequencyMode mode; mode.number = 4; mode.displacement = {{0.2f, 0, 0}, {-0.2f, 0, 0}};
    const auto start = AnimationController::coordinatesAtPhase(base, mode, 0.8, 0.0);
    const auto end = AnimationController::coordinatesAtPhase(base, mode, 0.8, 2.0 * 3.14159265358979323846);
    QCOMPARE(start.size(), end.size());
    for (int i = 0; i < start.size(); ++i) QVERIFY((start[i] - end[i]).length() < 1e-5f);
    const auto peak = AnimationController::coordinatesAtPhase(base, mode, 0.8, 3.14159265358979323846 / 2.0);
    QVERIFY(qAbs(peak[0].x() - 0.16f) < 1e-5f);
}

void ModelTest::savesTransparentGif() {
    QTemporaryDir directory;
    QImage frame(8, 8, QImage::Format_ARGB32); frame.fill(Qt::transparent);
    frame.setPixelColor(4, 4, Qt::red);
    QString error;
    const QString path = directory.filePath(QStringLiteral("transparent.gif"));
    QVERIFY2(ImageExporter::saveGif({frame}, path, 50, true, true, &error), qPrintable(error));
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    QVERIFY(bytes.contains(QByteArray("NETSCAPE2.0")));
    QVERIFY(bytes.contains(QByteArray("\x21\xf9\x04\x09", 4)));
    QCOMPARE(quint8(bytes[11]), quint8(255));
    QImage second = frame;
    second.fill(Qt::transparent); second.setPixelColor(6, 4, Qt::blue);
    file.close();
    QVERIFY(ImageExporter::saveGif({frame, second}, path, 50, true, true, &error));
    QImageReader reader(path);
    const QImage decodedFirst = reader.read();
    QVERIFY(!decodedFirst.isNull());
    QVERIFY(decodedFirst.pixelColor(4,4).alpha() > 0);
    const QImage decodedSecond = reader.read();
    QVERIFY(!decodedSecond.isNull());
    QCOMPARE(decodedSecond.pixelColor(4,4).alpha(), 0); // No previous-frame trail.
    QCOMPARE(decodedSecond.pixelColor(0,0).alpha(), 0);
    QVERIFY(decodedSecond.pixelColor(6,4).alpha() > 0);
}

void ModelTest::cropsTransparentContentAndAnimation() {
    QImage first(100, 80, QImage::Format_ARGB32);
    first.fill(Qt::transparent);
    first.setDotsPerMeterX(23622); first.setDotsPerMeterY(23622);
    first.setPixelColor(30, 20, QColor(255, 0, 0, 1)); // Keep faint edges.
    first.setPixelColor(49, 39, Qt::red);
    const auto cropped = ImageExporter::cropToContent(first);
    QCOMPARE(cropped.size(), QSize(20,20));
    QCOMPARE(cropped.pixelColor(0,0).alpha(), 1);
    QCOMPARE(cropped.pixelColor(19,19), QColor(Qt::red));
    QCOMPARE(cropped.dotsPerMeterX(), first.dotsPerMeterX());
    QImage second = first;
    second.fill(Qt::transparent);
    second.setPixelColor(0, 0, Qt::blue);
    second.setPixelColor(99, 79, Qt::blue);
    const auto frames = ImageExporter::cropAnimation({first, second});
    QCOMPARE(frames.size(), 2);
    for (const auto &frame : frames) {
        QCOMPARE(frame.size(), QSize(104,84));
        QCOMPARE(frame.pixelColor(0,0).alpha(), 0);
        QCOMPARE(frame.pixelColor(103,83).alpha(), 0);
    }
    QCOMPARE(frames[0].pixelColor(32,22).alpha(), 1);
    QCOMPARE(frames[1].pixelColor(2,2), QColor(Qt::blue));
    QCOMPARE(frames[1].pixelColor(101,81), QColor(Qt::blue));
    QImage empty(20,30,QImage::Format_ARGB32); empty.fill(Qt::transparent);
    QCOMPARE(ImageExporter::cropToContent(empty), empty);
    QVERIFY(ImageExporter::cropToContent({}).isNull());
    QVERIFY(ImageExporter::cropAnimation({}).isEmpty());
    QImage opaque(20,30,QImage::Format_RGB32); opaque.fill(Qt::white);
    QCOMPARE(ImageExporter::cropToContent(opaque), opaque);
}

void ModelTest::distinctPresetsPreserveLegacySettings() {
    QSet<QByteArray> fingerprints;
    const auto names = StyleManager::presetNames();
    QVERIFY(names.contains(QStringLiteral("GaussView 유사 스타일")));
    for (const QString &name : names) {
        if (name == QStringLiteral("사용자 지정")) continue;
        const auto style = StyleManager::preset(name);
        QByteArray key; QDataStream stream(&key, QIODevice::WriteOnly);
        stream << int(style.displayMode) << int(style.bondColorMode) << style.background << style.elementColors
               << style.atomScale << style.spaceFillingScale << style.bondRadius << style.lighting
               << style.twoLights << style.shadows << style.specularIntensity << style.shininess << style.perspective;
        QVERIFY2(!fingerprints.contains(key), qPrintable("Duplicate visible preset: " + name));
        fingerprints.insert(key);
    }
    const auto cpk = StyleManager::preset(QStringLiteral("CPK / Space-Filling"));
    QCOMPARE(cpk.displayMode, DisplayMode::SpaceFilling); QCOMPARE(cpk.spaceFillingScale, 1.0f);
    QCOMPARE(StyleManager::preset(QStringLiteral("Stick")).displayMode, DisplayMode::Stick);
    QVERIFY(!StyleManager::preset(QStringLiteral("Stick")).showBondOrders);
    QCOMPARE(StyleManager::preset(QStringLiteral("GaussView 유사 스타일")).bondColorMode, BondColorMode::Uniform);
    for (const QString &old : {QStringLiteral("Chemcraft 참조"), QStringLiteral("Classic CPK"), QStringLiteral("Dark"),
                              QStringLiteral("출판용 3"), QStringLiteral("2D 스타일 7")})
        QVERIFY(names.contains(StyleManager::preset(old).name));
    QCoreApplication::setOrganizationName(QStringLiteral("YoungcraftTest"));
    QCoreApplication::setApplicationName(QStringLiteral("LegacyStyleIsolation"));
    QSettings settings; settings.clear();
    settings.setValue("style/name", "Classic CPK");
    settings.setValue("style/displayMode", int(DisplayMode::BallAndStick));
    settings.setValue("style/atomScale", 0.67f);
    settings.setValue("style/elementColors/6", QColor(11,22,33));
    const auto legacy = StyleManager::load();
    QCOMPARE(legacy.name, QStringLiteral("CPK / Space-Filling"));
    QCOMPARE(legacy.displayMode, DisplayMode::BallAndStick);
    QVERIFY(qAbs(legacy.atomScale-0.67f) < 0.0001f);
    QCOMPARE(legacy.elementColors.value(6), QColor(11,22,33));
    QCOMPARE(legacy.elementColors.size(), 1);
    QVERIFY(!legacy.cameraRelativeLights); QCOMPARE(legacy.spaceFillingScale, 0.72f);
    settings.remove("style/elementColors");
    QVERIFY(StyleManager::load().elementColors.isEmpty());
    settings.clear();
}

void ModelTest::studioPaletteAndMigration(){
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("StudioMigration");QSettings settings;settings.clear();
    const auto studio=StyleManager::preset("Youngcraft Studio");
    QCOMPARE(studio.elementColors[1],QColor(250,250,250));QCOMPARE(studio.elementColors[6],QColor(12,12,12));
    QVERIFY(studio.elementColors[7].blue()>220);QVERIFY(studio.elementColors[8].red()>220);
    QCOMPARE(studio.secondLightColor,QColor(Qt::white));QVERIFY(studio.bondRadius<0.11f);QVERIFY(studio.elementRadiusScales[1]<1);
    QCOMPARE(studio.elementColors[7],QColor(0,0,255));QCOMPARE(studio.elementColors[8],QColor(255,0,0));
    QCOMPARE(studio.elementColors[9],QColor(145,220,175));QCOMPARE(studio.bondRadius,0.032f);
    QCOMPARE(studio.atomScale,0.28f);QCOMPARE(studio.atomSheenStrength,0.08f);
    QVERIFY(!studio.perspective);QVERIFY(!studio.twoLights);QCOMPARE(studio.background,QColor(Qt::white));
    settings.setValue("style/name","Youngcraft Studio");settings.setValue("style/bondRadius",0.135f);
    settings.setValue("style/studioRevision",2);
    settings.setValue("style/elementColors/6",QColor(164,181,201));settings.setValue("style/elementColors/8",QColor(1,2,3));
    auto migrated=StyleManager::load();QCOMPARE(migrated.elementColors[6],studio.elementColors[6]);
    QCOMPARE(migrated.elementColors[8],studio.elementColors[8]);QCOMPARE(migrated.bondRadius,studio.bondRadius);
    QCOMPARE(migrated.elementRadiusScales,studio.elementRadiusScales);
    QCOMPARE(migrated.atomSheenStrength,studio.atomSheenStrength);
    QCOMPARE(settings.value("style/studioRevision").toInt(),3);
    migrated.elementColors[8]=QColor(1,2,3);
    StyleManager::save(migrated);QCOMPARE(StyleManager::load().elementColors,migrated.elementColors);settings.clear();
    const auto dark=StyleManager::preset("Dark Studio");
    QCOMPARE(dark.background,QColor(Qt::black));
    QCOMPARE(dark.elementColors,studio.elementColors);QCOMPARE(dark.elementRadiusScales,studio.elementRadiusScales);
    QCOMPARE(dark.atomScale,studio.atomScale);QCOMPARE(dark.bondRadius,studio.bondRadius);
    QCOMPARE(dark.ambientIntensity,studio.ambientIntensity);QCOMPARE(dark.diffuseIntensity,studio.diffuseIntensity);
    QCOMPARE(dark.specularIntensity,studio.specularIntensity);QCOMPARE(dark.shininess,studio.shininess);
    QCOMPARE(dark.atomSheenStrength,studio.atomSheenStrength);QCOMPARE(dark.shadowStrength,studio.shadowStrength);
    QCOMPARE(dark.lightDirection,studio.lightDirection);QCOMPARE(dark.perspective,studio.perspective);
    QCOMPARE(dark.depthCueing,studio.depthCueing);
    settings.setValue("style/name","Dark Studio");settings.setValue("style/bondRadius",0.12f);
    settings.setValue("style/background",QColor(29,37,54));
    settings.setValue("style/elementColors/6",QColor(185,202,222));
    auto darkMigrated=StyleManager::load();QCOMPARE(darkMigrated.background,dark.background);
    QCOMPARE(darkMigrated.elementColors,dark.elementColors);QCOMPARE(darkMigrated.bondRadius,dark.bondRadius);
    QCOMPARE(settings.value("style/darkStudioRevision").toInt(),1);
    darkMigrated.bondRadius=0.04f;StyleManager::save(darkMigrated);
    QCOMPARE(StyleManager::load().bondRadius,0.04f);settings.clear();
}
void ModelTest::persistsRenderOptions() {
    QCoreApplication::setOrganizationName(QStringLiteral("YoungcraftTest"));
    QCoreApplication::setApplicationName(QStringLiteral("StyleRoundTrip"));
    QSettings().clear();
    RenderStyle style; style.twoLights = false; style.shadows = true; style.shadowStrength = 0.61f;
    style.cameraRelativeLights = true; style.spaceFillingScale = 0.93f; style.showBondOrders = false;
    style.depthCueing = true; style.doubleBondSpacing = 3.1f; style.elementColors.insert(6, QColor(90, 90, 90));
    style.atomSheenStrength = 0.13f;
    StyleManager::save(style);
    const RenderStyle loaded = StyleManager::load();
    QCOMPARE(loaded.twoLights, false); QCOMPARE(loaded.shadows, true);
    QCOMPARE(loaded.cameraRelativeLights, true); QVERIFY(qAbs(loaded.spaceFillingScale - 0.93f) < 1e-5f);
    QCOMPARE(loaded.showBondOrders, false);
    QCOMPARE(loaded.atomSheenStrength,0.13f);
    QVERIFY(qAbs(loaded.shadowStrength - 0.61f) < 1e-5f);
    QCOMPARE(loaded.depthCueing, true); QCOMPARE(loaded.elementColors.value(6), QColor(90, 90, 90));
    QSettings().clear();
}

void ModelTest::parsesCubeAndSharesGrid() {
    const QString path = QFINDTESTDATA("tests/data/density.cube");
    QVERIFY2(!path.isEmpty(), "tests/data/density.cube를 찾지 못했습니다.");
    const CubeGrid grid = CubeParser::parse(path);
    QVERIFY(grid.valid);
    QCOMPARE(grid.molecule.atoms.size(), 1);
    QCOMPARE(grid.values.size(), 27);
    QVERIFY(grid.minValue < 0.0f && grid.maxValue > 0.0f);
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY(loaded.parseResult.success);
    QVERIFY(loaded.cube);
    QCOMPARE(loaded.cube->values.size(), 27);
}

void ModelTest::reactionCapturePathsAlignmentAndPanels() {
    const auto paths=ReactionCapture::companionPaths(QStringLiteral("C:/a b/한글-freq-ts-freq.out"));
    QCOMPARE(paths,QStringList({QStringLiteral("C:/a b/한글-freq-ts-freq-ircfwd-opt-freq.out"),QStringLiteral("C:/a b/한글-freq-ts-freq-ircrev-opt-freq.out")}));
    QVERIFY(ReactionCapture::companionPaths("other.out").isEmpty());
    QCOMPARE(ReactionCapture::companionPaths("ABC-FREQ-TS-FREQ.OUT").size(),2);
    Molecule ts;ts.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1,0,0}},{3,7,"N",{0,2,0}},{4,6,"C",{0,0,3}}};
    Molecule endpoint=ts;
    const auto rotation=QQuaternion::fromAxisAndAngle(QVector3D(1,2,3),123);
    for(auto &atom:endpoint.atoms)atom.position=rotation.rotatedVector(atom.position)+QVector3D(12,-3,5);
    const auto before=endpoint;
    QVERIFY(ReactionCapture::alignTo(endpoint,ts));
    for(int i=0;i<ts.atoms.size();++i)QVERIFY((endpoint.atoms[i].position-ts.atoms[i].position).length()<1e-4);
    QVERIFY((before.atoms[0].position-endpoint.atoms[0].position).length()>1);
    endpoint.atoms[0].atomicNumber=9;QVERIFY(!ReactionCapture::alignTo(endpoint,ts));
    QImage left(100,100,QImage::Format_ARGB32);left.fill(Qt::transparent);
    QImage right=left, first=left,second=left;
    for(int y=40;y<60;++y)for(int x=10;x<90;++x)left.setPixelColor(x,y,Qt::red);
    for(int y=10;y<90;++y)for(int x=40;x<60;++x)right.setPixelColor(x,y,Qt::green);
    first.setPixelColor(30,40,Qt::blue);second.setPixelColor(70,60,Qt::blue);
    auto panels=ReactionCapture::equalPanels(left,{first,second},right,200,5);
    QCOMPARE(panels.size(),4);
    for(const auto &panel:panels){QCOMPARE(panel.size(),panels[0].size());QCOMPARE(panel.dotsPerMeterX(),4000);QCOMPARE(panel.pixelColor(0,0).alpha(),0);}
    QVERIFY(panels[1]!=panels[2]);
    QCOMPARE(ImageExporter::contentBounds(panels[1]).size(),ImageExporter::contentBounds(panels[2]).size());
    QCOMPARE(ReactionCapture::equalPanels(left,{first,second},right,200,10)[0].dotsPerMeterX(),2000);
    QVERIFY(ReactionCapture::equalPanels({}, {first},right,200,5).isEmpty());
}

QTEST_GUILESS_MAIN(ModelTest)
#include "test_model.moc"
