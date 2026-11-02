#include "io/FileDispatcher.h"

#include <QtTest>

using namespace youngcraft;

class StructureTest final : public QObject {
    Q_OBJECT
private slots:
    void cifCellAndSymmetry();
    void molecularFormats();
};

void StructureTest::cifCellAndSymmetry() {
    const auto p1 = FileDispatcher::load(QStringLiteral("tests/data/structure/p1.cif"));
    QVERIFY2(p1.parseResult.success, qPrintable(p1.parseResult.error));
    QVERIFY(p1.crystal);
    QCOMPARE(p1.crystal->unitCell.atoms.size(), 2);
    QCOMPARE(p1.crystal->packing.atoms.size(), 16);
    QCOMPARE(p1.parseResult.data.molecule.atoms[0].atomicNumber, 6);
    QCOMPARE(p1.parseResult.data.molecule.atoms[0].occupancy, 0.5);
    QCOMPARE(p1.parseResult.data.molecule.atoms[0].disorderGroup, QStringLiteral("1"));
    QCOMPARE(p1.parseResult.data.molecule.bonds[0].order, 2.0);
    QVERIFY(qAbs(p1.parseResult.data.molecule.atoms[0].position.x() - 7.5f) < 0.001f);
    QVERIFY(qAbs(p1.parseResult.data.molecule.atoms[0].position.y() - 4.330127f) < 0.001f);
    const auto inversion = FileDispatcher::load(QStringLiteral("tests/data/structure/inversion.cif"));
    QVERIFY2(inversion.parseResult.success, qPrintable(inversion.parseResult.error));
    QCOMPARE(inversion.crystal->unitCell.atoms.size(), 2);
    QCOMPARE(inversion.crystal->unitCell.atoms[0].atomicNumber, 26);
    const auto number = FileDispatcher::load(QStringLiteral("tests/data/structure/number_only.mmcif"));
    QVERIFY2(number.parseResult.success, qPrintable(number.parseResult.error));
    QCOMPARE(number.crystal->unitCell.atoms.size(), 2);
    QCOMPARE(number.parseResult.data.molecule.atoms[0].atomicNumber, 17);
    const auto boundary = FileDispatcher::load(QStringLiteral("tests/data/structure/boundary.cif"));
    QVERIFY2(boundary.parseResult.success, qPrintable(boundary.parseResult.error));
    QCOMPARE(boundary.crystal->unitCell.bonds.size(), 0);
    QCOMPARE(boundary.crystal->packing.bonds.size(), 4);
}

void StructureTest::molecularFormats() {
    struct Expected { const char *file; int firstElement; double order; bool crystal; };
    for (const Expected &expected : {Expected{"sample.mol", 6, 2, false},
                                     {"sample.sdf", 8, 1, false},
                                     {"sample.mol2", 6, 2, false},
                                     {"sample.pdb", 6, 1, false},
                                     {"sample.res", 6, 1, true},
                                     {"sample.cssr", 6, 1, true}}) {
        const auto loaded = FileDispatcher::load(QStringLiteral("tests/data/structure/") +
                                                 QString::fromLatin1(expected.file));
        QVERIFY2(loaded.parseResult.success, qPrintable(QString::fromLatin1(expected.file) + ": " + loaded.parseResult.error));
        QCOMPARE(loaded.parseResult.data.molecule.atoms.size(), 2);
        QCOMPARE(loaded.parseResult.data.molecule.atoms[0].atomicNumber, expected.firstElement);
        QCOMPARE(bool(loaded.crystal), expected.crystal);
        QVERIFY(!loaded.parseResult.data.molecule.bonds.isEmpty() || expected.crystal);
        if (!expected.crystal) QCOMPARE(loaded.parseResult.data.molecule.bonds[0].order, expected.order);
    }
}

QTEST_MAIN(StructureTest)
#include "test_structure.moc"
