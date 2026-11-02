#include "app/MainWindow.h"

#include "animation/AnimationController.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>

#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace youngcraft {

void MainWindow::newDrawingDocument(const Molecule &initial) {
    // Callers may pass m_view->molecule(). Keep a snapshot before changing
    // the active document and its widgets.
    const Molecule startingMolecule = initial;
    stopMMOptimization();
    saveDocumentState();
    m_data = {};
    m_currentFile.clear();
    m_currentFrame = -1;
    m_currentFrequency = -1;
    m_drawingMode = false;
    m_dragAtoms = false;
    m_geometryMode = false;
    m_bondEditMode = false;
    createDocumentUi();
    auto document = captureDocument();
    document->data.molecule = startingMolecule;
    document->data.molecule.title = QStringLiteral("새 구조");
    document->workspace->setProperty("loadFinished", true);
    document->workspace->setProperty("loadSuccess", true);
    m_documents.insert(document->workspace, document);
    int index;
    {
        const QSignalBlocker blocker(m_documentTabs);
        index = m_documentTabs->addTab(document->workspace, QStringLiteral("새 구조"));
        m_documentTabs->setCurrentIndex(index);
    }
    bindDocument(document);
    activateDocument(index);
    m_scanWorkspace->hide();
    m_view->setMolecule(m_data.molecule);
    m_analysis->setData(m_data);
    populateExplorer();
    setDrawingMode(true);
}

void MainWindow::setDrawingMode(bool enabled) {
    if (enabled && m_geometryMode) toggleGeometryMode();
    if (enabled && m_bondEditMode) toggleBondEditMode();
    if (enabled) { m_animation->stop(); stopMMOptimization(); setAtomDragMode(false); }
    m_drawingMode = enabled;
    m_view->setDrawingMode(enabled);
    m_drawBar->setVisible(enabled);
    if (enabled) m_workspace->setCurrentWidget(m_moleculeWorkspace);
    setStatus(enabled ? QStringLiteral("구조 그리기: 빈 곳 클릭으로 원자 추가, 원자에서 드래그하여 결합 · Alt: 길이 스냅 해제")
                      : QStringLiteral("구조 그리기 종료"), enabled ? 0 : 4000);
}

void MainWindow::setAtomDragMode(bool enabled) {
    if (enabled && m_drawingMode) setDrawingMode(false);
    if (enabled) { m_animation->stop(); stopMMOptimization(); }
    m_dragAtoms = enabled;
    m_view->setAtomDragMode(enabled);
    setStatus(enabled ? QStringLiteral("원자 이동: 원자를 왼쪽 버튼으로 드래그하세요.")
                      : QStringLiteral("원자 이동 종료"));
}

void MainWindow::commitEditedMolecule(const Molecule &molecule, bool topologyChanged) {
    if (m_mmProcess) stopMMOptimization(QStringLiteral("구조 편집으로 MM 최적화를 중단했습니다."));
    Molecule committed = molecule;
    if (topologyChanged) {
        if (m_currentFile.isEmpty()) {
            m_undoMolecules.push_back(m_data.molecule);
            if (m_undoMolecules.size() > 100) m_undoMolecules.removeFirst();
            m_redoMolecules.clear();
        }
        if (m_autoHButton && m_autoHButton->isChecked()) committed.reconcileAutoHydrogens();
    }
    m_data.molecule = committed;
    m_view->setEditedMolecule(committed);
    if (topologyChanged) {
        m_data.frames.clear();
        m_currentFrame = -1;
        m_animation->setFrames({});
    } else if (m_currentFrame >= 0 && m_currentFrame < m_data.frames.size() &&
               m_data.frames[m_currentFrame].coordinates.size() == committed.atoms.size()) {
        auto &coordinates = m_data.frames[m_currentFrame].coordinates;
        for (int i = 0; i < committed.atoms.size(); ++i) coordinates[i] = committed.atoms[i].position;
    }
    m_analysis->setData(m_data);
    populateExplorer();
}

