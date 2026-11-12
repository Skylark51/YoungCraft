#include <QCoreApplication>
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
    if(endpoint.isEmpty() || token.isEmpty()) return 1;
    QLocalSocket socket; socket.connectToServer(endpoint);
    if(!socket.waitForConnected(5000))return 1;
    socket.write(QJsonDocument(QJsonObject{{"token",token},
        {"scope",qEnvironmentVariable("YOUNGCRAFT_SSH_SCOPE")},
        {"prompt",app.arguments().mid(1).join(' ')},
        {"allowPrompt",qEnvironmentVariable("YOUNGCRAFT_SSH_ALLOW_PROMPT")=="1"}}).toJson(QJsonDocument::Compact)+'\n');
    if(socket.bytesToWrite() && !socket.waitForBytesWritten(5000))return 1;
    QByteArray reply; QElapsedTimer timer;timer.start();
    while(!reply.endsWith('\n') && timer.elapsed()<180000){
        reply+=socket.readAll();
        if(reply.size()>16384)return 1;
        if(reply.endsWith('\n'))break;
        if(!socket.waitForReadyRead(1000) && socket.state()==QLocalSocket::UnconnectedState) {reply+=socket.readAll();break;}
    }
    const auto result=QJsonDocument::fromJson(reply).object();
    if(!result["ok"].toBool())return 1;
    QByteArray password=result["password"].toString().toUtf8();
    if(password.isEmpty() || password.contains('\n') || password.contains('\r'))return 1;
    // stdout is consumed only by OpenSSH, never by the app's logs or settings.
    fwrite(password.constData(),1,size_t(password.size()),stdout);fputc('\n',stdout);fflush(stdout);
    password.fill('\0');reply.fill('\0');return 0;
}
