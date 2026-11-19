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
    void attachesIrcSummaryToCompleteTrajectory();
    void readsNormalModeBlocks();
    void readsExcitedStatesAndSpinForbiddenTransitions();
    void acceptsIncompleteOptimization();
    void keepsGaussianDispatch();
    void rejectsUnrecognizedOutput();
    void readsMolecularGsmStringfile();
    void readsPyGsmMoldenPath();
    void readsLabeledGsmEnergies();
    void preservesOrdinaryXyz();
    void rejectsMixedGsmAtoms();
};

void OrcaParserTest::readsExcitedStatesAndSpinForbiddenTransitions() {
    QTemporaryDir dir;
    QFile file(dir.filePath("td.out"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"OUT(O   R   C   A
CARTESIAN COORDINATES (ANGSTROEM)
--------------------------------
H 0 0 0
H 0 0 0.74

TD-DFT/TDA EXCITED STATES (SINGLETS)
STATE  1:  E=   0.196686 au      5.352 eV    43167.6 cm**-1
    33a ->  35a  :     0.563723 (c=  0.75081502)
TD-DFT/TDA EXCITED STATES (TRIPLETS)
STATE  1:  E=   0.115235 au      3.136 eV    25291.1 cm**-1
    34a ->  35a  :     0.99 (c= -0.995)
ABSORPTION SPECTRUM VIA TRANSITION ELECTRIC DIPOLE MOMENTS
State Energy Wavelength fosc
1 43167.6 231.7 0.005629700
2 25291.1 395.4 spin forbidden (mult=3)
ABSORPTION SPECTRUM VIA TRANSITION VELOCITY DIPOLE MOMENTS
1 43167.6 231.7 0.999
ORCA TERMINATED NORMALLY
)OUT");
    file.close();
    const auto result = OrcaParser::parse(file.fileName());
    QVERIFY(result.success);
    QCOMPARE(result.data.excitedStates.size(), 2);
    const auto &singlet = result.data.excitedStates[0];
    const auto &triplet = result.data.excitedStates[1];
    QCOMPARE(singlet.energyEv, 5.352);
    QCOMPARE(singlet.wavelengthNm, 231.7);
    QCOMPARE(singlet.oscillatorStrength, 0.005629700);
    QCOMPARE(singlet.transitions.size(), 1);
    QCOMPARE(singlet.transitions[0].coefficient, 0.75081502);
    QCOMPARE(singlet.label, QString("Singlet 1"));
    QCOMPARE(singlet.frameIndex, 0);
    QCOMPARE(triplet.label, QString("Triplet 1"));
    QCOMPARE(triplet.oscillatorStrength, 0.0);
    QCOMPARE(triplet.transitions[0].coefficient, -0.995);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Append));
    file.write("\nTD-DFT/TDA EXCITED STATES (SINGLETS)\n"
               "STATE 1: E= 0.25 au 6.5 eV 52426 cm**-1\n");
    file.close();
    const auto repeated = OrcaParser::parse(file.fileName());
    QCOMPARE(repeated.data.excitedStates.size(), 1);
    QCOMPARE(repeated.data.excitedStates[0].energyEv, 6.5);
    const auto offsets = repeated.data.index.sectionOffsets.value("excited_states");
    QCOMPARE(offsets.size(), 1);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.seek(offsets[0]));
    QVERIFY(file.readLine().contains("6.5 eV"));
    file.close();
}

void OrcaParserTest::readsNormalModeBlocks() {
    QTemporaryDir dir;
    QFile file(dir.filePath("modes.out"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"OUT(O   R   C   A
CARTESIAN COORDINATES (ANGSTROEM)
------------------------------
H 0 0 0

VIBRATIONAL FREQUENCIES
0: -10.0 cm**-1
1: 20.0 cm**-1
NORMAL MODES
------------
 0 1
0 0.1 0.4
1 0.2 0.5
2 0.3 0.6
IR SPECTRUM
Mode freq (cm**-1) T**2 TX TY TZ
0: -10.0 2.5 (0 0 0)
1: 20.0 3.5 (0 0 0)

RAMAN SPECTRUM
Mode freq (cm**-1) Activity Depolarization
0: -10.0 4.5 0.5
1: 20.0 5.5 0.5

ORCA TERMINATED NORMALLY
)OUT");
    file.close();
    const auto result = OrcaParser::parse(file.fileName());
    QVERIFY(result.success);
    QCOMPARE(result.data.frequencies.size(), 2);
    QCOMPARE(result.data.frequencies[0].displacement, QVector<QVector3D>({{0.1f, 0.2f, 0.3f}}));
    QCOMPARE(result.data.frequencies[1].displacement, QVector<QVector3D>({{0.4f, 0.5f, 0.6f}}));
    QVERIFY(result.data.frequencies[0].imaginary);
    QCOMPARE(result.data.frequencies[0].irIntensity, 2.5);
    QCOMPARE(result.data.frequencies[1].ramanActivity, 5.5);
    QCOMPARE(result.data.status, CalculationStatus::NormalTermination);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Append));
    file.write("\nIR SPECTRUM\nMode freq eps Int T**2 TX TY TZ\n"
               "cm**-1 L/(mol*cm) km/mol a.u.\n0: -10 0.1 42.5 0.3 (0 0 0)\n");
    file.close();
    const auto modern = OrcaParser::parse(file.fileName());
    QCOMPARE(modern.data.frequencies[0].irIntensity, 42.5);
    QCOMPARE(modern.data.frequencies[0].irIntensityUnit, QString("km/mol"));
}

