#include "io/GaussianParser.h"
#include "model/MoleculeEditController.h"
#include "model/FragmentLibrary.h"
#include "model/LigandReplacement.h"
#include "model/GeometryEditor.h"
#include "model/LabComplexBuilder.h"
#include <QTemporaryDir>
#include <QtTest>
#include <QQuaternion>
#include <cmath>
using namespace youngcraft;
class EditorTest : public QObject {
    Q_OBJECT
private slots:
    void ligandReplacementPreservesCore();
    void ligandReplacementRejectsUnsafeEdits();
    void drawingTransactions();
    void chargedHydrogens();
    void scopedHydrogens();
    void snappingAndAutomatic();
    void selectionAndRigidBody();
    void attachedFragmentRotation();
    void templates();
    void fragmentsAndLigands();
    void bondCentric();
    void invalidEdits();
    void approachPreservesFragments();
    void labOxygenAssembliesAndSavedLigands();
    void ligandWithSelectedMetal();
    void oxygenWithColigands();
    void optimizedManganesePeroxoReferences();
};
void EditorTest::drawingTransactions(){
    Molecule m;DrawState s;int a=MoleculeEditController::addAtom(m,{0,0,0},s);QCOMPARE(a,0);QCOMPARE(m.atoms[0].element,QString("C"));
    int b=MoleculeEditController::addAtom(m,{1.54f,0,0},s);QVERIFY(MoleculeEditController::drawBond(m,a,b,s));QCOMPARE(m.atoms.size(),2);QCOMPARE(m.bonds.size(),1);
    QVERIFY(MoleculeEditController::drawBond(m,a,b,s));QCOMPARE(m.bonds.size(),1);
    for(int order:{1,2,3}){s.bondOrder=order;QVERIFY(MoleculeEditController::drawBond(m,a,b,s));QCOMPARE(m.bonds[0].order,double(order));}
    Molecule before=m;m.removeBond(a,b);QCOMPARE(m.atoms.size(),2);QCOMPARE(m.bonds.size(),0);m=before;m.removeAtom(a);QCOMPARE(m.atoms.size(),1);QCOMPARE(m.bonds.size(),0);QVERIFY(MoleculeEditController::validate(m));
    m=before;QVERIFY(MoleculeEditController::replace(m,{a},7,1));QCOMPARE(m.atoms[a].element,QString("N"));QCOMPARE(m.atoms[a].formalCharge,1);QCOMPARE(m.bonds.size(),1);
}
void EditorTest::scopedHydrogens() {
    Molecule m;
    // A hydrogen before its owner exercises selection remapping during adjustment.
    m.atoms={{1,1,"H",{-1,0,0}},{2,6,"C",{}},{3,8,"O",{1.4f,0,0}},
             {4,1,"H",{2,1,0}},{5,27,"Co",{4,0,0}},{6,1,"H",{5,0,0}}};
    m.setBond(0,1,1);m.setBond(1,2,1);m.setBond(2,3,1);m.setBond(4,5,1);
    m.atoms[3].autoHydrogen=true;m.atoms[5].autoHydrogen=true;
    const auto source=m;
    MoleculeEditController::hydrogens(m,HydrogenAction::Adjust,{1});
    QCOMPARE(m.atoms.size(),8);QCOMPARE(m.atoms[0].element,QString("C"));
    QCOMPARE(m.atoms[1].position,source.atoms[2].position);
    QCOMPARE(m.atoms[2].position,source.atoms[3].position);
    QCOMPARE(m.atoms[4].position,source.atoms[5].position);
    QCOMPARE(m.atoms[2].autoHydrogen,true);QVERIFY(MoleculeEditController::validate(m));
    MoleculeEditController::hydrogens(m,HydrogenAction::Add,{0});QCOMPARE(m.atoms.size(),8);
    auto unchanged=source;
    for(auto selection:{QSet<int>{-1,99},QSet<int>{0}}) {
        MoleculeEditController::hydrogens(unchanged,HydrogenAction::Adjust,selection);
        QCOMPARE(unchanged.atoms.size(),source.atoms.size());QCOMPARE(unchanged.atoms[0].autoHydrogen,false);
    }
    auto fixed=source;fixed.atoms[1].optimizationFixed=true;
    MoleculeEditController::hydrogens(fixed,HydrogenAction::Adjust,{1});QCOMPARE(fixed.atoms.size(),source.atoms.size());
    auto locked=source;locked.atoms[1].autoHydrogenLocked=true;
    MoleculeEditController::hydrogens(locked,HydrogenAction::Adjust,{1});QCOMPARE(locked.atoms.size(),source.atoms.size());
    auto fixedH=source;fixedH.atoms[0].optimizationFixed=true;
    MoleculeEditController::hydrogens(fixedH,HydrogenAction::Adjust,{1});
    QCOMPARE(fixedH.atoms.size(),8);QCOMPARE(fixedH.atoms[0].position,source.atoms[0].position);QVERIFY(!fixedH.atoms[0].autoHydrogen);
    auto bridge=source;bridge.setBond(0,4,1,true);bridge.atoms[0].autoHydrogen=true;
    MoleculeEditController::hydrogens(bridge,HydrogenAction::Adjust,{1});
    QCOMPARE(bridge.atoms[0].position,source.atoms[0].position);QCOMPARE(bridge.bondIndex(0,4)>=0,true);
    Molecule charged;charged.atoms={{1,7,"N",{}},{2,8,"O",{4,0,0}}};charged.atoms[0].formalCharge=1;charged.charge=1;
    MoleculeEditController::hydrogens(charged,HydrogenAction::Add,{0});QCOMPARE(charged.atoms.size(),6);QCOMPARE(charged.charge,1);
}
void EditorTest::chargedHydrogens(){
    for(auto test:{QVector<int>{6,0,4}, {7,0,3},{7,1,4},{8,0,2},{8,-1,1},{8,-2,0},{15,0,3},{16,0,2},{17,-1,0}}){
        Molecule m;DrawState s;s.element=test[0];s.formalCharge=test[1];MoleculeEditController::addAtom(m,{0,0,0},s);m.reconcileAutoHydrogens();QCOMPARE(m.atoms.size(),test[2]+1);QVERIFY(MoleculeEditController::validate(m));
    }
    for(int metal:{25,26,27,57,92}){Molecule m;DrawState s;s.element=metal;MoleculeEditController::addAtom(m,{0,0,0},s);m.reconcileAutoHydrogens();QCOMPARE(m.atoms.size(),1);}
    for(int order:{1,2,3}){Molecule m;DrawState s;MoleculeEditController::addAtom(m,{0,0,0},s);MoleculeEditController::addAtom(m,{1.5f,0,0},s);s.bondOrder=order;MoleculeEditController::drawBond(m,0,1,s);m.reconcileAutoHydrogens();QCOMPARE(m.atoms.size(),10-2*order);}
    Molecule coordinated;DrawState s;s.element=27;MoleculeEditController::addAtom(coordinated,{0,0,0},s);s.element=7;MoleculeEditController::addAtom(coordinated,{2,0,0},s);MoleculeEditController::drawBond(coordinated,0,1,s);coordinated.reconcileAutoHydrogens();QCOMPARE(coordinated.atoms.size(),5); // NH3 retains H when bound to metal.
    MoleculeEditController::hydrogens(coordinated,HydrogenAction::RemoveAll);QCOMPARE(coordinated.atoms.size(),2);
    MoleculeEditController::hydrogens(coordinated,HydrogenAction::Add);QCOMPARE(coordinated.atoms.size(),5);
    auto original=coordinated;MoleculeEditController::hydrogens(coordinated,HydrogenAction::Add);QCOMPARE(coordinated.atoms.size(),original.atoms.size());
    coordinated.atoms[1].formalCharge=-1;MoleculeEditController::hydrogens(coordinated,HydrogenAction::RemoveExtra);QCOMPARE(coordinated.atoms.size(),4);
}
void EditorTest::snappingAndAutomatic(){
    QVector3D origin,cursor{3,0,0};auto snapped=MoleculeEditController::snappedPosition(origin,cursor,6,6,2,false);QVERIFY(qAbs(snapped.length()-1.34)<1e-5);QCOMPARE(MoleculeEditController::snappedPosition(origin,cursor,6,6,2,true),cursor);
    QCOMPARE(MoleculeEditController::predictOrder(6,6,1.34),2.0);QCOMPARE(MoleculeEditController::predictOrder(6,6,1.20),3.0);QCOMPARE(MoleculeEditController::predictOrder(6,6,1.7),1.0);QCOMPARE(MoleculeEditController::predictOrder(26,6,1.2),1.0);
    QCOMPARE(MoleculeEditController::predictOrder(8,8,1.10),1.0);QCOMPARE(MoleculeEditController::predictOrder(7,8,1.10),1.0);
    QCOMPARE(MoleculeEditController::snappedPosition(origin,cursor,92,8,1,false),cursor);
    Molecule m;DrawState s;MoleculeEditController::addAtom(m,{},s);MoleculeEditController::addAtom(m,{1.2f,0,0},s);s.bondOrder=2;MoleculeEditController::drawBond(m,0,1,s);s.bondOrder=0;MoleculeEditController::drawBond(m,0,1,s);QCOMPARE(m.bonds[0].order,2.0);
}
void EditorTest::selectionAndRigidBody(){
    Molecule m;DrawState s;for(int i=0;i<4;++i)MoleculeEditController::addAtom(m,{float(i),0,0},s);m.setBond(0,1,1);m.setBond(1,2,1);
    SelectionModel selection;selection.click(0,false,false);selection.click(1,true,false);QCOMPARE(selection.atoms.size(),2);selection.click(0,false,true);QVERIFY(!selection.atoms.contains(0));selection.connected(m,0);QCOMPARE(selection.atoms.size(),3);QVERIFY(!selection.atoms.contains(3));
    auto before=m;MoleculeEditController::transform(m,selection.atoms,{3,2,1},{40,50,60},{1,0,0});QVERIFY(qAbs(GeometryEditor::distance(m,0,2)-2)<1e-5);QCOMPARE(m.atoms[3].position,before.atoms[3].position);
}
void EditorTest::attachedFragmentRotation(){
    Molecule m;
    m.atoms={{1,6,"C",{0,0,0}},{2,6,"C",{1.54f,0,0}},
             {3,1,"H",{2,1,0}},{4,1,"H",{2,-0.5f,0.86f}},
             {5,1,"H",{2,-0.5f,-0.86f}},{6,8,"O",{6,0,0}}};
    m.setBond(0,1,1);for(int h=2;h<=4;++h)m.setBond(1,h,1);
    QSet<int> branch;
    QVector3D pivot,axis;
    QVERIFY(MoleculeEditController::rotatableBranch(m,1,branch,pivot,axis));
    QCOMPARE(branch,QSet<int>({1,2,3,4}));
    QCOMPARE(pivot,m.atoms[0].position);
    QVERIFY((axis-QVector3D(1,0,0)).length()<1e-5f);
    QSet<int> hydrogenBranch;
    QVector3D hydrogenPivot,hydrogenAxis;
    QVERIFY(MoleculeEditController::rotatableBranch(m,2,hydrogenBranch,hydrogenPivot,hydrogenAxis));
    QCOMPARE(hydrogenBranch,branch);
    const Molecule before=m;
    const auto rotation=QQuaternion::fromAxisAndAngle(axis,90.0f);
    for(int i:branch)m.atoms[i].position=pivot+rotation.rotatedVector(m.atoms[i].position-pivot);
    QVERIFY((m.atoms[2].position-before.atoms[2].position).length()>0.1f);
    QCOMPARE(m.atoms[0].position,before.atoms[0].position);
    QCOMPARE(m.atoms[5].position,before.atoms[5].position);
    QVERIFY(qAbs(GeometryEditor::distance(m,0,1)-GeometryEditor::distance(before,0,1))<1e-5);
    Molecule ring;
    ring.atoms={{1,6,"C",{0,0,0}},{2,6,"C",{1,0,0}},{3,6,"C",{0.5f,1,0}}};
    ring.setBond(0,1,1);ring.setBond(1,2,1);ring.setBond(2,0,1);
    QVERIFY(!MoleculeEditController::rotatableBranch(ring,0,branch,pivot,axis));
    QVERIFY(branch.isEmpty());
    Molecule doubleBond=m;
    doubleBond.bonds[0].order=2.0;
    QVERIFY(!MoleculeEditController::rotatableBranch(doubleBond,1,branch,pivot,axis));
    Molecule coordination=m;
    coordination.bonds[0].coordination=true;
    QVERIFY(!MoleculeEditController::rotatableBranch(coordination,1,branch,pivot,axis));
}
void EditorTest::templates(){
    const QVector<int> counts{1,2,3,4,4,5,5,6,6,7,8};for(int i=0;i<counts.size();++i){auto dirs=FragmentLibrary::directions(i);QCOMPARE(dirs.size(),counts[i]);for(auto d:dirs)QVERIFY(qAbs(d.length()-1)<1e-6);}
    auto tetra=FragmentLibrary::directions(3);for(int i=0;i<4;++i)for(int j=i+1;j<4;++j)QVERIFY(qAbs(QVector3D::dotProduct(tetra[i],tetra[j])+1.0/3)<1e-5);
    auto square=FragmentLibrary::directions(4);for(auto d:square)QCOMPARE(d.z(),0.0f);
    Molecule m;FragmentLibrary::coordinationCenter(m,27,2,7,{});QCOMPARE(m.atoms.size(),1);QCOMPARE(m.coordinationSites.size(),6);m.reconcileAutoHydrogens();QCOMPARE(m.atoms.size(),1);QVERIFY(MoleculeEditController::validate(m));
}
void EditorTest::fragmentsAndLigands(){
    QString errors;auto library=FragmentLibrary::load(&errors);QVERIFY2(errors.isEmpty(),qPrintable(errors));QVERIFY(library.size()>=251);
    auto find=[&](const QString&name){for(auto f:library)if(f.name==name)return f;return Fragment{};};
    auto phenyl=find("phenyl"),co=find("carbonyl"),bpy=find("bpy-bipyridine"),haptic=find("eta6-benzene");QVERIFY(!phenyl.atoms.isEmpty());QVERIFY(!co.atoms.isEmpty());QCOMPARE(bpy.attachments.size(),2);QCOMPARE(haptic.hapticAtoms.size(),6);
    Molecule m;DrawState s;MoleculeEditController::addAtom(m,{},s);m.reconcileAutoHydrogens();QVERIFY(FragmentLibrary::insert(m,phenyl,1,{}, {1,0,0},&errors));m.reconcileAutoHydrogens();QVERIFY(MoleculeEditController::validate(m));int carbons=0;for(auto a:m.atoms)if(a.atomicNumber==6)++carbons;QCOMPARE(carbons,7);
    Molecule metal;FragmentLibrary::coordinationCenter(metal,27,2,7,{});QVERIFY(FragmentLibrary::insert(metal,co,0,{}, {1,0,0},&errors));QCOMPARE(metal.atoms.size(),3);QCOMPARE(metal.bonds.size(),2);QVERIFY(metal.bonds.last().coordination);QCOMPARE(metal.bonds.first().order,3.0);metal.reconcileAutoHydrogens();QCOMPARE(metal.atoms.size(),3);
    metal.removeAtom(1);for(auto site:metal.coordinationSites)QVERIFY(!site.occupied);QVERIFY(MoleculeEditController::validate(metal));
    Molecule chelate;FragmentLibrary::coordinationCenter(chelate,26,2,7,{});QVERIFY(FragmentLibrary::insert(chelate,bpy,0,{}, {1,0,0},&errors));int donors=0;for(auto b:chelate.bonds)if(b.coordination)++donors;QCOMPARE(donors,2);QVERIFY(MoleculeEditController::validate(chelate));
    Molecule pi;FragmentLibrary::coordinationCenter(pi,26,0,7,{});QVERIFY(FragmentLibrary::insert(pi,haptic,0,{}, {0,0,1},&errors));QCOMPARE(pi.hapticContacts.size(),1);QCOMPARE(pi.hapticContacts[0].atoms.size(),6);for(auto a:pi.atoms)QVERIFY(a.atomicNumber>0);QVERIFY(MoleculeEditController::validate(pi));
    // Validate every resource after dummy removal, including standalone insertion.
    for(auto f:library){Molecule inserted;QVERIFY2(FragmentLibrary::insert(inserted,f,-1,{},{1,0,0},&errors),qPrintable(f.name+errors));}
}
void EditorTest::bondCentric(){
    Molecule m;m.atoms={{1,6,"C",{0,1,0}},{2,6,"C",{0,0,0}},{3,6,"C",{1,0,0}},{4,6,"C",{1,1,1}},{5,6,"C",{1,2,1}}};for(int i=0;i<4;++i)m.setBond(i,i+1,1);
    QString error;QVERIFY(MoleculeEditController::bondGeometry(m,1,2,0,1.8,false,&error));QVERIFY(qAbs(GeometryEditor::distance(m,1,2)-1.8)<1e-4);
    // Smaller side A moves; measurement order is reversed.
    QVERIFY(MoleculeEditController::bondGeometry(m,1,2,1,115,false,&error));QVERIFY(qAbs(GeometryEditor::angleDegrees(m,3,2,1)-115)<1e-3);
    QVERIFY(MoleculeEditController::bondGeometry(m,1,2,2,70,false,&error));QVERIFY(qAbs(GeometryEditor::dihedralDegrees(m,3,2,1,0)-70)<1e-3);
    auto before=m;m.setBond(0,3,1);QVERIFY(!MoleculeEditController::bondGeometry(m,1,2,0,2,false,&error));for(int i=0;i<m.atoms.size();++i)QCOMPARE(m.atoms[i].position,before.atoms[i].position);
}
void EditorTest::invalidEdits(){
    Molecule m;DrawState s;MoleculeEditController::addAtom(m,{},s);m.setBond(0,0,1);QVERIFY(m.bonds.isEmpty());m.bonds<<Bond{0,99,1};QVERIFY(!MoleculeEditController::validate(m));m.bonds.clear();m.atoms[0].position.setX(std::numeric_limits<float>::infinity());QVERIFY(!MoleculeEditController::validate(m));
}
void EditorTest::approachPreservesFragments(){
    Molecule m;m.atoms={{1,6,"C",{0,0,0}},{2,1,"H",{1,0,0}},
        {3,8,"O",{5,1,0}},{4,1,"H",{5,2,0}},{5,26,"Fe",{0,0,10}}};
    m.setBond(0,1,1);m.setBond(2,3,1);
    const auto before=m;QString error;
    QVERIFY(GeometryEditor::arrangeApproach(m,0,2,2.75,&error));
    QVERIFY(qAbs(GeometryEditor::distance(m,0,2)-2.75)<1e-5);
    for(auto pair:{QPair{0,1},QPair{2,3}})
        QVERIFY(qAbs(GeometryEditor::distance(m,pair.first,pair.second)-GeometryEditor::distance(before,pair.first,pair.second))<1e-5);
    QCOMPARE(m.atoms[4].position,before.atoms[4].position);
    auto axis=(m.atoms[2].position-m.atoms[0].position).normalized();
    QVERIFY(QVector3D::dotProduct((m.atoms[0].position-m.atoms[1].position).normalized(),axis)>.999f);
    QVERIFY(QVector3D::dotProduct((m.atoms[2].position-m.atoms[3].position).normalized(),-axis)>.999f);
    const auto arranged=m;
    QVERIFY(!GeometryEditor::arrangeApproach(m,0,0,2,&error));
    QVERIFY(!GeometryEditor::arrangeApproach(m,0,2,std::numeric_limits<double>::quiet_NaN(),&error));
    for(int i=0;i<m.atoms.size();++i)QCOMPARE(m.atoms[i].position,arranged.atoms[i].position);
    m.setBond(0,2,1);m.setBond(1,3,1);m.setBond(1,2,1);
    QVERIFY(!GeometryEditor::arrangeApproach(m,0,2,3,&error));QVERIFY(!error.isEmpty());
}
void EditorTest::labOxygenAssembliesAndSavedLigands(){
    QString error;auto all=FragmentLibrary::load(&error);QVERIFY2(error.isEmpty(),qPrintable(error));
    QVector<Fragment> ligands,oxygen;
    for(auto f:all){if(f.category=="lab-ligand")ligands<<f;if(f.category=="lab-adduct")oxygen<<f;}
    QVERIFY(ligands.size()>=5);QCOMPARE(oxygen.size(),6);
    for(auto ligand:ligands)for(auto adduct:oxygen)for(int metal:{25,26,27}){
        Molecule assembled;
        QVERIFY2(LabComplexBuilder::assemble(ligand,adduct,metal,3,3+ligand.totalCharge+adduct.totalCharge,5,assembled,&error),qPrintable(ligand.name+" + "+adduct.name+": "+error));
        QVERIFY(MoleculeEditController::validate(assembled,&error));
        int contacts=0;for(auto b:assembled.bonds)if(b.coordination)++contacts;
        QCOMPARE(contacts,ligand.attachments.size()+adduct.attachments.size());
        int expected=adduct.name.contains("side-on")||adduct.name.contains("eta2")?2:1;
        QCOMPARE(adduct.attachments.size(),expected);
        int oxygens=0,hydrogens=0;for(auto a:adduct.atoms){if(a.atomicNumber==8)++oxygens;if(a.atomicNumber==1)++hydrogens;}
        QCOMPARE(oxygens,2);QCOMPARE(hydrogens,adduct.name.startsWith("hydroperoxo")?1:0);
    }
    const auto ligand=ligands.first();Molecule source;source.atoms=ligand.atoms;source.bonds=ligand.bonds;
    int center=ligand.dummyAtoms.first();source.atoms[center].atomicNumber=26;source.atoms[center].element="Fe";
    QTemporaryDir directory;QVERIFY(directory.isValid());QString path=directory.filePath("ligand.cjson");
    QVERIFY(FragmentLibrary::saveLigand(source,center,{},"Saved ligand",path,&error));
    Fragment restored;QVERIFY(FragmentLibrary::read(path,restored,&error));
    QCOMPARE(restored.attachments.size(),ligand.attachments.size());QCOMPARE(restored.atoms.size(),ligand.atoms.size());
    for(int i=0;i<source.atoms.size();++i)QVERIFY((restored.atoms[i].position-(source.atoms[i].position-source.atoms[center].position)).length()<1e-5);
    QVERIFY(!FragmentLibrary::saveLigand(source,center,{1},"Bad ligand",path,&error));
}
void EditorTest::ligandWithSelectedMetal(){
    const auto fragments=FragmentLibrary::load();int count=0;
    for(const auto &f:fragments)if(f.category=="lab-ligand"){
        ++count;
        for(int z:{25,26,27,28,29}){
            Molecule result;QString error;
            QVERIFY2(FragmentLibrary::metalComplex(f,z,2,2+f.totalCharge,3,result,&error),qPrintable(error));
            QCOMPARE(result.atoms[0].atomicNumber,z);QCOMPARE(result.atoms[0].formalCharge,2);
            QCOMPARE(result.charge,2+f.totalCharge);QCOMPARE(result.multiplicity,3);
            QCOMPARE(result.atoms.size(),f.atoms.size()-f.dummyAtoms.size()+1);
            int donors=0;for(const auto &bond:result.bonds)if(bond.coordination){++donors;QVERIFY(bond.first==0||bond.second==0);}
            QCOMPARE(donors,f.attachments.size());
            for(const auto &atom:result.atoms)QVERIFY(atom.atomicNumber>0);
        }
    }
    QVERIFY(count>=5);Molecule unchanged;unchanged.title="original";QString error;
    QVERIFY(!FragmentLibrary::metalComplex(fragments.first(),6,2,2,1,unchanged,&error));QCOMPARE(unchanged.title,QString("original"));
}
void EditorTest::oxygenWithColigands(){
    const auto all=FragmentLibrary::load();int cases=0;
    for(const auto &ligand:all)if(ligand.category=="lab-ligand")
    for(const auto &oxygen:all)if(oxygen.category=="lab-adduct")
    for(const auto &solvent:all)if(solvent.category=="lab-coligand"){
        Molecule base,result;QString error;
        QVERIFY(LabComplexBuilder::assemble(ligand,oxygen,26,3,1,6,base,&error));
        QVERIFY2(LabComplexBuilder::assemble(ligand,oxygen,26,3,1,6,result,&error,&solvent),qPrintable(ligand.name+" / "+oxygen.name+" / "+solvent.name+": "+error));
        QCOMPARE(result.atoms.size(),base.atoms.size()+solvent.atoms.size()-1);
        QCOMPARE(result.bonds.size(),base.bonds.size()+solvent.bonds.size());
        int donor=base.atoms.size();QVERIFY(result.bondIndex(0,donor)>=0);
        QVERIFY(result.bonds[result.bondIndex(0,donor)].coordination);
        QCOMPARE(result.atoms[donor].atomicNumber,solvent.name.startsWith("MeCN")?7:8);
        for(int i=0;i<base.atoms.size();++i)QCOMPARE(result.atoms[i].position,base.atoms[i].position);
        for(const auto &b:result.bonds)QVERIFY(b.manual);
        QCOMPARE(result.charge,1);QCOMPARE(result.multiplicity,6);++cases;
    }
    QVERIFY(cases>=60);
}
void EditorTest::optimizedManganesePeroxoReferences(){
    QString error;const auto fragments=FragmentLibrary::load(&error);QVERIFY2(error.isEmpty(),qPrintable(error));
    int count=0;
    for(const auto &ligand:fragments)for(const auto &reference:ligand.optimizedGeometries){
        Fragment oxygen,solvent;bool found=false;
        for(const auto &f:fragments){if(QFileInfo(f.resource).completeBaseName()==reference.adduct){oxygen=f;found=true;}if(QFileInfo(f.resource).completeBaseName()==reference.coligand)solvent=f;}
        QVERIFY(found);const auto *coligand=reference.coligand.isEmpty()?nullptr:&solvent;
        Molecule molecule;QVERIFY2(LabComplexBuilder::assemble(ligand,oxygen,25,3,1,reference.multiplicity,molecule,&error,coligand),qPrintable(error));
        QCOMPARE(molecule.atoms.size(),reference.positions.size());QCOMPARE(molecule.charge,1);QCOMPARE(molecule.multiplicity,reference.multiplicity);
        QVERIFY(molecule.title.contains(reference.source));
        int oxygenContacts=0;for(const auto &bond:molecule.bonds)if(bond.coordination&&(bond.first==0||bond.second==0)){
            const auto donor=molecule.atoms[bond.first==0?bond.second:bond.first].atomicNumber;if(donor==8)++oxygenContacts;
        }
        QCOMPARE(oxygenContacts,reference.adduct=="peroxo-eta2"?2:1);
        QVERIFY(!LabComplexBuilder::referenceFor(ligand,oxygen,26,3,1,reference.multiplicity,coligand));
        QVERIFY(!LabComplexBuilder::referenceFor(ligand,oxygen,25,3,2,reference.multiplicity,coligand));
        const auto directory=qEnvironmentVariable("YOUNGCRAFT_MN_REFERENCE_DIR");
        if(!directory.isEmpty()){
            const auto parsed=GaussianParser::parse(QDir(directory).filePath(reference.source));QVERIFY2(parsed.success,qPrintable(parsed.error));
            QCOMPARE(parsed.data.molecule.atoms.size(),molecule.atoms.size());QCOMPARE(parsed.data.molecule.multiplicity,reference.multiplicity);
            for(int i=0;i<molecule.atoms.size();++i){
                QCOMPARE(molecule.atoms[i].atomicNumber,parsed.data.molecule.atoms[i].atomicNumber);
                for(int j=0;j<i;++j){
                    const auto expected=(parsed.data.molecule.atoms[i].position-parsed.data.molecule.atoms[j].position).length();
                    const auto actual=(molecule.atoms[i].position-molecule.atoms[j].position).length();
                    QVERIFY2(qAbs(actual-expected)<3e-5,"Every interatomic distance must match the original server geometry after rigid alignment");
                }
            }
        }
        ++count;
    }
    QCOMPARE(count,7);
}


