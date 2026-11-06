#include "compute/XtbRunner.h"
#include "model/MoleculeEditController.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QProcessEnvironment>
#include <QUuid>
#include <QThread>
#include <cmath>
#include <algorithm>
#include <limits>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace youngcraft {
namespace {
const QString number = QStringLiteral("([-+]?\\d*\\.?\\d+(?:[EeDd][-+]?\\d+)?)");
double metric(const QString &line, const QString &label) {
    auto match = QRegularExpression(label + QStringLiteral("\\s*[:=]?\\s*") + number,
        QRegularExpression::CaseInsensitiveOption).match(line);
    return match.hasMatch() ? match.captured(1).replace('D','E').replace('d','e').toDouble() : kNaN;
}
}
QVector<XtbFrame> XtbTrajectoryParser::append(const QByteArray &bytes, const Molecule &expected) {
    pending += bytes;
    QVector<XtbFrame> frames;
    while (!pending.isEmpty() && error.isEmpty()) {
        int first = pending.indexOf('\n');
        if (first < 0) break;
        if (pending.left(first).trimmed().isEmpty()) { pending.remove(0, first+1); continue; }
        bool ok = false;
        int count = pending.left(first).trimmed().toInt(&ok);
        if (!ok || count != expected.atoms.size()) { error = QStringLiteral("xTB XYZ 원자 수가 입력 구조와 다릅니다."); break; }
        int end = first;
        bool complete = true;
        for (int i=0;i<count+1;++i) { end = pending.indexOf('\n',end+1); if(end<0){complete=false;break;} }
        if (!complete) break;
        auto lines = pending.left(end+1).split('\n');
        XtbFrame frame; frame.step = ++frameIndex;
        QString comment = QString::fromUtf8(lines[1]);
        frame.energy = metric(comment, QStringLiteral("energy"));
        // xtbscan.log uses "SCF done  -7.123..." rather than "energy:".
        if (!std::isfinite(frame.energy)) frame.energy = metric(comment, QStringLiteral("SCF\\s+done"));
        frame.gradient = metric(comment, QStringLiteral("gnorm"));
        for (int i=0;i<count;++i) {
            auto words = QString::fromUtf8(lines[i+2]).simplified().split(' ');
            if(words.size()<4 || atomicNumberFor(words[0]) != expected.atoms[i].atomicNumber) { error=QStringLiteral("xTB XYZ 원소 순서가 입력 구조와 다릅니다."); break; }
            double p[3];
            for(int j=0;j<3;++j) {p[j]=words[j+1].toDouble(&ok);if(!ok||!std::isfinite(p[j])){error=QStringLiteral("xTB XYZ 좌표가 유효하지 않습니다.");break;}}
            if(!error.isEmpty())break;
            frame.coordinates.push_back(QVector3D(p[0],p[1],p[2]));
        }
        if(!error.isEmpty())break;
        frames.push_back(frame);
        pending.remove(0,end+1);
    }
    return frames;
}
QStringList XtbRunner::solvents() {return {"water","acetonitrile","benzene","toluene","thf","ch2cl2","chcl3","dmf","dmso","methanol"};}
QString XtbRunner::xyz(const Molecule &m) {
    QString text=QString::number(m.atoms.size())+"\nYoungcraft xTB\n";
    for(const auto &a:m.atoms) text+=QStringLiteral("%1 %2 %3 %4\n").arg(a.element).arg(a.position.x(),0,'f',9).arg(a.position.y(),0,'f',9).arg(a.position.z(),0,'f',9);
    return text;
}
XtbRunner::XtbRunner(QObject *parent):QObject(parent) {
    timer.setInterval(50);
    connect(&timer,&QTimer::timeout,this,&XtbRunner::pollTrajectory);
    connect(&process,&QProcess::readyReadStandardOutput,this,&XtbRunner::readOutput);
    connect(&process,&QProcess::started,this,[this]{if(state==State::Starting)state=State::Running;timer.start();});
    connect(&process,&QProcess::finished,this,&XtbRunner::finish);
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
        if(e==QProcess::FailedToStart)fail(QStringLiteral("xTB 실행 오류: ")+process.errorString());
    });
    process.setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a){a->flags|=CREATE_NO_WINDOW;});
