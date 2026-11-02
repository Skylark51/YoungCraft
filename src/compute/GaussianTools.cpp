#include "compute/GaussianRemote.h"
#include <QThread>
#include "compute/GaussianTools.h"
#include "io/FchkParser.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

namespace youngcraft {
QString GaussianTools::executable(const QString &name) {
    const QString configured = QSettings().value("orbitals/" + name).toString();
    if (!configured.isEmpty()) return configured;
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) return found;
    for (const QString &root : {qEnvironmentVariable("g16root"), qEnvironmentVariable("g09root"),
                               QStringLiteral("C:/G16W"), QStringLiteral("C:/G09W")}) {
        if (root.isEmpty()) continue;
        for (const QString &sub : {QString(), QStringLiteral("/g16"), QStringLiteral("/g09")}) {
            const QString candidate = root + sub + '/' + name + ".exe";
            if (QFileInfo::exists(candidate)) return candidate;
        }
    }
    return name;
}
bool GaussianTools::isAvailable(const QString &name) {
    if (QSettings().value("orbitals/remoteEnabled", false).toBool() && (name=="formchk" || name=="cubegen"))
        return GaussianRemoteSettings::load().error().isEmpty();
    const auto program = executable(name);
    return QFileInfo(program).isFile() || !QStandardPaths::findExecutable(program).isEmpty();
}
QString GaussianTools::convertCheckpoint(const QString &source, QString *error) {
    if (error) error->clear();
    if (QSettings().value("orbitals/remoteEnabled", false).toBool()) {
        const auto result = GaussianRemote::convert(source, GaussianRemoteSettings::load());
        if (error) *error = result.error;
        return result.formattedPath;
    }
    if (!isAvailable("formchk")) {
        if (error) *error = QStringLiteral("CHK 변환에 필요한 Gaussian formchk 실행 파일을 찾을 수 없습니다.\nOrbitals > Gaussian 도구 경로에서 formchk.exe를 지정한 뒤 CHK를 다시 여세요.\nMO / MEP 생성에는 cubegen.exe도 필요합니다.\n확인한 경로: %1").arg(executable("formchk"));
        return {};
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/checkpoints/" + QUuid::createUuid().toString(QUuid::Id128);
    QDir().mkpath(base);
    const QString input = base + "/input.chk", output = base + "/converted.fchk";
    if (!QFile::copy(source, input)) { *error = QStringLiteral("CHK 임시 복사 실패"); return {}; }
    QProcess process;
    process.setWorkingDirectory(base);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable("formchk"), {"input.chk", "converted.fchk"});
    if (!process.waitForStarted(10000)) {
        *error = QStringLiteral("formchk 실행 실패. Orbitals > Gaussian 도구에서 실행 파일 경로를 지정하세요.\n%1").arg(process.errorString());
    } else if (!process.waitForFinished(120000)) {
        process.kill(); process.waitForFinished(); *error = QStringLiteral("formchk 제한 시간(120초)을 초과했습니다.");
    } else if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || !FchkParser::parse(output).success) {
        *error = QStringLiteral("formchk 변환 실패:\n%1").arg(QString::fromLocal8Bit(process.readAll()).right(8000));
    } else { QFile::remove(input); return output; }
    QDir(base).removeRecursively(); // Only this operation's UUID cache directory.
    return {};
}
GaussianTools::GaussianTools(QObject *parent) : QObject(parent) {
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_log += m_process.readAllStandardOutput(); if (m_log.size() > 16000) m_log = m_log.right(16000);
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) fail(QStringLiteral("Gaussian 도구 실행 실패: %1\nOrbitals의 실행 파일 경로를 확인하세요.").arg(m_process.errorString()));
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus status) {
        if (!m_work) return;
        if (status != QProcess::NormalExit || code != 0) { fail(QStringLiteral("Gaussian 도구 실패 (%1):\n%2").arg(code).arg(QString::fromLocal8Bit(m_log))); return; }
        auto next = std::move(m_next); if (next) next();
    });
}
GaussianTools::~GaussianTools() {
    if (m_remoteThread) {
        m_remoteCancel->store(true); m_remoteThread->wait(); delete m_remoteThread; m_remoteThread=nullptr;
    }
    disconnect(&m_process, nullptr, this, nullptr); m_process.kill(); m_process.waitForFinished(3000); }
