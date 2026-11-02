#include "compute/GaussianConnection.h"
#include "compute/GaussianRemote.h"
#include <QCoreApplication>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>

namespace youngcraft {
GaussianConnection *GaussianConnection::instance() {
    static QPointer<GaussianConnection> connection;
    if (!connection) connection = new GaussianConnection(QCoreApplication::instance());
    return connection;
}
GaussianConnection::GaussianConnection(QObject *parent) : QObject(parent) {
    m_text = QStringLiteral("계산 서버 연결 대기 중…");
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_timeout.setSingleShot(true); m_timeout.setInterval(25000);
    connect(&m_process,&QProcess::readyReadStandardOutput,this,[this]{m_output+=m_process.readAll();m_output=m_output.right(16000);});
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){
        if(error==QProcess::FailedToStart){m_timeout.stop();publish(QStringLiteral("서버 자동 연결 실패: ")+m_process.errorString(),false);}
    });
    connect(&m_timeout,&QTimer::timeout,this,[this]{m_process.kill();publish(QStringLiteral("서버 연결 시간 초과 · 자동 재연결을 누르세요."),false);});
    connect(&m_process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus exit){
        m_timeout.stop();m_output+=m_process.readAll();
        if(exit==QProcess::NormalExit&&code==0&&m_output.contains("YOUNGCRAFT_GAUSSIAN_READY"))
            publish(QStringLiteral("서버 연결됨 · %1 · Gaussian 사용 가능").arg(GaussianRemoteSettings::load().host),true);
        else publish(QStringLiteral("서버 연결 실패 · %1").arg(QString::fromUtf8(m_output).trimmed().right(1000)),false);
    });
}
void GaussianConnection::publish(const QString &text,bool connected) {m_text=text;m_connected=connected;emit changed(text,connected);}
void GaussianConnection::connectNow(bool force) {
    if(m_process.state()!=QProcess::NotRunning)return;
    if(!QSettings().value("orbitals/remoteEnabled",false).toBool()){publish(QStringLiteral("로컬 Gaussian 실행 모드"),false);return;}
    const auto settings=GaussianRemoteSettings::load();
    if(const auto error=settings.error();!error.isEmpty()){publish(QStringLiteral("서버 설정 확인: ")+error,false);return;}
    const auto signature=settings.host+QString::number(settings.port)+settings.configFile+settings.identityFile+settings.gaussianDirectory;
    if(!force&&m_connected&&m_signature==signature)return;
    m_signature=signature;m_output.clear();publish(QStringLiteral("서버 자동 연결 중 · %1:%2").arg(settings.host).arg(settings.port),false);
    QStringList args{"-o","BatchMode=yes","-o","StrictHostKeyChecking=yes","-o","ConnectTimeout=15","-p",QString::number(settings.port)};
    if(!settings.configFile.isEmpty())args<<"-F"<<settings.configFile;
    if(!settings.identityFile.isEmpty())args<<"-i"<<settings.identityFile<<"-o"<<"IdentitiesOnly=yes";
    args<<settings.host<<"bash -s";
    m_process.start(QStandardPaths::findExecutable("ssh"),args);
    const auto script="set -e\ntest -x "+GaussianRemote::quote(settings.gaussianDirectory+"/formchk")
        +"\ntest -x "+GaussianRemote::quote(settings.gaussianDirectory+"/cubegen")+"\nprintf 'YOUNGCRAFT_GAUSSIAN_READY\\n'\n";
    m_process.write(script.toUtf8());m_process.closeWriteChannel();m_timeout.start();
}
}
