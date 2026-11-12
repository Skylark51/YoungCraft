#pragma once
#include <QObject>
#include <QLocalServer>
#include <QHash>
#include <QProcessEnvironment>

namespace youngcraft {
// The GUI owns passwords. SSH children receive only a random, user-local IPC endpoint.
class SshPasswordBroker final : public QObject {
    Q_OBJECT
public:
    static SshPasswordBroker *instance(); // GUI thread only
    static QProcessEnvironment environment(const QString &scope, bool allowPrompt = true);
    static void forgetPasswords();
    bool hasSession() const { return !m_passwords.isEmpty(); }
    bool available() const { return m_server.isListening(); }
private:
    explicit SshPasswordBroker(QObject *parent);
    QLocalServer m_server;
    QHash<QString, QByteArray> m_passwords;
    QString m_endpoint, m_token;
};
}
