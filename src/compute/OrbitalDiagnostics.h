#pragma once
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
namespace youngcraft {
// Opt-in local QA trace. Never includes file contents, SSH arguments or credentials.
inline void recordOrbitalDiagnostic(QJsonObject event) {
    const auto path=qEnvironmentVariable("YOUNGCRAFT_ORBITAL_DIAGNOSTICS");
    if(path.isEmpty())return;
    static QMutex mutex;const QMutexLocker lock(&mutex);
    event["time_ms"]=double(QDateTime::currentMSecsSinceEpoch());
    QFile out(path);if(out.open(QIODevice::WriteOnly|QIODevice::Append))out.write(QJsonDocument(event).toJson(QJsonDocument::Compact)+'\n');
}
}
