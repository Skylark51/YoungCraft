#include "ui/SidebarButton.h"
#include "app/MainWindow.h"
#include "compute/OptimizationInput.h"

#include "animation/AnimationController.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include "ui/XtbDialog.h"
#include "ui/ConstraintEditor.h"

#include <QComboBox>
#include <QDockWidget>
#include <QCheckBox>
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
#include <QPointer>
#include <QScrollArea>
#include <QTabBar>
#include <algorithm>

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
    m_view->setMolecule(m_data.molecule);
    applySynchronizedView(m_view);
    m_analysis->setData(m_data);
    populateExplorer();
    setDrawingMode(true);
}

void MainWindow::setDrawingMode(bool enabled) {
    if (enabled) ensureEditableDocument();
    if (enabled && m_geometryMode) toggleGeometryMode();
    if (enabled && m_bondEditMode) toggleBondEditMode();
    if (enabled) { m_animation->stop(); stopMMOptimization(); setAtomDragMode(false); }
    m_drawingMode = enabled;
    m_view->setDrawingMode(enabled);
    if (enabled) {
        m_workspace->setCurrentWidget(m_moleculeWorkspace);
        m_structureDock->show();
        m_structureTabs->setCurrentIndex(1);
    }
    updatePanelActions();
    setStatus(enabled ? QStringLiteral("원소 또는 ‘원자 / 결합 그리기’를 먼저 선택하세요 · 클릭으로 사슬 연장 · 드래그로 결합 · Esc/우클릭: 사슬 끝내기 · Alt: 자유 배치 · Lab Ligands: 금속 배위 구조")
                      : QStringLiteral("구조 그리기 종료"), enabled ? 0 : 4000);
}

void MainWindow::setAtomDragMode(bool enabled) {
    if(enabled) ensureEditableDocument();
    if (enabled && m_drawingMode) setDrawingMode(false);
    if (enabled) { m_animation->stop(); stopMMOptimization(); }
    m_dragAtoms = enabled;
    m_view->setAtomDragMode(enabled);
    if(enabled)m_view->setBondCentricMode(false);
    updatePanelActions();
    setStatus(enabled ? QStringLiteral("원자 이동: 원자를 왼쪽 버튼으로 드래그하세요.")
                      : QStringLiteral("원자 이동 종료"));
}

void MainWindow::commitEditedMolecule(const Molecule &molecule, bool topologyChanged, bool adjustHydrogens) {
    if (m_mmProcess) stopMMOptimization(QStringLiteral("구조 편집으로 MM 최적화를 중단했습니다."));
    Molecule committed = molecule;
    ensureEditableDocument();
    if (topologyChanged && adjustHydrogens && m_autoHButton && m_autoHButton->isChecked()) committed.reconcileAutoHydrogens();
    QString error;
    if (!MoleculeEditController::validate(committed, &error)) { setStatus(error); m_view->setEditedMolecule(m_data.molecule); return; }
    bool same=committed.atoms.size()==m_data.molecule.atoms.size()&&committed.bonds.size()==m_data.molecule.bonds.size()&&committed.coordinationSites.size()==m_data.molecule.coordinationSites.size()&&committed.hapticContacts.size()==m_data.molecule.hapticContacts.size();
    if(same) for(int i=0;i<committed.atoms.size();++i){const auto&a=committed.atoms[i];const auto&b=m_data.molecule.atoms[i];if(a.position!=b.position||a.atomicNumber!=b.atomicNumber||a.formalCharge!=b.formalCharge||a.autoHydrogen!=b.autoHydrogen||a.colorOverride!=b.colorOverride||a.radiusScale!=b.radiusScale||a.optimizationFixed!=b.optimizationFixed||a.autoHydrogenLocked!=b.autoHydrogenLocked){same=false;break;}}
    if(same) for(int i=0;i<committed.bonds.size();++i){const auto&a=committed.bonds[i];const auto&b=m_data.molecule.bonds[i];if(a.first!=b.first||a.second!=b.second||a.order!=b.order||a.coordination!=b.coordination||a.style!=b.style||a.manual!=b.manual){same=false;break;}}
    if(same) for(int i=0;i<committed.coordinationSites.size();++i){const auto&a=committed.coordinationSites[i];const auto&b=m_data.molecule.coordinationSites[i];if(a.direction!=b.direction||a.center!=b.center||a.occupied!=b.occupied||a.donor!=b.donor){same=false;break;}}
    if(same && committed.charge==m_data.molecule.charge)return;
    m_undoMolecules.push_back(m_data.molecule);
    m_redoMolecules.clear();
    if(topologyChanged) { m_view->selection().clear(); m_view->setSelectedAtom(-1); clearGeometrySelection(); }
    m_data.molecule = committed;
    m_view->setEditedMolecule(committed);
    if (topologyChanged) {
        m_data.frames.clear();
        m_currentFrame = -1;
        m_animation->setFrames({});
    }
    m_analysis->setData(m_data);
    populateExplorer();
}

