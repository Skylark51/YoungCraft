#include "io/GaussianParser.h"

#include <QFile>
#include <QDir>
#include <cmath>
#include <QTemporaryFile>
#include <QtTest>

using namespace youngcraft;

class GaussianParserTest final : public QObject {
    Q_OBJECT
private slots:
    void extractsGeometryEnergyFrequencyAndProperties();
    void parsesScanAndIrcFixture();
    void toleratesIncompleteOutput();
    void relaxedScan_data();
    void relaxedScan();
    void scanOffsetAndMissingGeometry();
    void forceBoundaryAndOrientation();
    void scanCoordinatesAndJobs();
    void realScanInputs();
    void scanOptimizationHistory();
};

void GaussianParserTest::extractsGeometryEnergyFrequencyAndProperties() {
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray fixture = R"OUT(#p B3LYP/6-31G(d) opt freq TD(NStates=2)

 Test molecule

 Charge =  0 Multiplicity = 1
 Standard orientation:
 ---------------------------------------------------------------------
 Center     Atomic      Atomic             Coordinates (Angstroms)
 Number     Number       Type             X           Y           Z
 ---------------------------------------------------------------------
      1          8           0        0.000000    0.000000    0.000000
      2          1           0        0.000000    0.000000    0.960000
      3          1           0        0.920000    0.000000   -0.240000
 ---------------------------------------------------------------------
 SCF Done:  E(RB3LYP) =  -75.123456789     A.U. after   8 cycles
 Low frequencies ----120.50   -7.0   -5.0    0.01    0.02    0.03
 Low frequencies ---    4.0   19.0   27.0
 Frequencies --  -120.50  1550.25  3650.10
 Red. masses --  1.0  1.0  1.0
 Frc consts  --  0.1  0.2  0.3
 IR Inten    --  12.0  2.0  30.0
 Raman Activ --  4.0  5.0  6.0
 Atom  AN      X      Y      Z        X      Y      Z        X      Y      Z
 ---------------------------------------------------------------------
    1   8     0.01 0.02 0.03  0.04 0.05 0.06  0.07 0.08 0.09
    2   1     0.02 0.03 0.04  0.05 0.06 0.07  0.08 0.09 0.10
    3   1     0.03 0.04 0.05  0.06 0.07 0.08  0.09 0.10 0.11

 Mulliken charges:
               1
     1  O   -0.80000
     2  H    0.40000
     3  H    0.40000
 Sum of Mulliken charges =   0.00000
 Mulliken atomic spin densities:
               1
     1  O    0.10000
     2  H    0.00000
     3  H    0.00000
 Sum of Mulliken spin densities =   0.10000
 Dipole moment (field-independent basis, Debye):
     X=      1.0000    Y=      2.0000    Z=      3.0000  Tot=      3.7417
 Excited State   1: Singlet-A  3.50 eV  354.2 nm  f=0.1234
      5 -> 6        0.70
 Normal termination of Gaussian 16
)OUT";
    QCOMPARE(file.write(fixture), fixture.size());
    file.flush();
    const ParseResult result = GaussianParser::parse(file.fileName());
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.data.molecule.atoms.size(), 3);
    QCOMPARE(result.data.molecule.charge, 0);
    QCOMPARE(result.data.molecule.multiplicity, 1);
    QCOMPARE(result.data.frames.size(), 1);
    QCOMPARE(result.data.scfEnergies.size(), 1);
    QCOMPARE(result.data.frequencies.size(), 3);
    QVERIFY(result.data.frequencies.first().imaginary);
    QCOMPARE(result.data.frequencies[0].displacement[0], QVector3D(0.01f, 0.02f, 0.03f));
    QCOMPARE(result.data.frequencies[1].displacement[1], QVector3D(0.05f, 0.06f, 0.07f));
    QCOMPARE(result.data.frequencies[2].displacement[2], QVector3D(0.09f, 0.10f, 0.11f));
    QCOMPARE(result.data.excitedStates.size(), 1);
    QCOMPARE(result.data.excitedStates.first().transitions.size(), 1);
    QVERIFY(result.data.hasDipole);
    QVERIFY(result.data.hasNbo == false);
    QCOMPARE(result.data.status, CalculationStatus::NormalTermination);
    QCOMPARE(result.data.molecule.atoms.first().charge, -0.8);
}

