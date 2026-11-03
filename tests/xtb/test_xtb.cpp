#include "compute/XtbRunner.h"
#include "model/GeometryEditor.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QFileInfo>
#include <QDir>
#include <cmath>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include "model/LabComplexBuilder.h"
using namespace youngcraft;
class XtbTest:public QObject {
 Q_OBJECT
 Molecule water(){Molecule m;m.atoms={{1,8,"O",{0,0,0}},{2,1,"H",{1.2f,0,0}},{3,1,"H",{-0.3f,1.1f,0}}};return m;}
 QString runtime(){return qEnvironmentVariable("YOUNGCRAFT_XTB_RUNTIME",QDir::current().filePath("xtb_runtime"));}
 void runSP(Molecule m,XtbOptions options){options.optimize=false;XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);QVERIFY2(r.start(m,options,runtime()),qPrintable(r.error));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,30000);QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output+r.error));QVERIFY(std::isfinite(r.energy));QVERIFY(std::isfinite(r.gradient));QVERIFY(std::isfinite(r.gap));QVERIFY(r.arguments.contains("--grad"));QCOMPARE(r.result.charge,options.charge);QCOMPARE(r.result.multiplicity,options.multiplicity);}
private slots:
 void fixedCoreAndTargetDistance(){
    auto m=water();m.setBond(0,1,1);m.setBond(0,2,1);
    for(auto &a:m.atoms)a.position+=QVector3D(2,3,1);
    for(int method:{0,1,2}){
        XtbOptions o;o.method=method;o.level="tight";o.constraints={{"fixed",{0},0},{"fixed",{1},0},{"distance",{0,2},1.05}};
        XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);
        QVERIFY2(r.start(m,o,runtime()),qPrintable(r.error));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,45000);
        QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output+r.error));
        for(int i:{0,1})QVERIFY2((r.result.atoms[i].position-m.atoms[i].position).length()<0.00001,qPrintable(QString::number((r.result.atoms[i].position-m.atoms[i].position).length())));
        QVERIFY(std::abs(GeometryEditor::distance(r.result,0,2)-1.05)<0.04);
        QFile control(r.directory+"/framework.inp");QVERIFY(control.open(QIODevice::ReadOnly));QVERIFY(control.readAll().contains("$fix\n atoms: 1, 2\n$end"));
        QFile input(r.directory+"/input.xyz");QVERIFY(input.open(QIODevice::ReadOnly));XtbTrajectoryParser parser;auto frames=parser.append(input.readAll()+"\n",m);QCOMPARE(frames.size(),1);
        QVERIFY(std::abs((frames[0].coordinates[0]-frames[0].coordinates[2]).length()-1.05)<0.00001);
    }
 }

 void metalComplexBondRetention(){
    QFile xyz("tests/xtb/fe_oxo_approach.xyz");QVERIFY(xyz.open(QIODevice::ReadOnly));
    auto lines=xyz.readAll().split('\n');Molecule m;
    for(int i=0;i<lines[0].trimmed().toInt();++i){auto w=QString::fromUtf8(lines[i+2]).simplified().split(' ');m.atoms<<Atom{i+1,atomicNumberFor(w[0]),w[0],{w[1].toFloat(),w[2].toFloat(),w[3].toFloat()}};}
    QFile bonds("tests/xtb/fe_oxo_approach_bonds.json");QVERIFY(bonds.open(QIODevice::ReadOnly));
    for(const auto &value:QJsonDocument::fromJson(bonds.readAll()).array()){auto pair=value.toArray();m.setBond(pair[0].toInt(),pair[1].toInt(),1,pair[0].toInt()==0);}
    m.charge=2;m.multiplicity=5;
    for(int method:{0,2}) {
        XtbOptions o;o.method=method;o.charge=2;o.multiplicity=5;o.preserveBondLengths=true;
        o.constraints={{"distance",{1,91},2.0}};
        XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);
        QVERIFY2(r.start(m,o,runtime()),qPrintable(r.error));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,180000);
        QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output.right(2000)+r.error));
        double maximum=0;
        for(const auto &bond:m.bonds){double before=GeometryEditor::distance(m,bond.first,bond.second),after=GeometryEditor::distance(r.result,bond.first,bond.second);maximum=qMax(maximum,qAbs(after-before));}
        qInfo()<<"method"<<method<<"Fe-O"<<GeometryEditor::distance(r.result,0,1)<<"max bond deviation"<<maximum;
        QVERIFY2(maximum<0.12,qPrintable(QString::number(maximum)));QVERIFY(r.geometryWarning.isEmpty());
        QVERIFY(qAbs(GeometryEditor::distance(r.result,1,91)-2.0)<0.08);
    }
 }
 void internalCoordinateConstraints(){
    Molecule m;m.atoms={{1,1,"H",{0,1,0}},{2,8,"O",{0,0,0}},{3,8,"O",{1.5f,0,0}},{4,1,"H",{1.5f,0,1}}};
    m.setBond(0,1,1);m.setBond(1,2,1);m.setBond(2,3,1);
    for(int method:{0,1,2}) {
        XtbOptions o;o.method=method;o.level="tight";
        o.constraints={{"distance",{0,1},1.05},{"angle",{0,1,2},105},{"dihedral",{0,1,2,3},-80}};
        XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);
        QVERIFY2(r.start(m,o,runtime()),qPrintable(r.error));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,30000);
        QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output+r.error));
        QFile file(r.directory+"/framework.inp");QVERIFY(file.open(QIODevice::ReadOnly));auto text=file.readAll();
        QVERIFY(text.contains("distance: 1, 2, 1.05000000"));QVERIFY(text.contains("angle: 1, 2, 3, 105.00000000"));QVERIFY(text.contains("dihedral: 1, 2, 3, 4, -80.00000000"));
        const double distance=GeometryEditor::distance(r.result,0,1),angle=GeometryEditor::angleDegrees(r.result,0,1,2),torsion=GeometryEditor::dihedralDegrees(r.result,0,1,2,3);
        qInfo()<<"method"<<method<<distance<<angle<<torsion;
        QVERIFY(qAbs(distance-1.05)<0.05);QVERIFY(qAbs(angle-105)<5);QVERIFY(qAbs(torsion+80)<5);
    }
 }
 void invalidConstraints(){
    QString error;
    QVERIFY(!validateConstraints({{"distance",{0,3},1}},3,&error));
    QVERIFY(!validateConstraints({{"angle",{0,1,1},90}},3,&error));
    QVERIFY(!validateConstraints({{"angle",{0,1,2},180}},3,&error));
    QVERIFY(!validateConstraints({{"distance",{0,1},1},{"distance",{1,0},2}},3,&error));
 }

 void gfnffUsesEditedConnectivity(){
    auto m=water();m.setBond(0,1,1);m.setBond(0,2,1);
    double firstEnergy=kNaN;
    for(bool connected:{true,false,true}) {
        if(connected)m.setBond(0,2,1);else m.removeBond(0,2);
        XtbOptions o;o.method=0;o.optimize=false;XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);
        QVERIFY2(r.start(m,o,runtime()),qPrintable(r.error));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,30000);
        QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output+r.error));
        QVERIFY(r.output.contains("neighborlist has been adjusted"));
        QFile file(r.directory+"/gfnff_lists.json");QVERIFY(file.open(QIODevice::ReadOnly));
        const auto text=QString::fromUtf8(file.readAll());
        QRegularExpression rows("\\[([0-9, \\r\\n]+)\\]");auto it=rows.globalMatch(text);QVector<QVector<int>> values;
        while(it.hasNext()){QVector<int> row;for(const auto &n:it.next().captured(1).split(','))row<<n.trimmed().toInt();values<<row;}
        QCOMPARE(values.size(),3);QCOMPARE(values[0].last(),connected?2:1);QCOMPARE(values[2].last(),connected?1:0);
        QCOMPARE(r.result.bonds.size(),connected?2:1);
        if(connected){if(std::isfinite(firstEnergy))QVERIFY(qAbs(r.energy-firstEnergy)<1e-8);else firstEnergy=r.energy;}
        else QVERIFY(qAbs(r.energy-firstEnergy)>1e-5);
    }
 }
 void endOnColigandMM(){
    const auto all=FragmentLibrary::load();Fragment ligand,oxygen;
    for(const auto &f:all){if(f.name=="12-MTP (Me3-TPADP)")ligand=f;if(f.name.startsWith("hydroperoxo")&&f.name.contains("end-on"))oxygen=f;}
    // This executable does not embed fragment resources; load the external library explicitly.
    if(ligand.atoms.isEmpty())for(const auto &f:FragmentLibrary::load(nullptr,QDir::current().filePath("lab_fragments"))){if(f.name=="12-MTP (Me3-TPADP)")ligand=f;if(f.name.startsWith("hydroperoxo")&&f.name.contains("end-on"))oxygen=f;}
    QVERIFY(!ligand.atoms.isEmpty());QVERIFY(!oxygen.atoms.isEmpty());
    for(const auto &solvent:FragmentLibrary::load(nullptr,QDir::current().filePath("lab_fragments/coligands"))){
        Molecule m;QString error;QVERIFY2(LabComplexBuilder::assemble(ligand,oxygen,26,3,2,6,m,&error,&solvent),qPrintable(error));
        XtbOptions o;o.method=0;o.charge=2;o.optimize=true;o.cycles=10;o.preserveCoordination=true;
        XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);QVERIFY2(r.start(m,o,runtime()),qPrintable(r.error));
        QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,30000);QVERIFY2(r.state==XtbRunner::State::Finished||r.state==XtbRunner::State::NotConverged,qPrintable(r.output+r.error));
        QFile control(r.directory+"/framework.inp");QVERIFY(control.open(QIODevice::ReadOnly));const auto text=control.readAll();
        const int o1=ligand.atoms.size()+1;QVERIFY(text.contains(QString("distance: %1, %2,").arg(o1).arg(o1+1).toUtf8()));
        QCOMPARE(r.result.bonds.size(),m.bonds.size());QVERIFY(std::isfinite(r.energy));
    }
 }
 void fragmentedTrajectory(){XtbTrajectoryParser p;auto m=water();QByteArray frame="3\n energy: -5.2 gnorm: 0.01\nO 0 0 0\nH 1 0 0\nH 0 1 0\n";for(int i=0;i<frame.size()-1;++i)QVERIFY(p.append(frame.mid(i,1),m).isEmpty());auto f=p.append(frame.right(1),m);QCOMPARE(f.size(),1);QCOMPARE(f[0].energy,-5.2);QCOMPARE(f[0].gradient,0.01);QVERIFY(p.append({},m).isEmpty());QCOMPARE(p.append(frame+frame,m).size(),2);QVERIFY(p.error.isEmpty());p.append("2\n",m);QVERIFY(!p.error.isEmpty());}
 void scanTrajectoryComments(){
    XtbTrajectoryParser parser;const auto m=water();
    QByteArray payload="3\nSCF done   -5.5001000\nO 0 0 0\nH 1 0 0\nH 0 1 0\n";
    QVERIFY(parser.append(payload.left(9),m).isEmpty());
    auto frames=parser.append(payload.mid(9),m);
    QCOMPARE(frames.size(),1);QCOMPARE(frames[0].energy,-5.5001);
    QVERIFY(parser.error.isEmpty());
 }
 void invalidScanSettings(){
    const auto m=water();
    for(const auto &s:QVector<XtbScan>{{"distance",{0,0},1.0,1.5,5},
        {"distance",{0,1},1.0,1.5,2},{"angle",{0,1,2},90.0,180.0,5}}){
        XtbOptions o;o.scan=s;XtbRunner r;
        QSignalSpy done(&r,&XtbRunner::completed);
        QVERIFY(!r.start(m,o,runtime()));
        QCOMPARE(r.state,XtbRunner::State::Failed);QCOMPARE(done.size(),1);
    }
    XtbOptions fixed;fixed.scan=XtbScan{"distance",{0,1},1.0,1.5,5};
    fixed.constraints={{"distance",{1,0},1.2}};
    XtbRunner r;QVERIFY(!r.start(m,fixed,runtime()));
    QVERIFY(r.error.contains("이미"));
 }
 void relaxedScanAndCandidateSelection(){
    auto m=water();m.setBond(0,1,1);m.setBond(0,2,1);
    XtbOptions o;o.method=2;o.cycles=250;o.level="normal";
    o.preserveBondLengths=true;
    o.scan=XtbScan{"distance",{0,1},1.0,1.4,5};
    XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);
    QVERIFY2(r.start(m,o,runtime()),qPrintable(r.error));
    QFile control(r.directory+"/framework.inp");QVERIFY(control.open(QIODevice::ReadOnly));
    const QByteArray input=control.readAll();
    QVERIFY(input.contains("$scan\n"));
    QVERIFY(input.contains("distance: 1, 2, 1.00000000; 1.00000000, 1.40000000, 5"));
    // Bond-retention constraints must not pin the coordinate being scanned.
    QVERIFY(!input.contains("distance: 1, 2, 1.20000000"));
    QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,120000);
    QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output.right(2000)+r.error));
    QCOMPARE(r.scanFrames.size(),5);
    for(const auto &frame:r.scanFrames)QVERIFY(std::isfinite(frame.energy));
    if(r.tsCandidateIndex>=0) {
        const int i=r.tsCandidateIndex;
        QVERIFY(i>0 && i+1<r.scanFrames.size());
        QVERIFY(r.scanFrames[i].energy>r.scanFrames[i-1].energy);
        QVERIFY(r.scanFrames[i].energy>r.scanFrames[i+1].energy);
    }
    QVERIFY(r.selectScanFrame(0));QCOMPARE(r.cycle,1);
    QVERIFY(std::abs(r.energy-r.scanFrames[0].energy)<1e-10);
    QVERIFY(!r.selectScanFrame(5));
 }
 void missingRuntime(){XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);QVERIFY(!r.start(water(),{},"Z:/absent-runtime"));QCOMPARE(r.state,XtbRunner::State::Failed);QCOMPARE(done.size(),1);QVERIFY(r.error.contains("xtb.exe"));}
 void neutralSP(){runSP(water(),{});}
 void chargedSP(){auto m=water();m.atoms.push_back({4,1,"H",{-0.3f,-0.9f,0}});XtbOptions o;o.charge=1;runSP(m,o);}
 void openShellSP(){Molecule m;m.atoms={{1,7,"N",{0,0,0}},{2,8,"O",{1.15f,0,0}}};XtbOptions o;o.multiplicity=2;runSP(m,o);}
 void gfn1ALPB(){XtbOptions o;o.method=1;o.solvent="water";runSP(water(),o);}
 void allBundledSolvents(){for(int method:{1,2})for(const auto &solvent:XtbRunner::solvents()){XtbRunner r;XtbOptions o;o.method=method;o.solvent=solvent;o.optimize=false;QSignalSpy done(&r,&XtbRunner::completed);QVERIFY(r.start(water(),o,runtime()));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,15000);QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(solvent+r.output+r.error));}}
 void realOptimization(){XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);QVector<XtbFrame> frames;connect(&r,&XtbRunner::frameReady,this,[&](const XtbFrame &f){frames<<f;});QVERIFY(r.start(water(),{},runtime()));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,30000);QVERIFY2(r.state==XtbRunner::State::Finished,qPrintable(r.output+r.error));QVERIFY(frames.size()>1);QVERIFY(r.energy<frames.first().energy);QVERIFY((r.result.atoms[1].position-water().atoms[1].position).length()>0.01f);QVERIFY(QFileInfo::exists(r.directory+"/xtbopt.xyz"));QVERIFY(std::isfinite(r.gap));}
 void realLiveNonconvergence(){
    QFile file(QDir::current().filePath("tests/xtb/dodecane.xyz"));QVERIFY(file.open(QIODevice::ReadOnly));auto lines=file.readAll().split('\n');Molecule m;for(int i=0;i<lines[0].trimmed().toInt();++i){auto words=QString::fromUtf8(lines[i+2]).simplified().split(' ');Atom a;a.index=i+1;a.element=words[0];a.atomicNumber=atomicNumberFor(a.element);a.position={words[1].toFloat(),words[2].toFloat(),words[3].toFloat()};m.atoms<<a;}
    XtbRunner r;XtbOptions o;o.cycles=10;QSignalSpy done(&r,&XtbRunner::completed);int live=0;connect(&r,&XtbRunner::frameReady,this,[&](const XtbFrame &){if(r.isRunning())++live;});QVERIFY(r.start(m,o,runtime()));QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,60000);QVERIFY2(live>0,qPrintable(r.output));QCOMPARE(r.state,XtbRunner::State::NotConverged);QVERIFY(r.output.contains("FAILED TO CONVERGE GEOMETRY OPTIMIZATION"));QVERIFY(std::isfinite(r.energy));
 }
 void controlledProcess_data(){QTest::addColumn<QString>("mode");QTest::newRow("live-final-import")<<QString("success");QTest::newRow("NOT_CONVERGED")<<QString("notconverged");QTest::newRow("Cancel")<<QString("cancel");}
 void controlledProcess(){QFETCH(QString,mode);QTemporaryDir dir(QDir::tempPath()+QString::fromUtf8("/Youngcraft test 한글 공백-XXXXXX"));QVERIFY(dir.isValid());QDir app(QCoreApplication::applicationDirPath());QVERIFY(QFile::copy(app.filePath("mock_xtb.exe"),dir.filePath("xtb.exe")));QVERIFY(QFile::copy(app.filePath("Qt6Core.dll"),dir.filePath("Qt6Core.dll")));qputenv("YOUNGCRAFT_FAKE_XTB_MODE",mode.toUtf8());XtbRunner r;QSignalSpy done(&r,&XtbRunner::completed);int live=0;connect(&r,&XtbRunner::frameReady,this,[&](const XtbFrame &f){QVERIFY(f.coordinates.size()==3);if(r.isRunning())++live;});QVERIFY(r.start(water(),{},dir.path()));QTRY_VERIFY_WITH_TIMEOUT(live>0,3000);if(mode=="cancel"){QElapsedTimer t;t.start();r.cancel();QVERIFY(t.elapsed()<100);QCOMPARE(r.state,XtbRunner::State::Stopping);}QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,5000);if(mode=="cancel")QCOMPARE(r.state,XtbRunner::State::Cancelled);else{QCOMPARE(r.state,mode=="success"?XtbRunner::State::Finished:XtbRunner::State::NotConverged);QVERIFY(qAbs(r.result.atoms[1].position.x()-0.96f)<1e-6);QCOMPARE(r.gap,10.0);}qunsetenv("YOUNGCRAFT_FAKE_XTB_MODE");}
};
QTEST_GUILESS_MAIN(XtbTest)
#include "test_xtb.moc"