void MainWindow::undoDrawingEdit() {
    if (m_undoMolecules.isEmpty()) { setStatus(QStringLiteral("실행 취소할 편집이 없습니다")); return; }
    if (m_mmProcess) stopMMOptimization(QStringLiteral("구조 편집으로 MM 최적화를 중단했습니다."));
    m_redoMolecules.push_back(m_data.molecule);
    m_data.molecule = m_undoMolecules.takeLast();
    m_view->selection().clear();
    m_view->setSelectedAtom(-1);
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
    m_view->selection().clear();
    m_view->setSelectedAtom(-1);
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
    atom.formalCharge = drawState().formalCharge;
    edited.charge+=atom.formalCharge;
    edited.atoms.push_back(atom);
    if (bondFrom >= 0 && bondFrom < edited.atoms.size() - 1) {
        MoleculeEditController::drawBond(edited, bondFrom, edited.atoms.size()-1, drawState());
    }
    commitEditedMolecule(edited, true);
    m_view->setSelectedAtom(newAtomIndex);
    m_view->setDrawingAnchorAt(position);
    setStatus(QStringLiteral("%1 원자를 추가했습니다. 원자 %2개, 결합 %3개")
                  .arg(atom.element).arg(m_data.molecule.atoms.size()).arg(m_data.molecule.bonds.size()));
}