void MainWindow::undoDrawingEdit() {
    if (m_undoMolecules.isEmpty()) { setStatus(QStringLiteral("실행 취소할 편집이 없습니다")); return; }
    if (m_mmProcess) stopMMOptimization(QStringLiteral("구조 편집으로 MM 최적화를 중단했습니다."));
    m_redoMolecules.push_back(m_data.molecule);
    m_data.molecule = m_undoMolecules.takeLast();
    m_view->setEditedMolecule(m_data.molecule);
    if (m_view->selectedAtom() >= m_data.molecule.atoms.size()) m_view->setSelectedAtom(-1);
    m_analysis->setData(m_data);
    populateExplorer();
    setStatus(QStringLiteral("구조 편집을 취소했습니다."));
}

void MainWindow::redoDrawingEdit() {
    if (m_redoMolecules.isEmpty()) { setStatus(QStringLiteral("다시 실행할 편집이 없습니다")); return; }
    if (m_mmProcess) stopMMOptimization(QStringLiteral("구조 편집으로 MM 최적화를 중단했습니다."));
    m_undoMolecules.push_back(m_data.molecule);
    m_data.molecule = m_redoMolecules.takeLast();
    m_view->setEditedMolecule(m_data.molecule);
    m_analysis->setData(m_data);
    populateExplorer();
    setStatus(QStringLiteral("구조 편집을 다시 실행했습니다."));
}

void MainWindow::addDrawnAtom(const QVector3D &position, int bondFrom) {
    Molecule edited = m_view->molecule();
    int newAtomIndex = edited.atoms.size();
    if (m_autoHButton && m_autoHButton->isChecked()) {
        newAtomIndex = 0;
        for (const Atom &existing : edited.atoms) if (!existing.autoHydrogen) ++newAtomIndex;
    }
    const int z = atomicNumberFor(m_drawElement->currentText());
    if (z <= 0) { setStatus(QStringLiteral("유효한 원소 기호를 선택하세요.")); return; }
    Atom atom;
    atom.index = edited.atoms.size() + 1;
    atom.element = elementSymbol(z);
    atom.atomicNumber = z;
    atom.position = position;
    edited.atoms.push_back(atom);
    if (bondFrom >= 0 && bondFrom < edited.atoms.size() - 1) {
        const double order = m_drawBondOrder->currentData().toDouble();
        edited.setBond(bondFrom, edited.atoms.size() - 1, order, false,
                       order == 1.5 ? BondStyle::Dashed : BondStyle::Solid, true);
    }
    commitEditedMolecule(edited, true);
    m_view->setSelectedAtom(newAtomIndex);
    setStatus(QStringLiteral("%1 원자를 추가했습니다. 원자 %2개, 결합 %3개")
                  .arg(atom.element).arg(m_data.molecule.atoms.size()).arg(m_data.molecule.bonds.size()));
}