void GaussianParserTest::toleratesIncompleteOutput() {
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.write("Charge = 0 Multiplicity = 1\nStandard orientation:\n") > 0);
    file.flush();
    const ParseResult result = GaussianParser::parse(file.fileName());
    QVERIFY(!result.success || result.data.status != CalculationStatus::NormalTermination);
}

void GaussianParserTest::parsesScanAndIrcFixture() {
    const QString path = QFINDTESTDATA("tests/data/mvp.out");
    QVERIFY2(!path.isEmpty(), "tests/data/mvp.out를 찾지 못했습니다.");
    const ParseResult result = GaussianParser::parse(path);
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.data.frames.size(), 2);
    QCOMPARE(result.data.scanPoints.size(), 2);
    QCOMPARE(result.data.ircPoints.size(), 2);
    QCOMPARE(result.data.scanPoints.first().frameIndex, 0);
    QVERIFY(result.data.scanPoints.first().relativeKcal > 0.0);
    QVERIFY(qFuzzyIsNull(result.data.scanPoints.last().relativeKcal));
}

void GaussianParserTest::relaxedScan_data() {
    QTest::addColumn<QByteArray>("summary");
    QTest::addColumn<bool>("knownCoordinate");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("final-only") << QByteArray(" 1 2\n Eigenvalues -- -100.1-100.2\n R1 1.8 1.9\n") << true << 2;
    QTest::newRow("preceding-Link1") << QByteArray(" 1 2\n Eigenvalues -- -100.1-100.2\n R1 1.8 1.9\n") << true << 2;
    QTest::newRow("multi-block-D-E") << QByteArray(" 1\n Eigenvalues -- -1.001D+02\n R1 1.8\n\n 2\n Eigenvalues -- -1.002E+02\n R1 1.9\n") << true << 2;
    QTest::newRow("unknown-coordinate") << QByteArray(" 1 2\n Eigenvalues -- -100.1-100.2\n R1 1.8 1.9\n") << false << 2;
    QTest::newRow("invalid-slot") << QByteArray(" 1 2\n Eigenvalues -- *****-100.2\n R1 1.8 1.9\n") << true << 1;
    QTest::newRow("invalid-coordinate") << QByteArray(" 1 2\n Eigenvalues -- -100.1-100.2\n R1 ***** 1.9\n") << true << 1;
    QTest::newRow("incomplete") << QByteArray() << true << 3;
}