void GaussianTools::fail(const QString &message) { m_next = {}; m_work.reset(); emit failed(message); }
void GaussianTools::cancel() {
    if (m_remoteThread) {
        m_remoteCancel->store(true);
        emit status(QStringLiteral("서버 계산 취소 및 임시 파일 정리 중…")); return;
    }
    m_next = {}; auto work = std::move(m_work); // Reclaim after the child has released its files.
    m_process.kill(); m_process.waitForFinished(3000);
    emit failed(QStringLiteral("계산을 취소했습니다."));
}
void GaussianTools::run(const QString &program, const QStringList &args, std::function<void()> next) {
    m_next = std::move(next); m_log.clear();
    emit status(program + ' ' + args.join(' '));
    m_process.setWorkingDirectory(m_work->path());
    m_process.start(executable(program), args);
    m_process.closeWriteChannel();
}
void GaussianTools::generate(const QString &source, const QString &kind, int points, const QString &densityType) {
    if (busy()) return;
    if (QSettings().value("orbitals/remoteEnabled", false).toBool()) {
        const auto settings = GaussianRemoteSettings::load();
        if (const auto error=settings.error(); !error.isEmpty()) { emit failed(error); return; }
        auto token=std::make_shared<std::atomic_bool>(false); m_remoteCancel=token;
        m_remoteThread=QThread::create([this, source, kind, points, densityType, settings, token] {
            auto progress=[this, token](const QString &message) {
                QMetaObject::invokeMethod(this, [this, token, message] {
                    if (m_remoteCancel==token) emit status(message);
                }, Qt::QueuedConnection);
            };
            const auto result=GaussianRemote::generate(source,kind,points,densityType,settings,token,progress);
            QMetaObject::invokeMethod(this, [this, token, result] {
                if (m_remoteCancel!=token) return;
                m_remoteThread->wait(); delete m_remoteThread; m_remoteThread=nullptr; m_remoteCancel.reset();
                if (token->load()) emit failed(QStringLiteral("서버 계산을 취소했습니다."));
                else if (!result.error.isEmpty()) emit failed(result.error);
                else emit ready(result.surface,result.potential);
            }, Qt::QueuedConnection);
        });
        m_remoteThread->start(); return;
    }
    for (const auto &tool : QFileInfo(source).suffix().compare("chk", Qt::CaseInsensitive) == 0
            ? QStringList{"formchk", "cubegen"} : QStringList{"cubegen"}) {
        if (!isAvailable(tool)) {
            emit failed(QStringLiteral("%1 실행 파일을 찾을 수 없어 생성하지 못했습니다.\nOrbitals > Gaussian 도구 경로에서 %1.exe를 지정하세요.\n확인한 경로: %2").arg(tool, executable(tool)));
            return;
        }
    }
    m_work = std::make_unique<QTemporaryDir>();
    if (!m_work->isValid()) { fail(QStringLiteral("임시 폴더 생성 실패")); return; }
    const bool chk = QFileInfo(source).suffix().compare("chk", Qt::CaseInsensitive) == 0;
    if (!QFile::copy(source, m_work->filePath(chk ? "input.chk" : "input.fchk"))) { fail(QStringLiteral("입력 파일 복사 실패")); return; }
    auto build = [this, kind, points, densityType] {
        if (!FchkParser::parse(m_work->filePath("input.fchk")).success) { fail(QStringLiteral("유효한 FCHK가 아닙니다.")); return; }
        const bool mep = kind == "MEP";
        const QString field = mep ? "Density=" + densityType : kind;
        run("cubegen", {"0", field, "input.fchk", "surface.cube", QString::number(points), "h"}, [this, mep, points, densityType] {
            auto surface = std::make_shared<CubeGrid>(CubeParser::parse(m_work->filePath("surface.cube")));
            if (!surface->valid) { fail(surface->error); return; }
            if (!mep) { m_work.reset(); emit ready(surface, {}); return; }
            run("cubegen", {"0", "Potential=" + densityType, "input.fchk", "potential.cube", QString::number(points), "h"}, [this, surface] {
                auto potential = std::make_shared<CubeGrid>(CubeParser::parse(m_work->filePath("potential.cube")));
                if (!potential->valid) { fail(potential->error); return; }
                m_work.reset(); emit ready(surface, potential);
            });
        });
    };
    if (chk) run("formchk", {"input.chk", "input.fchk"}, build); else build();
}
}
