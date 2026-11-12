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
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QElapsedTimer>
#include <QProcess>
#include <QTemporaryDir>

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
GaussianTools::GaussianTools(QObject *parent) : QObject(parent) {}
GaussianTools::~GaussianTools() {
    if (m_worker) {
        m_cancelToken->store(true); m_worker->wait(); delete m_worker;
    }
}
void GaussianTools::cancel() {
    if (!m_worker) return;
    m_cancelToken->store(true);
    emit status(QStringLiteral("계산 취소 및 임시 파일 정리 중…"));
}
void GaussianTools::generate(const QString &source, const QString &kind, int points, const QString &densityType) {
    if (busy()) return;
    const bool remote = QSettings().value("orbitals/remoteEnabled", false).toBool();
    const auto settings = GaussianRemoteSettings::load();
    const QString cubegen = executable("cubegen"), formchk = executable("formchk");
    const QFileInfo info(source);
    if (!info.isFile()) {
        emit failed(!remote && !isAvailable("cubegen")
            ? QStringLiteral("cubegen 실행 파일을 찾을 수 없습니다: ") + cubegen
            : QStringLiteral("입력 파일을 찾을 수 없습니다: ") + source);
        return;
    }
    const QByteArray options = QJsonDocument(QJsonArray{kind, points, densityType, remote,
        remote ? settings.host : cubegen, remote ? settings.gaussianDirectory : formchk,
        settings.port, settings.checkpointDirectory}).toJson(QJsonDocument::Compact);
    const QString key = QString::fromUtf8(QJsonDocument(QJsonArray{info.canonicalFilePath(),
        QString::number(info.size()), QString::number(info.lastModified().toMSecsSinceEpoch()),
        QString::fromUtf8(options)}).toJson(QJsonDocument::Compact));
    if (auto *cached = m_cache.object(key)) {
        const auto surface = cached->surface, potential = cached->potential;
        emit status(QStringLiteral("저장된 오비탈을 바로 표시합니다."));
        emit ready(surface, potential); return;
    }
    auto token = std::make_shared<std::atomic_bool>(false); m_cancelToken = token;
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/orbital-library-v1";
    emit status(QStringLiteral("오비탈 저장소 확인 / 생성 중… (다른 화면 조작 가능)"));
    m_worker = QThread::create([this, source, kind, points, densityType, remote, settings,
                                     cubegen, formchk, options, key, cacheRoot, token] {
        std::shared_ptr<const CubeGrid> surface, potential;
        QString error;
        auto progress = [this, token](const QString &message) {
            QMetaObject::invokeMethod(this, [this, token, message] {
                if (m_cancelToken == token && !token->load()) emit status(message);
            }, Qt::QueuedConnection);
        };
        // Hashing, disk I/O, Gaussian execution and parsing all stay off the UI thread.
        QCryptographicHash hash(QCryptographicHash::Sha256); QFile input(source);
        if (!input.open(QIODevice::ReadOnly)) error = QStringLiteral("입력 파일을 열 수 없습니다: ") + source;
        while (error.isEmpty() && !input.atEnd() && !token->load()) {
            const auto bytes = input.read(1024 * 1024);
            if (bytes.isEmpty() && input.error() != QFile::NoError) error = input.errorString();
            hash.addData(bytes);
        }
        input.close(); hash.addData(options);
        const QString folder = cacheRoot + '/' + QString::fromLatin1(hash.result().toHex());
        const QString surfacePath = folder + "/surface.cube", potentialPath = folder + "/potential.cube";
        const bool mep = kind == "MEP";
        if (error.isEmpty() && !token->load() && QFileInfo::exists(surfacePath)) {
            auto s = std::make_shared<CubeGrid>(CubeParser::parse(surfacePath));
            auto p = mep ? std::make_shared<CubeGrid>(CubeParser::parse(potentialPath)) : nullptr;
            if (s->valid && (!mep || (p && p->valid))) {
                surface = s; potential = p; progress(QStringLiteral("디스크에 저장된 오비탈을 불러왔습니다."));
            }
        }
        QTemporaryDir work;
        auto runLocal = [&](const QString &program, const QStringList &args) {
            if (token->load()) return false;
            QProcess process; process.setWorkingDirectory(work.path()); process.setProcessChannelMode(QProcess::MergedChannels);
            process.start(program, args); process.closeWriteChannel();
            if (!process.waitForStarted(10000)) { error = process.errorString(); return false; }
            QElapsedTimer timer; timer.start(); QByteArray log;
            while (!process.waitForFinished(50)) {
                log += process.readAll(); log = log.right(16000);
                if (token->load() || timer.elapsed() > 300000) {
                    process.kill(); process.waitForFinished(3000);
                    error = QStringLiteral("계산 취소 또는 제한 시간 초과"); return false;
                }
            }
            log += process.readAll();
            if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
                error = QStringLiteral("Gaussian 도구 실패: ") + QString::fromLocal8Bit(log.right(16000)); return false;
            }
            return true;
        };
        if (!surface && error.isEmpty() && !token->load()) {
            if (remote && !settings.error().isEmpty()) error = settings.error();
            if (!remote) {
                for (const auto &program : QFileInfo(source).suffix().compare("chk", Qt::CaseInsensitive) == 0
                        ? QStringList{formchk, cubegen} : QStringList{cubegen}) {
                    if (!QFileInfo(program).isFile() && QStandardPaths::findExecutable(program).isEmpty())
                        error = QStringLiteral("Gaussian 실행 파일을 찾을 수 없습니다: ") + program;
                }
            }
            if (!error.isEmpty()) { /* Report below without starting Gaussian. */ }
            else if (remote) {
                const auto result = GaussianRemote::generate(source, kind, points, densityType, settings, token, progress);
                surface = result.surface; potential = result.potential; error = result.error;
            } else if (!work.isValid()) error = QStringLiteral("임시 폴더 생성 실패");
            else {
                const bool chk = QFileInfo(source).suffix().compare("chk", Qt::CaseInsensitive) == 0;
                if (!QFile::copy(source, work.filePath(chk ? "input.chk" : "input.fchk"))) error = QStringLiteral("입력 파일 복사 실패");
                if (error.isEmpty() && chk) runLocal(formchk, {"input.chk", "input.fchk"});
                if (error.isEmpty() && !token->load() && !FchkParser::parse(work.filePath("input.fchk")).success) error = QStringLiteral("유효한 FCHK가 아닙니다.");
                if (error.isEmpty() && runLocal(cubegen, {"0", mep ? "Density=" + densityType : kind,
                        "input.fchk", "surface.cube", QString::number(points), "h"})) {
                    auto s = std::make_shared<CubeGrid>(CubeParser::parse(work.filePath("surface.cube")));
                    if (!s->valid) error = s->error; else surface = s;
                    if (error.isEmpty() && mep && runLocal(cubegen, {"0", "Potential=" + densityType,
                            "input.fchk", "potential.cube", QString::number(points), "h"})) {
                        auto p = std::make_shared<CubeGrid>(CubeParser::parse(work.filePath("potential.cube")));
                        if (!p->valid) error = p->error; else potential = p;
                    }
                }
            }
            if (surface && error.isEmpty() && !token->load()) {
                QDir().mkpath(folder);
                auto persist = [](const std::shared_ptr<const CubeGrid> &grid, const QString &path) {
                    QFile in(grid->filePath); QSaveFile out(path);
                    if (!in.open(QIODevice::ReadOnly) || !out.open(QIODevice::WriteOnly)) return false;
                    while (!in.atEnd()) {
                        const auto bytes = in.read(1024 * 1024);
                        if ((bytes.isEmpty() && in.error() != QFile::NoError) || out.write(bytes) != bytes.size()) return false;
                    }
                    return out.commit();
                };
                const bool savedSurface = persist(surface, surfacePath);
                const bool savedPotential = potential && persist(potential, potentialPath);
                if (savedSurface) {
                    auto saved = std::make_shared<CubeGrid>(*surface); saved->filePath = surfacePath; surface = saved;
                }
                if (savedPotential) {
                    auto saved = std::make_shared<CubeGrid>(*potential); saved->filePath = potentialPath; potential = saved;
                }
                QSaveFile metadata(folder + "/info.json");
                if (metadata.open(QIODevice::WriteOnly)) {
                    metadata.write(QJsonDocument(QJsonArray{source, kind, points, densityType}).toJson()); metadata.commit();
                }
                // Limit persisted cube storage to 2 GiB; only this dedicated library is pruned.
                QDir library(cacheRoot);
                const auto folders = library.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
                qint64 total = 0;
                for (const auto &entry : folders)
                    for (const auto &file : QDir(entry.absoluteFilePath()).entryInfoList(QDir::Files)) total += file.size();
                for (const auto &entry : folders) {
                    if (total <= 2LL * 1024 * 1024 * 1024) break;
                    if (entry.absoluteFilePath() == folder) continue;
                    qint64 size = 0;
                    for (const auto &file : QDir(entry.absoluteFilePath()).entryInfoList(QDir::Files)) size += file.size();
                    if (QDir(entry.absoluteFilePath()).removeRecursively()) total -= size;
                }
            }
        }
        QMetaObject::invokeMethod(this, [this, token, key, surface, potential, error] {
            if (m_cancelToken != token) return;
            m_worker->wait(); delete m_worker; m_worker = nullptr; m_cancelToken.reset();
            if (token->load()) emit failed(QStringLiteral("계산을 취소했습니다."));
            else if (!error.isEmpty()) emit failed(error);
            else if (!surface) emit failed(QStringLiteral("오비탈 생성 결과가 없습니다."));
            else {
                const qsizetype bytes = (surface->values.size() + (potential ? potential->values.size() : 0)) * sizeof(float);
                m_cache.insert(key, new CachedOrbital{surface, potential}, int(qMin<qsizetype>(INT_MAX, qMax<qsizetype>(1, bytes / 1024))));
                emit ready(surface, potential);
            }
        }, Qt::QueuedConnection);
    });
    m_worker->start();
}
}
