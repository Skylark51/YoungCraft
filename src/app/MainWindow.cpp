#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/StyleEditorDialog.h"

#include <QActionGroup>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QIcon>
#include <QCloseEvent>
#include <QCursor>
#include <QDockWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QProcess>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>

namespace youngcraft {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setObjectName(QStringLiteral("YoungcraftMainWindow"));
    setWindowIcon(QIcon(QStringLiteral(":/youngcraft/icon.png")));
    m_style = StyleManager::load();
    // C-bound hydrogens are hidden per atom on file load, matching the toolbar.
    // Keep other H visible and allow the reset action to restore C-H.
    m_style.showHydrogen = true;
    createUi();
    setAcceptDrops(true);
    createMenus();
    createToolBar();
    const QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    restoreState(settings.value(QStringLiteral("window/sidebarPanelState")).toByteArray());
    if (width() < 900 || height() < 600) resize(1400, 900);
    updateWindowTitle();
}

MainWindow::~MainWindow() {
    if (m_mmProcess) {
        QProcess *process = m_mmProcess;
        m_mmProcess = nullptr;
        process->disconnect(this);
        process->kill();
        process->waitForFinished(3000);
        delete process;
    }
}

} // namespace youngcraft
