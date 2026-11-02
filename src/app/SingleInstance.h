#pragma once
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <functional>
#include <memory>

namespace youngcraft {
// The lock elects one process even when WinSCP launches many files concurrently.
class SingleInstance final {
public:
    enum class Result { Primary, Forwarded, Error };
    std::function<void(const QStringList &, bool)> openRequested;
    Result start(const QStringList &paths, bool newWindow, const QString &key = QStringLiteral("Youngcraft-desktop-v1")) {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/Youngcraft";
        if (!QDir().mkpath(directory)) return Result::Error;
        m_lock = std::make_unique<QLockFile>(directory + "/" + key + ".lock");
        // Do not evict a live instance merely because it has been open for a while.
        m_lock->setStaleLockTime(0);
        QElapsedTimer timer; timer.start();
        const QByteArray message = QJsonDocument(QJsonObject{{"paths", QJsonArray::fromStringList(paths)},
                                                            {"newWindow", newWindow}}).toJson(QJsonDocument::Compact) + '\n';
        do {
            if (m_lock->tryLock(0)) {
                QLocalServer::removeServer(key);
                m_server.setSocketOptions(QLocalServer::UserAccessOption);
                if (!m_server.listen(key)) { m_lock->unlock(); return Result::Error; }
                QObject::connect(&m_server, &QLocalServer::newConnection, &m_server, [this] {
                    while (auto *socket = m_server.nextPendingConnection()) {
                        socket->setParent(&m_server);
                        auto buffer = std::make_shared<QByteArray>();
                        QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
                        auto receive = [this, socket, buffer] {
                            *buffer += socket->readAll();
                            if (buffer->size() > 1024 * 1024) { socket->abort(); return; }
                            if (!buffer->endsWith('\n')) return;
                            QJsonParseError error;
                            const auto json = QJsonDocument::fromJson(*buffer, &error);
                            if (error.error != QJsonParseError::NoError || !json.isObject()) { socket->abort(); return; }
                            QStringList requested;
                            for (const auto &path : json.object().value("paths").toArray()) requested.append(path.toString());
                            if (openRequested) openRequested(requested, json.object().value("newWindow").toBool());
                            socket->write("OK\n"); socket->flush(); socket->disconnectFromServer();
                        };
                        QObject::connect(socket, &QLocalSocket::readyRead, socket, receive);
                        if (socket->bytesAvailable()) receive();
                    }
                });
                return Result::Primary;
            }
            QLocalSocket socket;
            socket.connectToServer(key);
            if (socket.waitForConnected(200)) {
                if (socket.write(message) != message.size()) return Result::Error;
                // A local pipe can finish writing before waitForBytesWritten is called.
                if (socket.bytesToWrite() > 0 && !socket.waitForBytesWritten(3000) &&
                    socket.bytesToWrite() > 0) return Result::Error;
                QByteArray reply;
                QElapsedTimer acknowledgement; acknowledgement.start();
                while (acknowledgement.elapsed() < 15000) {
                    reply += socket.readAll();
                    if (reply == "OK\n") return Result::Forwarded;
                    if (!socket.waitForReadyRead(1000) && socket.state() == QLocalSocket::UnconnectedState) {
                        reply += socket.readAll();
                        return reply == "OK\n" ? Result::Forwarded : Result::Error;
                    }
                }
                return Result::Error;
            }
            QThread::msleep(50);
        } while (timer.elapsed() < 15000);
        return Result::Error;
    }
private:
    std::unique_ptr<QLockFile> m_lock;
    QLocalServer m_server;
};
}