void MainWindow::startMMOptimization() {
    if (auto *xtb = findChild<XtbDialog *>(); xtb && xtb->calculationActive()) { setStatus(QStringLiteral("xTB 계산을 완료하거나 중단한 뒤 MM 최적화를 시작하세요.")); return; }
    if (m_view->molecule().atoms.isEmpty()) { setStatus(QStringLiteral("최적화할 구조를 먼저 그리거나 여세요.")); return; }
    // A document owns its progress controls; detach them before replacing options.
    m_mmBar->hide();m_mmBar->setParent(m_moleculeWorkspace);
    if(auto *existing=findChild<QWidget *>("MMOptimizationPanel")) {
        const int tab=m_structureTabs->indexOf(existing->parentWidget()->parentWidget());
        if(tab>=0)m_structureTabs->removeTab(tab);
        delete existing->parentWidget()->parentWidget();
    }
    auto *scroll=new QScrollArea(m_structureTabs);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
    auto *panel=new QWidget;panel->setObjectName("MMOptimizationPanel");scroll->setWidget(panel);
    auto *form=new QFormLayout(panel);form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    auto *field=new QComboBox(panel);field->setObjectName("MMForceField");
    field->addItems({"Auto","MMFF94","MMFF94s","UFF","GFN-FF"});
    auto *steps=new QSpinBox(panel);steps->setObjectName("MMMaximumSteps");steps->setRange(1,100000);steps->setValue(500);
    form->addRow(QStringLiteral("힘장"),field);form->addRow(QStringLiteral("최대 반복"),steps);
    auto *hint=new QLabel(QStringLiteral("금속·배위 구조: Auto에서 GFN-FF 사용. 드래그로 선택 후 자물쇠 버튼으로 고정하세요."),panel);
    hint->setWordWrap(true);form->addRow(hint);
    auto *retainBonds=new QCheckBox(QStringLiteral("초기 결합 거리 유지"),panel);retainBonds->setObjectName("MMPreserveBonds");
    bool hasMetal=false;for(const auto &atom:m_view->molecule().atoms)hasMetal|=isMetal(atom.atomicNumber);
    retainBonds->setChecked(hasMetal);form->addRow(retainBonds);
    const QPointer<MoleculeView> source=m_view;
    const Molecule snapshot=source->molecule();
    auto *constraints=new ConstraintEditor(snapshot,source->geometrySelection(),panel,true);
    constraints->setSelectionProvider([source]{QVector<int> ids;if(source){ids=source->selection().atoms.values().toVector();std::sort(ids.begin(),ids.end());}return ids;});
    form->addRow(constraints);
    form->addRow(m_mmBar);
    auto *run=new QPushButton(QStringLiteral("MM 최적화 시작"),panel);run->setObjectName("MMStart");
    makeSidebarIconButton(run,QStyle::SP_MediaPlay);
    auto *runRow=new QHBoxLayout;runRow->addStretch();runRow->addWidget(run);form->addRow(runRow);
    connect(run,&QPushButton::clicked,panel,[this,source,snapshot,field,steps,retainBonds,constraints]{
        if(!source || source!=m_view){setStatus(QStringLiteral("현재 molecule에서 MM 패널을 다시 여세요."));return;}
        const auto &current=source->molecule();
        const bool changed=!sameOptimizationInput(current,snapshot);
        if(changed){setStatus(QStringLiteral("구조가 변경되었습니다. MM 패널을 다시 열어 설정하세요."));return;}
        QVector<GeometryConstraint> entries;QString error;
        if(!constraints->read(entries,&error)){setStatus(error,8000);return;}
        runMMOptimization(field->currentText(),steps->value(),retainBonds->isChecked(),entries);
    });
    const int tab=m_structureTabs->addTab(scroll,style()->standardIcon(QStyle::SP_BrowserReload),{});
    m_structureTabs->setTabToolTip(tab,QStringLiteral("MM 최적화"));m_structureTabs->tabBar()->setAccessibleTabName(tab,QStringLiteral("MM 최적화"));
    m_structureTabs->setCurrentIndex(tab);m_structureDock->show();m_structureDock->raise();resizeDocks({m_structureDock},{320},Qt::Horizontal);
}

void MainWindow::runMMOptimization(const QString &field, int steps, bool retainBonds, const QVector<GeometryConstraint> &entries) {
    if(auto *xtb=findChild<XtbDialog *>();xtb && xtb->calculationActive()){setStatus(QStringLiteral("진행 중인 QM 계산을 먼저 중단하세요."));return;}
    bool hasMetal=false;for(const auto &atom:m_view->molecule().atoms)hasMetal|=isMetal(atom.atomicNumber);
    if(field=="GFN-FF" || (field=="Auto" && hasMetal)){
        startXtbCalculation(0,true,entries,steps);
        if(auto *xtb=findChild<XtbDialog *>())xtb->setBondRetention(retainBonds);
        return;
    }
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
            QMessageBox::warning(this, QStringLiteral("구조 최적화"),
                QStringLiteral("MM 실행 프로그램을 찾지 못했습니다. 휴대용 배포본에는 mm_runtime/YoungcraftMM.exe가 필요합니다."));
            return;
        }
        arguments = {QStringLiteral("-u"), script};
    }

