#include "animation/MechanismTimeline.h"
#include "export/ImageExporter.h"
#include "io/FileDispatcher.h"
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuaternion>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>
using namespace youngcraft;
namespace {
MechanismStep step(float x) {
    MechanismStep s;s.name=QString::number(x);s.seconds=2;
    s.molecule.atoms={{1,6,"C",{x,0,0}},{2,8,"O",{x+1.4f,0,0}},{3,7,"N",{x,1.4f,0}}};
    s.molecule.setBond(0,1,1);s.molecule.setBond(0,2,1);return s;
}
MechanismOptions raw() {MechanismOptions o;o.align=false;o.preserveBonds=false;return o;}
}
class MechanismTest final:public QObject {
    Q_OBJECT
private slots:
    void endpointsAndSmoothVelocity() {
        MechanismTimeline t;QString error;QVERIFY2(t.build({step(0),step(2),step(5)},raw(),&error),qPrintable(error));
        QCOMPARE(t.duration(),4.);QCOMPARE(t.sample(0).atoms[0].position.x(),0.f);QCOMPARE(t.sample(2).atoms[0].position.x(),2.f);QCOMPARE(t.sample(4).atoms[0].position.x(),5.f);
        QVERIFY(t.sample(1).atoms[0].position.x()>0&&t.sample(1).atoms[0].position.x()<2);
        const double epsilon=.001;const float before=(t.sample(2).atoms[0].position.x()-t.sample(2-epsilon).atoms[0].position.x())/epsilon;
        const float after=(t.sample(2+epsilon).atoms[0].position.x()-t.sample(2).atoms[0].position.x())/epsilon;
        QVERIFY(qAbs(after-before)<.02f);
        const auto end=t.sample(4);for(const auto&a:end.atoms)QVERIFY(std::isfinite(a.position.x()));
    }
    void alignmentRemovesRigidMotion() {
        auto a=step(0),b=a;const auto rotation=QQuaternion::fromEulerAngles(38,80,12);
        for(auto &atom:b.molecule.atoms)atom.position=rotation.rotatedVector(atom.position)+QVector3D(8,4,-2);
        MechanismTimeline t;QString error;QVERIFY2(t.build({a,b},{},&error),qPrintable(error));
        for(int i=0;i<3;++i)QVERIFY((t.sample(2).atoms[i].position-a.molecule.atoms[i].position).length()<1e-4f);
    }
    void varyingCountsAndDepartingFragment() {
        auto a=step(0),b=step(1),c=step(2);
        b.molecule.atoms.insert(1,{4,16,"S",{4,0,0}});b.molecule.atoms.insert(2,{5,9,"F",{5.5f,0,0}});
        b.molecule.bonds.clear();b.molecule.setBond(0,3,1);b.molecule.setBond(0,4,1);b.molecule.setBond(1,2,1);
        MechanismTimeline t;QString error;QVERIFY2(t.build({a,b,c},raw(),&error),qPrintable(error));
        QCOMPARE(t.trackCount(),5);QCOMPARE(t.mappings()[1],QVector<int>({0,3,4,1,2}));QCOMPARE(t.mappings()[2],QVector<int>({0,1,2}));
        const auto start=t.sample(0),middle=t.sample(2),end=t.sample(4);
        QVERIFY(!start.atoms[3].visible);QCOMPARE(middle.atoms[3].position,QVector3D(4,0,0));
        QCOMPARE(end.atoms[3].position,QVector3D(4,6,0));QCOMPARE(end.atoms[3].displayOpacity,.18f);
        QVERIFY(end.bondIndex(3,4)>=0);QVERIFY(qAbs((end.atoms[3].position-end.atoms[4].position).length()-1.5f)<1e-5f);
        auto options=raw();options.keepDeparted=false;QVERIFY(t.build({a,b,c},options));QVERIFY(!t.sample(4).atoms[3].visible);
        auto broken=b;broken.molecule.bonds.removeLast();
        QVERIFY(t.build({b,broken,c},raw()));QVERIFY(t.sample(4).bondIndex(1,2)<0); // Never restore a bond broken before departure.
    }
    void explicitMappingAndValidation() {
        auto a=step(0),b=step(2);std::swap(b.molecule.atoms[0],b.molecule.atoms[1]);b.molecule.bonds.clear();b.mapping={1,0,2};
        MechanismTimeline t;QString error;QVERIFY2(t.build({a,b},raw(),&error),qPrintable(error));QCOMPARE(t.trackCount(),3);
        b.mapping={0,0,2};QVERIFY(!t.build({a,b},raw(),&error));QVERIFY(!error.isEmpty());QCOMPARE(t.trackCount(),3);
        b.mapping={1,0,999};QVERIFY(!t.build({a,b},raw()));
        b=step(0);b.frames={{{0,0,0}}};QVERIFY(!t.build({a,b},raw()));
    }
    void multiframeReverseAndFocus() {
        auto s=step(0);s.frames={{{0,0,0},{1.4f,0,0},{0,1.4f,0}},{{1,0,0},{2.4f,0,0},{1,1.4f,0}},{{2,0,0},{3.4f,0,0},{2,1.4f,0}}};s.reversed=true;
        MechanismTimeline t;QVERIFY(t.build({s},raw()));QCOMPARE(t.sample(0).atoms[0].position.x(),2.f);QCOMPARE(t.sample(2).atoms[0].position.x(),0.f);
        t.emphasize({0},1);QCOMPARE(t.sample(1).atoms[1].displayOpacity,.2f);QCOMPARE(t.sample(1).atoms[0].displayOpacity,1.f);
        t.emphasize({0},0);QVERIFY(!t.sample(1).atoms[1].visible);t.emphasize({},2);QVERIFY(t.sample(1).atoms[1].visible);
    }
    void unchangedBondsAndChangingBonds() {
        auto a=step(0),b=step(0);b.molecule.atoms[1].position={0,0,1.4f};b.molecule.bonds.removeLast();
        MechanismTimeline t;auto options=raw();options.preserveBonds=true;QVERIFY(t.build({a,b},options));
        const auto m=t.sample(1);QVERIFY(qAbs((m.atoms[0].position-m.atoms[1].position).length()-1.4f)<.01f);
        const int bond=m.bondIndex(0,2);QVERIFY(bond>=0);QVERIFY(qAbs(m.bonds[bond].displayOpacity-.5f)<.001f);QVERIFY(t.sample(2).bondIndex(0,2)<0);
    }
    void photoCleavageFlashesOnlyMarkedBondAndAtoms() {
        auto a=step(0),b=step(1);
        b.molecule.removeBond(0,1);
        MechanismTimeline timeline;
        QVERIFY(timeline.build({a,b},raw()));
        QCOMPARE(timeline.cleavableBonds(0),(QVector<QPair<int,int>>{{0,1}}));
        QCOMPARE(timeline.sample(1).atoms[1].displayFlash,0.f);
        a.presentation["photoCleavages"]=QJsonArray{QJsonArray{0,1},QJsonArray{0,2}};
        QVERIFY(timeline.build({a,b},raw()));
        const auto middle=timeline.sample(1);
        QVERIFY(middle.atoms[0].displayFlash>0);
        QVERIFY(middle.atoms[1].displayFlash>0);
        QCOMPARE(middle.atoms[2].displayFlash,0.f);
        QVERIFY(middle.bonds.at(middle.bondIndex(0,1)).displayFlash>0);
        QCOMPARE(middle.bonds.at(middle.bondIndex(0,2)).displayFlash,0.f);
        QCOMPARE(timeline.sample(2).atoms[1].displayFlash,0.f);
        QCOMPARE(timeline.sample(2).bondIndex(0,1),-1);
    }
    void projectRoundTripAndInvalidInput() {
        auto a=step(0),b=step(2);a.molecule.atoms[1].displayOpacity=.2f;b.mapping={0,1,2};b.reversed=true;
        const auto json=MechanismTimeline::saveProject({a,b},raw());QVector<MechanismStep> loaded;MechanismOptions o;QString error;
        QVERIFY2(MechanismTimeline::loadProject(json,loaded,o,&error),qPrintable(error));QCOMPARE(loaded.size(),2);QCOMPARE(loaded[1].mapping,b.mapping);QCOMPARE(loaded[0].molecule.atoms[1].displayOpacity,.2f);QVERIFY(!o.align);
        QVERIFY(!MechanismTimeline::loadProject("{}",loaded,o,&error));QCOMPARE(loaded.size(),2);
        a.molecule.atoms[0].position.setX(std::numeric_limits<float>::quiet_NaN());MechanismTimeline t;QVERIFY(!t.build({a},raw(),&error));
    }
    void segmentHoldSpeedAndRepeatedCleavage() {
        auto a=step(0),b=step(0);
        a.presentation["hold"]=.5;
        a.presentation["photoCleavages"]=QJsonArray{QJsonArray{0,1,1}};
        b.presentation["speed"]=2;
        b.presentation["hold"]=.25;
        b.frames={{{0,0,0},{4,0,0},{0,1.4f,0}},
                  {{0,0,0},{1.4f,0,0},{0,1.4f,0}},
                  {{0,0,0},{4,0,0},{0,1.4f,0}}};
        MechanismTimeline timeline;
        QVERIFY(timeline.build({a,b},raw()));
        QCOMPARE(timeline.stageTime(0),0.);
        QCOMPARE(timeline.stageTime(1),1.5);
        QCOMPARE(timeline.duration(),1.75);
        QCOMPARE(timeline.sample(.25).atoms[1].position,a.molecule.atoms[1].position);
        QCOMPARE(timeline.sample(1.65).atoms[1].position,timeline.sample(1.5).atoms[1].position);
        const auto events=timeline.cleavageTimes(0,0,1);
        QCOMPARE(events.size(),2);
        QCOMPARE(timeline.sample(events[0]).atoms[1].displayFlash,0.f);
        QVERIFY(timeline.sample(events[1]).atoms[1].displayFlash>0);
    }
    void calculationEnergyTracksFrameOrder() {
        CalculationData data;data.format="XYZ";data.molecule=step(0).molecule;
        for(double energy:{-100.0,-99.9}) {
            Frame frame;frame.metrics.energyHartree=energy;
            for(const auto &atom:data.molecule.atoms)frame.coordinates.push_back(atom.position);
            data.frames.push_back(frame);
        }
        const auto imported=MechanismStep::fromCalculation(data);
        QCOMPARE(imported.presentation["sourceEnergiesHartree"].toArray(),(QJsonArray{-100.0,-99.9}));
    }
    void selectedSpectatorMovesAsRigidFragment() {
        auto a=step(0),b=step(1);MechanismTimeline t;QVERIFY(t.build({a,b},raw()));
        const auto baseline=t.sample(2);t.park({1,2},0,1);
        const auto start=t.sample(0),end=t.sample(2);QCOMPARE(start.atoms[1].position,a.molecule.atoms[1].position);
        QCOMPARE(end.atoms[1].position,baseline.atoms[1].position+QVector3D(0,6,0));
        QVERIFY(((end.atoms[2].position-end.atoms[1].position)-(baseline.atoms[2].position-baseline.atoms[1].position)).length()<1e-5f);
        QVERIFY(qAbs(end.atoms[1].displayOpacity-.35f)<1e-5f);QVERIFY(end.bonds.first().displayOpacity<.001f);
        t.park({},0);QCOMPARE(t.sample(2).atoms[1].position,baseline.atoms[1].position);
    }
    void streamingGifAndCancellation() {
        QTemporaryDir dir;const auto path=dir.filePath("movie.gif");int rendered=0;QString error;
        auto make=[](int i){QImage image(64,48,QImage::Format_ARGB32);image.fill(i%2?Qt::red:Qt::blue);return image;};
        QVERIFY2(ImageExporter::saveGifFrames(5,{64,48},path,[&](int i){++rendered;return make(i);},40,false,false,&error),qPrintable(error));QCOMPARE(rendered,5);
        QImageReader reader(path);QVERIFY(reader.canRead());QCOMPARE(reader.imageCount(),5);const auto first=reader.read();QVERIFY(!first.isNull());
        QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));const auto original=file.readAll();file.close();
        QVERIFY(!ImageExporter::saveGifFrames(5,{64,48},path,[&](int i){return i==2?QImage():make(i);},40,false,false,&error));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),original);
    }
    void suppliedDeformylationPath() {
        const auto directory=qEnvironmentVariable("YOUNGCRAFT_DEFORMYLATION_DIR");if(directory.isEmpty())QSKIP("User dataset is optional");
        const QStringList names={"1-CoIII-Me3-TPADP-CC293-BS-reverse-scan-freq-ts-freq-ircrev-opt-freq.out",
            "1-CoIII-Me3-TPADP-CC293-BS-reverse-scan-freq-ts-freq.out",
            "1-CoIII-Me3-TPADP-CC293-BS-reverse-scan-freq-ts-freq-ircfwd-opt-freq.out",
            "1-CoIII-Me3-TPADP-AfterCC-forOO_Cleavage-scan-freq-ts-freq.out",
            "1-CoIII-Me3-TPADP-AfterCC-forOO_Cleavage-scan-freq-ts-freq-ircfwd-opt-freq.out"};
        QVector<MechanismStep> steps;const QStringList labels={"Reactant","TS1 — C–C cleavage","After TS1","TS2 — O–O cleavage","After TS2"};
        for(int i=0;i<names.size();++i){const auto parsed=FileDispatcher::load(directory+"/"+names[i]).parseResult;QVERIFY2(parsed.success,qPrintable(parsed.error));auto s=MechanismStep::fromCalculation(parsed.data);s.name=labels[i];qInfo()<<names[i]<<"atoms"<<s.molecule.atoms.size()<<"frames"<<s.frames.size();steps.push_back(s);}
        MechanismTimeline t;QString error;QVERIFY2(t.build(steps,{},&error),qPrintable(error));QCOMPARE(t.trackCount(),steps.first().molecule.atoms.size());
        float maximum=0;for(int f=0;f<=200;++f){const auto frame=t.sample(t.duration()*f/200.);for(const auto&a:frame.atoms){QVERIFY(std::isfinite(a.position.x()));QVERIFY(std::isfinite(a.position.y()));QVERIFY(std::isfinite(a.position.z()));}if(f){const auto previous=t.sample(t.duration()*(f-1)/200.);for(int i=0;i<frame.atoms.size();++i)maximum=qMax(maximum,(frame.atoms[i].position-previous.atoms[i].position).length());}}
        qInfo()<<"Track summaries"<<t.summaries()<<"maximum movement per preview sample"<<maximum;
        QVERIFY(maximum<1.0f);
        if(const auto output=qEnvironmentVariable("YOUNGCRAFT_MECHANISM_PROJECT");!output.isEmpty()){QSaveFile file(output);QVERIFY(file.open(QIODevice::WriteOnly));const auto json=MechanismTimeline::saveProject(steps,{});QCOMPARE(file.write(json),json.size());QVERIFY(file.commit());}
    }
};
QTEST_GUILESS_MAIN(MechanismTest)
#include "test_mechanism.moc"
