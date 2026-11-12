#pragma once
#include "io/CubeParser.h"
#include <atomic>
#include <functional>
#include <memory>

namespace youngcraft {
struct GaussianRemoteSettings {
    QString host, configFile, identityFile, gaussianDirectory, checkpointDirectory;
    int port = 22;
    bool passwordAuthentication = true;
    static void initializeDefaults(const QString &profilePath = {});
    static GaussianRemoteSettings load();
    QString error() const;
};
class GaussianRemote {
public:
    using Progress = std::function<void(const QString &)>;
    using Cancellation = std::shared_ptr<std::atomic_bool>;
    struct Result {
        QString error, formattedPath;
        std::shared_ptr<const CubeGrid> surface, potential;
    };
    static Result convert(const QString &source, const GaussianRemoteSettings &settings,
                          Cancellation cancel = {}, Progress progress = {});
    static Result generate(const QString &source, const QString &kind, int points,
                           const QString &density, const GaussianRemoteSettings &settings,
                           Cancellation cancel = {}, Progress progress = {});
    static QString quote(const QString &value);
};
}
