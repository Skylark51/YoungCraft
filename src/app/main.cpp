#include "compute/GaussianConnection.h"
#include "compute/GaussianRemote.h"
#include "app/MainWindow.h"
#include "app/SingleInstance.h"
#include "ui/StartupSplash.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <utility>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    format.setAlphaBufferSize(8);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication application(argc, argv);
    application.setWindowIcon(QIcon(QStringLiteral(":/youngcraft/icon.png")));
    QCoreApplication::setOrganizationName(QStringLiteral("Youngcraft"));
    QCoreApplication::setApplicationName(QStringLiteral("Youngcraft"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    qRegisterMetaType<youngcraft::LoadedFile>();
    QStringList paths;
    bool newWindow = false;
    bool literalPaths = false;
    for (const auto &argument : application.arguments().mid(1)) {
        if (!literalPaths && argument == QStringLiteral("--")) { literalPaths = true; continue; }
        if (!literalPaths && argument == QStringLiteral("--new-window")) { newWindow = true; continue; }
        paths.append(QFileInfo(argument).absoluteFilePath());
    }
#ifdef Q_OS_WIN
    // Let the existing process bring its receiving window to the foreground.
    AllowSetForegroundWindow(ASFW_ANY);
#endif
    youngcraft::SingleInstance instance;
    const auto result = instance.start(paths, newWindow);
    if (result == youngcraft::SingleInstance::Result::Forwarded) return 0;
    if (result == youngcraft::SingleInstance::Result::Error) {
        QMessageBox::critical(nullptr, QStringLiteral("Youngcraft"),
            QStringLiteral("기존 Youngcraft 창에 파일을 전달하지 못했습니다. 잠시 후 다시 실행하세요."));
        return 1;
    }
    // Splash painting pumps events; retain file-open requests until the window exists.
    QVector<QPair<QStringList,bool>> pendingRequests;
    instance.openRequested=[&pendingRequests](const QStringList &requested,bool separate){pendingRequests.push_back({requested,separate});};
    youngcraft::StartupSplash splash;
    splash.show();
    // Paint the splash before constructing the workspace and renderer.
    application.processEvents(QEventLoop::ExcludeUserInputEvents);
    youngcraft::GaussianRemoteSettings::initializeDefaults();
    youngcraft::GaussianConnection::instance()->connectNow();
    auto *initial = new youngcraft::MainWindow;
    initial->setAttribute(Qt::WA_DeleteOnClose);
    QPointer<youngcraft::MainWindow> lastWindow(initial);
    bool startupReady = false;
    instance.openRequested = [&lastWindow, &startupReady, &pendingRequests](const QStringList &requested, bool separate) {
        if (!startupReady) { pendingRequests.push_back({requested,separate}); return; }
        youngcraft::MainWindow *target = nullptr;
        if (!separate) {
            target = qobject_cast<youngcraft::MainWindow *>(QApplication::activeWindow());
            if (!target) target = lastWindow;
            if (!target) for (auto *widget : QApplication::topLevelWidgets()) {
                target = qobject_cast<youngcraft::MainWindow *>(widget);
                if (target) break;
            }
        }
        if (!target) { target = new youngcraft::MainWindow; target->setAttribute(Qt::WA_DeleteOnClose); }
        lastWindow = target;
        for (const auto &path : requested) target->openFile(path);
        if (target->isMinimized()) target->showNormal();
        target->show(); target->raise(); target->activateWindow();
    };
    splash.finishWhenReady(initial, [initial, paths, &startupReady, &pendingRequests, &instance] {
        startupReady = true;
        // Show the receiving window before parsing initial arguments.
        QTimer::singleShot(0, initial, [initial, paths, &pendingRequests, &instance] {
            for (const auto &path : paths) initial->openFile(path);
            const auto requests = std::exchange(pendingRequests, {});
            for(const auto &request:requests)instance.openRequested(request.first,request.second);
        });
    });
    return application.exec();
}