void MainWindow::startMMOptimization() {
    if (m_view->molecule().atoms.isEmpty()) { setStatus(QStringLiteral("최적화할 구조를 먼저 그리거나 여세요.")); return; }
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Geometry optimization 옵션"));
    QFormLayout form(&dialog);
    QComboBox field(&dialog);
    field.addItems({QStringLiteral("Auto"), QStringLiteral("MMFF94"),
                    QStringLiteral("MMFF94s"), QStringLiteral("UFF")});
    QSpinBox steps(&dialog);
    steps.setRange(1, 5000); steps.setValue(500);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(QStringLiteral("Force field"), &field);
    form.addRow(QStringLiteral("최대 iteration"), &steps);
    form.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;

    const QDir applicationDir(QCoreApplication::applicationDirPath());
    const QString bundled = applicationDir.filePath(QStringLiteral("mm_runtime/YoungcraftMM.exe"));
    const QString projectBuildWorker = applicationDir.filePath(
        QStringLiteral("dist/mm_runtime/YoungcraftMM.exe"));
    QString program;
    QStringList arguments;
    if (QFileInfo::exists(bundled)) {
        program = bundled;
    } else if (QFileInfo::exists(projectBuildWorker)) {
        // The in-tree Youngcraft.exe is built in the project root, while the
        // portable runtime is staged under dist/. Use that runtime for local runs.
        program = projectBuildWorker;
    } else {
        program = QStandardPaths::findExecutable(QStringLiteral("python"));
        const QString script = applicationDir.filePath(QStringLiteral("tools/mm_optimize.py"));
        if (program.isEmpty() || !QFileInfo::exists(script)) {
            QMessageBox::warning(this, QStringLiteral("Geometry optimization"),
                QStringLiteral("MM worker를 찾지 못했습니다. portable 배포본에는 mm_runtime/YoungcraftMM.exe가 필요합니다."));
            return;
        }
        arguments = {QStringLiteral("-u"), script};
    }

    stopMMOptimization();
    const Molecule molecule = m_view->molecule();
    if (!m_mmAnimationTimer) {
        m_mmAnimationTimer = new QTimer(this);
        m_mmAnimationTimer->setInterval(20); // 50 FPS; display work stays on the GUI timer.
        connect(m_mmAnimationTimer, &QTimer::timeout, this, &MainWindow::advanceMMAnimation);
    }
    m_mmAnimationTimer->stop();
    m_mmFrameQueue.clear();
    m_mmQueuedIterations.clear();
    m_mmDisplayedCoords.clear();
    m_mmTransitionStart.clear();
    m_mmTransitionTick = 0;
    m_mmDisplayedIteration = 0;
    m_mmFinalPending = false;
    m_mmFinalMessage.clear();
    if (m_drawingMode) setDrawingMode(false);
    if (m_dragAtoms) setAtomDragMode(false);
    m_animation->stop();
    m_view->setFrequencyMode(nullptr);
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    m_mmMaximum = steps.value();
    m_mmStep = 0;
    m_mmField.clear();
    m_mmPaused = false;
    m_mmStepPending = false;
    m_mmOutput.clear();
    m_mmStderr.clear();
    m_mmSeries.clear();
    m_mmOriginalCoords.clear();
    for (const Atom &atom : molecule.atoms) {
        m_mmOriginalCoords.push_back(atom.position);
        m_mmDisplayedCoords.push_back(atom.position);
    }
    m_mmPlot->setSeries({}, QStringLiteral("Optimization step"), QStringLiteral("Energy (kcal/mol)"));
    m_mmProgress->setRange(0, m_mmMaximum);
    m_mmProgress->setValue(0);
    m_mmPauseButton->setEnabled(true);
    m_mmPauseButton->setText(QStringLiteral("일시정지"));
    m_mmStopButton->setText(QStringLiteral("중단"));
    m_mmRestoreButton->setEnabled(true);
    m_mmLabel->setText(QStringLiteral("%1 초기화 중...").arg(field.currentText()));
    m_mmBar->show();

    QJsonObject input;
    input.insert(QStringLiteral("field"), field.currentText());
    input.insert(QStringLiteral("charge"), molecule.charge);
    input.insert(QStringLiteral("max_iterations"), m_mmMaximum);
    QJsonArray atoms;
    for (const Atom &atom : molecule.atoms) {
        QJsonObject item;
        item.insert(QStringLiteral("z"), atom.atomicNumber);
        item.insert(QStringLiteral("p"), QJsonArray{atom.position.x(), atom.position.y(), atom.position.z()});
        atoms.append(item);
    }
    QJsonArray bonds;
    for (const Bond &bond : molecule.bonds)
        bonds.append(QJsonArray{bond.first, bond.second, bond.order, bond.coordination});
    input.insert(QStringLiteral("atoms"), atoms);
    input.insert(QStringLiteral("bonds"), bonds);
    const QByteArray request = QJsonDocument(input).toJson(QJsonDocument::Compact) + '\n';

    auto *process = new QProcess(this);
    m_mmProcess = process;
    process->setProcessChannelMode(QProcess::SeparateChannels);
#ifdef Q_OS_WIN
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    connect(process, &QProcess::started, this, [this, process, request] {
        if (process == m_mmProcess) process->write(request);
    });
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] {
        if (process == m_mmProcess) handleMMOutput();
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process] {
        if (process == m_mmProcess) {
            m_mmStderr += process->readAllStandardError();
            if (m_mmStderr.size() > 4096) m_mmStderr = m_mmStderr.right(4096);
        }
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError) {
        if (process == m_mmProcess)
            stopMMOptimization(QStringLiteral("MM worker 오류: %1").arg(process->errorString()));
    });
    connect(process, &QProcess::finished, this, [this, process] {
        if (process != m_mmProcess) return;
        handleMMOutput();
        if (process == m_mmProcess && !m_mmFinalPending)
            stopMMOptimization(QStringLiteral("MM worker가 완료 응답 없이 종료되었습니다."));
    });
    process->start(program, arguments);
}

