#include <QCoreApplication>
#include <QDebug>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <cstdio>
#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

int main(int argc, char **argv) {
#ifdef Q_OS_WIN
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    QCoreApplication app(argc,argv);
    const auto endpoint=qEnvironmentVariable("YOUNGCRAFT_SSH_BROKER");
    const auto token=qEnvironmentVariable("YOUNGCRAFT_SSH_TOKEN");
    if(endpoint.isEmpty() || token.isEmpty()) {qWarning("SSH askpass IPC missing context");return 1;}
    QLocalSocket socket; socket.connectToServer(endpoint);
    if(!socket.waitForConnected(5000)) {qWarning()<<"SSH askpass IPC connect failed"<<socket.error();return 1;}
    socket.write(QJsonDocument(QJsonObject{{"token",token},
        {"scope",qEnvironmentVariable("YOUNGCRAFT_SSH_SCOPE")},
        {"prompt",app.arguments().mid(1).join(' ')},
        {"allowPrompt",qEnvironmentVariable("YOUNGCRAFT_SSH_ALLOW_PROMPT")=="1"}}).toJson(QJsonDocument::Compact)+'\n');
    // Windows may report a peer close after a fast reply instead of a write
    // notification. Read that reply if the request drained or data arrived.
    if(socket.bytesToWrite() && !socket.waitForBytesWritten(5000)
            && socket.bytesToWrite() && !socket.bytesAvailable()) {
        qWarning()<<"SSH askpass IPC write wait failed"<<socket.state()<<socket.bytesToWrite()<<socket.bytesAvailable();return 1;
    }
    QByteArray reply; QElapsedTimer timer;timer.start();
    while(!reply.endsWith('\n') && timer.elapsed()<180000){
        reply+=socket.readAll();
        if(reply.size()>16384)return 1;
        if(reply.endsWith('\n'))break;
        if(!socket.waitForReadyRead(1000) && socket.state()==QLocalSocket::UnconnectedState) {reply+=socket.readAll();break;}
    }
    const auto result=QJsonDocument::fromJson(reply).object();
    if(!result["ok"].toBool()) {qWarning()<<"SSH askpass IPC reply rejected or invalid"<<socket.state()<<reply.size();return 1;}
    QByteArray password=result["password"].toString().toUtf8();
    if(password.isEmpty() || password.contains('\n') || password.contains('\r'))return 1;
    // stdout is consumed only by OpenSSH, never by the app's logs or settings.
    fwrite(password.constData(),1,size_t(password.size()),stdout);fputc('\n',stdout);fflush(stdout);
    password.fill('\0');reply.fill('\0');return 0;
}
