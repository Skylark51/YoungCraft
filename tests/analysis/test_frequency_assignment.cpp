#include "analysis/FrequencyAssignment.h"

#include <QtTest>
#include <limits>

using namespace youngcraft;

namespace {
CalculationData diatomic() {
    CalculationData data;
    data.molecule.atoms = {{7, 6, "C", {0, 0, 0}}, {12, 8, "O", {1.2f, 0, 0}}};
    data.molecule.setBond(0, 1, 2);
    FrequencyMode mode;
    mode.number = 4;
    mode.wavenumber = 1234.567;
    mode.displacement = {{-.2f, 0, 0}, {.2f, 0, 0}};
    data.frequencies = {mode};
    return data;
}
}

class FrequencyAssignmentTest final : public QObject {
    Q_OBJECT
private slots:
    void stretchingAndImaginary();
    void coupledStretching();
    void bendingAndLinearBending();
    void missingAndInvalidDisplacements();
    void followsPlaybackGeometryAndInfersMissingBonds();
    void rigidMotionDoesNotBecomeStretching();
    void handlesInvalidBondsAndEmptyData();
};

void FrequencyAssignmentTest::stretchingAndImaginary() {
    auto data = diatomic();
    auto annotation = frequencyAnnotations(data).first();
    QCOMPARE(annotation.label, QStringLiteral("1234.567 cm⁻¹ (C7–O12)"));
    QVERIFY(annotation.tooltip.contains(QStringLiteral("모드 4")));
    QVERIFY(annotation.tooltip.contains(QStringLiteral("자동 추정")));
    data.frequencies[0].wavenumber = -123.4;
    annotation = frequencyAnnotations(data).first();
    QCOMPARE(annotation.label, QStringLiteral("-123.400 cm⁻¹ (C7–O12) [허수]"));
    data.frequencies[0].wavenumber = 1;
    data.frequencies[0].imaginary = true;
    QVERIFY(frequencyAnnotations(data).first().label.endsWith(QStringLiteral("[허수]")));
}

void FrequencyAssignmentTest::coupledStretching() {
    auto data = diatomic();
    data.molecule.atoms.append({15, 8, "O", {-1.2f, 0, 0}});
    data.molecule.setBond(0, 2, 2);
    data.frequencies[0].displacement = {{0, 0, 0}, {.2f, 0, 0}, {-.2f, 0, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("C7–O12, C7–O15"));
    // Reversing a mode's phase cannot change its assignment.
    for (auto &d : data.frequencies[0].displacement) d = -d;
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("C7–O12, C7–O15"));
}

void FrequencyAssignmentTest::bendingAndLinearBending() {
    auto data = diatomic();
    data.molecule.atoms = {{1, 8, "O", {0, 0, 0}}, {2, 1, "H", {1, 0, 0}}, {3, 1, "H", {0, 1, 0}}};
    data.molecule.bonds.clear();
    data.molecule.setBond(0, 1, 1);
    data.molecule.setBond(0, 2, 1);
    data.frequencies[0].displacement = {{0, 0, 0}, {0, .1f, 0}, {.1f, 0, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("H2–O1–H3 굽힘"));
    data.molecule.atoms[2].position = {-1, 0, 0};
    data.frequencies[0].displacement = {{0, -.2f, 0}, {0, .1f, 0}, {0, .1f, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("H2–O1–H3 굽힘"));
}

void FrequencyAssignmentTest::missingAndInvalidDisplacements() {
    auto data = diatomic();
    data.frequencies[0].displacement.clear();
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("변위 정보 없음"));
    data.frequencies[0].displacement = {{.1f, 0, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("변위 정보 없음"));
    data.frequencies[0].displacement = {{}, {std::numeric_limits<float>::quiet_NaN(), 0, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("변위 정보 없음"));
    data.frequencies[0].displacement = {{}, {}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("변위 없음"));
    data.frequencies[0].wavenumber = kNaN;
    QVERIFY(frequencyAnnotations(data).first().label.startsWith(QStringLiteral("— cm⁻¹")));
}

void FrequencyAssignmentTest::followsPlaybackGeometryAndInfersMissingBonds() {
    auto data = diatomic();
    data.molecule.bonds.clear();
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("C7–O12"));
    QVERIFY(data.molecule.bonds.isEmpty()); // Annotation never edits source data.
    Frame frame;
    frame.coordinates = {{0, 0, 0}, {0, 1.2f, 0}};
    data.frames = {frame};
    data.frequencies[0].displacement = {{0, -.1f, 0}, {0, .1f, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("C7–O12"));
    // Mismatched frame coordinates must not remove atoms or crash.
    data.frames[0].coordinates = {{0, 0, 0}};
    data.frequencies[0].displacement = {{-.1f, 0, 0}, {.1f, 0, 0}};
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("C7–O12"));
}

void FrequencyAssignmentTest::rigidMotionDoesNotBecomeStretching() {
    auto data = diatomic();
    data.frequencies[0].displacement = {{.1f, 0, 0}, {.1f, 0, 0}};
    QVERIFY(frequencyAnnotations(data).first().atoms.startsWith(QStringLiteral("주요 원자:")));
    // Infinitesimal rigid rotation preserves bond lengths and angles.
    data.molecule.atoms.append({15, 8, "O", {0, 1.2f, 0}});
    data.molecule.setBond(0, 2, 1);
    data.frequencies[0].displacement = {{0, 0, 0}, {0, .1f, 0}, {-.1f, 0, 0}};
    QVERIFY(frequencyAnnotations(data).first().atoms.startsWith(QStringLiteral("주요 원자:")));
}

void FrequencyAssignmentTest::handlesInvalidBondsAndEmptyData() {
    QVERIFY(frequencyAnnotations({}).isEmpty());
    auto data = diatomic();
    data.molecule.bonds.append({-1, 20});
    data.molecule.bonds.append({0, 0});
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("C7–O12"));
    data.molecule.atoms[1].position = {};
    QVERIFY(frequencyAnnotations(data).first().atoms.startsWith(QStringLiteral("주요 원자:")));
    data.molecule.atoms.clear();
    QCOMPARE(frequencyAnnotations(data).first().atoms, QStringLiteral("변위 정보 없음"));
}

QTEST_GUILESS_MAIN(FrequencyAssignmentTest)
#include "test_frequency_assignment.moc"