#ifdef YOUNGCRAFT_TESTING
    if (!QString::fromUtf8(qgetenv("YOUNGCRAFT_MM_TEST_WORKER")).isEmpty()) {
        program = QString::fromUtf8(qgetenv("YOUNGCRAFT_MM_TEST_WORKER")); arguments.clear();
    }
#endif
    stopMMOptimization();
    Molecule molecule = m_view->molecule();
    QString placementError;if(!prepareFixedDistances(molecule,entries,&placementError)){QMessageBox::warning(this,QStringLiteral("거리 배치"),placementError);return;}
    m_mmDisplayedIteration = 0;
    m_mmFinalMessage.clear();
    if (m_drawingMode) setDrawingMode(false);
    if (m_dragAtoms) setAtomDragMode(false);
    m_animation->stop();
    m_view->setFrequencyMode(nullptr);
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    m_mmMaximum = steps;
    m_mmStep = 0;
    m_mmField.clear();
    m_mmState = MMState::Starting;
    m_mmStepPending = false;
    m_mmOutput.clear();
    m_mmStderr.clear();
    m_mmSeries.clear();
    m_mmOriginalCoords.clear();
    for (const Atom &atom : m_view->molecule().atoms) {
        m_mmOriginalCoords.push_back(atom.position);
    }
    m_mmPlot->setSeries({}, QStringLiteral("Optimization step"), QStringLiteral("Energy (kcal mol⁻¹)"));
    m_mmProgress->setRange(0, m_mmMaximum);
    m_mmProgress->setValue(0);
    m_mmPauseButton->setEnabled(true);
    setSidebarButtonIcon(m_mmPauseButton, QStyle::SP_MediaPause, QStringLiteral("일시정지"));
    setSidebarButtonIcon(m_mmStopButton, QStyle::SP_MediaStop, QStringLiteral("중단"));
    m_mmRestoreButton->setEnabled(true);
    m_mmLabel->setText(QStringLiteral("%1 초기화 중...").arg(field));
    m_mmBar->show();

    QJsonObject input;
    input.insert(QStringLiteral("constraints"), constraintsJson(entries));
    input.insert(QStringLiteral("preserve_bond_lengths"),retainBonds);
    input.insert(QStringLiteral("field"), field);
    input.insert(QStringLiteral("charge"), molecule.charge);
    input.insert(QStringLiteral("max_iterations"), m_mmMaximum);
    input.insert(QStringLiteral("chunk"), 8);
    QJsonArray atoms;
    for (const Atom &atom : molecule.atoms) {
        QJsonObject item;
        item.insert(QStringLiteral("z"), atom.atomicNumber);
        item.insert(QStringLiteral("formal_charge"), atom.formalCharge);
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
    process->setProperty("mmRequest",request);
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
            failMMOptimization(QStringLiteral("MM worker 오류: %1").arg(process->errorString()));
    });
    connect(process, &QProcess::finished, this, [this, process] {
        if (process != m_mmProcess) return;
        handleMMOutput();
        if (process == m_mmProcess && m_mmState != MMState::Finished)
            failMMOptimization(QStringLiteral("MM worker가 완료 응답 없이 종료되었습니다."));
    });
    process->start(program, arguments);
}