void GaussianParserTest::relaxedScan() {
    QFETCH(QByteArray, summary);
    QFETCH(bool, knownCoordinate);
    QFETCH(int, expectedCount);
    QByteArray fixture("#p opt=modredundant\n");
    const auto orientation = [](int index) {
        return QByteArray(" Standard orientation:\n -----\n Center Atomic Atomic Coordinates\n Number Number Type X Y Z\n -----\n 1 1 0 0 0 0\n 2 1 0 0 0 ")
            + QByteArray::number(index + 1) + "\n -----\n";
    };
    const QByteArray energies[] = {"-99", "-100.1", "-99.5", "-100.2"};
    // A previous Link1 optimization is excluded by the last-N summary mapping.
    const int frameOffset = QByteArray(QTest::currentDataTag()) == "preceding-Link1" ? 1 : 0;
    if (frameOffset) fixture += orientation(8) + " SCF Done: E(RHF) = -90 A.U.\n Optimization completed.\n --Link1--\n";
    if (knownCoordinate) fixture += " ! R1 R(1,2) 1.8 Scan !\n";
    else fixture += " PES scan\n";
    for (int i = 0; i < 4; ++i) {
        fixture += orientation(i) + " SCF Done: E(RHF) = " + energies[i] + " A.U.\n";
        fixture += " Maximum Force 0.0001 0.0005 YES\n";
        if (i % 2 == 1) fixture += " Optimization completed.\n";
    }
    if (!summary.isEmpty()) fixture += " Summary of Optimized Potential Surface Scan\n\n" + summary;
    // A later, unfinished iteration is visible as an in-progress scan point.
    if (summary.isEmpty()) fixture += orientation(4) + " SCF Done: E(RHF) = -98 A.U.\n";
    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write(fixture), fixture.size());
    file.flush();
    const auto result = GaussianParser::parse(file.fileName());
    QVERIFY(result.success);
    QCOMPARE(result.data.scfEnergies.size(), 4 + frameOffset + (summary.isEmpty() ? 1 : 0));
    QCOMPARE(result.data.scanPoints.size(), expectedCount);
    QCOMPARE(result.data.scanAtoms, knownCoordinate ? QVector<int>({0, 1}) : QVector<int>());
    for (int i = 0; i < (summary.isEmpty() ? expectedCount - 1 : expectedCount); ++i) {
        const int ordinal = expectedCount == 1 ? 1 : i;
        const auto &point = result.data.scanPoints[i];
        QCOMPARE(point.energyHartree, ordinal == 0 ? -100.1 : -100.2);
        QCOMPARE(point.frameIndex, frameOffset + ordinal * 2 + 1);
        QCOMPARE(result.data.frames[point.frameIndex].coordinates[1].z(), float(ordinal * 2 + 2));
        QCOMPARE(point.coordinate, knownCoordinate ? (!summary.isEmpty() ? (ordinal == 0 ? 1.8 : 1.9) : double(ordinal * 2 + 2)) : double(ordinal + 1));
    }
    QCOMPARE(result.data.scanPoints[summary.isEmpty() ? 1 : expectedCount - 1].relativeKcal, 0.0);
    if (summary.isEmpty()) {
        const auto &pending = result.data.scanPoints.last();
        QVERIFY(pending.inProgress);
        QCOMPARE(pending.energyHartree, -98.0);
        QCOMPARE(pending.optimizationFrames, QVector<int>({4}));
    }
}

void GaussianParserTest::scanOptimizationHistory() {
    const QByteArray block(" Standard orientation:\n -----\n Center Atomic Atomic Coordinates\n Number Number Type X Y Z\n -----\n 1 1 0 0 0 0\n 2 1 0 0 0 2\n -----\n");
    const auto parse = [](const QByteArray &text) {
        QTemporaryFile file;
        if (!file.open()) return ParseResult{};
        file.write(text); file.flush();
        return GaussianParser::parse(file.fileName());
    };
    const QByteArray prior = block + " SCF Done: E(RHF) = -50 A.U.\n Optimization completed.\n --Link1--\n";
    const QByteArray scan = " ! R1 R(1,2) 2 Scan !\n" + block + " SCF Done: E(RHF) = -9 A.U.\n" +
        block + " SCF Done: E(RHF) = -10 A.U.\n Optimization completed.\n";
    const QByteArray pending = block + " SCF Done: E(RHF) = -8 A.U.\n" + block + " SCF Done: E(RHF) = -8.5 A.U.\n";
    auto r = parse(prior + scan + pending);
    QCOMPARE(r.data.scanPoints.size(), 2);
    QCOMPARE(r.data.scanPoints[0].optimizationFrames, QVector<int>({1, 2}));
    QCOMPARE(r.data.scanPoints[1].optimizationFrames, QVector<int>({3, 4}));
    QVERIFY(!r.data.scanPoints[0].inProgress);
    QVERIFY(r.data.scanPoints[1].inProgress);
    QCOMPARE(r.data.scanPoints[1].frameIndex, 4);
    QVERIFY(r.data.scanStatusText().contains(QStringLiteral("진행 중")));
    r = parse(prior + scan + pending + " Optimization completed.\n Summary of Optimized Potential Surface Scan\n 1 2\n Eigenvalues -- -10 -8.5\n R1 2 2\n");
    QCOMPARE(r.data.scanPoints.size(), 2);
    QCOMPARE(r.data.scanPoints[1].optimizationFrames, QVector<int>({3, 4}));
    QVERIFY(!r.data.scanPoints[1].inProgress);
    r = parse(prior + scan + " --Link1--\n" + pending);
    QCOMPARE(r.data.scanPoints.size(), 1);
    QVERIFY(!r.data.scanPoints.last().inProgress);
    r = parse(" ! R1 R(1,2) 2 Scan !\n" + pending);
    QCOMPARE(r.data.scanPoints.size(), 1);
    QVERIFY(r.data.scanPoints.first().inProgress);
    QCOMPARE(r.data.scanPoints.first().optimizationFrames, QVector<int>({0, 1}));
    r = parse(" ! R1 R(1,2) 2 Scan !\n" + block);
    QCOMPARE(r.data.scanPoints.size(), 1);
    QVERIFY(r.data.scanPoints.first().inProgress);
    QCOMPARE(r.data.scanPoints.first().frameIndex, 0);
    QVERIFY(std::isnan(r.data.scanPoints.first().energyHartree));
    r = parse(" ! R1 R(1,2) 2 Scan !\n ! R2 R(2,1) 2 Scan !\n" + pending);
    QCOMPARE(r.data.scanDimensions, 2);
    QCOMPARE(r.data.scanPoints.size(), 1);
    QVERIFY(r.data.scanPoints.first().inProgress);
    QCOMPARE(r.data.scanPoints.first().coordinate, 2.0);
    QCOMPARE(r.data.scanPoints.first().coordinate2, 2.0);
}

