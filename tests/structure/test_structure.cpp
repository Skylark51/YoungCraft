#include "io/FileDispatcher.h"
#include "io/CrystalContacts.h"

#include <QtTest>
#include <stdexcept>
#include <QTemporaryFile>

using namespace youngcraft;

class StructureTest final : public QObject {
    Q_OBJECT
private slots:
    void cifCellAndSymmetry();
    void unknownSymmetryIsNotAssumedP1();
    void dualCoordinateColumns();
    void periodicContacts();
    void molecularFormats();
    void invalidCifCoordinates();
    void cifRoundTrip();
    void cifMetadataAndAtomicFailure();
};

void StructureTest::periodicContacts() {
    const auto loaded = FileDispatcher::load("tests/data/structure/boundary.cif");
    QVERIFY(loaded.parseResult.success);
    const auto contacts = crystalContacts(*loaded.crystal, 2.0);
    QCOMPARE(contacts.size(), 1);
    QVERIFY(qAbs(contacts[0].distance - 1.6) < 1e-5);
    QCOMPARE(contacts[0].translation[0], 1);
    // Short cell: include the second lattice neighbor, without inverse duplicates.
    CrystalData shortCell;
    shortCell.a = 2; shortCell.b = 10; shortCell.c = 10;
    Atom atom; atom.position = {0, 0, 0};
    shortCell.unitCell.atoms.push_back(atom);
    const auto repeated = crystalContacts(shortCell, 4.1);
    QCOMPARE(repeated.size(), 2);
    QCOMPARE(repeated[0].distance, 2.0);
    QCOMPARE(repeated[1].distance, 4.0);
    QVERIFY_EXCEPTION_THROWN(crystalContacts(shortCell, -1), std::runtime_error);
    shortCell.b = 2; shortCell.gamma = 30;
    const auto skew = crystalContacts(shortCell, 1.1);
    QCOMPARE(skew.size(), 1);
    QCOMPARE(skew[0].translation[0], 1);
    QCOMPARE(skew[0].translation[1], -1);
    QVERIFY(qAbs(skew[0].distance - 1.0352761804) < 1e-8);
}

void StructureTest::dualCoordinateColumns() {
    QTemporaryDir dir;
    const auto path = dir.filePath("dual.cif");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("data_dual\n_cell_length_a 10\n_cell_length_b 10\n_cell_length_c 10\n"
        "loop_\n_atom_site_label\n_atom_site_type_symbol\n"
        "_atom_site_fract_x\n_atom_site_fract_y\n_atom_site_fract_z\n"
        "_atom_site_Cartn_x\n_atom_site_Cartn_y\n_atom_site_Cartn_z\n"
        "C1 C 0.1 0.2 0.3 1 2 3\n");
    file.close();
    const auto loaded = FileDispatcher::load(path);
    QVERIFY(loaded.parseResult.success);
    auto edited = loaded.crystal->asymmetricUnit;
    edited.atoms[0].position = {2, 3, 4};
    QString error;
    QVERIFY2(StructureReader::saveCif(path, *loaded.crystal, edited, &error), qPrintable(error));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().contains("0.2 0.3 0.4 2 3 4"));
    const auto reloaded = FileDispatcher::load(path);
    QVERIFY(reloaded.parseResult.success);
    QCOMPARE(reloaded.crystal->asymmetricUnit.atoms[0].position, QVector3D(2, 3, 4));
}

void StructureTest::unknownSymmetryIsNotAssumedP1() {
    QTemporaryDir dir;
    const auto path = dir.filePath("unknown.cif");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("data_unknown\n_cell_length_a 10\n_cell_length_b 10\n_cell_length_c 10\n"
        "_space_group_name_H-M_alt 'not a space group'\n"
        "loop_\n_atom_site_label\n_atom_site_type_symbol\n"
        "_atom_site_fract_x\n_atom_site_fract_y\n_atom_site_fract_z\nC1 C 0.1 0.2 0.3\n");
    file.close();
    const auto rejected = FileDispatcher::load(path);
    QVERIFY(!rejected.parseResult.success);
    QVERIFY(rejected.parseResult.error.contains("space group"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Append));
    file.write("loop_\n_space_group_symop_operation_xyz\n'x,y,z'\n'-x,-y,-z'\n");
    file.close();
    const auto explicitOperations = FileDispatcher::load(path);
    QVERIFY2(explicitOperations.parseResult.success, qPrintable(explicitOperations.parseResult.error));
    QCOMPARE(explicitOperations.crystal->unitCell.atoms.size(), 2);
}

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

