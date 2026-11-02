#include <QScopeGuard>
#include "compute/GaussianRemote.h"
#include <QDir>
#include "compute/GaussianTools.h"
#include "io/FchkParser.h"
#include "io/FileDispatcher.h"
#include "io/NboParser.h"
#include "render/IsosurfaceBuilder.h"
#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>
using namespace youngcraft;
static QByteArray fchk(bool beta=false) {
    QByteArray text="Fixture\nSP UHF STO-3G\nNumber of atoms                            I                1\nNumber of alpha electrons                  I                1\nNumber of beta electrons                   I                0\nAtomic numbers                             I   N=           1\n 1\nCurrent cartesian coordinates              R   N=           3\n 0.0 0.0 0.0\nAlpha Orbital Energies                      R   N=           3\n -0.5 0.1 0.4\n";
    if(beta) text+="Beta Orbital Energies                       R   N=           3\n -0.3 0.2 0.5\n";
    return text;
}
static bool write(const QString &path,const QByteArray &text){QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(text)==text.size();}
static QByteArray cube(bool mo=false) {
    return QByteArray("Fixture\naxis order\n")+(mo?"-1":"1")+" 0 0 0\n2 1 0 0\n2 0 1 0\n2 0 0 1\n1 1 0 0 0\n"+(mo?"1 7\n":"")+"0 1 10 11 100 101 110 111\n";
}
class OrbitalTest : public QObject {
    Q_OBJECT
private slots:
    void fchkOccupationsAndSpin() {
        QTemporaryDir dir;const auto path=dir.filePath("test.fchk");QVERIFY(write(path,fchk(true)));
        const auto r=FchkParser::parse(path);QVERIFY(r.success);QCOMPARE(r.data.orbitals.size(),6);
        QCOMPARE(r.data.orbitals[0].occupation,1.0);QCOMPARE(r.data.orbitals[3].spin,QString("Beta"));QCOMPARE(r.data.orbitals[3].number,1);QCOMPARE(r.data.orbitals[3].occupation,0.0);
        QVERIFY(write(path,fchk(false)));const auto restricted=FchkParser::parse(path);QCOMPARE(restricted.data.orbitals[0].occupation,1.0);
    }
    void cubeMoUnitsAndAxisOrder() {
        QTemporaryDir dir;const auto path=dir.filePath("mo.cube");QVERIFY(write(path,cube(true)));
        const auto grid=CubeParser::parse(path);QVERIFY2(grid.valid,qPrintable(grid.error));
        const auto cubPath=dir.filePath("orbital.CUB");QVERIFY(write(cubPath,cube(true)));
        const auto loaded=FileDispatcher::load(cubPath,{});
        QVERIFY(loaded.isCube);QVERIFY(loaded.parseResult.success);QCOMPARE(loaded.format,QString("CUBE"));
        QVERIFY(qAbs(grid.axisX.x()-0.529177210903)<1e-6);QCOMPARE(grid.value(1,0,0),100.0f);QCOMPARE(grid.value(0,0,1),1.0f);
        QVERIFY(qAbs(grid.sample((grid.axisX+grid.axisY+grid.axisZ)*.5f)-55.5f)<1e-3);QVERIFY(std::isnan(grid.sample(QVector3D(-100,0,0))));
        auto malformed=cube(true);malformed.replace("1 7\n","2 7 8\n");QVERIFY(write(path,malformed));QVERIFY(!CubeParser::parse(path).valid);
        auto angstrom=cube(false);angstrom.replace("2 1 0 0","-2 1 0 0");angstrom.replace("2 0 1 0","-2 0 1 0");angstrom.replace("2 0 0 1","-2 0 0 1");QVERIFY(write(path,angstrom));QCOMPARE(CubeParser::parse(path).axisX.x(),1.f);
    }
    void skewInterpolationAndSurface() {
        CubeGrid grid;grid.valid=true;grid.nx=grid.ny=grid.nz=2;grid.axisX={1,0,0};grid.axisY={.2f,1,0};grid.axisZ={0,.3f,1};grid.values={0,100,10,110,1,101,11,111};
        QCOMPARE(grid.sample((grid.axisX+grid.axisY+grid.axisZ)*.5f),55.5f);
        auto mesh=IsosurfaceBuilder::build(grid,50,true);QVERIFY(!mesh.isEmpty());for(const auto &point:mesh.positions)QVERIFY(qAbs(grid.sample(point)-50)<1e-3);
    }
    void nboRepeatedBlocks() {
        QTemporaryDir dir;const auto path=dir.filePath("nbo.out");
        const QByteArray block="Summary of Natural Population Analysis:\n Atom No Charge Core Valence Rydberg Total\n C 1 -0.20 2.0 4.1 0.1 6.2\n H 2 0.20 0.0 0.8 0.0 0.8\n * Total *\n (Occupancy) Bond orbital / Coefficients / Hybrids\n 1. (1.98000) BD ( 1) C 1 - H 2\n\n\n\nSecond Order Perturbation Theory Analysis of Fock Matrix in NBO Basis\n 1. BD ( 1) C 1 - H 2      7. BD*( 1) C 1 - H 3       12.34 0.50 0.070\n\n\n\nWiberg bond index matrix in the NAO basis:\n Atom 1 2\n -----\n 1. C 0.0000 0.9500\n 2. H 0.9500 0.0000\n\n Atom 3\n 1. C 0.1500\n 2. H 0.2500\n 3. H 0.0000\nWiberg bond index, Totals by atom:\n";
        QVERIFY(write(path,block+block));auto n=NboParser::parse(path);
        QCOMPARE(n.charges.size(),4);QCOMPARE(n.orbitals.size(),2);QCOMPARE(n.interactions.size(),2);QCOMPARE(n.wiberg.size(),6);
        QCOMPARE(n.charges[0].values[2],QString("-0.20"));QCOMPARE(n.interactions[0].values[2],QString("12.34"));QCOMPARE(n.wiberg[0].values[2],QString("0.95"));QVERIFY(n.wiberg[0].section!=n.wiberg[3].section);
    }
    void runnerAndConversion() {
        QTemporaryDir dir;const auto source=dir.filePath(QStringLiteral("한글 입력.chk"));QVERIFY(write(source,"test"));
        QSettings().setValue("orbitals/formchk",QCoreApplication::applicationFilePath());QSettings().setValue("orbitals/cubegen",QCoreApplication::applicationFilePath());
        QString error;const auto converted=GaussianTools::convertCheckpoint(source,&error);QVERIFY2(!converted.isEmpty(),qPrintable(error));QVERIFY(FchkParser::parse(converted).success);
        GaussianTools runner;QSignalSpy failed(&runner,&GaussianTools::failed);int received=0;std::shared_ptr<const CubeGrid> surface,potential;
        connect(&runner,&GaussianTools::ready,this,[&](auto s,auto p){++received;surface=s;potential=p;});
        runner.generate(source,"MEP",40,"SCF");QTRY_COMPARE_WITH_TIMEOUT(received,1,15000);QCOMPARE(failed.size(),0);QVERIFY(surface&&potential);QVERIFY(!runner.busy());
        runner.generate(converted,"AMO=1",40,"SCF");QTRY_COMPARE_WITH_TIMEOUT(received,2,15000);QVERIFY(surface&&!potential);
        qputenv("YOUNGCRAFT_FAKE_GAUSSIAN_MODE","fail");runner.generate(source,"MEP",40,"SCF");QTRY_COMPARE_WITH_TIMEOUT(failed.size(),1,15000);QVERIFY(!runner.busy());
        qputenv("YOUNGCRAFT_FAKE_GAUSSIAN_MODE","hang");runner.generate(source,"MEP",40,"SCF");QTest::qWait(100);runner.cancel();QCOMPARE(failed.size(),2);QVERIFY(!runner.busy());
        qunsetenv("YOUNGCRAFT_FAKE_GAUSSIAN_MODE");
        QSettings().setValue("orbitals/cubegen",dir.filePath("missing.exe"));runner.generate(converted,"AMO=1",40,"SCF");QTRY_COMPARE(failed.size(),3);QVERIFY(!runner.busy());
    }
    void missingToolsFailBeforeCopy() {
        QTemporaryDir dir;
        QSettings().setValue("orbitals/formchk",dir.filePath("missing-formchk.exe"));
        QSettings().setValue("orbitals/cubegen",dir.filePath("missing-cubegen.exe"));
        QVERIFY(!GaussianTools::isAvailable("formchk"));
        QString error;
        QVERIFY(GaussianTools::convertCheckpoint(dir.filePath("not-copied.chk"),&error).isEmpty());
        QVERIFY(error.contains("formchk"));
        GaussianTools runner; QSignalSpy failed(&runner,&GaussianTools::failed);
        runner.generate(dir.filePath("not-copied.fchk"),"AMO=1",40,"SCF");
        QCOMPARE(failed.size(),1); QVERIFY(failed.first().first().toString().contains("cubegen"));
        QVERIFY(!runner.busy());
        const auto real=qEnvironmentVariable("YOUNGCRAFT_TEST_CHK");
        if(!real.isEmpty()) {
            QElapsedTimer timer; timer.start();const auto loaded=FileDispatcher::load(real,{});
            QVERIFY(!loaded.parseResult.success); QVERIFY(loaded.parseResult.error.contains("formchk"));
            QVERIFY(timer.elapsed()<2000);
        }
    }
    void bundledAutomaticServerDefaults() {
        const auto org=QCoreApplication::organizationName(), app=QCoreApplication::applicationName();
        QCoreApplication::setOrganizationName("YoungcraftTests");QCoreApplication::setApplicationName("AutoServerDefaults");QSettings().clear();
        auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(org);QCoreApplication::setApplicationName(app);});
        QTemporaryDir dir;const auto path=dir.filePath("server.json");
        QVERIFY(write(path,R"({"remoteHost":"user@host","remotePort":10022,"remoteConfig":"~/.ssh/lion.conf","remoteEnabled":true,"autoGenerateHomo":true})"));
        GaussianRemoteSettings::initializeDefaults(path);
        QVERIFY(QSettings().value("orbitals/remoteEnabled").toBool());QVERIFY(QSettings().value("orbitals/autoGenerateHomo").toBool());
        QCOMPARE(GaussianRemoteSettings::load().host,QString("user@host"));QCOMPARE(GaussianRemoteSettings::load().port,10022);
        QCOMPARE(GaussianRemoteSettings::load().configFile,QDir::homePath()+"/.ssh/lion.conf");
        QSettings().setValue("orbitals/remotePort",20022);GaussianRemoteSettings::initializeDefaults(path);
        QCOMPARE(GaussianRemoteSettings::load().port,20022);
    }
    void remoteSettingsAndQuoting() {
        QCOMPARE(GaussianRemote::quote("a'b; $(echo x)"),QString("'a'\"'\"'b; $(echo x)'"));
        GaussianRemoteSettings settings;settings.host="-oProxyCommand=x";settings.gaussianDirectory="/opt/gaussian/g16";
        QVERIFY(!settings.error().isEmpty());settings.host="user@host";settings.port=0;QVERIFY(!settings.error().isEmpty());
        settings.port=22;settings.gaussianDirectory="relative";QVERIFY(!settings.error().isEmpty());
        settings.gaussianDirectory="/opt/gaussian/g16";settings.checkpointDirectory="relative";QVERIFY(!settings.error().isEmpty());
    }
    void realRemoteServerIfConfigured() {
        const auto host=qEnvironmentVariable("YOUNGCRAFT_REMOTE_HOST");if(host.isEmpty())QSKIP("Remote integration not requested");
        QSettings settings;settings.setValue("orbitals/remoteEnabled",true);settings.setValue("orbitals/remoteHost",host);
        settings.setValue("orbitals/remotePort",qEnvironmentVariableIntValue("YOUNGCRAFT_REMOTE_PORT"));
        settings.setValue("orbitals/remoteConfig",qEnvironmentVariable("YOUNGCRAFT_REMOTE_CONFIG"));
        settings.setValue("orbitals/remoteGaussianDirectory","/opt/gaussian/g16");
        settings.setValue("orbitals/remoteCheckpointDirectory","/home/skylark/chk");
        const auto source=qEnvironmentVariable("YOUNGCRAFT_REMOTE_CHK");QVERIFY(QFileInfo::exists(source));
        QString error;const auto formatted=GaussianTools::convertCheckpoint(source,&error);QVERIFY2(!formatted.isEmpty(),qPrintable(error));
        const auto parsed=FchkParser::parse(formatted);QVERIFY(parsed.success);QVERIFY(!parsed.data.orbitals.isEmpty());
        int homo=0;for(const auto &o:parsed.data.orbitals)if(o.spin=="Alpha"&&o.occupation>0)homo=qMax(homo,o.number);QVERIFY(homo>0);
        qInfo()<<"Actual server FCHK atoms"<<parsed.data.molecule.atoms.size()<<"MO"<<parsed.data.orbitals.size()<<"HOMO"<<homo;
        const auto dest=qEnvironmentVariable("YOUNGCRAFT_REMOTE_RESULTS");if(!dest.isEmpty()){QDir().mkpath(dest);QVERIFY(QFile::copy(formatted,dest+"/converted.fchk"));}
        GaussianTools runner;QSignalSpy failed(&runner,&GaussianTools::failed);int received=0;
        connect(&runner,&GaussianTools::status,this,[](const QString &message){qInfo().noquote()<<message;});
        std::shared_ptr<const CubeGrid> surface,potential;
        connect(&runner,&GaussianTools::ready,this,[&](auto s,auto p){surface=s;potential=p;++received;});
        runner.generate(formatted,"AMO="+QString::number(homo),40,"SCF");
        QTRY_VERIFY_WITH_TIMEOUT(received==1 || !failed.isEmpty(),300000);
        QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));QVERIFY(surface&&surface->valid);QVERIFY(!potential);
        QVERIFY(surface->minValue<0);QVERIFY(surface->maxValue>0);QVERIFY(!IsosurfaceBuilder::build(*surface,.02f,true).isEmpty());
        if(!dest.isEmpty())QVERIFY(QFile::copy(surface->filePath,dest+"/homo.cube"));
        qInfo()<<"Actual HOMO grid"<<surface->nx<<surface->ny<<surface->nz<<"range"<<surface->minValue<<surface->maxValue;
        runner.generate(formatted,"MEP",80,"SCF");
        QTRY_VERIFY_WITH_TIMEOUT(received==2 || !failed.isEmpty(),300000);
        QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));QVERIFY(surface&&potential&&potential->valid);
        QVERIFY(!IsosurfaceBuilder::build(*surface,.001f,true).isEmpty());
        QVERIFY(std::isfinite(potential->sample(surface->origin+(surface->nx/2)*surface->axisX+(surface->ny/2)*surface->axisY+(surface->nz/2)*surface->axisZ)));
        if(!dest.isEmpty()){QVERIFY(QFile::copy(surface->filePath,dest+"/density.cube"));QVERIFY(QFile::copy(potential->filePath,dest+"/potential.cube"));}
        qInfo()<<"Actual MEP grid"<<surface->nx<<surface->ny<<surface->nz<<"potential range"<<potential->minValue<<potential->maxValue;
        const auto cancellation=connect(&runner,&GaussianTools::status,this,[&](const QString &message){if(message.startsWith(QStringLiteral("서버 cubegen:")))runner.cancel();});
        runner.generate(formatted,"AMO="+QString::number(homo),120,"SCF");
        QTRY_VERIFY_WITH_TIMEOUT(!runner.busy(),60000);disconnect(cancellation);
        QCOMPARE(received,2);QCOMPARE(failed.size(),1);QVERIFY(failed.first().first().toString().contains(QStringLiteral("취소")));
        const auto large=qEnvironmentVariable("YOUNGCRAFT_REMOTE_LARGE_CHK");
        if(!large.isEmpty()) {
            bool reused=false;
            const auto converted=GaussianRemote::convert(large,GaussianRemoteSettings::load(),{},[&](const QString &message){
                qInfo().noquote()<<message;if(message.contains(QStringLiteral("SHA-256 일치")))reused=true;
            });
            QVERIFY2(converted.error.isEmpty(),qPrintable(converted.error));QVERIFY(reused);
            const auto largeParsed=FchkParser::parse(converted.formattedPath);QVERIFY(largeParsed.success);QVERIFY(!largeParsed.data.orbitals.isEmpty());
            qInfo()<<"Actual TD-CHK conversion"<<QFileInfo(large).size()<<"bytes, atoms"<<largeParsed.data.molecule.atoms.size()<<"MO"<<largeParsed.data.orbitals.size();
            if(!dest.isEmpty())QVERIFY(QFile::copy(converted.formattedPath,dest+"/td-converted.fchk"));
        }
        settings.setValue("orbitals/remoteEnabled",false);
    }
    void realFchkIfConfigured() {
        const auto path=qEnvironmentVariable("YOUNGCRAFT_TEST_FCHK");if(path.isEmpty())QSKIP("No real FCHK supplied");
        const auto result=FchkParser::parse(path);QVERIFY2(result.success,qPrintable(result.error));QVERIFY(!result.data.orbitals.isEmpty());int occupied=0;for(const auto &o:result.data.orbitals)if(o.occupation>0)++occupied;QVERIFY(occupied>0);
        qInfo()<<"Real FCHK atoms"<<result.data.molecule.atoms.size()<<"orbitals"<<result.data.orbitals.size()<<"occupied"<<occupied;
    }
    void cleanupTestCase(){QSettings().clear();}
};
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QCoreApplication::setOrganizationName("YoungcraftTests");QCoreApplication::setApplicationName("OrbitalsRegression");
    const auto args=app.arguments();
    if(args.size()>=3 && (args[1]=="input.chk"||args[1]=="0")) {
        const auto mode=qEnvironmentVariable("YOUNGCRAFT_FAKE_GAUSSIAN_MODE");if(mode=="fail")return 3;if(mode=="hang")QThread::sleep(30);
        if(args[1]=="input.chk")return write(args[2],fchk(true))?0:2;
        if(args.size()!=7||args[3]!="input.fchk"||args[6]!="h")return 4;
        return write(args[4],cube(args[2].startsWith("AMO=")))?0:2;
    }
    OrbitalTest test;return QTest::qExec(&test,argc,argv);
}
#include "test_orbitals.moc"