void EditorTest::ligandReplacementPreservesCore(){
    const auto library=FragmentLibrary::load();int tested=0;
    for(const auto &ligand:library)if(ligand.category=="lab-ligand"){
        const Fragment *adduct=nullptr;for(const auto &f:library)if(f.category=="lab-adduct"){adduct=&f;break;}
        QVERIFY(adduct);Molecule source;QString error;
        QVERIFY(LabComplexBuilder::assemble(ligand,*adduct,26,3,1,6,source,&error));
        // Rotate and translate the entire complex to catch reference-frame mistakes.
        const auto rotation=QQuaternion::fromEulerAngles(37,81,-26);
        for(auto &a:source.atoms)a.position=rotation.rotatedVector(a.position)+QVector3D(7,-3,11);
        const auto candidates=LigandReplacement::recognize(source,0,library);
        const LigandCandidate *chosen=nullptr;
        for(const auto &c:candidates)if(c.atoms.size()==ligand.atoms.size()-1){chosen=&c;break;}
        QVERIFY(chosen);QVERIFY(chosen->name.contains(ligand.name));
        QSet<int> core;for(int i=0;i<source.atoms.size();++i)if(!chosen->atoms.contains(i))core.insert(i);
        // Replace with the same geometry: donor directions must survive rotation.
        Molecule result;QVERIFY2(LigandReplacement::replace(source,*chosen,ligand,core,result,&error),qPrintable(error));
        QCOMPARE(result.charge,source.charge);QCOMPARE(result.multiplicity,source.multiplicity);
        QVector<int> mapping(source.atoms.size(),-1);int next=0;
        for(int i=0;i<source.atoms.size();++i)if(core.contains(i)){
            mapping[i]=next;QCOMPARE(result.atoms[next].position,source.atoms[i].position);
            QCOMPARE(result.atoms[next].atomicNumber,source.atoms[i].atomicNumber);++next;
        }
        for(const auto &b:source.bonds)if(core.contains(b.first)&&core.contains(b.second)){
            int bi=result.bondIndex(mapping[b.first],mapping[b.second]);QVERIFY(bi>=0);
            const auto &actual=result.bonds[bi];QCOMPARE(actual.order,b.order);QCOMPARE(actual.coordination,b.coordination);QCOMPARE(actual.manual,b.manual);QCOMPARE(actual.style,b.style);
        }
        for(int donor:chosen->donors){float nearest=1000;
            for(const auto &b:result.bonds)if(b.first==0||b.second==0){int other=b.first==0?b.second:b.first;if(other>=next)nearest=qMin(nearest,(result.atoms[other].position-source.atoms[donor].position).length());}
            QVERIFY2(nearest<0.002f,qPrintable(QString("donor alignment error %1 for %2").arg(nearest).arg(ligand.name)));
        }
        // All compatible template pairs must retain the exact same core.
        for(const auto &target:library)if(target.category=="lab-ligand"&&target.attachments.size()==chosen->donors.size()){
            QVERIFY2(LigandReplacement::replace(source,*chosen,target,core,result,&error),qPrintable(error));
            int k=0;for(int i=0;i<source.atoms.size();++i)if(core.contains(i))QCOMPARE(result.atoms[k++].position,source.atoms[i].position);
            QVERIFY(MoleculeEditController::validate(result));++tested;
        }
    }
    QVERIFY(tested>=25);
}
void EditorTest::ligandReplacementRejectsUnsafeEdits(){
    Molecule source;source.atoms={{1,7,"N",{2,0,0}},{2,6,"C",{3.4f,0,0}},{3,26,"Fe",{}},{4,8,"O",{0,2,0}}};
    source.setBond(0,1,1);source.setBond(2,0,1,true);source.setBond(2,3,1,true);
    source.charge=2;source.multiplicity=5;
    Fragment f;f.name="test";f.ligand=true;f.atoms={{1,0,"X",{}},{2,7,"N",{2,0,0}},{3,6,"C",{3.4f,0,0}}};f.dummyAtoms={0};f.attachments={1};f.bonds={{0,1,1},{1,2,1}};
    auto candidates=LigandReplacement::recognize(source,2);QCOMPARE(candidates.size(),2);
    auto c=candidates[0];QVERIFY(c.atoms.contains(0));Molecule result;QString error;
    QVERIFY(LigandReplacement::replace(source,c,f,{2,3},result,&error));
    QCOMPARE(result.atoms[0].atomicNumber,26);QCOMPARE(result.atoms[1].position,source.atoms[3].position);
    QCOMPARE(result.charge,2);QCOMPARE(result.multiplicity,5);
    result.title="untouched";
    QVERIFY(!LigandReplacement::replace(source,c,f,{0},result,&error));QCOMPARE(result.title,QString("untouched"));
    source.atoms[1].optimizationFixed=true;
    QVERIFY(!LigandReplacement::replace(source,c,f,{},result,&error));source.atoms[1].optimizationFixed=false;
    auto forged=c;forged.atoms.remove(1);QVERIFY(!LigandReplacement::replace(source,forged,f,{},result,&error));
    auto bad=f;bad.attachments={1,2};QVERIFY(!LigandReplacement::replace(source,c,bad,{},result,&error));
    bad=f;bad.ligand=false;QVERIFY(!LigandReplacement::replace(source,c,bad,{},result,&error));
    source.hapticContacts<<HapticContact{2,{0,1},"eta"};
    QVERIFY(!LigandReplacement::replace(source,c,f,{},result,&error));source.hapticContacts.clear();
    source.atoms<<Atom{5,27,"Co",{5,0,0}};source.setBond(1,4,1,true);
    QVERIFY(!LigandReplacement::replace(source,c,f,{},result,&error));
    QCOMPARE(result.title,QString("untouched"));
}

QTEST_APPLESS_MAIN(EditorTest)
#include "test_editor.moc"