void GaussianParserTest::scanOffsetAndMissingGeometry() {
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(" ! R1 R(1,2) 1.0 Scan !\n Summary of Optimized Potential Surface Scan (add -1234.56789 to energies):\n 1 2 3 4\n Eigenvalues -- -1.0D-01*****-2.0E-01\n R1 1.0 2.0 3.0 4.0\n Normal termination of Gaussian\n");
    file.flush();
    const auto r = GaussianParser::parse(file.fileName());
    QVERIFY(r.success);
    QVERIFY(r.data.scanDetected);
    QCOMPARE(r.data.scanPoints.size(), 2);
    QCOMPARE(r.data.scanPoints[0].energyHartree, -1234.66789);
    QCOMPARE(r.data.scanPoints[1].energyHartree, -1234.76789);
    QCOMPARE(r.data.scanPoints[1].index, 3);
    QCOMPARE(r.data.scanPoints[1].coordinate, 3.0);
    for (const auto &point : r.data.scanPoints) QCOMPARE(point.frameIndex, -1);
    QVERIFY(r.data.frames.isEmpty());
}

void GaussianParserTest::forceBoundaryAndOrientation() {
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray block(" -----\n Center Atomic Atomic Coordinates\n Number Number Type X Y Z\n -----\n 1 1 0 0 0 0\n 2 1 0 0 0 2\n -----\n");
    file.write(" ! R1 R(1,2) 2 Scan !\n Input orientation:\n" + block +
               " Standard orientation:\n" + block + " SCF Done: E(RHF) = -10 A.U.\n Center Atomic Forces (Hartrees/Bohr)\n Number Number X Y Z\n -----\n 1 1 0.1 0.2 0.3\n 2 1 0.2 0.3 0.4\n -----\n Maximum Force 0.3\n Optimization completed.\n Input orientation:\n" + block);
    file.flush();
    const auto r = GaussianParser::parse(file.fileName());
    QCOMPARE(r.data.frames.size(), 2);
    QCOMPARE(r.data.scanPoints.size(), 1);
    QCOMPARE(r.data.scanPoints[0].frameIndex, 0);
    QCOMPARE(r.data.scanPoints[0].energyHartree, -10.0);
    QCOMPARE(r.data.scanPoints[0].coordinate, 2.0);
    QVERIFY(std::isnan(r.data.frames[1].metrics.energyHartree));
    QVERIFY(std::isnan(r.data.frames[1].metrics.maxForce));
    QTemporaryFile nonScan(QDir::tempPath() + "/scan-XXXXXX.out");
    QVERIFY(nonScan.open());
    nonScan.write(" %chk=C:/scan/test.chk\n Input orientation:\n" + block + " SCF Done: E(RHF) = -10 A.U.\n Optimization completed.\n");
    nonScan.flush();
    const auto ordinary = GaussianParser::parse(nonScan.fileName());
    QVERIFY(!ordinary.data.scanDetected);
    QVERIFY(ordinary.data.scanPoints.isEmpty());
}