void MainWindow::handleMMOutput() {
    if (!m_mmProcess) return;
    m_mmOutput += m_mmProcess->readAllStandardOutput();
    if (m_mmOutput.size() > 4 * 1024 * 1024) {
        stopMMOptimization(QStringLiteral("MM worker 응답이 너무 큽니다.")); return;
    }
    qsizetype end;
    while (m_mmProcess && (end = m_mmOutput.indexOf('\n')) >= 0) {
        const QByteArray line = m_mmOutput.left(end);
        m_mmOutput.remove(0, end + 1);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            stopMMOptimization(QStringLiteral("MM worker가 잘못된 JSON을 보냈습니다: %1").arg(parseError.errorString()));
            return;
        }
        const QJsonObject result = document.object();
        const QString type = result.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("error")) {
            stopMMOptimization(QStringLiteral("MM 최적화 실패: %1").arg(result.value(QStringLiteral("message")).toString()));
            return;
        }
        const QJsonValue energyValue = result.value(QStringLiteral("energy"));
        const QJsonValue rmsValue = result.value(QStringLiteral("rms_gradient"));
        const QJsonValue maxValue = result.value(QStringLiteral("max_gradient"));
        const QJsonValue stepValue = result.value(QStringLiteral("iteration"));
        const QString field = result.value(QStringLiteral("field")).toString();
        if ((type != QStringLiteral("ready") && type != QStringLiteral("step") && type != QStringLiteral("done")) ||
            !energyValue.isDouble() || !rmsValue.isDouble() || !maxValue.isDouble() || !stepValue.isDouble() ||
            !std::isfinite(energyValue.toDouble()) || !std::isfinite(rmsValue.toDouble()) ||
            !std::isfinite(maxValue.toDouble()) ||
            (field != QStringLiteral("MMFF94") && field != QStringLiteral("MMFF94s") && field != QStringLiteral("UFF"))) {
            stopMMOptimization(QStringLiteral("MM worker 응답 필드가 유효하지 않습니다.")); return;
        }
        const int iteration = stepValue.toInt(-1);
        if (type == QStringLiteral("ready")) {
            if (iteration != 0 || !m_mmField.isEmpty()) {
                stopMMOptimization(QStringLiteral("MM worker ready 응답이 유효하지 않습니다.")); return;
            }
            m_mmField = field;
            m_mmSeries.push_back(QPointF(0, energyValue.toDouble()));
            m_mmPlot->setSeries(m_mmSeries, QStringLiteral("Optimization step"), QStringLiteral("Energy (kcal/mol)"));
            m_mmLabel->setText(QStringLiteral("%1 · 0/%2 · E %3 kcal/mol · RMS ∇ %4 · Max ∇ %5")
                .arg(field).arg(m_mmMaximum).arg(energyValue.toDouble(), 0, 'f', 5)
                .arg(rmsValue.toDouble(), 0, 'f', 4).arg(maxValue.toDouble(), 0, 'f', 4));
            m_mmAnimationTimer->start();
            scheduleMMStep();
            continue;
        }
        const QJsonValue deltaValue = result.value(QStringLiteral("delta_energy"));
        const QJsonArray positions = result.value(QStringLiteral("positions")).toArray();
        if (field != m_mmField || iteration <= m_mmStep || iteration > m_mmMaximum ||
            !deltaValue.isDouble() || !std::isfinite(deltaValue.toDouble()) ||
            !result.value(QStringLiteral("converged")).isBool() ||
            positions.size() != m_view->molecule().atoms.size()) {
            stopMMOptimization(QStringLiteral("MM worker step 응답이 유효하지 않습니다.")); return;
        }
        QVector<QVector3D> coordinates;
        coordinates.reserve(positions.size());
        for (const QJsonValue &value : positions) {
            const QJsonArray p = value.toArray();
            if (p.size() != 3 || !p[0].isDouble() || !p[1].isDouble() || !p[2].isDouble() ||
                !std::isfinite(p[0].toDouble()) || !std::isfinite(p[1].toDouble()) || !std::isfinite(p[2].toDouble())) {
                stopMMOptimization(QStringLiteral("MM worker 좌표가 유효하지 않습니다.")); return;
            }
            coordinates.push_back(QVector3D(p[0].toDouble(), p[1].toDouble(), p[2].toDouble()));
        }
        m_mmStepPending = false;
        m_mmStep = iteration;
        m_mmProgress->setValue(iteration);
        m_mmFrameQueue.push_back(coordinates);
        m_mmQueuedIterations.push_back(iteration);
        m_mmSeries.push_back(QPointF(iteration, energyValue.toDouble()));
        m_mmPlot->setSeries(m_mmSeries, QStringLiteral("Optimization step"), QStringLiteral("Energy (kcal/mol)"));
        const QString iterationText = result.value(QStringLiteral("iteration_exact")).toBool(true)
            ? QString::number(iteration) : QStringLiteral("≤%1").arg(iteration);
        m_mmLabel->setText(QStringLiteral("%1 · %2/%3 · E %4 kcal/mol · ΔE %5 · RMS ∇ %6 · Max ∇ %7 · %8")
            .arg(field).arg(iterationText).arg(m_mmMaximum).arg(energyValue.toDouble(), 0, 'f', 5)
            .arg(deltaValue.toDouble(), 0, 'f', 5).arg(rmsValue.toDouble(), 0, 'f', 4)
            .arg(maxValue.toDouble(), 0, 'f', 4)
            .arg(type == QStringLiteral("done") ?
                 (result.value(QStringLiteral("converged")).toBool() ? QStringLiteral("수렴") : QStringLiteral("최대 단계"))
                 : QStringLiteral("진행 중")));
        if (type == QStringLiteral("done")) {
            m_mmFinalPending = true;
            m_mmFinalMessage = result.value(QStringLiteral("converged")).toBool()
                ? QStringLiteral("MM 최적화 수렴: %1단계").arg(iteration)
                : QStringLiteral("MM 최대 단계 %1에 도달했습니다.").arg(iteration);
            return;
        }
    }
}

