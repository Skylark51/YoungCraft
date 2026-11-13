#include <QTimer>
#include "ui/OrbitalStudyWidget.h"
#include "app/MainWindow.h"
#include "app/DocumentTabs.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include "ui/StyleEditorDialog.h"

#include <QActionGroup>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDockWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
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
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QPointer>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <algorithm>
#include <functional>

namespace youngcraft {

std::shared_ptr<MainWindow::DocumentSession> MainWindow::captureDocument() {
    auto document = std::make_shared<DocumentSession>();
    document->connections = m_documentConnections;
    document->view = m_view; document->analysis = m_analysis;
    document->explorer = m_explorer; document->animation = m_animation;
    document->workspace = m_workspace; document->sidebar = m_currentSidebar;
    document->moleculeWorkspace = m_moleculeWorkspace;
    document->tddftWorkspace = m_tddftWorkspace;
    document->tddftPlot = m_tddftWorkspacePlot;
    document->tddftInfo = m_tddftWorkspaceInfo; document->geometryBar = m_geometryBar;
    document->measurementReadout = m_measurementReadout;
    document->drawBar = m_drawBar; document->mmBar = m_mmBar;
    document->crystalBar = m_crystalBar; document->crystalMode = m_crystalMode;
    document->cellToggle = m_cellToggle; document->crystal = m_crystal;
    document->autoHButton = m_autoHButton;
    document->drawElement = m_drawElement; document->drawBondOrder = m_drawBondOrder;
    document->mmLabel = m_mmLabel; document->mmPauseButton = m_mmPauseButton;
    document->mmStopButton = m_mmStopButton; document->mmRestoreButton = m_mmRestoreButton;
    document->mmPlot = m_mmPlot;
    document->mmProgress = m_mmProgress;
    document->style = m_style;
    return document;
}

void MainWindow::saveDocumentState() {
    if (!m_activeDocument) return;
    stopMMOptimization();
    if(m_mmBar && m_mmBar->parentWidget()!=m_moleculeWorkspace){m_mmBar->hide();m_mmBar->setParent(m_moleculeWorkspace);}
    auto &d = *m_activeDocument;
    d.data = m_data; d.frame = m_currentFrame; d.frequency = m_currentFrequency;
    m_style = m_view->style();
    d.style = m_style; d.dragAtoms = m_dragAtoms; d.bondEdit = m_bondEditMode;
    d.geometryMode = m_geometryMode; d.geometrySelection = m_geometrySelection;
    d.drawingMode = m_drawingMode;
    d.mmSeries = m_mmSeries; d.mmOriginalCoords = m_mmOriginalCoords;
    d.crystal = m_crystal;
    d.undoMolecules = m_undoMolecules; d.redoMolecules = m_redoMolecules;
    d.firstBondAtom = m_firstBondAtom; d.status = m_statusLabel->text();
    d.resumeAnimation = m_animation->isPlaying();
    m_animation->stop();
}

void MainWindow::bindDocument(const std::shared_ptr<DocumentSession> &document) {
    m_activeDocument = document;
    m_documentConnections = document->connections;
    const auto &d = *document;
    m_data = d.data; m_currentFile = d.path; m_currentFrame = d.frame; m_currentFrequency = d.frequency;
    m_view = d.view; m_analysis = d.analysis; m_explorer = d.explorer; m_animation = d.animation;
    m_workspace = d.workspace; m_currentSidebar = d.sidebar;
    m_moleculeWorkspace = d.moleculeWorkspace;
    m_tddftWorkspace = d.tddftWorkspace;
    m_tddftWorkspacePlot = d.tddftPlot;
    m_tddftWorkspaceInfo = d.tddftInfo; m_geometryBar = d.geometryBar;
    m_measurementReadout = d.measurementReadout;
    m_drawBar = d.drawBar; m_mmBar = d.mmBar;
    m_crystalBar = d.crystalBar; m_crystalMode = d.crystalMode;
    m_cellToggle = d.cellToggle; m_crystal = d.crystal;
    m_autoHButton = d.autoHButton;
    m_drawElement = d.drawElement; m_drawBondOrder = d.drawBondOrder;
    m_mmLabel = d.mmLabel; m_mmPauseButton = d.mmPauseButton;
    m_mmStopButton = d.mmStopButton; m_mmRestoreButton = d.mmRestoreButton;
    m_mmPlot = d.mmPlot;
    m_mmProgress = d.mmProgress;
    m_mmSeries = d.mmSeries; m_mmOriginalCoords = d.mmOriginalCoords;
    m_undoMolecules = d.undoMolecules; m_redoMolecules = d.redoMolecules;
    m_style = d.style; m_dragAtoms = d.dragAtoms; m_bondEditMode = d.bondEdit;
    m_geometryMode = d.geometryMode; m_geometrySelection = d.geometrySelection;
    updateGeometryReadout();
    m_drawingMode = d.drawingMode;
    m_view->setAtomDragMode(m_dragAtoms);
    m_firstBondAtom = d.firstBondAtom;
    if (m_geometryAction) m_geometryAction->setChecked(m_geometryMode);
    for (QAction *action : findChildren<QAction *>()) {
        const auto &text = action->text();
        if (text == QStringLiteral("축 표시")) action->setChecked(m_style.showAxes);
        else if (text == QStringLiteral("molecule 중심으로 축 이동")) action->setChecked(m_style.centerAxes);
        else if (text == QStringLiteral("수소 표시")) action->setChecked(m_style.showHydrogen);
        else if (action == m_atomLabelsAction) action->setChecked(m_style.showLabels);
        else if (text == QStringLiteral("전하/스핀 밀도 표시")) action->setChecked(m_style.showProperties);
        else if (text == QStringLiteral("van der Waals 구 표시")) action->setChecked(m_style.showVdwSpheres);
        else if (text == QStringLiteral("원근 보기")) action->setChecked(m_style.perspective);
        else if (text == QStringLiteral("직교 보기")) action->setChecked(!m_style.perspective);
    }
}

void MainWindow::synchronizeViews(MoleculeView *source) {
    if (!m_syncViewsAction || !m_syncViewsAction->isChecked() || !source) return;
    const int count = source->molecule().atoms.size();
    if (count == 0) return;
    const CameraState state = source->cameraState();
    m_sharedCameras.insert(count, state);
    for (const auto &document : m_documents) {
        auto *view = document->view;
        if (view->molecule().atoms.size() != count) continue;
        if (view != source) view->setCameraState(state);
        document->style.perspective = state.perspective;
        document->style.perspectiveFov = state.perspectiveFov;
    }
    if (m_view && m_view->molecule().atoms.size() == count) {
        m_style.perspective = state.perspective;
        m_style.perspectiveFov = state.perspectiveFov;
    }
}

void MainWindow::applySynchronizedView(MoleculeView *view) {
    if (!m_syncViewsAction || !m_syncViewsAction->isChecked() || !view) return;
    const int count = view->molecule().atoms.size();
    if (count == 0) return;
    const auto shared = m_sharedCameras.constFind(count);
    if (shared == m_sharedCameras.cend()) { synchronizeViews(view); return; }
    view->setCameraState(shared.value());
    for (const auto &document : m_documents) {
        if (document->view != view) continue;
        document->style.perspective = shared->perspective;
        document->style.perspectiveFov = shared->perspectiveFov;
    }
    if (view == m_view) {
        m_style.perspective = shared->perspective;
        m_style.perspectiveFov = shared->perspectiveFov;
    }
}

void MainWindow::activateDocument(int index) {
    const auto target = index >= 0 ? m_documents.value(m_documentTabs->widget(index)) : m_emptyDocument;
    if (!target) return;
    if (m_activeDocument != target) {
        saveDocumentState();
        applySynchronizedView(target->view);
        bindDocument(target);
    }
    m_documentArea->setCurrentWidget(index >= 0 ? static_cast<QWidget *>(m_documentTabs) : m_emptyDocument->workspace);
    m_sidebars->setCurrentWidget(target->sidebar);
    m_drawPanels->setCurrentWidget(target->drawBar);
    if (m_drawingMode) m_structureTabs->setCurrentIndex(1);
    updateOutputSource();
    updatePanelActions();
    const bool ready = target == m_emptyDocument || target->workspace->property("loadFinished").toBool();
    for (QAction *action : menuBar()->actions()) {
        if (action->text() == QStringLiteral("편집") || action->text() == QStringLiteral("도구")) action->setEnabled(ready);
    }
    if (target->resumeAnimation) {
        if (m_animation->kind() == AnimationController::Kind::Frames) m_animation->startFrames();
        else if (m_animation->kind() == AnimationController::Kind::Frequency) m_animation->startFrequency();
    }
    updateWindowTitle();
    setStatus(target->status.isEmpty() ? QStringLiteral("파일을 열어 시작하세요") : target->status, 0);
}

void MainWindow::closeDocument(int index) {
    QWidget *page = m_documentTabs->widget(index);
    const auto document = m_documents.value(page);
    if (!document) return;
    if (m_activeDocument == document) { saveDocumentState(); bindDocument(m_emptyDocument); }
    document->animation->stop();
    m_documents.remove(page);
    {
        const QSignalBlocker blocker(m_documentTabs);
        m_documentTabs->removeTab(index);
    }
    m_sidebars->removeWidget(document->sidebar);
    document->sidebar->deleteLater();
    m_drawPanels->removeWidget(document->drawBar);
    document->drawBar->deleteLater();
    page->deleteLater();
    activateDocument(m_documentTabs->currentIndex());
}

bool MainWindow::moveDocumentTo(int index, MainWindow *target) {
    QWidget *page = m_documentTabs->widget(index);
    const auto document = m_documents.value(page);
    if (!document || !target) return false;
    if (target == this) return true;
    if (!page->property("loadFinished").toBool()) {
        setStatus(QStringLiteral("파일 분석이 끝난 뒤 탭을 이동하세요."), 5000);
        return false;
    }
    const QString title = m_documentTabs->tabText(index);
    const QString tooltip = m_documentTabs->tabToolTip(index);
    const auto previous = m_activeDocument;
    saveDocumentState();
    // Inactive documents are already saved. Bind only to stop their animation.
    bindDocument(document);
    document->animation->stop();
    for (const auto &connection : document->connections) QObject::disconnect(connection);
    document->connections.clear();
    bindDocument(previous == document ? m_emptyDocument : previous);
    m_documents.remove(page);
    {
        const QSignalBlocker blocker(m_documentTabs);
        m_documentTabs->removeTab(index);
    }
    m_sidebars->removeWidget(document->sidebar);
    m_drawPanels->removeWidget(document->drawBar);
    activateDocument(m_documentTabs->currentIndex());

    target->saveDocumentState();
    page->setParent(target->m_documentTabs);
    document->sidebar->setParent(target->m_sidebars);
    target->m_sidebars->addWidget(document->sidebar);
    target->m_drawPanels->addWidget(document->drawBar);
    target->m_documents.insert(page, document);
    target->bindDocument(document);
    target->m_documentConnections.clear();
    target->connectDocumentUi();
    document->connections = target->m_documentConnections;
    int destination;
    {
        const QSignalBlocker blocker(target->m_documentTabs);
        destination = target->m_documentTabs->addTab(page, title);
        target->m_documentTabs->setTabToolTip(destination, tooltip);
        target->m_documentTabs->setCurrentIndex(destination);
    }
    target->activateDocument(destination);
    target->show(); target->raise(); target->activateWindow();
    // Keep the source window alive: its tab bar may still be inside QDrag::exec.
    return true;
}

void MainWindow::detachDocument(int index) {
    const auto document = m_documents.value(m_documentTabs->widget(index));
    if (!document || !document->workspace->property("loadFinished").toBool()) {
        setStatus(QStringLiteral("파일 분석이 끝난 뒤 탭을 분리하세요."), 5000);
        return;
    }
    auto *window = new MainWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->move(pos() + QPoint(40, 40));
    if (!moveDocumentTo(index, window)) window->deleteLater();
}

void MainWindow::openFile(const QString &path) {
    QString selected = path;
    if (selected.isEmpty()) {
        const auto paths = QFileDialog::getOpenFileNames(this, QStringLiteral("파일 열기"), {}, FileDispatcher::openFilter());
        for (const auto &file : paths) openFile(file);
        return;
    }
    if (selected.isEmpty()) return;
    const QFileInfo info(selected);
    selected = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();
    for (auto it = m_documents.cbegin(); it != m_documents.cend(); ++it) {
        if (it.value()->path.compare(selected, Qt::CaseInsensitive) == 0) {
            m_documentTabs->setCurrentWidget(it.key()); return;
        }
    }
    saveDocumentState();
    createDocumentUi();
    auto document = captureDocument();
    document->path = selected;
    document->status = info.suffix().compare("chk",Qt::CaseInsensitive)==0 && QSettings().value("orbitals/remoteEnabled",false).toBool()
        ? QStringLiteral("서버 CHK 변환·다운로드 중: %1 (대용량 파일은 시간이 걸릴 수 있습니다)").arg(info.fileName())
        : QStringLiteral("파일 분석 중: %1").arg(info.fileName());
    bindDocument(document);
    m_documents.insert(document->workspace, document);
    int index;
    {
        const QSignalBlocker blocker(m_documentTabs);
        index = m_documentTabs->addTab(document->workspace, info.fileName());
        m_documentTabs->setTabToolTip(index, selected);
        m_documentTabs->setCurrentIndex(index);
    }
    activateDocument(index);
    setStatus(document->status, 0);
    // The worker may finish after its document/window closes. Its lifetime is independent.
    auto *thread = new QThread;
    auto *worker = new ParseWorker(selected, ParseOptions{}, nullptr);
    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker, &ParseWorker::run);
    const QPointer<QWidget> identity(document->workspace);
    connect(worker, &ParseWorker::finished, this, [this, identity](const LoadedFile &loaded) {
        if (!identity || !m_documents.contains(identity.data())) return;
        const auto target = m_documents.value(identity.data());
        const auto previous = m_activeDocument;
        saveDocumentState();
        bindDocument(target);
        target->cube = loaded.cube;
        onFileLoaded(loaded);
        target->workspace->setProperty("loadFinished", true);
        target->workspace->setProperty("loadSuccess", loaded.parseResult.success);
        saveDocumentState();
        bindDocument(previous);
        activateDocument(m_documentTabs->currentIndex());
        emit documentLoaded(target->path, loaded.parseResult.success);
    }, Qt::QueuedConnection);
    connect(worker, &ParseWorker::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(worker, &ParseWorker::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void MainWindow::onFileLoaded(const youngcraft::LoadedFile &loaded) {
    if (!loaded.parseResult.success) {
        setStatus(QStringLiteral("파일 열기 실패: %1").arg(loaded.parseResult.error), 0);
        if (loaded.format == QStringLiteral("CHK"))
            QMessageBox::warning(this, QStringLiteral("CHK 파일 열기 실패"), loaded.parseResult.error);
        return;
    }
    m_data = loaded.parseResult.data;
    m_crystal = loaded.crystal;
    m_crystalBar->setVisible(bool(m_crystal));
    if (m_crystal) {
        m_crystalMode->setCurrentIndex(0);
        m_view->setCrystalCell(m_crystal->axisA, m_crystal->axisB, m_crystal->axisC,
                               1, m_cellToggle->isChecked());
    } else {
        m_view->setCrystalCell({}, {}, {}, 1, false);
    }
    m_currentFrequency = -1;
    m_currentFrame = -1;
    if (m_geometryMode) toggleGeometryMode();
    if (loaded.isCube && loaded.cube) {
        m_data.molecule = loaded.cube->molecule;
        if (!m_data.molecule.atoms.isEmpty() && m_data.frames.isEmpty()) {
            Frame frame; frame.metrics.step = 1;
            for (const Atom &atom : m_data.molecule.atoms) frame.coordinates.push_back(atom.position);
            m_data.frames.push_back(frame);
        }
        m_view->setCubeGrid(loaded.cube);
    } else {
        m_view->clearCubeGrid();
    }
    m_currentFile = loaded.format=="CHK" && m_activeDocument ? m_activeDocument->path : m_data.filePath;
    addRecentFile(m_currentFile);
    m_animation->stop();
    m_animation->setFrames(m_data.frames);
    if (!m_data.molecule.atoms.isEmpty() && m_data.molecule.bonds.isEmpty())
        m_data.molecule.rebuildBonds(m_style.recountCutoffScale, m_style.recountHydrogen,
                                     m_style.recountMetalCoordination, false);
    clearGeometrySelection();
    for (int kind = 0; kind < 3; ++kind) m_view->setMeasurementVisible(kind, false);
    m_view->setMolecule(m_data.molecule);
    setCarbonHydrogensVisible(false);
    if (m_data.scanDetected && m_data.scanAtoms.size() >= 2 && m_data.scanAtoms.size() <= 4) {
        bool valid = true;
        for (int atom : m_data.scanAtoms)
            if (atom < 0 || atom >= m_data.molecule.atoms.size()) valid = false;
        if (valid) {
            m_geometrySelection = m_data.scanAtoms;
            m_view->setGeometrySelection(m_geometrySelection);
            m_view->setMeasurementVisible(m_geometrySelection.size() - 2, true);
        }
    }
    updateGeometryReadout();
    m_analysis->setData(m_data);
    populateExplorer();
    if (!m_data.frames.isEmpty()) showFrame(m_data.frames.size() - 1);
    m_analysis->selectDefaultTab();
    if (m_analysis->currentTab() == 2 && !m_data.scanPoints.isEmpty()) showScanPoint(0);
    updatePanelActions();
    updateWindowTitle();
    QString message = QStringLiteral("열림: %1 | 원자 %2 | 프레임 %3 | 진동 %4")
                          .arg(QFileInfo(m_currentFile).fileName()).arg(m_data.molecule.atoms.size())
                          .arg(m_data.frames.size()).arg(m_data.frequencies.size());
    applySynchronizedView(m_view);
    if (!m_data.warningMessage.isEmpty()) message += QStringLiteral(" | ") + m_data.warningMessage;
    setStatus(message, 0);
    if(m_data.format=="FCHK" && QStringList{"fchk","fch"}.contains(QFileInfo(m_data.filePath).suffix().toLower())) {
        auto *orbitals=m_analysis->findChild<OrbitalStudyWidget*>();
        if(orbitals)QTimer::singleShot(0,orbitals,&OrbitalStudyWidget::startDefaultOrbital);
    }
}

void MainWindow::showCrystalMode(int mode) {
    if (!m_crystal || mode < 0 || mode > 2) return;
    stopMMOptimization();
    m_animation->stop();
    m_data.molecule = mode == 0 ? m_crystal->asymmetricUnit
                    : mode == 1 ? m_crystal->unitCell : m_crystal->packing;
    m_data.frames.clear();
    m_currentFrame = -1;
    m_animation->setFrames({});
    m_view->setMolecule(m_data.molecule);
    m_view->setCrystalCell(m_crystal->axisA, m_crystal->axisB, m_crystal->axisC,
                           mode == 2 ? 2 : 1, m_cellToggle->isChecked());
    m_analysis->setData(m_data);
    populateExplorer();
    setStatus(QStringLiteral("결정 표시: %1 · 원자 %2개")
                  .arg(m_crystalMode->currentText()).arg(m_data.molecule.atoms.size()));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (DocumentTabBar::dragSource(event->source(), event->mimeData())) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    if (auto *source = DocumentTabBar::dragSource(event->source(), event->mimeData())) {
        if (source->dropInto(this)) event->acceptProposedAction();
        return;
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    const auto type = event->type();
    if (type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop)
        return QMainWindow::eventFilter(watched, event);

    const auto *target = qobject_cast<QWidget *>(watched);
    if (!target || target->window() != this) return QMainWindow::eventFilter(watched, event);

    // Text fields and item views can consume drops before they reach MainWindow.
    // Handle external file drags first and leave text and document-tab drags to Qt.
    auto *drop = static_cast<QDropEvent *>(event);
    if (drop->source() || !drop->mimeData()->hasUrls())
        return QMainWindow::eventFilter(watched, event);

    const auto urls = drop->mimeData()->urls();
    if (!drop->possibleActions().testFlag(Qt::CopyAction) ||
        std::none_of(urls.cbegin(), urls.cend(), [](const QUrl &url) { return url.isLocalFile(); })) {
        drop->ignore();
        return true;
    }

    if (type == QEvent::Drop) {
        QStringList files;
        for (const auto &url : urls) {
            if (!url.isLocalFile()) continue;
            const auto path = url.toLocalFile();
            if (QFileInfo(path).isFile()) files.push_back(path);
        }
        if (files.isEmpty()) {
            setStatus(QStringLiteral("끌어 놓은 항목에서 열 수 있는 파일을 찾지 못했습니다."));
            drop->ignore();
            return true;
        }
        for (const auto &path : files) openFile(path);
    }
    // Opening a file is read-only. Never report a MoveAction back to Explorer.
    drop->setDropAction(Qt::CopyAction);
    drop->accept();
    return true;
}

void MainWindow::addRecentFile(const QString &path) {
    QSettings settings;
    QStringList recent = settings.value(QStringLiteral("recentFiles")).toStringList();
    recent.removeAll(path); recent.prepend(path); while (recent.size() > 8) recent.removeLast();
    settings.setValue(QStringLiteral("recentFiles"), recent);
    if (!m_recentMenu) return;
    m_recentMenu->clear();
    for (const QString &recentPath : recent) if (QFileInfo::exists(recentPath)) {
        auto *action = m_recentMenu->addAction(QFileInfo(recentPath).fileName());
        connect(action, &QAction::triggered, this, [this, recentPath] { openFile(recentPath); });
    }
}

void MainWindow::clearDocument() {
    if (m_documentTabs->currentIndex() >= 0) { closeDocument(m_documentTabs->currentIndex()); return; }
    m_animation->stop();
    m_data = {};
    m_currentFile.clear(); m_currentFrequency = -1; m_currentFrame = -1;
    if (m_geometryMode) toggleGeometryMode();
    m_view->clearCubeGrid();
    m_view->setMolecule({});
    m_analysis->setData(m_data);
    populateExplorer();
    updateWindowTitle();
    setStatus(QStringLiteral("문서를 비웠습니다"));
}

void MainWindow::updateWindowTitle() {
    setWindowTitle(m_currentFile.isEmpty() ? QStringLiteral("Youngcraft") : QStringLiteral("Youngcraft — %1").arg(QFileInfo(m_currentFile).fileName()));
}

} // namespace youngcraft
