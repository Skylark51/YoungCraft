#include "io/FileDispatcher.h"
#include "io/OrcaParser.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

using namespace youngcraft;

class OrcaParserTest final : public QObject {
    Q_OBJECT
private slots:
    void parsesCompleteOutput();
    void acceptsIncompleteOptimization();
    void keepsGaussianDispatch();
    void rejectsUnrecognizedOutput();
    void readsMolecularGsmStringfile();
    void readsPyGsmMoldenPath();
    void readsLabeledGsmEnergies();
    void preservesOrdinaryXyz();
    void rejectsMixedGsmAtoms();
};

void OrcaParserTest::parsesCompleteOutput() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("orca.out"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray output = R"OUT(
                             O   R   C   A
Program Version 5.0.4
Total Charge           ....      -1
Multiplicity           ....       2
CARTESIAN COORDINATES (ANGSTROEM)
---------------------------------
C   0.000000 0.000000 0.000000
H   0.000000 0.000000 1.080000

FINAL SINGLE POINT ENERGY      -40.123456789
MULLIKEN ATOMIC CHARGES AND SPIN POPULATIONS
--------------------------------------------
0 C : -0.2000 0.8000
1 H :  0.2000 0.2000

VIBRATIONAL FREQUENCIES
-----------------------
0:      -123.45 cm**-1
1:      1000.00 cm**-1

ORCA TERMINATED NORMALLY
)OUT";
    QCOMPARE(file.write(output), output.size());
    file.close();
    QVERIFY(OrcaParser::isOrcaOutput(path));
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY2(loaded.parseResult.success, qPrintable(loaded.parseResult.error));
    QCOMPARE(loaded.format, QStringLiteral("ORCA"));
    QCOMPARE(loaded.parseResult.data.format, QStringLiteral("ORCA"));
    const auto &data = loaded.parseResult.data;
    QCOMPARE(data.molecule.atoms.size(), 2);
    QCOMPARE(data.molecule.charge, -1);
    QCOMPARE(data.molecule.multiplicity, 2);
    QCOMPARE(data.frames.size(), 1);
    QCOMPARE(data.scfEnergies.size(), 1);
    QCOMPARE(data.scfEnergies.last(), -40.123456789);
    QCOMPARE(data.frames.last().metrics.energyHartree, -40.123456789);
    QCOMPARE(data.molecule.atoms.first().charge, -0.2);
    QCOMPARE(data.molecule.atoms.first().spinDensity, 0.8);
    QCOMPARE(data.frequencies.size(), 2);
    QCOMPARE(data.frequencies.first().wavenumber, -123.45);
    QVERIFY(data.frequencies.first().imaginary);
    QCOMPARE(data.status, CalculationStatus::NormalTermination);
}

void OrcaParserTest::acceptsIncompleteOptimization() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("pending.log"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("O   R   C   A\nCARTESIAN COORDINATES (ANGSTROEM)\n"
               "-----------------\nO 0 0 0\nH 0 0 1\n\n"
               "FINAL SINGLE POINT ENERGY -75.0\n"
               "CARTESIAN COORDINATES (ANGSTROEM)\n"
               "-----------------\nO 0 0 0\nH 0 0 0.96\n\n");
    file.close();
    const auto loaded = FileDispatcher::load(path);
    QVERIFY(loaded.parseResult.success);
    QCOMPARE(loaded.parseResult.data.frames.size(), 2);
    QCOMPARE(loaded.parseResult.data.status, CalculationStatus::Incomplete);
    QCOMPARE(loaded.parseResult.data.molecule.atoms[1].position.z(), 0.96f);
}

