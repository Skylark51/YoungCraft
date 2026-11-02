#pragma once
#include "io/CubeParser.h"
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <functional>
#include <memory>
#include <atomic>
class QThread;

namespace youngcraft {
class GaussianTools final : public QObject {
    Q_OBJECT
public:
    explicit GaussianTools(QObject *parent = nullptr);
    ~GaussianTools() override;
    static QString executable(const QString &name);
    static bool isAvailable(const QString &name);
    // Called by the file parser worker; never blocks the GUI.
    static QString convertCheckpoint(const QString &source, QString *error);
    void generate(const QString &source, const QString &kind, int points, const QString &densityType);
    void cancel();
    bool busy() const { return m_remoteThread || m_process.state() != QProcess::NotRunning || bool(m_work); }
signals:
    void status(const QString &text);
    void failed(const QString &message);
    void ready(std::shared_ptr<const CubeGrid> surface, std::shared_ptr<const CubeGrid> potential);
private:
    void run(const QString &program, const QStringList &args, std::function<void()> next);
    void fail(const QString &message);
    QThread *m_remoteThread = nullptr;
    std::shared_ptr<std::atomic_bool> m_remoteCancel;
    QProcess m_process;
    std::unique_ptr<QTemporaryDir> m_work;
    std::function<void()> m_next;
    QByteArray m_log;
};
}
