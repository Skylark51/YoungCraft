#include "app/SingleInstance.h"
#include <QCoreApplication>
#include <QProcess>
#include <QTimer>
#include <QUuid>
#include <QtTest>
#include <iostream>
using youngcraft::SingleInstance;
class InstanceTest final : public QObject {
    Q_OBJECT
private slots:
    void simultaneousStartupElectsOnePrimary() {
        const QString key = "Youngcraft-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QVector<std::shared_ptr<QProcess>> processes;
        for (int i = 0; i < 8; ++i) {
            auto process = std::make_shared<QProcess>();
            process->start(QCoreApplication::applicationFilePath(), {"--instance-helper", key, QString::number(i)});
            processes << process;
        }
        int primaryCount = 0, forwardedCount = 0;
        QProcess *host = nullptr;
        QByteArray requests;
        for (const auto &process : processes) {
            if (!process->bytesAvailable()) process->waitForReadyRead(20000);
            const auto output = process->readAllStandardOutput();
            if (output.startsWith("Primary\n") || output.startsWith("Primary\r\n")) {
                ++primaryCount; host = process.get(); requests += output;
            } else {
                if (process->state() != QProcess::NotRunning) QVERIFY(process->waitForFinished(20000));
                const auto rest = output + process->readAllStandardOutput();
                QCOMPARE(rest.trimmed(), QByteArray("Forwarded"));
                QCOMPARE(process->exitCode(), 0);
                ++forwardedCount;
            }
        }
        QCOMPARE(primaryCount, 1); QCOMPARE(forwardedCount, 7);
        QProcess close;
        close.start(QCoreApplication::applicationFilePath(), {"--instance-helper", key, "close"});
        QVERIFY(close.waitForFinished(20000)); QVERIFY(host->waitForFinished(10000));
        requests += host->readAllStandardOutput();
        QCOMPARE(requests.count('\n'), 9); // Primary, seven forwarded requests, shutdown.
    }
    void concurrentProcessesForwardUnicodePaths() {
        const QString key = "Youngcraft-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QProcess host;
        host.start(QCoreApplication::applicationFilePath(), {"--instance-helper", key, "initial"});
        QVERIFY(host.waitForStarted()); QVERIFY(host.waitForReadyRead(10000));
        QCOMPARE(host.readAllStandardOutput().trimmed(), QByteArray("Primary"));
        QVector<std::shared_ptr<QProcess>> clients;
        QStringList paths;
        for (int i=0;i<8;++i) {
            const QString path = QString::fromUtf8("C:/WinSCP 임시 경로/분자 %1.out").arg(i);
            paths << path;
            auto client = std::make_shared<QProcess>();
            client->start(QCoreApplication::applicationFilePath(), {"--instance-helper", key, path});
            clients << client;
        }
        for (const auto &client : clients) {
            QVERIFY(client->waitForFinished(20000));
            QCOMPARE(client->exitCode(), 0);
            QCOMPARE(client->readAllStandardOutput().trimmed(), QByteArray("Forwarded"));
        }
        QProcess close;
        close.start(QCoreApplication::applicationFilePath(), {"--instance-helper", key, "close"});
        QVERIFY(close.waitForFinished(20000));
        QVERIFY(host.waitForFinished(10000));
        const QByteArray output = host.readAllStandardOutput();
        for (const auto &path : paths) QVERIFY2(output.contains(path.toUtf8()), output.constData());
        QCOMPARE(output.count('\n'), 9); // Eight requests and one shutdown.
        // A clean shutdown releases the lock and endpoint for the next launch.
        QProcess restart;
        restart.start(QCoreApplication::applicationFilePath(), {"--instance-helper", key, "close"});
        QVERIFY(restart.waitForFinished(10000));
        QCOMPARE(restart.readAllStandardOutput().trimmed(), QByteArray("Primary"));
    }
};
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.value(1) == "--instance-helper") {
        SingleInstance instance;
        instance.openRequested = [&](const QStringList &paths, bool) {
            for (const auto &path : paths) std::cout << path.toUtf8().constData() << std::endl;
            if (paths.contains("close")) QTimer::singleShot(50, &app, &QCoreApplication::quit);
        };
        const auto result = instance.start({args.value(3)}, false, args.value(2));
        std::cout << (result == SingleInstance::Result::Primary ? "Primary" :
                      result == SingleInstance::Result::Forwarded ? "Forwarded" : "Error") << std::endl;
        if (result == SingleInstance::Result::Error) return 1;
        if (result == SingleInstance::Result::Forwarded || args.value(3) == "close") return 0;
        QTimer::singleShot(30000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    InstanceTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_instance.moc"
