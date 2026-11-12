#include "compute/SshPasswordBroker.h"
#include <QApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QThread>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <QDir>
#include <memory>

namespace youngcraft {
namespace { QPointer<SshPasswordBroker> broker; }
SshPasswordBroker *SshPasswordBroker::instance() {
    if (!broker) broker = new SshPasswordBroker(QCoreApplication::instance());
    return broker;
}
QProcessEnvironment SshPasswordBroker::environment(const QString &scope, bool allowPrompt) {
    auto env = QProcessEnvironment::systemEnvironment();
    // Windows OpenSSH cannot reliably spawn an askpass path containing spaces/Unicode.
    // SSH runs with the portable directory as its working directory, so no shell quoting
    // or executable path containing the user's profile name is needed here.
    env.insert("SSH_ASKPASS", ".\\YoungcraftSshAskpass.exe");
    env.insert("SSH_ASKPASS_REQUIRE", "force");
    env.insert("DISPLAY", "Youngcraft");
    env.insert("YOUNGCRAFT_SSH_SCOPE", scope);
    env.insert("YOUNGCRAFT_SSH_ALLOW_PROMPT", allowPrompt ? "1" : "0");
    return env;
}
void SshPasswordBroker::forgetPasswords() {
    if (!broker) return;
    const auto clear=[] {
        if (!broker) return;
        for (auto &password : broker->m_passwords) password.fill('\0');
        broker->m_passwords.clear();
    };
    if(QThread::currentThread()==broker->thread())clear();
    else QMetaObject::invokeMethod(broker,clear,Qt::QueuedConnection);
}
SshPasswordBroker::SshPasswordBroker(QObject *parent) : QObject(parent) {
    m_endpoint = "Youngcraft-ssh-"+QUuid::createUuid().toString(QUuid::Id128);
    m_token = QUuid::createUuid().toString(QUuid::Id128)+QUuid::createUuid().toString(QUuid::Id128);
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server.listen(m_endpoint)) return;
    // These values identify the broker; neither contains a password.
    qputenv("YOUNGCRAFT_SSH_BROKER", m_endpoint.toUtf8());
    qputenv("YOUNGCRAFT_SSH_TOKEN", m_token.toUtf8());
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (auto *socket = m_server.nextPendingConnection()) {
            socket->setParent(this);
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            QTimer::singleShot(185000, socket, [socket] { socket->abort(); });
            auto buffer = std::make_shared<QByteArray>();
            auto receive = [this, socket, buffer] {
                *buffer += socket->readAll();
                if (buffer->size()>4096) { socket->abort(); return; }
                if (!buffer->endsWith('\n') || socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);
                const auto request=QJsonDocument::fromJson(*buffer).object();
                if (request["token"].toString()!=m_token) { socket->abort(); return; }
                const auto prompt=request["prompt"].toString().left(512);
                const auto scope=request["scope"].toString().left(256);
                const QString key=scope+'\n'+prompt;
                const QPointer<QLocalSocket> client(socket);
                auto reply=[client](bool ok, const QByteArray &password) {
                    if (!client || client->state()!=QLocalSocket::ConnectedState) return;
                    client->write(QJsonDocument(QJsonObject{{"ok",ok},{"password",QString::fromUtf8(password)}}).toJson(QJsonDocument::Compact)+'\n');
                    client->flush(); client->disconnectFromServer();
                };
                if (m_passwords.contains(key)) { reply(true,m_passwords.value(key)); return; }
                if (!request["allowPrompt"].toBool() || !qobject_cast<QApplication*>(QCoreApplication::instance())) { reply(false,{}); return; }
                auto *dialog=new QInputDialog(QApplication::activeWindow());
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->setObjectName("LionServerPasswordDialog");
                dialog->setWindowTitle(QStringLiteral("라이온 서버 · 비밀번호 로그인"));
                dialog->setLabelText(QStringLiteral("%1\n%2\n\n비밀번호는 이 앱을 종료할 때까지 메모리에서만 사용합니다.").arg(scope,prompt));
                for(auto *label:dialog->findChildren<QLabel*>())label->setTextFormat(Qt::PlainText);
                dialog->setTextEchoMode(QLineEdit::Password);
                dialog->setOkButtonText(QStringLiteral("로그인"));
                dialog->setCancelButtonText(QStringLiteral("취소"));
                connect(socket,&QLocalSocket::disconnected,dialog,&QDialog::reject);
                connect(dialog,&QInputDialog::accepted,this,[this,dialog,key,client,reply] {
                    if (!client || client->state()!=QLocalSocket::ConnectedState) return;
                    const auto password=dialog->textValue().toUtf8();
                    if (password.isEmpty() || password.contains('\n') || password.contains('\r')) { reply(false,{}); return; }
                    m_passwords.insert(key,password); reply(true,password);
                    dialog->setTextValue({});
                });
                connect(dialog,&QDialog::rejected,this,[reply] { reply(false,{}); });
                dialog->open();
            };
            connect(socket,&QLocalSocket::readyRead,this,receive);
            if (socket->bytesAvailable()) receive();
        }
    });
}
}
