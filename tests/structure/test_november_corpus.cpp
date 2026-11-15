#include "io/FileDispatcher.h"
#include <gemmi/cif.hpp>
#include <functional>
#include <QFile>
#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

using namespace youngcraft;

static QMap<QString, QStringList> preservedCifValues(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const auto bytes = file.readAll();
    auto document = gemmi::cif::read_memory(bytes.constData(), size_t(bytes.size()), "preservation check");
    QMap<QString, QStringList> values;
    const auto isCoordinate = [](const std::string &tag) {
        const auto normalized = QString::fromStdString(tag).toLower().replace('.', '_');
        return QStringList{"_atom_site_fract_x", "_atom_site_fract_y", "_atom_site_fract_z",
                           "_atom_site_cartn_x", "_atom_site_cartn_y", "_atom_site_cartn_z"}.contains(normalized);
    };
    std::function<void(const gemmi::cif::Block&, const QString&)> visit;
    visit = [&](const auto &block, const QString &prefix) {
        values[prefix + "/block"] = {QString::fromStdString(block.name)};
        for (const auto &item : block.items) {
            if (item.type == gemmi::cif::ItemType::Pair && !isCoordinate(item.pair[0]))
                values[prefix + "/pair/" + QString::fromStdString(item.pair[0]).toLower()] =
                    {QString::fromStdString(gemmi::cif::as_string(item.pair[1]))};
            if (item.type == gemmi::cif::ItemType::Loop) {
                for (size_t col = 0; col < item.loop.tags.size(); ++col) {
                    QStringList column;
                    for (size_t row = isCoordinate(item.loop.tags[col]) ? 1 : 0; row < item.loop.length(); ++row)
                        column.push_back(QString::fromStdString(gemmi::cif::as_string(item.loop.values[row * item.loop.width() + col])));
                    values[prefix + "/loop/" + QString::fromStdString(item.loop.tags[col]).toLower()] = column;
                }
            }
            if (item.type == gemmi::cif::ItemType::Frame)
                visit(item.frame, prefix + "/save/" + QString::fromStdString(item.frame.name));
        }
    };
    for (size_t i = 0; i < document.blocks.size(); ++i) visit(document.blocks[i], QString::number(i));
    return values;
}

class NovemberCorpusTest final : public QObject {
    Q_OBJECT
private slots:
    void publicCorpus();
};