void MainWindow::advanceMMAnimation() {
    if (m_mmPaused || !m_mmProcess) return;
    if (!m_mmFrameQueue.isEmpty()) {
        if (m_mmTransitionTick == 0) m_mmTransitionStart = m_mmDisplayedCoords;
        constexpr int ticksPerTarget = 8;
        ++m_mmTransitionTick;
        const float fraction = float(m_mmTransitionTick) / ticksPerTarget;
        const auto &target = m_mmFrameQueue.first();
        if (target.size() != m_mmDisplayedCoords.size()) {
            stopMMOptimization(QStringLiteral("MM animation 좌표 수가 일치하지 않습니다."));
            return;
        }
        QVector<QVector3D> displayed;
        displayed.reserve(target.size());
        for (int i = 0; i < target.size(); ++i)
            displayed.push_back(m_mmTransitionStart[i] * (1.0f - fraction) + target[i] * fraction);
        m_mmDisplayedCoords = displayed;
        m_view->setCoordinates(displayed);
        if (m_mmTransitionTick >= ticksPerTarget) {
            m_mmDisplayedIteration = m_mmQueuedIterations.takeFirst();
            m_mmFrameQueue.removeFirst();
            m_mmTransitionTick = 0;
            m_data.molecule = m_view->molecule(); // Only a real worker geometry enters the model.
            if (m_analysis->coordinateViewActive()) m_analysis->setCoordinateMolecule(m_data.molecule);
        }
    }
    if (m_mmFinalPending && m_mmFrameQueue.isEmpty()) {
        stopMMOptimization(m_mmFinalMessage);
        return;
    }
    if (!m_mmFinalPending && m_mmFrameQueue.size() < 2) scheduleMMStep();
}