void OrcaParserTest::attachesIrcSummaryToCompleteTrajectory() {
    QTemporaryDir dir;
    const auto path = dir.filePath("reaction.out");
    QFile output(path);
    QVERIFY(output.open(QIODevice::WriteOnly));
    output.write("O   R   C   A\nCARTESIAN COORDINATES (ANGSTROEM)\n---\nH 0 0 0\n\n"
        "IRC PATH SUMMARY\n-----\nAll gradients are in Eh/Bohr.\n"
        "Step E(Eh) dE(kcal/mol) max(|G|) RMS(G)\n"
        "1 -1.2 -62.75 0.002 0.001\n2 -1.1 0 0.003 0.002 <= TS\n"
        "3 -1.3 -125.5 0.001 0.0005\n\nORCA TERMINATED NORMALLY\n");
    output.close();
    auto parsed = OrcaParser::parse(path);
    QVERIFY(parsed.success);
    QCOMPARE(parsed.data.ircPoints.size(), 3);
    QCOMPARE(parsed.data.ircPoints[0].frameIndex, -1);
    QVERIFY(!parsed.data.warningMessage.isEmpty());
    QFile trajectory(dir.filePath("reaction_IRC_Full_trj.xyz"));
    QVERIFY(trajectory.open(QIODevice::WriteOnly));
    trajectory.write("1\nfirst\nH 1 0 0\n1\nTS\nH 2 0 0\n1\nlast\nH 3 0 0\n");
    trajectory.close();
    parsed = OrcaParser::parse(path);
    QCOMPARE(parsed.data.ircPoints.size(), 3);
    QCOMPARE(parsed.data.ircPoints[0].coordinate, -1.0);
    QCOMPARE(parsed.data.ircPoints[1].coordinate, 0.0);
    QCOMPARE(parsed.data.ircPoints[1].direction, QString("TS"));
    for (int i = 0; i < 3; ++i) {
        const auto &point = parsed.data.ircPoints[i];
        QVERIFY(point.frameIndex >= 0);
        const auto &frame = parsed.data.frames[point.frameIndex];
        QCOMPARE(frame.coordinates[0].x(), float(i + 1));
        QCOMPARE(frame.metrics.energyHartree, point.energyHartree);
    }
    QVERIFY(trajectory.open(QIODevice::WriteOnly | QIODevice::Truncate));
    trajectory.write("1\nwrong element\nC 1 0 0\n"); trajectory.close();
    parsed = OrcaParser::parse(path);
    QVERIFY(parsed.success);
    QCOMPARE(parsed.data.ircPoints[0].frameIndex, -1);
    QVERIFY(!parsed.data.warningMessage.isEmpty());
}

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
          RMS gradient        0.0002      0.00003 NO
          MAX gradient        0.0003      0.0001 NO
          RMS step            0.0004      0.0006 YES
          MAX step            0.0005      0.001 YES
MULLIKEN ATOMIC CHARGES AND SPIN POPULATIONS
--------------------------------------------
0 C: -0.2000 0.8000
1 H :  0.2000 0.2000

Summary of Natural Population Analysis:
 Atom No Charge Core Valence Rydberg Total
 C 1 -0.3200 2.0 4.30 0.02 6.32
 H 2  0.3200 0.0 0.68 0.00 0.68
 * Total *

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
    QCOMPARE(data.frames.last().metrics.rmsForce, 0.0002);
    QCOMPARE(data.frames.last().metrics.maxForce, 0.0003);
    QCOMPARE(data.frames.last().metrics.rmsDisplacement, 0.0004);
    QCOMPARE(data.frames.last().metrics.maxDisplacement, 0.0005);
    QCOMPARE(data.molecule.atoms.first().charge, -0.2);
    QCOMPARE(data.molecule.atoms.first().spinDensity, 0.8);
    QCOMPARE(data.frames.last().mullikenSpin, QVector<double>({0.8, 0.2}));
    QCOMPARE(data.frames.last().nboCharges, QVector<double>({-0.32, 0.32}));
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