void MainWindow::handleMMOutput() {
    if (!m_mmProcess) return;
    m_mmOutput += m_mmProcess->readAllStandardOutput();
    if (m_mmOutput.size() > 4 * 1024 * 1024) {
        failMMOptimization(QStringLiteral("MM worker 응답이 너무 큽니다.")); return;
    }
    qsizetype end;
    while (m_mmProcess && (end = m_mmOutput.indexOf('\n')) >= 0) {
        const QByteArray line = m_mmOutput.left(end);
        m_mmOutput.remove(0, end + 1);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            failMMOptimization(QStringLiteral("MM worker가 잘못된 JSON을 보냈습니다: %1").arg(parseError.errorString()));
            return;
        }
        const QJsonObject result = document.object();
        const QString type = result.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("fallback") && result.value(QStringLiteral("method")).toString() == "GFN-FF" && m_mmState == MMState::Starting) {
            const auto request=QJsonDocument::fromJson(m_mmProcess->property("mmRequest").toByteArray()).object();
            QVector<GeometryConstraint> entries;
            for(const auto &value:request.value("constraints").toArray()) {
                const auto item=value.toObject(); GeometryConstraint c;
                c.type=item.value("type").toString();c.target=item.value("target").toDouble();
                for(const auto &id:item.value("atoms").toArray())c.atoms<<id.toInt();
                entries<<c;
            }
            const QString reason=result.value("message").toString();
            stopMMOptimization();m_mmState=MMState::Finished;m_mmBar->hide();
            startXtbCalculation(0,true,entries,request.value("max_iterations").toInt(500));
            if(auto *xtb=findChild<XtbDialog *>()){
                xtb->configureOptimization(entries,request.value("max_iterations").toInt(500));
                xtb->setBondRetention(request.value("preserve_bond_lengths").toBool());
            }
            setStatus(reason,0);
            return;
        }
        if (type == QStringLiteral("error")) {
            failMMOptimization(QStringLiteral("MM 최적화 실패: %1").arg(result.value(QStringLiteral("message")).toString()));
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
            failMMOptimization(QStringLiteral("MM worker 응답 필드가 유효하지 않습니다.")); return;
        }
        const int iteration = stepValue.toInt(-1);
        if (type == QStringLiteral("ready")) {
            if (iteration != 0 || !m_mmField.isEmpty()) {
                failMMOptimization(QStringLiteral("MM worker ready 응답이 유효하지 않습니다.")); return;
            }
            m_mmField = field;
            m_mmSeries.push_back(QPointF(0, energyValue.toDouble()));
            m_mmPlot->setSeries(m_mmSeries, QStringLiteral("Optimization step"), QStringLiteral("Energy (kcal mol⁻¹)"));
            m_mmLabel->setText(QStringLiteral("%1 · 0/%2 · E %3 kcal/mol · RMS ∇ %4 · Max ∇ %5")
                .arg(field).arg(m_mmMaximum).arg(energyValue.toDouble(), 0, 'f', 5)
                .arg(rmsValue.toDouble(), 0, 'f', 4).arg(maxValue.toDouble(), 0, 'f', 4));
            if (m_mmState == MMState::Starting) m_mmState = MMState::Running;
            scheduleMMStep();
            continue;
        }
        const QJsonValue deltaValue = result.value(QStringLiteral("delta_energy"));
        const QJsonArray positions = result.value(QStringLiteral("positions")).toArray();
        if (field != m_mmField || iteration <= m_mmStep || iteration > m_mmMaximum ||
            !deltaValue.isDouble() || !std::isfinite(deltaValue.toDouble()) ||
            !result.value(QStringLiteral("converged")).isBool() ||
            positions.size() != m_view->molecule().atoms.size()) {
            failMMOptimization(QStringLiteral("MM worker step 응답이 유효하지 않습니다.")); return;
        }
        QVector<QVector3D> coordinates;
        coordinates.reserve(positions.size());
        for (const QJsonValue &value : positions) {
            const QJsonArray p = value.toArray();
            if (p.size() != 3 || !p[0].isDouble() || !p[1].isDouble() || !p[2].isDouble() ||
                !std::isfinite(p[0].toDouble()) || !std::isfinite(p[1].toDouble()) || !std::isfinite(p[2].toDouble())) {
                failMMOptimization(QStringLiteral("MM worker 좌표가 유효하지 않습니다.")); return;
            }
            coordinates.push_back(QVector3D(p[0].toDouble(), p[1].toDouble(), p[2].toDouble()));
        }
        m_mmStepPending = false;
        m_mmStep = iteration;
        m_mmProgress->setValue(iteration);
        // Commit every real worker geometry; display FPS cannot gate calculation.
        m_view->setCoordinates(coordinates);
        m_data.molecule = m_view->molecule();
        m_mmDisplayedIteration = iteration;
        if (m_analysis->coordinateViewActive()) m_analysis->setCoordinateMolecule(m_data.molecule);
        m_mmSeries.push_back(QPointF(iteration, energyValue.toDouble()));
        m_mmPlot->setSeries(m_mmSeries, QStringLiteral("Optimization step"), QStringLiteral("Energy (kcal mol⁻¹)"));
        const QString iterationText = result.value(QStringLiteral("iteration_exact")).toBool(true)
            ? QString::number(iteration) : QStringLiteral("≤%1").arg(iteration);
        m_mmLabel->setText(QStringLiteral("%1 · %2/%3 · E %4 kcal/mol · ΔE %5 · RMS ∇ %6 · Max ∇ %7 · %8")
            .arg(field).arg(iterationText).arg(m_mmMaximum).arg(energyValue.toDouble(), 0, 'f', 5)
            .arg(deltaValue.toDouble(), 0, 'f', 5).arg(rmsValue.toDouble(), 0, 'f', 4)
            .arg(maxValue.toDouble(), 0, 'f', 4)
            .arg(type == QStringLiteral("done") ?
                 (result.value(QStringLiteral("converged")).toBool() ? QStringLiteral("수렴") : QStringLiteral("최대 단계"))
                 : QStringLiteral("진행 중")));
        if (result.value(QStringLiteral("converged")).toBool() || iteration >= m_mmMaximum) {
            m_mmFinalMessage = result.value(QStringLiteral("converged")).toBool()
                ? QStringLiteral("MM 최적화 수렴: %1단계").arg(iteration)
                : QStringLiteral("MM 최대 단계 %1에 도달했습니다.").arg(iteration);
            m_mmState = MMState::Finished;
            stopMMOptimization(m_mmFinalMessage);
            return;
        }
        if (type == QStringLiteral("done")) {
            failMMOptimization(QStringLiteral("MM worker가 수렴/iteration limit 없이 done을 보냈습니다.")); return;
        }
        scheduleMMStep();
    }
}

