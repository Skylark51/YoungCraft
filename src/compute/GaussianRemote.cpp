#include "compute/GaussianRemote.h"
#include "compute/SshPasswordBroker.h"
#include "io/FchkParser.h"
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

namespace youngcraft {
void GaussianRemoteSettings::initializeDefaults(const QString &profilePath) {
    QFile file(profilePath.isEmpty()?QCoreApplication::applicationDirPath()+"/youngcraft-server.json":profilePath);
    if(!file.open(QIODevice::ReadOnly))return;
    const auto profile=QJsonDocument::fromJson(file.readAll()).object();
    QSettings settings;
    for(auto it=profile.begin();it!=profile.end();++it) {
        if(!QStringList{"remoteEnabled","autoGenerateHomo","remoteHost","remotePort","remoteConfig","remoteIdentity","remoteGaussianDirectory","remoteCheckpointDirectory","remotePasswordAuthentication"}.contains(it.key()))continue;
        const auto key="orbitals/"+it.key();if(settings.contains(key))continue;
        auto value=it.value().toVariant();
        if((it.key()=="remoteConfig"||it.key()=="remoteIdentity")&&value.toString().startsWith("~/"))
            value=QDir::homePath()+value.toString().mid(1);
        settings.setValue(key,value);
    }
    // Retire the missing config path shipped by the previous portable version.
    const auto obsolete=QDir::homePath()+"/.ssh/lion.conf";
    if(settings.value("orbitals/remotePasswordAuthentication",true).toBool()
        && settings.value("orbitals/remoteConfig").toString()==obsolete && !QFileInfo::exists(obsolete))
        settings.remove("orbitals/remoteConfig");
    settings.sync();
}
GaussianRemoteSettings GaussianRemoteSettings::load() {
    QSettings s;
    return {s.value("orbitals/remoteHost").toString().trimmed(),
            s.value("orbitals/remoteConfig").toString().trimmed(),
            s.value("orbitals/remoteIdentity").toString().trimmed(),
            s.value("orbitals/remoteGaussianDirectory", "/opt/gaussian/g16").toString().trimmed(),
            s.value("orbitals/remoteCheckpointDirectory").toString().trimmed(),
            s.value("orbitals/remotePort", 22).toInt(),
            s.value("orbitals/remotePasswordAuthentication", true).toBool()};
}
QString GaussianRemoteSettings::error() const {
    static const QRegularExpression hostPattern(QStringLiteral("^(?:[A-Za-z0-9_][A-Za-z0-9_.-]*@)?[A-Za-z0-9_][A-Za-z0-9_.-]*$"));
    if (!hostPattern.match(host).hasMatch() || port < 1 || port > 65535)
        return QStringLiteral("계산 서버 주소 또는 SSH 포트를 확인하세요.");
    if (!gaussianDirectory.startsWith('/') || gaussianDirectory.contains('\n') || gaussianDirectory.contains('\r'))
        return QStringLiteral("서버 Gaussian 폴더는 절대 경로로 지정하세요.");
    if (!checkpointDirectory.isEmpty() && (!checkpointDirectory.startsWith('/') || checkpointDirectory.contains('\n') || checkpointDirectory.contains('\r')))
        return QStringLiteral("서버 CHK 폴더는 절대 경로로 지정하세요.");
    for (const auto &path : QStringList{configFile, passwordAuthentication ? QString() : identityFile})
        if (!path.isEmpty() && !QFileInfo(path).isFile()) return QStringLiteral("SSH 설정/키 파일을 찾을 수 없습니다: %1").arg(path);
    for (const auto &name : {"ssh", "scp"})
        if (QStandardPaths::findExecutable(QString::fromLatin1(name)).isEmpty()) return QStringLiteral("Windows OpenSSH %1 실행 파일을 찾을 수 없습니다.").arg(name);
    return {};
}
QString GaussianRemote::quote(const QString &value) {
    QString escaped = value;
    escaped.replace("'", "'\"'\"'");
    return "'" + escaped + "'";
}
namespace {
class Job {
public:
    GaussianRemoteSettings settings;
    GaussianRemote::Cancellation cancel;
    GaussianRemote::Progress progress;
    QString relative = ".cache/youngcraft/orbitals/job-" + QUuid::createUuid().toString(QUuid::Id128);
    QString local, error, stage;
    bool created = false;
    Job(const GaussianRemoteSettings &s, GaussianRemote::Cancellation c, GaussianRemote::Progress p)
        : settings(s), cancel(std::move(c)), progress(std::move(p)) {
        local = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/orbitals/" + QUuid::createUuid().toString(QUuid::Id128);
    }
    bool cancelled() const { return cancel && cancel->load(); }
    void status(const QString &s) { stage=s; if (progress) progress(s); }
    QStringList options(bool scp) const {
        QStringList args{"-o", settings.passwordAuthentication ? "BatchMode=no" : "BatchMode=yes", "-o", "StrictHostKeyChecking=yes", "-o", "ConnectTimeout=15",
                         "-o", "ServerAliveInterval=15", "-o", "ServerAliveCountMax=3"};
        if (!settings.configFile.isEmpty()) args << "-F" << settings.configFile;
        if (!settings.passwordAuthentication && !settings.identityFile.isEmpty()) args << "-i" << settings.identityFile << "-o" << "IdentitiesOnly=yes";
        if (settings.passwordAuthentication) args << "-o" << "PreferredAuthentications=password,keyboard-interactive"
            << "-o" << "PubkeyAuthentication=no" << "-o" << "NumberOfPasswordPrompts=1";
        args << (scp ? "-P" : "-p") << QString::number(settings.port);
        return args;
    }
    bool process(const QString &program, const QStringList &args, const QByteArray &input,
                 int timeoutMs, QByteArray *output = nullptr, bool cleanup = false) {
        if (!cleanup && cancelled()) { error = QStringLiteral("계산을 취소했습니다."); return false; }
        QProcess p;
        p.setWorkingDirectory(QCoreApplication::applicationDirPath());
        p.setProcessChannelMode(QProcess::MergedChannels);
        if (settings.passwordAuthentication)
            p.setProcessEnvironment(SshPasswordBroker::environment(settings.host+':'+QString::number(settings.port), !cleanup));
        p.start(QStandardPaths::findExecutable(program), args);
        if (!p.waitForStarted(10000)) {
            error = QStringLiteral("%1 시작 실패: %2").arg(program, p.errorString()); return false;
        }
        p.write(input); p.closeWriteChannel();
        QElapsedTimer elapsed; elapsed.start(); QByteArray log;
        while (!p.waitForFinished(100)) {
            log += p.readAll(); if (log.size() > 32000) log = log.right(32000);
            if ((!cleanup && cancelled()) || elapsed.elapsed() > timeoutMs) {
                p.kill(); p.waitForFinished(3000);
                error = cancelled() ? QStringLiteral("계산을 취소했습니다.") : QStringLiteral("서버 작업 제한 시간을 초과했습니다.");
                return false;
            }
        }
        log += p.readAll();
        if (output) *output = log;
        if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
            if (settings.passwordAuthentication && log.contains("Permission denied")) SshPasswordBroker::forgetPasswords();
            error = stage + QStringLiteral("\n서버 %1 실패 (%2):\n%3\n비밀번호 로그인, 호스트 키 등록, Gaussian 경로를 확인하세요.")
                .arg(program).arg(p.exitCode()).arg(QString::fromUtf8(log.right(16000)));
            return false;
        }
        return true;
    }
    QString prefix() const { return "set -e\numask 077\nwork=\"$HOME/" + relative + "\"\n"; }
    bool shell(const QString &script, int timeout = 180000, QByteArray *output = nullptr, bool cleanup = false) {
        auto args = options(false); args << settings.host << "bash -s";
        return process("ssh", args, (prefix() + script + '\n').toUtf8(), timeout, output, cleanup);
    }
    bool setup() {
        error = settings.error(); if (!error.isEmpty()) return false;
        if (!QDir().mkpath(local)) { error = QStringLiteral("로컬 결과 폴더 생성 실패"); return false; }
        status(QStringLiteral("계산 서버 연결 확인: %1").arg(settings.host));
        // Mark before SSH: a failed connection may still have created the directory.
        created = true;
        return shell("mkdir -p -- \"$work/scratch\"\ntest -x " + GaussianRemote::quote(settings.gaussianDirectory + "/formchk")
            + "\ntest -x " + GaussianRemote::quote(settings.gaussianDirectory + "/cubegen"));
    }
    bool upload(const QString &source, const QString &name) {
        status(QStringLiteral("계산 서버로 전송 중: %1").arg(QFileInfo(source).fileName()));
        auto args = options(true); args << "-q" << QFileInfo(source).absoluteFilePath() << settings.host + ':' + relative + '/' + name;
        return process("scp", args, {}, 60 * 60 * 1000);
    }
    bool download(const QString &name) {
        status(QStringLiteral("결과 다운로드 중: %1").arg(name));
        auto args = options(true); args << "-q" << settings.host + ':' + relative + '/' + name << local + '/' + name;
        return process("scp", args, {}, 60 * 60 * 1000);
    }
    QString environment() const {
        const QString root = QFileInfo(settings.gaussianDirectory).path();
        return "export g16root=" + GaussianRemote::quote(root) + "\nexport g09root=" + GaussianRemote::quote(root)
            + "\nset +e\nif [ -f " + GaussianRemote::quote(settings.gaussianDirectory + "/bsd/g16.profile") + " ]; then . "
            + GaussianRemote::quote(settings.gaussianDirectory + "/bsd/g16.profile") + "; elif [ -f "
            + GaussianRemote::quote(settings.gaussianDirectory + "/bsd/g09.profile") + " ]; then . "
            + GaussianRemote::quote(settings.gaussianDirectory + "/bsd/g09.profile") + "; fi\n"
              "set -e\nexport GAUSS_SCRDIR=\"$work/scratch\"\nexport OMP_NUM_THREADS=1\ncd -- \"$work\"\n";
    }
    bool gaussian(const QString &program, const QStringList &args) {
        QString command = "setsid timeout 1800 " + GaussianRemote::quote(settings.gaussianDirectory + '/' + program);
        for (const auto &arg : args) command += ' ' + GaussianRemote::quote(arg);
        // Record only this isolated job's process group so cancellation can stop it.
        return shell(environment() + command + " &\npid=$!\necho \"$pid\" > run.pid\nwait \"$pid\"\nrm -f run.pid", 31 * 60 * 1000);
    }
    bool inputCheckpoint(const QString &source) {
        if (!QFileInfo(source).isFile()) { error = QStringLiteral("CHK 파일을 찾을 수 없습니다."); return false; }
        if (!settings.checkpointDirectory.isEmpty()) {
            status(QStringLiteral("서버 CHK와 SHA-256 비교 중 (동일하면 업로드 생략)…"));
            QFile file(source);
            if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return false; }
            QCryptographicHash hash(QCryptographicHash::Sha256);
            while (!file.atEnd()) {
                if (cancelled()) { error = QStringLiteral("계산을 취소했습니다."); return false; }
                const auto bytes = file.read(4 * 1024 * 1024);
                if (bytes.isEmpty() && file.error() != QFile::NoError) { error = file.errorString(); return false; }
                hash.addData(bytes);
            }
            const QString candidate = settings.checkpointDirectory + '/' + QFileInfo(source).fileName();
            QByteArray output;
            if (!shell("candidate=" + GaussianRemote::quote(candidate) + "\nif [ -f \"$candidate\" ] && [ \"$(stat -c %s -- \"$candidate\")\" = "
                + GaussianRemote::quote(QString::number(QFileInfo(source).size())) + " ] && [ \"$(sha256sum -- \"$candidate\" | cut -d ' ' -f 1)\" = "
                + GaussianRemote::quote(QString::fromLatin1(hash.result().toHex())) + " ]; then\ncp --reflink=auto -- \"$candidate\" \"$work/input.chk\"\necho YOUNGCRAFT_INPUT_REUSED\nfi", 10 * 60 * 1000, &output)) return false;
            if (output.contains("YOUNGCRAFT_INPUT_REUSED")) { status(QStringLiteral("서버의 동일한 CHK 사용 (SHA-256 일치)")); return true; }
        }
        return upload(source, "input.chk");
    }
    ~Job() {
        if (!created) return;
        const QString saved = error;
        // Only an internally generated UUID path is ever removed. Never source paths.
        const bool cleaned = shell("if [ -f \"$work/run.pid\" ]; then\n"
              "pid=$(cat \"$work/run.pid\")\ncase \"$pid\" in ''|*[!0-9]*) ;; *)\n"
              "if [ \"$(readlink -f \"/proc/$pid/cwd\" 2>/dev/null)\" = \"$work\" ]; then kill -TERM -- \"-$pid\" 2>/dev/null || true; fi ;; esac\nfi\n"
              "rm -rf -- \"$work\"", 20000, nullptr, true);
        if (!cleaned) status(QStringLiteral("서버 임시 작업 폴더 정리 실패: ~/%1").arg(relative));
        error = saved;
    }
};
}
GaussianRemote::Result GaussianRemote::convert(const QString &source, const GaussianRemoteSettings &s, Cancellation c, Progress p) {
    Job job(s, std::move(c), std::move(p)); Result result;
    if (!job.setup() || !job.inputCheckpoint(source)) { result.error = job.error; return result; }
    job.status(QStringLiteral("서버 Gaussian: CHK → FCHK 변환 중…"));
    if (!job.gaussian("formchk", {"input.chk", "converted.fchk"}) || !job.download("converted.fchk")) { result.error = job.error; return result; }
    const QString path = job.local + "/converted.fchk";
    const auto parsed = FchkParser::parse(path);
    if (!parsed.success) { result.error = QStringLiteral("서버 FCHK 결과 오류: %1").arg(parsed.error); return result; }
    result.formattedPath = path;
    return result;
}
GaussianRemote::Result GaussianRemote::generate(const QString &source, const QString &kind, int points, const QString &density,
                                               const GaussianRemoteSettings &s, Cancellation c, Progress p) {
    Result result;
    static const QRegularExpression mo(QStringLiteral("^[AB]MO=[1-9][0-9]*$"));
    if ((kind != "MEP" && !mo.match(kind).hasMatch()) || !QStringList{"SCF", "MP2", "CI", "QCI"}.contains(density)
        || !QList<int>{40, 80, 120}.contains(points)) { result.error = QStringLiteral("MO / MEP 생성 옵션이 유효하지 않습니다."); return result; }
    Job job(s, std::move(c), std::move(p));
    if (!job.setup()) { result.error = job.error; return result; }
    const bool chk = QFileInfo(source).suffix().compare("chk", Qt::CaseInsensitive) == 0;
    if (chk) {
        if (!job.inputCheckpoint(source) || !job.gaussian("formchk", {"input.chk", "input.fchk"})) { result.error = job.error; return result; }
    } else if (!FchkParser::parse(source).success || !job.upload(source, "input.fchk")) {
        result.error = job.error.isEmpty() ? QStringLiteral("유효한 FCHK가 아닙니다.") : job.error; return result;
    }
    job.status(QStringLiteral("서버 cubegen: %1 생성 중…").arg(kind));
    if (!job.gaussian("cubegen", {"0", kind == "MEP" ? "Density=" + density : kind, "input.fchk", "surface.cube", QString::number(points), "h"})
        || !job.download("surface.cube")) { result.error = job.error; return result; }
    auto surface = std::make_shared<CubeGrid>(CubeParser::parse(job.local + "/surface.cube"));
    if (!surface->valid) { result.error = surface->error; return result; }
    if (kind == "MEP") {
        job.status(QStringLiteral("서버 cubegen: 정전기 퍼텐셜 생성 중…"));
        if (!job.gaussian("cubegen", {"0", "Potential=" + density, "input.fchk", "potential.cube", QString::number(points), "h"})
            || !job.download("potential.cube")) { result.error = job.error; return result; }
        auto potential = std::make_shared<CubeGrid>(CubeParser::parse(job.local + "/potential.cube"));
        if (!potential->valid) { result.error = potential->error; return result; }
        result.potential = potential;
    }
    result.surface = surface;
    return result;
}
}
