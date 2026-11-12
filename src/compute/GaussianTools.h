#pragma once
#include "io/CubeParser.h"
#include <QObject>
#include <QCache>
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
    bool busy() const { return m_worker != nullptr; }
signals:
    void status(const QString &text);
    void failed(const QString &message);
    void ready(std::shared_ptr<const CubeGrid> surface, std::shared_ptr<const CubeGrid> potential);
private:
    struct CachedOrbital {
        std::shared_ptr<const CubeGrid> surface, potential;
    };
    QCache<QString, CachedOrbital> m_cache{256 * 1024}; // KiB, bounded LRU
    QThread *m_worker = nullptr;
    std::shared_ptr<std::atomic_bool> m_cancelToken;
};
}