void MainWindow::scheduleMMStep() {
    if (!m_mmProcess || m_mmPaused || m_mmStepPending || m_mmFinalPending ||
        m_mmFrameQueue.size() >= 2) return;
    m_mmStepPending = true;
    QProcess *process = m_mmProcess;
    QTimer::singleShot(0, this, [this, process] {
        if (process != m_mmProcess) return;
        if (m_mmPaused) { m_mmStepPending = false; return; }
        process->write("chunk\n");
    });
}

void MainWindow::stopMMOptimization(const QString &message) {
    if (!m_mmProcess) return;
    if (m_mmAnimationTimer) m_mmAnimationTimer->stop();
    // A stopped interpolation must not leave a transient position in the view
    // while the model still holds the last completed worker geometry.
    if (m_mmTransitionTick > 0) {
        QVector<QVector3D> committed;
        committed.reserve(m_data.molecule.atoms.size());
        for (const Atom &atom : m_data.molecule.atoms) committed.push_back(atom.position);
        m_view->setCoordinates(committed);
        m_mmDisplayedCoords = committed;
    }
    QProcess *process = m_mmProcess;
    m_mmProcess = nullptr;
    m_mmStepPending = false;
    process->disconnect(this);
    process->kill();
    process->waitForFinished(100);
    process->deleteLater();
    m_mmFrameQueue.clear();
    m_mmQueuedIterations.clear();
    m_mmTransitionTick = 0;
    m_mmFinalPending = false;
    m_mmPauseButton->setEnabled(false);
    m_mmStopButton->setText(QStringLiteral("닫기"));
    if (m_mmDisplayedIteration > 0) {
        Frame frame;
        frame.metrics.step = m_mmDisplayedIteration;
        frame.metrics.label = QStringLiteral("MM geometry optimization (%1)").arg(m_mmField);
        for (const Atom &atom : m_data.molecule.atoms) frame.coordinates.push_back(atom.position);
        m_data.frames.push_back(frame);
        m_currentFrame = m_data.frames.size() - 1;
        m_animation->setFrames(m_data.frames);
        m_analysis->setData(m_data);
        m_analysis->setCurrentFrame(m_currentFrame);
        populateExplorer();
    }
    QString detail = message;
    if (!m_mmStderr.trimmed().isEmpty() &&
        (message.contains(QStringLiteral("오류")) || message.contains(QStringLiteral("실패")) ||
         message.contains(QStringLiteral("완료 응답 없이"))))
        detail += QStringLiteral(" · stderr: %1").arg(QString::fromUtf8(m_mmStderr.trimmed()));
    if (!detail.isEmpty()) {
        m_mmLabel->setText(m_mmLabel->text() + QStringLiteral("\n") + detail);
        setStatus(detail, 0);
    }
}

} // namespace youngcraft