void StructureTest::cifRoundTrip() {
    for (const QString name : {"p1.cif", "inversion.cif", "boundary.cif", "number_only.mmcif"}) {
        const auto input = FileDispatcher::load("tests/data/structure/" + name);
        QVERIFY(input.parseResult.success);
        auto molecule = input.crystal->asymmetricUnit;
        molecule.atoms[0].position += QVector3D(0.1f, -0.2f, 0.3f);
        QTemporaryDir dir;
        const QString path = dir.filePath("saved.cif");
        QString error;
        QVERIFY2(StructureReader::saveCif(path, *input.crystal, molecule, &error), qPrintable(error));
        const auto rebuilt = StructureReader::rebuildCif(*input.crystal, molecule, &error);
        QVERIFY2(rebuilt, qPrintable(error));
        QVERIFY((rebuilt->asymmetricUnit.atoms[0].position - molecule.atoms[0].position).length() < 1e-5f);
        QCOMPARE(rebuilt->packing.atoms.size(), rebuilt->unitCell.atoms.size() * 8);
        const auto output = FileDispatcher::load(path);
        QVERIFY2(output.parseResult.success, qPrintable(output.parseResult.error));
        QCOMPARE(output.crystal->a, input.crystal->a);
        QCOMPARE(output.crystal->spaceGroup, input.crystal->spaceGroup);
        QCOMPARE(output.crystal->asymmetricUnit.atoms.size(), molecule.atoms.size());
        QVERIFY((output.crystal->asymmetricUnit.atoms[0].position - molecule.atoms[0].position).length() < 1e-5f);
        QCOMPARE(output.crystal->asymmetricUnit.atoms[0].occupancy, molecule.atoms[0].occupancy);
        auto unsupported = molecule;
        unsupported.atoms[0].formalCharge = 1;
        QVERIFY(!StructureReader::saveCif(path, *input.crystal, unsupported, &error));
        unsupported = molecule;
        if (unsupported.atoms.size() > 1) {
            unsupported.setBond(0, 1, 3.0);
            QVERIFY(!StructureReader::saveCif(path, *input.crystal, unsupported, &error));
        }
        molecule.atoms.removeLast();
        QVERIFY(!StructureReader::saveCif(path, *input.crystal, molecule, &error));
    }
}

void StructureTest::cifMetadataAndAtomicFailure() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString source = dir.filePath("metadata.cif");
    QFile fixture("tests/data/structure/inversion.cif");
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    const QByteArray original = fixture.readAll() +
        "\n_research_note 'preserve unknown metadata'\ndata_other\n_custom_value 42\n";
    QFile file(source);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();
    const auto input = FileDispatcher::load(source);
    QVERIFY(input.parseResult.success);
    auto edited = input.crystal->asymmetricUnit;
    edited.atoms[0].position += QVector3D(0.2f, 0.1f, -0.1f);
    QString error;
    const QString target = dir.filePath("saved.cif");
    QVERIFY2(StructureReader::saveCif(target, *input.crystal, edited, &error), qPrintable(error));
    QFile saved(target);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    const auto bytes = saved.readAll();
    saved.close();
    QVERIFY(bytes.contains("preserve unknown metadata"));
    QVERIFY(bytes.contains("data_other"));
    QVERIFY(bytes.contains("_custom_value 42"));
    const auto output = FileDispatcher::load(target);
    QVERIFY(output.parseResult.success);
    QCOMPARE(output.crystal->unitCell.atoms.size(), input.crystal->unitCell.atoms.size());
    QCOMPARE(output.crystal->alpha, input.crystal->alpha);
    QCOMPARE(output.crystal->beta, input.crystal->beta);
    QCOMPARE(output.crystal->gamma, input.crystal->gamma);
    edited.atoms[0].atomicNumber = 8;
    QVERIFY(!StructureReader::saveCif(target, *input.crystal, edited, &error));
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), bytes);
}

void StructureTest::invalidCifCoordinates() {
    for (const QByteArray value : {QByteArray("?"), QByteArray("."), QByteArray("invalid")}) {
        QTemporaryFile file(QDir::tempPath() + "/youngcraft-XXXXXX.cif");
        QVERIFY(file.open());
        file.write("data_invalid\n_cell_length_a 10\n_cell_length_b 10\n_cell_length_c 10\n"
                   "loop_\n_atom_site_label\n_atom_site_type_symbol\n"
                   "_atom_site_fract_x\n_atom_site_fract_y\n_atom_site_fract_z\nC1 C ");
        file.write(value + " 0.2 0.3\n");
        file.flush();
        const auto loaded = FileDispatcher::load(file.fileName());
        QVERIFY(!loaded.parseResult.success);
        QVERIFY(!loaded.parseResult.error.isEmpty());
    }
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

QTEST_GUILESS_MAIN(StructureTest)
#include "test_structure.moc"
