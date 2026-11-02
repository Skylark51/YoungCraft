#include "model/StructureTools.h"
#include "model/GeometryEditor.h"
#include <QtTest>
#include <QRegularExpression>
using namespace youngcraft;
class StructureToolsTest : public QObject {
    Q_OBJECT
private slots:
    void selectionAndComponents() {
        Molecule m; DrawState state;
        for(int i=0;i<5;++i)MoleculeEditController::addAtom(m,{float(i),0,0},state);
        m.setBond(0,1,1);m.setBond(1,2,2);m.setBond(3,4,1,true,BondStyle::Dashed);
        QCOMPARE(StructureTools::expand(m,{0}),QSet<int>({0,1}));
        QCOMPARE(StructureTools::expand(m,{0},true),QSet<int>({0,1,2}));
        QCOMPARE(StructureTools::invert(m,{0,2,99}),QSet<int>({1,3,4}));
        QVERIFY(StructureTools::expand(m,{}).isEmpty());
        QCOMPARE(StructureTools::components(m).size(),2);
        auto f=StructureTools::fragment(m,{2,3,4});QCOMPARE(f.atoms.size(),3);QCOMPARE(f.bonds.size(),1);
        QCOMPARE(f.bonds[0].first,1);QCOMPARE(f.bonds[0].second,2);QVERIFY(f.bonds[0].coordination);
    }
    void clipboardRoundTripAndRemapping() {
        Molecule m;m.atoms={{1,26,"Fe",{}},{2,7,"N",{2,0,0}},{3,6,"C",{3,1,0}},{4,6,"C",{3,-1,0}}};
        m.atoms[1].formalCharge=1;m.atoms[1].charge=.25;m.atoms[1].label="N donor";m.atoms[1].colorOverride=QColor("red");
        m.atoms[2].autoHydrogenLocked=true;m.setBond(0,1,1,true,BondStyle::Dashed);m.setBond(2,3,1.5);
        m.coordinationSites<<CoordinationSite{0,{1,0,0},true,1};m.hapticContacts<<HapticContact{0,{2,3},"eta2"};m.charge=2;m.multiplicity=5;
        QCOMPARE(StructureTools::components(m).size(),1);
        QCOMPARE(StructureTools::expand(m,{0},true).size(),4);
        auto f=StructureTools::fragment(m,{0,2,3});QCOMPARE(f.coordinationSites[0].donor,-1);QVERIFY(!f.coordinationSites[0].occupied);
        QCOMPARE(f.hapticContacts[0].atoms,QVector<int>({1,2}));
        auto bytes=StructureTools::encode(m);Molecule decoded;QString error;QVERIFY2(StructureTools::decode(bytes,decoded,&error),qPrintable(error));
        QCOMPARE(StructureTools::encode(decoded),bytes);QCOMPARE(decoded.atoms[2].autoHydrogenLocked,true);
        Molecule combined=m;QVERIFY(StructureTools::append(combined,decoded,{10,2,1},&error));
        QCOMPARE(combined.atoms.size(),8);QCOMPARE(combined.bonds[2].first,4);QCOMPARE(combined.bonds[2].second,5);
        QCOMPARE(combined.coordinationSites[1].donor,5);QCOMPARE(combined.hapticContacts[1].atoms,QVector<int>({6,7}));
        QCOMPARE(combined.atoms[5].position,decoded.atoms[1].position+QVector3D(10,2,1));QCOMPARE(combined.charge,4);
        const auto original=StructureTools::encode(decoded);
        QVERIFY(!StructureTools::decode(bytes.left(bytes.size()-3),decoded,&error));QCOMPARE(StructureTools::encode(decoded),original);
        QVERIFY(!StructureTools::decode("invalid",decoded,&error));
        QVERIFY(!StructureTools::decode(bytes+"garbage",decoded,&error));
    }
    void internalCoordinates() {
        Molecule m;m.atoms={{1,6,"C",{}},{2,6,"C",{1.5f,0,0}},{3,8,"O",{1.7f,1.2f,0}},{4,1,"H",{2.1f,1.5f,1.0f}}};
        m.setBond(0,1,1);m.setBond(1,2,1);m.setBond(2,3,1);
        QString error;auto text=StructureTools::zMatrix(m,&error);QVERIFY2(!text.isEmpty(),qPrintable(error));
        const auto lines=text.trimmed().split('\n');QCOMPARE(lines.size(),4);
        for(int i=0;i<lines.size();++i){auto fields=lines[i].split(QRegularExpression("\\s+"));QCOMPARE(fields.size(),1+2*qMin(i,3));
            if(i>0){int a=fields[1].toInt()-1;QVERIFY(a<i);QVERIFY(qAbs(fields[2].toDouble()-GeometryEditor::distance(m,i,a))<1e-7);}
            if(i>1){int a=fields[1].toInt()-1,b=fields[3].toInt()-1;QVERIFY(a!=b);QVERIFY(b<i);QVERIFY(qAbs(fields[4].toDouble()-GeometryEditor::angleDegrees(m,i,a,b))<1e-7);}
            if(i>2){int a=fields[1].toInt()-1,b=fields[3].toInt()-1,c=fields[5].toInt()-1;QVERIFY(c<i&&c!=a&&c!=b);QVERIFY(qAbs(fields[6].toDouble()-GeometryEditor::dihedralDegrees(m,i,a,b,c))<1e-7);}
        }
        // A truly linear molecule must fail explicitly rather than export NaN torsions.
        for(int i=0;i<m.atoms.size();++i)m.atoms[i].position={float(i),0,0};
        QVERIFY(StructureTools::zMatrix(m,&error).isEmpty());QVERIFY(!error.isEmpty());
        m.atoms.resize(2);m.bonds.clear();QVERIFY(!StructureTools::zMatrix(m,&error).isEmpty());
        m.atoms[1].position={};QVERIFY(StructureTools::zMatrix(m,&error).isEmpty());
    }
    void gaussianInputValidation() {
        Molecule water;water.title="water\nsecond line";water.atoms={{1,8,"O",{}},{2,1,"H",{.95f,0,0}},{3,1,"H",{-.3f,.9f,0}}};
        QString error;const auto input=StructureTools::gaussianInput(water,"#p B3LYP/6-31G(d) opt",0,1,&error);
        QVERIFY2(!input.isEmpty(),qPrintable(error));QVERIFY(input.startsWith("#p B3LYP/6-31G(d) opt\n\nwater second line\n\n0 1\nO"));QVERIFY(input.endsWith("\n\n"));
        QVERIFY(StructureTools::gaussianInput(water,"#p HF/STO-3G",0,2,&error).isEmpty());
        QVERIFY(!StructureTools::gaussianInput(water,"#p HF/STO-3G",1,2,&error).isEmpty());
        for(const auto &route:{"B3LYP", "#p HF/gen", "#p HF/genecp", "#p geom=allcheck", "#p HF --Link1--"})QVERIFY(StructureTools::gaussianInput(water,route,0,1,&error).isEmpty());
    }
};
QTEST_GUILESS_MAIN(StructureToolsTest)
#include "test_structure_tools.moc"