void GaussianParserTest::realScanInputs() {
    const QString directory = qEnvironmentVariable("YOUNGCRAFT_SCAN_INPUT_DIR");
    if (directory.isEmpty()) QSKIP("Set YOUNGCRAFT_SCAN_INPUT_DIR for read-only real-file validation");
    const auto paths = QDir(directory).entryInfoList({"*scan*.out"}, QDir::Files, QDir::Name);
    QVERIFY(!paths.isEmpty());
    for (const auto &path : paths) {
        const auto r = GaussianParser::parse(path.absoluteFilePath());
        QVERIFY2(r.success, qPrintable(path.fileName() + ": " + r.error));
        QVERIFY(r.data.scanDetected);
        QVERIFY(!r.data.scanPoints.isEmpty());
        int valid = 0;
        for (const auto &point : r.data.scanPoints) {
            QVERIFY(std::isfinite(point.energyHartree));
            QVERIFY(std::isfinite(point.coordinate));
            QVERIFY(point.frameIndex >= -1 && point.frameIndex < r.data.frames.size());
            if (point.frameIndex >= 0) {
                ++valid;
                QVERIFY(std::abs(r.data.frames[point.frameIndex].metrics.energyHartree - point.energyHartree) < 0.000006);
            }
        }
        qInfo().noquote() << path.fileName() << "| detected |" << r.data.scanPoints.size()
                         << (r.data.scanHasSummary ? "| summary |" : "| partial |")
                         << r.data.scanAxisLabel << "| valid frames" << valid << "/" << r.data.scanPoints.size();
    }
}

void GaussianParserTest::scanCoordinatesAndJobs() {
    const QByteArray block(" Input orientation:\n -----\n Center Atomic Atomic Coordinates\n Number Number Type X Y Z\n -----\n 1 1 0 1 0 0\n 2 1 0 0 0 0\n 3 1 0 0 1 0\n 4 1 0 0 1 1\n -----\n");
    for (const QByteArray &definition : {QByteArray(" ! A1 A(1,2,3) 90 Scan !\n"), QByteArray(" ! D1 D(1,2,3,4) 90 Scan !\n"), QByteArray(" A 1 2 3 S 5 10.0\n")}) {
        QTemporaryFile file; QVERIFY(file.open());
        file.write(block + " SCF Done: E(RHF) = -30 A.U.\n Optimization completed.\n --Link1--\n" + definition + block + " SCF Done: E(RHF) = -10 A.U.\n Optimization completed.\n");
        file.flush();
        const auto r = GaussianParser::parse(file.fileName());
        QCOMPARE(r.data.frames.size(), 2);
        QCOMPARE(r.data.scanPoints.size(), 1);
        QCOMPARE(r.data.scanPoints[0].frameIndex, 1);
        QVERIFY(std::abs(std::abs(r.data.scanPoints[0].coordinate) - 90.0) < 0.0001);
        QVERIFY(r.data.scanAxisLabel.contains("degree"));
    }
    QTemporaryFile missing; QVERIFY(missing.open());
    missing.write(" Summary of Optimized Potential Surface Scan\n                           1         2         3\n     Eigenvalues --    -0.81418            -0.79979\n");
    missing.flush();
    const auto r = GaussianParser::parse(missing.fileName());
    QCOMPARE(r.data.scanPoints.size(), 2);
    QCOMPARE(r.data.scanPoints[1].index, 3);
    QCOMPARE(r.data.scanPoints[1].energyHartree, -0.79979);
}

QTEST_APPLESS_MAIN(GaussianParserTest)
#include "test_gaussian_parser.moc"