void NovemberCorpusTest::publicCorpus() {
    const QString root = qEnvironmentVariable("YOUNGCRAFT_NOVEMBER_CORPUS");
    if (root.isEmpty()) QSKIP("Set YOUNGCRAFT_NOVEMBER_CORPUS to the fetched corpus directory");
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    int calculations = 0, crystals = 0;
    QDirIterator files(root, {"*.out", "*.log", "*.cif"}, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString path = files.next();
        const auto input = FileDispatcher::load(path);
        QVERIFY2(input.parseResult.success, qPrintable(path + ": " + input.parseResult.error));
        const auto &data = input.parseResult.data;
        QVERIFY(!data.molecule.isEmpty());
        if (path.endsWith(".cif")) {
            ++crystals;
            QVERIFY(input.crystal);
            auto edited = input.crystal->asymmetricUnit;
            edited.atoms[0].position += QVector3D(0.013f, -0.027f, 0.039f);
            QString error;
            const QString saved = temporary.filePath(QFileInfo(path).fileName());
            QVERIFY2(StructureReader::saveCif(saved, *input.crystal, edited, &error), qPrintable(error));
            const auto preserved = preservedCifValues(path);
            QVERIFY(!preserved.isEmpty());
            QCOMPARE(preservedCifValues(saved), preserved);
            const auto output = FileDispatcher::load(saved);
            QVERIFY2(output.parseResult.success, qPrintable(output.parseResult.error));
            QVERIFY(output.crystal);
            QCOMPARE(output.crystal->a, input.crystal->a);
            QCOMPARE(output.crystal->b, input.crystal->b);
            QCOMPARE(output.crystal->c, input.crystal->c);
            QCOMPARE(output.crystal->alpha, input.crystal->alpha);
            QCOMPARE(output.crystal->beta, input.crystal->beta);
            QCOMPARE(output.crystal->gamma, input.crystal->gamma);
            QCOMPARE(output.crystal->spaceGroup, input.crystal->spaceGroup);
            QCOMPARE(output.crystal->asymmetricUnit.atoms.size(), edited.atoms.size());
            for (int i = 0; i < edited.atoms.size(); ++i) {
                const auto &a = edited.atoms[i];
                const auto &b = output.crystal->asymmetricUnit.atoms[i];
                QCOMPARE(a.atomicNumber, b.atomicNumber);
                QCOMPARE(a.occupancy, b.occupancy);
                QCOMPARE(a.disorderGroup, b.disorderGroup);
                QVERIFY((a.position - b.position).length() < 2e-5f);
            }
            QCOMPARE(output.crystal->packing.atoms.size(), output.crystal->unitCell.atoms.size() * 8);
        } else {
            ++calculations;
            if (path.contains("ORCA6.1/1509.out")) {
                QCOMPARE(data.excitedStates.size(), 50);
                QCOMPARE(data.excitedStates[0].energyEv, 3.412305);
                QCOMPARE(data.excitedStates[0].wavelengthNm, 363.3);
                QCOMPARE(data.excitedStates[0].oscillatorStrength, 0.0);
                QVERIFY(!data.excitedStates[0].transitions.isEmpty());
            }
            if (path.contains("Benzene_opt_etsyms.log")) {
                QCOMPARE(data.excitedStates.size(), 10);
                QCOMPARE(data.index.sectionOffsets.value("excited_states").size(), 10);
                QCOMPARE(data.excitedStates[2].oscillatorStrength, 1.087369886);
            }
            if (path.contains("ansaBPFP2_acetylene_ircf_LQA.out")) {
                QCOMPARE(data.ircPoints.size(), 103);
                QCOMPARE(data.ircPoints[1].coordinate, 0.23513);
                QCOMPARE(data.ircPoints.last().coordinate, 23.17564);
                for (const auto &point : data.ircPoints)
                    QCOMPARE(point.energyHartree, data.frames[point.frameIndex].metrics.energyHartree);
            }
            if (path.contains("ORCA4.2/dvb_scan.out")) {
                QCOMPARE(data.scanPoints.size(), 12);
                QCOMPARE(data.scanPoints.first().coordinate, 0.0);
                QCOMPARE(data.scanPoints.last().coordinate, 360.0);
                QVERIFY(qAbs(data.scanPoints.first().energyHartree - (-379.77794933)) < 1e-7);
                QVERIFY(qAbs(data.scanPoints.last().energyHartree - (-379.77794979)) < 1e-7);
                QVERIFY(data.scanHasSummary);
                for (const auto &point : data.scanPoints) {
                    QVERIFY(!point.inProgress);
                    QVERIFY(!point.optimizationFrames.isEmpty());
                    QCOMPARE(point.energyHartree, data.frames[point.frameIndex].metrics.energyHartree);
                    QCOMPARE(point.coordinate, data.frames[point.frameIndex].metrics.scanCoordinate);
                }
            }
            if (path.contains("ORCA4.0/dvb_td.out")) {
                QCOMPARE(data.excitedStates.size(), 10);
                QCOMPARE(data.excitedStates[0].energyEv, 5.352);
                QCOMPARE(data.excitedStates[1].oscillatorStrength, 1.165369790);
                QCOMPARE(data.excitedStates[5].label, QString("Triplet 1"));
                QCOMPARE(data.excitedStates[5].oscillatorStrength, 0.0);
                QVERIFY(!data.excitedStates[0].transitions.isEmpty());
            }
            if (path.contains("1-ttt_td.out")) QCOMPARE(data.excitedStates.size(), 24);
            if (path.contains("H2-_CASSCF_opt.out")) {
                QVERIFY(data.frames.size() >= 3);
                QCOMPARE(data.frames[2].metrics.maxForce, 0.0000012910);
                QCOMPARE(data.frames[2].metrics.rmsDisplacement, 0.0000782895);
            }
            if (path.contains("ORCA4.0/dvb_ir.out")) {
                QCOMPARE(data.frequencies.size(), 60);
                QCOMPARE(data.frequencies[6].displacement.size(), data.molecule.atoms.size());
                QVERIFY(qAbs(data.frequencies[6].displacement[0].z() - 0.078859f) < 1e-6f);
                QCOMPARE(data.frequencies[6].irIntensity, 0.030792);
                QCOMPARE(data.frequencies[6].irIntensityUnit, QString("ORCA T²"));
            }
            if (path.contains("ORCA4.0/dvb_raman.out")) {
                QCOMPARE(data.frequencies[7].ramanActivity, 2.483069);
                QCOMPARE(data.frequencies[7].displacement.size(), data.molecule.atoms.size());
            }

            for (const auto &frame : data.frames) {
                QCOMPARE(frame.coordinates.size(), data.molecule.atoms.size());
                for (const auto &p : frame.coordinates)
                    QVERIFY(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
            }
            for (const auto &point : data.scanPoints) {
                QVERIFY(point.frameIndex >= 0 && point.frameIndex < data.frames.size());
                for (int frame : point.optimizationFrames) QVERIFY(frame >= 0 && frame < data.frames.size());
            }
            for (const auto &point : data.ircPoints)
                QVERIFY(point.frameIndex >= 0 && point.frameIndex < data.frames.size());
            qInfo().noquote() << QFileInfo(path).fileName() << "frames" << data.frames.size()
                << "modes" << data.frequencies.size() << "scan" << data.scanPoints.size()
                << "IRC" << data.ircPoints.size() << "TD" << data.excitedStates.size();
        }
    }
    QCOMPARE(calculations, 24);
    QCOMPARE(crystals, 10);
}

QTEST_GUILESS_MAIN(NovemberCorpusTest)
#include "test_november_corpus.moc"
