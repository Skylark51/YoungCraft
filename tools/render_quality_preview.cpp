// Reproducible screenshots of the real OpenGL renderer. Does not use application settings.
#include "render/MoleculeView.h"
#include "model/FragmentLibrary.h"
#include <QApplication>
#include <QMouseEvent>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QTimer>
#include <QTextStream>

using namespace youngcraft;

int main(int argc, char **argv) {
    QSurfaceFormat format;
    format.setVersion(3, 3); format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24); format.setAlphaBufferSize(8); format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication app(argc, argv);
    app.setOrganizationName("YoungcraftRenderPreview"); app.setApplicationName("IsolatedCapture");
    const QStringList args = app.arguments();
    if (args.size() < 3) return 2; // project directory, output directory
    QDir output(args[2]); if (!output.mkpath(".")) return 3;
    Fragment ligand, adduct; QString error;
    if (!FragmentLibrary::read(QDir(args[1]).filePath("lab_fragments/ligands/12-mtp.cjson"), ligand, &error) ||
        !FragmentLibrary::read(QDir(args[1]).filePath("lab_fragments/adducts/peroxo-eta2.cjson"), adduct, &error)) {
        QTextStream(stderr) << error; return 4;
    }
    Molecule molecule;
    molecule.atoms = ligand.atoms; molecule.bonds = ligand.bonds;
    int metal = ligand.dummyAtoms.value(0, -1);
    if (metal < 0) return 5;
    molecule.atoms[metal].atomicNumber = 26; molecule.atoms[metal].element = "Fe";
    QVector<int> mapping;
    for (const auto &atom : adduct.atoms) {
        if (atom.atomicNumber == 0) mapping << metal;
        else { mapping << molecule.atoms.size(); molecule.atoms << atom; }
    }
    for (const auto &bond : adduct.bonds)
        molecule.setBond(mapping[bond.first], mapping[bond.second], bond.order,
                         mapping[bond.first] == metal || mapping[bond.second] == metal);
    for (int i = 0; i < molecule.atoms.size(); ++i) molecule.atoms[i].index = i + 1;
    for (auto &bond : molecule.bonds)
        if (bond.first == metal || bond.second == metal) bond.coordination = true;
    MoleculeView view; view.resize(1000, 800);
    view.setStyle(StyleManager::preset("Youngcraft Studio")); view.setMolecule(molecule); view.show();
    int status = 0;
    QTimer::singleShot(500, &app, [&] {
        if (!view.isValid()) { status = 6; app.quit(); return; }
        view.bestView();
        // Rotate through the real input path; leave the user's clipboard untouched.
        const QPointF start(30, 30), end(100, 65);
        QMouseEvent press(QEvent::MouseButtonPress, start, view.mapToGlobal(start.toPoint()),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent move(QEvent::MouseMove, end, view.mapToGlobal(end.toPoint()),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, end, view.mapToGlobal(end.toPoint()),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &press); QApplication::sendEvent(&view, &move);
        QApplication::sendEvent(&view, &release);
        const QStringList names = args.size() > 3 ? QStringList{args[3]} : StyleManager::presetNames();
        QJsonArray manifest;
        for (const QString &name : names) {
            if (name == QStringLiteral("사용자 지정")) continue;
            RenderStyle style = StyleManager::preset(name);
            if (args.contains("--no-shadows")) style.shadows = false;
            view.setStyle(style);
            const QImage image = view.image(false, true);
            const QString file = QString("preset-%1.png").arg(manifest.size(), 2, 10, QLatin1Char('0'));
            if (!image.save(output.filePath(file))) status = 7;
            manifest.append(QJsonObject{{"name", name}, {"file", file}});
        }
        QFile manifestFile(output.filePath("manifest.json"));
        if (manifestFile.open(QIODevice::WriteOnly)) manifestFile.write(QJsonDocument(manifest).toJson());
        app.quit();
    });
    app.exec(); return status;
}