#endif
}
XtbRunner::~XtbRunner() { if(process.state()!=QProcess::NotRunning){process.kill();process.waitForFinished(1000);} }
bool XtbRunner::fail(const QString &message) {error=message;state=State::Failed;timer.stop();emit completed();return false;}
bool XtbRunner::start(const Molecule &initial,const XtbOptions &o,const QString &runtime) {
    if(process.state()!=QProcess::NotRunning)return false;
    Molecule m=initial;
    if(!validateConstraints(o.constraints,m.atoms.size(),&error))return fail(error);
    if(o.optimize&&!prepareFixedDistances(m,o.constraints,&error))return fail(error);
    options=o;input=m;result=m;result.charge=o.charge;result.multiplicity=o.multiplicity;
    output.clear();error.clear();geometryWarning.clear();outputPending.clear();parser={};offset=0;cycle=0;
    scanFrames.clear();tsCandidateIndex=-1;
    energy=gradient=gap=kNaN;convergence=nonconvergence=false;
    QString exe=QDir(runtime).absoluteFilePath("xtb.exe");
    const bool exactFix=std::any_of(o.constraints.cbegin(),o.constraints.cend(),[](const GeometryConstraint &c){return c.type=="fixed";});
    if(exactFix&&!QFileInfo::exists(QDir(runtime).absoluteFilePath("gfnff/xtb.exe")))return fail(QStringLiteral("정밀 원자 고정을 지원하는 동봉 xTB runtime이 필요합니다 (gfnff/xtb.exe)."));
    // The bundled newer build supports GFN1/2 as well as GFN-FF and honors Cartesian fixing.
    if((o.method==0 || exactFix) && QFileInfo::exists(QDir(runtime).absoluteFilePath("gfnff/xtb.exe")))
        exe=QDir(runtime).absoluteFilePath("gfnff/xtb.exe");
    if(!MoleculeEditController::validate(m,&error))return fail(error);
    if(!QFileInfo::exists(exe))return fail(QStringLiteral("xTB runtime을 찾지 못했습니다: ")+exe);
    if(m.isEmpty() || (o.method!=0&&o.method!=1&&o.method!=2) || o.multiplicity<1 || o.cycles<10 || o.cycles>5000 ||
       !QStringList{"loose","normal","tight"}.contains(o.level) || (!o.solvent.isEmpty()&&!solvents().contains(o.solvent)))return fail(QStringLiteral("xTB 계산 설정이 유효하지 않습니다."));
    if(!validateConstraints(o.constraints,m.atoms.size(),&error))return fail(error);
    if(o.scan) {
        const auto &s=*o.scan;
        const GeometryConstraint scanned{s.type,s.atoms,s.from};
        const bool endValid=std::isfinite(s.to) && (s.type=="distance" ? s.to>0 :
            s.type=="angle" ? s.to>0 && s.to<180 : s.type=="dihedral" && std::abs(s.to)<=720);
        if(!o.optimize || o.method==0 || s.points<3 || s.points>501 ||
           !endValid || std::abs(s.to-s.from)<1e-8 || !validateConstraints({scanned},m.atoms.size(),&error))
            return fail(QStringLiteral("PES Scan 설정이 유효하지 않습니다. GFN1/2, 유효한 원자·범위와 3–501개 point를 사용하세요."));
        for(const auto &existing:o.constraints) {
            if(existing.type!=s.type)continue;
            auto reverse=s.atoms;std::reverse(reverse.begin(),reverse.end());
            if(existing.atoms==s.atoms || existing.atoms==reverse)
                return fail(QStringLiteral("Scan 좌표가 다른 constraint에 이미 고정되어 있습니다."));
        }
    }
    int electrons=-o.charge;
    for(const auto &a:m.atoms){electrons+=a.atomicNumber;if(a.atomicNumber<1 || !std::isfinite(a.position.lengthSquared()))return fail(QStringLiteral("원소 또는 좌표가 유효하지 않습니다."));}
    if(o.method!=0 && (electrons<o.multiplicity-1 || (electrons-(o.multiplicity-1))%2))return fail(QStringLiteral("전자 수와 multiplicity가 맞지 않습니다."));
    if(o.method==0&&!o.solvent.isEmpty())return fail(QStringLiteral("GFN-FF 초기 버전은 gas phase 계산을 사용합니다."));
    directory=QDir::tempPath()+"/Youngcraft/xtb/"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    if(!QDir().mkpath(directory))return fail(QStringLiteral("계산 폴더를 만들 수 없습니다."));
    QFile file(directory+"/input.xyz");
    if(!file.open(QIODevice::WriteOnly)||file.write(xyz(m).toUtf8())<0)return fail(QStringLiteral("input.xyz를 저장할 수 없습니다."));
    file.close();
    logFile.setFileName(directory+"/xtb.out");
    if(!logFile.open(QIODevice::WriteOnly))return fail(QStringLiteral("xtb.out을 저장할 수 없습니다."));
    arguments={"input.xyz"};
    if(o.method==0)arguments<<"--gfnff";
    else arguments<<"--gfn"<<QString::number(o.method)<<"--uhf"<<QString::number(o.multiplicity-1);
    arguments<<"--chrg"<<QString::number(o.charge);
    QString controls;
    QStringList frozen;for(const auto &c:o.constraints)if(c.type=="fixed")frozen<<QString::number(c.atoms[0]+1);
    if(o.optimize&&frozen.size()==m.atoms.size())return fail(QStringLiteral("모든 원자가 고정되어 있습니다."));
    if(!frozen.isEmpty())controls+="$fix\n atoms: "+frozen.join(", ")+"\n$end\n";
    if(o.method==0) {
        // Both endpoints must be specified: GFN-FF otherwise infers missing neighbors.
        QVector<QSet<int>> neighbors(m.atoms.size());
        for(const auto &b:m.bonds){neighbors[b.first].insert(b.second);neighbors[b.second].insert(b.first);}
        for(const auto &h:m.hapticContacts)for(int i:h.atoms){neighbors[h.metal].insert(i);neighbors[i].insert(h.metal);}
        controls+="$ffnb\n";
        for(int i=0;i<neighbors.size();++i) {
            if(neighbors[i].size()>40)return fail(QStringLiteral("GFN-FF는 원자당 연결 원자 40개까지 지원합니다."));
            QStringList ids;auto ordered=neighbors[i].values();std::sort(ordered.begin(),ordered.end());
            for(int j:ordered)ids<<QString::number(j+1);
            controls+=QString(" nb = %1:%2\n").arg(i+1).arg(ids.isEmpty()?QStringLiteral("0"):ids.join(", "));
        }
        controls+="$end\n";
        arguments<<"--wrtopo"<<"nb";
    }
    if(o.preserveCoordination || o.preserveBondLengths || !o.constraints.isEmpty()) {
        controls+="$constrain\n force constant=1.0\n";
        for(const auto &bond:m.bonds) {
            bool retain=bond.coordination || isMetal(m.atoms[bond.first].atomicNumber) || isMetal(m.atoms[bond.second].atomicNumber);
            if(m.atoms[bond.first].atomicNumber==8&&m.atoms[bond.second].atomicNumber==8)
                for(int i=0;i<m.atoms.size();++i)
                    if(isMetal(m.atoms[i].atomicNumber)&&(m.bondIndex(i,bond.first)>=0||m.bondIndex(i,bond.second)>=0))retain=true;
            bool explicitDistance=false;
            for(const auto &c:o.constraints) if(c.type=="distance" && c.atoms.contains(bond.first) && c.atoms.contains(bond.second)) explicitDistance=true;
            const bool scannedBond=o.scan && o.scan->type=="distance" && o.scan->atoms.size()==2 &&
                ((o.scan->atoms[0]==bond.first && o.scan->atoms[1]==bond.second) ||
                 (o.scan->atoms[0]==bond.second && o.scan->atoms[1]==bond.first));
            if((o.preserveBondLengths || (o.preserveCoordination && retain)) && !explicitDistance && !scannedBond)controls+=QString(" distance: %1, %2, %3\n").arg(bond.first+1).arg(bond.second+1)
                .arg((m.atoms[bond.first].position-m.atoms[bond.second].position).length(),0,'f',8);
        }
        for(const auto &c:o.constraints) {
            if(c.type=="fixed")continue;
            QStringList ids; for(int i:c.atoms)ids<<QString::number(i+1);
            controls+=QString(" %1: %2, %3\n").arg(c.type,ids.join(", "),QString::number(c.target,'f',8));
        }
        controls+="$end\n";
    }
    if(o.scan) {
        const auto &s=*o.scan;
        QStringList ids;for(int atom:s.atoms)ids<<QString::number(atom+1);
        controls+="$scan\n";
        controls+=QString(" %1: %2, %3; %4, %5, %6\n")
            .arg(s.type,ids.join(", ")).arg(s.from,0,'f',8)
            .arg(s.from,0,'f',8).arg(s.to,0,'f',8).arg(s.points);
        controls+="$end\n";
    }
    if(!controls.isEmpty()) {
        QFile control(directory+"/framework.inp");const auto bytes=controls.toUtf8();
        if(!control.open(QIODevice::WriteOnly)||control.write(bytes)!=bytes.size())
            return fail(QStringLiteral("배위 골격 constraint 파일을 저장하지 못했습니다."));
        control.close();arguments<<"--input"<<"framework.inp";
    }
    if(o.optimize)arguments<<"--opt"<<o.level<<"--cycles"<<QString::number(o.cycles);
    else arguments<<"--grad"; // SP gradient, without a geometry optimization.
    if(!o.solvent.isEmpty())arguments<<"--alpb"<<o.solvent;
    auto env=QProcessEnvironment::systemEnvironment();
    int threads=o.threads==0?qMin(4,qMax(1,QThread::idealThreadCount()/2)):qBound(1,o.threads,8);
    env.insert("OPENBLAS_NUM_THREADS",QString::number(threads));env.insert("OMP_NUM_THREADS",QString::number(threads));env.insert("MKL_NUM_THREADS",QString::number(threads));
    env.insert("OMP_STACKSIZE","256M");env.insert("OMP_MAX_ACTIVE_LEVELS","1");
    env.insert("XTBPATH",QDir(runtime).absoluteFilePath("share"));
    env.remove("XTBHOME");
    env.insert("PATH",QDir(runtime).absolutePath()+QDir::listSeparator()+env.value("PATH"));
    process.setProcessEnvironment(env);process.setWorkingDirectory(directory);
    state=State::Starting;process.start(exe,arguments);return true;
}
void XtbRunner::readOutput() {
    const QByteArray bytes=process.readAllStandardOutput();
    logFile.write(bytes);logFile.flush();output+=QString::fromUtf8(bytes);outputPending+=bytes;
    int end;
    while((end=outputPending.indexOf('\n'))>=0) {
        QString line=QString::fromUtf8(outputPending.left(end));outputPending.remove(0,end+1);
        for(auto pair:{qMakePair(QStringLiteral("TOTAL ENERGY"),&energy),qMakePair(QStringLiteral("GRADIENT NORM"),&gradient),qMakePair(QStringLiteral("HOMO-LUMO GAP"),&gap)}) {
            double value=metric(line,pair.first);if(std::isfinite(value))*pair.second=value;
        }
        auto match=QRegularExpression("CYCLE\\s+(\\d+)").match(line);if(match.hasMatch())cycle=match.captured(1).toInt();
        if(line.contains("GEOMETRY OPTIMIZATION CONVERGED",Qt::CaseInsensitive))convergence=true;
        if(line.contains("FAILED TO CONVERGE",Qt::CaseInsensitive)||line.contains("NOT CONVERGED",Qt::CaseInsensitive)||line.contains("did not converge",Qt::CaseInsensitive))nonconvergence=true;
    }
    emit metricsChanged();
}
void XtbRunner::pollTrajectory() {
    QFile file(directory+(options.scan ? "/xtbscan.log" : "/xtbopt.log"));if(!file.open(QIODevice::ReadOnly))return;
    if(file.size()<offset){error=QStringLiteral("xTB trajectory 파일이 계산 중 잘렸습니다.");cancel();return;}
    file.seek(offset);const auto bytes=file.readAll();offset=file.pos();
    for(const auto &frame:parser.append(bytes,input)) {
        if(options.scan && !std::isfinite(frame.energy)) {error=QStringLiteral("xTB Scan frame의 에너지를 읽을 수 없습니다.");cancel();return;}
        for(const auto &c:options.constraints)if(c.type=="fixed"&&(frame.coordinates[c.atoms[0]]-input.atoms[c.atoms[0]].position).length()>0.00002f){error=QStringLiteral("xTB 실행기가 고정 원자의 좌표를 변경했습니다. 결과를 적용하지 않습니다.");cancel();return;}
        for(int i=0;i<result.atoms.size();++i)result.atoms[i].position=frame.coordinates[i];
        if(std::isfinite(frame.energy))energy=frame.energy;if(std::isfinite(frame.gradient))gradient=frame.gradient;
        cycle=frame.step;if(options.scan)scanFrames.push_back(frame);emit frameReady(frame);
    }
    if(!parser.error.isEmpty()){error=parser.error;cancel();}
}
void XtbRunner::finish(int code,QProcess::ExitStatus status) {
    readOutput();pollTrajectory();timer.stop();logFile.close();
    bool cancelled=state==State::Stopping;
    bool marker=QFileInfo::exists(directory+"/NOT_CONVERGED");
    QFile finalFile(directory+"/xtbopt.xyz");
    if(options.optimize&&!options.scan&&finalFile.open(QIODevice::ReadOnly)) {
        XtbTrajectoryParser finalParser;auto frames=finalParser.append(finalFile.readAll()+"\n",input);
        if(!finalParser.error.isEmpty())error=finalParser.error;
        else if(!frames.isEmpty()){const auto &f=frames.last();for(int i=0;i<result.atoms.size();++i)result.atoms[i].position=f.coordinates[i];}
    }
    if(options.scan) {
        double best=-std::numeric_limits<double>::infinity();
        for(int i=1;i+1<scanFrames.size();++i) {
            const double e=scanFrames[i].energy;
            if(std::isfinite(e) && std::isfinite(scanFrames[i-1].energy) &&
               std::isfinite(scanFrames[i+1].energy) && e>scanFrames[i-1].energy &&
               e>scanFrames[i+1].energy && e>best) {best=e;tsCandidateIndex=i;}
        }
        if(!scanFrames.isEmpty()) {
            const int choice=tsCandidateIndex>=0?tsCandidateIndex:scanFrames.size()-1;
            const auto &frame=scanFrames[choice];
            for(int i=0;i<result.atoms.size();++i)result.atoms[i].position=frame.coordinates[i];
            energy=frame.energy;cycle=choice+1;
        }
    }
    for(const auto &c:options.constraints)if(c.type=="fixed"&&(result.atoms[c.atoms[0]].position-input.atoms[c.atoms[0]].position).length()>0.00002f)error=QStringLiteral("xTB 실행기가 고정 원자의 좌표를 변경했습니다. 결과를 적용하지 않습니다.");
    if(!error.isEmpty())state=State::Failed;
    else if(cancelled)state=State::Cancelled;
    else if(options.method==0 && !output.contains("neighborlist has been adjusted")) {
        state=State::Failed;error=QStringLiteral("GFN-FF 실행기가 명시적 결합 목록을 적용하지 않았습니다. 함께 제공된 gfnff runtime을 사용하세요.");
    }
    else if(options.optimize&&(marker||nonconvergence))state=State::NotConverged;
    else if(options.scan && scanFrames.isEmpty()) {state=State::Failed;error=QStringLiteral("xTB Scan trajectory(xtbscan.log)가 생성되지 않았습니다.");}
    else if(code!=0||status!=QProcess::NormalExit||!std::isfinite(energy)) {state=State::Failed;error=QStringLiteral("xTB 실행 실패 (exit %1). 출력 보기를 확인하세요.").arg(code);}
    else if(options.scan)state=scanFrames.size()==options.scan->points?State::Finished:State::NotConverged;
    else if(options.optimize&&!convergence)state=State::NotConverged;
    else state=State::Finished;
    if(options.optimize && (state==State::Finished || state==State::NotConverged)) {
        QStringList changes;
        for(const auto &b:input.bonds) {
            const double before=(input.atoms[b.first].position-input.atoms[b.second].position).length();
            const double after=(result.atoms[b.first].position-result.atoms[b.second].position).length();
            bool requested=false;
            for(const auto &c:options.constraints) if(c.type=="distance" && c.atoms.contains(b.first) && c.atoms.contains(b.second))requested=true;
            if(!requested && before>0.1 && (after<before*0.65 || (after>before*1.35 && after-before>0.35)))
                changes<<QStringLiteral("%1–%2: %3 → %4 Å").arg(b.first+1).arg(b.second+1).arg(before,0,'f',2).arg(after,0,'f',2);
        }
        if(!changes.isEmpty())geometryWarning=QStringLiteral("결합 연결이 크게 변했습니다 (%1개): %2").arg(changes.size()).arg(changes.mid(0,5).join("; "));
    }
    emit completed();
}
bool XtbRunner::selectScanFrame(int index) {
    if(isRunning() || index<0 || index>=scanFrames.size())return false;
    const auto &frame=scanFrames[index];
    if(frame.coordinates.size()!=result.atoms.size())return false;
    for(int i=0;i<result.atoms.size();++i)result.atoms[i].position=frame.coordinates[i];
    energy=frame.energy;cycle=index+1;
    return true;
}
void XtbRunner::cancel() {
    if(state!=State::Starting&&state!=State::Running)return;
    state=State::Stopping;process.terminate();
    QTimer::singleShot(1500,this,[this]{if(state==State::Stopping&&process.state()!=QProcess::NotRunning)process.kill();});
}
}