void MainWindow::scheduleMMStep() {
    if (!m_mmProcess || m_mmState != MMState::Running || m_mmStepPending || m_mmStep >= m_mmMaximum) return;
    m_mmStepPending = true;
    QProcess *process = m_mmProcess;
    QTimer::singleShot(20, this, [this, process] {
        if (process != m_mmProcess) return;
        if (m_mmState != MMState::Running) { m_mmStepPending = false; return; }
        if (process->write("chunk\n") < 0) failMMOptimization(QStringLiteral("MM worker 명령 전송 실패"));
    });
}

void MainWindow::failMMOptimization(const QString &message) {
    m_mmState = MMState::Failed;
    stopMMOptimization(message);
}

void MainWindow::stopMMOptimization(const QString &message) {
    if (!m_mmProcess) return;
    if (m_mmState != MMState::Finished && m_mmState != MMState::Failed) m_mmState = MMState::Stopping;
    QProcess *process = m_mmProcess;
    m_mmProcess = nullptr;
    m_mmStepPending = false;
    process->disconnect(this);
    connect(process, &QProcess::finished, this, [this] {
        if (!m_mmProcess && m_mmState == MMState::Stopping) m_mmState = MMState::Finished;
    });
    connect(process, &QProcess::finished, process, &QObject::deleteLater);
    if (process->state() == QProcess::NotRunning) process->deleteLater();
    else process->kill();
    if (process->state() == QProcess::NotRunning && m_mmState == MMState::Stopping) m_mmState = MMState::Finished;
    m_mmPauseButton->setEnabled(false);
    setSidebarButtonIcon(m_mmStopButton, QStyle::SP_DialogCloseButton, QStringLiteral("닫기"));
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