void OrcaParserTest::keepsGaussianDispatch() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("gaussian.out"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("Standard orientation:\n -----\n Center Atomic Atomic Coordinates\n"
               " Number Number Type X Y Z\n -----\n"
               " 1 8 0 0 0 0\n 2 1 0 0 0 1\n -----\n"
               "SCF Done: E(RHF) = -75.0 A.U.\nNormal termination of Gaussian 16\n");
    file.close();
    const auto loaded = FileDispatcher::load(path);
    QCOMPARE(loaded.format, QStringLiteral("Gaussian"));
    QCOMPARE(loaded.parseResult.data.format, QStringLiteral("Gaussian"));
    QVERIFY(loaded.parseResult.success);
}

void OrcaParserTest::rejectsUnrecognizedOutput() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("unknown.out"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("Not an ORCA or Gaussian output\n");
    file.close();
    QVERIFY(!OrcaParser::isOrcaOutput(path));
}


void OrcaParserTest::readsMolecularGsmStringfile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("stringfile.xyz0001"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("2\n0.000000\nH 0 0 0\nH 0 0 1\n"
               "2\n12.500000\nH 0 0 0\nH 0 0 1.2\n"
               "2\n-3.250000\nH 0 0 0\nH 0 0 1.5\n");
    file.close();
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY2(loaded.parseResult.success, qPrintable(loaded.parseResult.error));
    QCOMPARE(loaded.format, QStringLiteral("GSM"));
    const auto &data = loaded.parseResult.data;
    QCOMPARE(data.frames.size(), 3);
    QCOMPARE(data.scanPoints.size(), 3);
    QCOMPARE(data.scanPoints[1].frameIndex, 1);
    QCOMPARE(data.scanPoints[1].relativeKcal, 12.5);
    QCOMPARE(data.scanPoints[2].relativeKcal, -3.25);
    QVERIFY(!std::isfinite(data.scanPoints[1].energyHartree));
    QCOMPARE(data.frames[1].coordinates[1].z(), 1.2f);
}

void OrcaParserTest::readsPyGsmMoldenPath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("opt_converged_000.xyz"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[Molden Format]\n[Geometries] (XYZ)\n"
               "2\n\nH 0 0 0\nH 0 0 1\n"
               "2\n\nH 0 0 0\nH 0 0 1.2\n"
               "[GEOCONV]\nenergy\n0.0\n8.75\nmax-force\n0.1\n0.2\n");
    file.close();
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY2(loaded.parseResult.success, qPrintable(loaded.parseResult.error));
    QCOMPARE(loaded.format, QStringLiteral("GSM"));
    QCOMPARE(loaded.parseResult.data.frames.size(), 2);
    QCOMPARE(loaded.parseResult.data.scanPoints.size(), 2);
    QCOMPARE(loaded.parseResult.data.scanPoints.last().relativeKcal, 8.75);
    QVERIFY(loaded.parseResult.data.scanHasSummary);
}

void OrcaParserTest::readsLabeledGsmEnergies() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("stringfile.standard"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("1\n-100.0 kcal/mol\nH 0 0 0\n"
               "1\n-95.5 kcal/mol\nH 0 0 1\n");
    file.close();
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY2(loaded.parseResult.success, qPrintable(loaded.parseResult.error));
    QCOMPARE(loaded.parseResult.data.scanPoints.last().relativeKcal, 4.5);
}

void OrcaParserTest::preservesOrdinaryXyz() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("plain.xyz"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("1\nordinary molecule\nH 0 0 0\n");
    file.close();
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY2(loaded.parseResult.success, qPrintable(loaded.parseResult.error));
    QCOMPARE(loaded.format, QStringLiteral("XYZ"));
    QVERIFY(loaded.parseResult.data.scanPoints.isEmpty());
}

void OrcaParserTest::rejectsMixedGsmAtoms() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("stringfile.xyz0002"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("1\n0\nH 0 0 0\n"
               "1\n1\nO 0 0 0\n");
    file.close();
    const LoadedFile loaded = FileDispatcher::load(path);
    QVERIFY(!loaded.parseResult.success);
    QVERIFY(loaded.parseResult.error.contains(QStringLiteral("원자 순서")));
}

QTEST_APPLESS_MAIN(OrcaParserTest)
#include "test_orca_parser.moc"
