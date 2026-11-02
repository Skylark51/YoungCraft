#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "model/StructureTools.h"
#include <QInputDialog>
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
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
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>

namespace youngcraft {

void MainWindow::createMenus() {
    auto add = [](QMenu *menu, const QString &text, const QKeySequence &shortcut,
                  const std::function<void()> &function) {
        auto *action = menu->addAction(text);
        if (!shortcut.isEmpty()) action->setShortcut(shortcut);
        QObject::connect(action, &QAction::triggered, function);
        return action;
    };

    QMenu *file = menuBar()->addMenu(QStringLiteral("파일"));
    add(file, QStringLiteral("새로 비우기"), {}, [this] { clearDocument(); });
    add(file, QStringLiteral("열기"), QKeySequence(QStringLiteral("Ctrl+O")), [this] { openFile(); });
    m_recentMenu = file->addMenu(QStringLiteral("최근 파일 다시 열기"));
    const QStringList recent = QSettings().value(QStringLiteral("recentFiles")).toStringList();
    for (const QString &path : recent) {
        if (QFileInfo::exists(path)) add(m_recentMenu, QFileInfo(path).fileName(), {}, [this, path] { openFile(path); });
    }
    add(file, QStringLiteral("여러 파일 열기"), {}, [this] {
        const QStringList paths = QFileDialog::getOpenFileNames(this, QStringLiteral("여러 파일 열기"), {}, FileDispatcher::openFilter());
        for (const auto &path : paths) openFile(path);
    });
    add(file, QStringLiteral("새 창"), QKeySequence(QStringLiteral("Ctrl+Shift+N")), [] {
        auto *window = new MainWindow;
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    });
    add(file, QStringLiteral("현재 탭을 새 창으로 분리"), {}, [this] { detachDocument(m_documentTabs->currentIndex()); });
    add(file, QStringLiteral("현재 탭 닫기"), QKeySequence(QStringLiteral("Ctrl+W")), [this] { closeDocument(m_documentTabs->currentIndex()); });
    add(file, QStringLiteral("다음 문서 탭"), QKeySequence(QStringLiteral("Ctrl+Tab")), [this] {
        if (m_documentTabs->count()) m_documentTabs->setCurrentIndex((m_documentTabs->currentIndex() + 1) % m_documentTabs->count());
    });
    add(file, QStringLiteral("다른 이름으로 저장"), QKeySequence(QStringLiteral("Ctrl+S")), [this] { saveImage(); });
    add(file, QStringLiteral("여러 파일 저장"), {}, [this] { showToolMessage(QStringLiteral("여러 파일 저장"), QStringLiteral("현재 문서의 여러 객체 저장은 그림 내보내기 대화상자에서 순차적으로 지원할 예정입니다.")); });
    file->addSeparator();
    add(file, QStringLiteral("종료"), {}, [this] { close(); });

    QMenu *edit = menuBar()->addMenu(QStringLiteral("편집"));
    add(edit, QStringLiteral("실행 취소"), QKeySequence(QStringLiteral("Ctrl+Z")), [this] { undoDrawingEdit(); });
    add(edit, QStringLiteral("다시 실행"), QKeySequence(QStringLiteral("Ctrl+Y")), [this] { redoDrawingEdit(); });
    add(edit, QStringLiteral("다시 실행 (Ctrl+Shift+Z)"), QKeySequence(QStringLiteral("Ctrl+Shift+Z")), [this] { redoDrawingEdit(); });
    createBuildMenu();
    edit->addSeparator();
    add(edit, QStringLiteral("새 구조 그리기"), {}, [this] { newDrawingDocument(); });
    add(edit, QStringLiteral("원자 추가"), {}, [this] { if (!m_drawingMode) newDrawingDocument(m_view->molecule()); else setStatus(QStringLiteral("빈 곳을 클릭하여 원자를 추가하세요.")); });
    add(edit, QStringLiteral("조각(fragment) 추가"), {}, [this] { openFragmentBrowser(); });
    add(edit, QStringLiteral("금속–리간드–산소 조합..."), {}, [this] { buildLabComplex(); });
    add(edit, QStringLiteral("결합 추가/삭제"), QKeySequence(QStringLiteral("Ctrl+B")), [this] { toggleSelectedBond(); });
    edit->addSeparator();
    add(edit, QStringLiteral("전체 선택"), QKeySequence::SelectAll, [this] {
        if (isTextInputFocused() || !m_view) return;
        QSet<int> visible;
        const auto &molecule=m_view->molecule();
        for (int i=0;i<molecule.atoms.size();++i)
            if (molecule.atoms[i].visible && (m_view->style().showHydrogen || molecule.atoms[i].atomicNumber!=1))
                visible.insert(i);
        applyAtomSelection(visible);
    })->setObjectName(QStringLiteral("SelectAllVisibleAtoms"));
    auto *flexible = edit->addMenu(QStringLiteral("유연한 선택"));
    for (const auto &entry : {qMakePair(QStringLiteral("수소만 선택"), 1), qMakePair(QStringLiteral("수소 이외 원자 선택"), -1)})
        add(flexible, entry.first, {}, [this, entry] { QSet<int> ids; const auto &atoms=m_view->molecule().atoms; for(int i=0;i<atoms.size();++i) if(entry.second==1 ? atoms[i].atomicNumber==1 : atoms[i].atomicNumber!=1) ids.insert(i); applyAtomSelection(ids); });
    add(flexible, QStringLiteral("원소로 선택..."), {}, [this] {
        QStringList elements;for(const auto &a:m_view->molecule().atoms)if(!elements.contains(a.element))elements<<a.element;
        if(elements.isEmpty())return;bool ok=false;auto element=QInputDialog::getItem(this,QStringLiteral("원소로 선택"),QStringLiteral("원소"),elements,0,false,&ok);if(!ok)return;
        QSet<int> ids;for(int i=0;i<m_view->molecule().atoms.size();++i)if(m_view->molecule().atoms[i].element==element)ids.insert(i);applyAtomSelection(ids);
    });
    add(edit, QStringLiteral("선택 해제"), {}, [this] { applyAtomSelection({}); });
    add(edit, QStringLiteral("선택 확장"), {}, [this] { applyAtomSelection(StructureTools::expand(m_view->molecule(),selectedStructureAtoms())); });
    add(edit, QStringLiteral("선택 반전"), {}, [this] { applyAtomSelection(StructureTools::invert(m_view->molecule(),selectedStructureAtoms())); });
    add(edit, QStringLiteral("선택 원자 일괄 작업"), {}, [this] { changeSelectionElement(); });
    add(edit, QStringLiteral("기본 편집기 옵션"), {}, [this] { openStyleEditor(); });
    add(edit, QStringLiteral("선택 조각을 분자 조각 데이터베이스에 추가"), {}, [this] { setStatus(QStringLiteral("선택 조각을 분자 조각 데이터베이스에 추가")); });
    add(edit, QStringLiteral("선택 조각 복사"), QKeySequence(QStringLiteral("Ctrl+C")), [this] { copyStructureFragment(); })->setObjectName("CopyStructureFragment");
    add(edit, QStringLiteral("분자 조각 붙여넣기"), QKeySequence(QStringLiteral("Ctrl+V")), [this] { pasteStructureFragment(); })->setObjectName("PasteStructureFragment");
    add(edit, QStringLiteral("선택 원자 중심에 더미 원자 추가"), {}, [this] { setStatus(QStringLiteral("더미 원자 추가")); });
    add(edit, QStringLiteral("선택 분자 조각 중첩"), {}, [this] { setStatus(QStringLiteral("선택 분자 조각 중첩")); });
    add(edit, QStringLiteral("두 그룹 결합"), {}, [this] { setStatus(QStringLiteral("두 그룹 결합")); });
    add(edit, QStringLiteral("선택 원자에 결합된 원자 전체 선택"), {}, [this] { applyAtomSelection(StructureTools::expand(m_view->molecule(),selectedStructureAtoms(),true)); });
    add(edit, QStringLiteral("선택 삭제"), QKeySequence(QStringLiteral("Delete")), [this] { if(!isTextInputFocused())deleteSelection(); });
    edit->addSeparator();
    add(edit, QStringLiteral("벡터/평면 관련 작업"), {}, [this] { setStatus(QStringLiteral("벡터/평면 관련 작업")); });
    add(edit, QStringLiteral("원자 좌표 평행이동"), {}, [this] { manipulateSelection(); });
    add(edit, QStringLiteral("원자 좌표에 작은 무작위 변위 추가"), {}, [this] { setStatus(QStringLiteral("무작위 변위 추가")); });
    add(edit, QStringLiteral("원자 순서 빠른 재배열"), {}, [this] { setStatus(QStringLiteral("원자 순서 빠른 재배열")); });
    add(edit, QStringLiteral("결합 재계산"), {}, [this] { recountBonds(false); });
    add(edit, QStringLiteral("전체 초기화 후 결합 재계산"), {}, [this] { recountBonds(true); });
    add(edit, QStringLiteral("모든 결합 지우기"), {}, [this] { Molecule edited = m_view->molecule(); edited.bonds.clear(); commitEditedMolecule(edited, true); });
    add(edit, QStringLiteral("구조 정리"), {}, [this] { m_view->bestView(); setStatus(QStringLiteral("구조 보기를 정리했습니다")); });
    add(edit, QStringLiteral("점군 설정"), {}, [this] { setStatus(QStringLiteral("점군 설정")); });
    add(edit, QStringLiteral("주축 설정"), {}, [this] { setStatus(QStringLiteral("주축 설정")); });
    edit->addSeparator();
    add(edit, QStringLiteral("원자 드래그"), QKeySequence(QStringLiteral("Ctrl+D")), [this] { setAtomDragMode(!m_dragAtoms); });
    add(edit, QStringLiteral("드래그 방식"), {}, [this] { setStatus(QStringLiteral("드래그 방식: 화면 평면")); });
    add(edit, QStringLiteral("선택 축을 따라 회전"), {}, [this] { setStatus(QStringLiteral("선택 축을 따라 회전")); });
    add(edit, QStringLiteral("TS 애니메이션 복사"), QKeySequence(QStringLiteral("Ctrl+Shift+I")), [this] { copyAnimation(); });
    add(edit, QStringLiteral("이미지 복사"), QKeySequence(QStringLiteral("Ctrl+I")), [this] { copyImage(); });
    add(edit, QStringLiteral("맞춤 캡처 (IRC · TS · IRC)"), QKeySequence(QStringLiteral("Ctrl+Alt+I")), [this] { openReactionCapture(); });
    add(edit, QStringLiteral("맞춤 캡처 — 열린 창·탭 다중 캡처"), QKeySequence(), [this] { openMultiCapture(); });
    add(edit, QStringLiteral("안티앨리어싱 없이 현재 이미지 복사"), {}, [this] { copyImage(); });

    QMenu *view = menuBar()->addMenu(QStringLiteral("보기"));
    m_syncViewsAction = view->addAction(QStringLiteral("뷰 동기화"));
    m_syncViewsAction->setObjectName(QStringLiteral("SyncMatchingAtomViews"));
    m_syncViewsAction->setCheckable(true);
    m_syncViewsAction->setToolTip(QStringLiteral("원자 수가 같은 탭의 회전·확대·이동·원근 설정 동기화"));
    connect(m_syncViewsAction, &QAction::toggled, this, [this](bool enabled) {
        m_sharedCameras.clear();
        if (enabled) synchronizeViews(m_view);
        setStatus(enabled ? QStringLiteral("뷰 동기화 켜짐 · 원자 수가 같은 탭에 적용")
                          : QStringLiteral("뷰 동기화 꺼짐"));
    });
    view->addAction(m_explorerDock->toggleViewAction());
    m_explorerDock->toggleViewAction()->setText(QStringLiteral("분석 패널 표시"));
    add(view, QStringLiteral("기본 패널 배치 복원"), {}, [this] {
        addDockWidget(Qt::LeftDockWidgetArea, m_explorerDock);
        m_explorerDock->show();
        m_explorerDock->raise();
        resizeDocks({m_explorerDock}, {320}, Qt::Horizontal);
        setStatus(QStringLiteral("기본 패널 배치를 복원했습니다"));
    });
    view->addSeparator();
    auto *projectionGroup = new QActionGroup(this);
    projectionGroup->setExclusive(true);
    auto *orthographic = add(view, QStringLiteral("직교 보기"), {}, [this] { setPerspective(false); });
    auto *isometric = add(view, QStringLiteral("등각 보기"), {}, [this] { setPerspective(false); m_view->bestView(); });
    auto *perspective = add(view, QStringLiteral("원근 보기"), {}, [this] { setPerspective(true); });
    for (QAction *action : {orthographic, isometric, perspective}) { action->setCheckable(true); projectionGroup->addAction(action); }
    perspective->setChecked(m_style.perspective);
    view->addSeparator();
    add(view, QStringLiteral("회전 중심 갱신"), QKeySequence(QStringLiteral("Num5")), [this] { m_view->updateRotationCenter(); });
    add(view, QStringLiteral("최적 보기"), QKeySequence(QStringLiteral("Enter")), [this] { m_view->bestView(); });
    add(view, QStringLiteral("중앙 정렬"), QKeySequence(QStringLiteral("Space")), [this] { m_view->centerOnMolecule(); });
    add(view, QStringLiteral("카메라 위치 직접 지정"), {}, [this] { showToolMessage(QStringLiteral("카메라 위치 직접 지정"), QStringLiteral("마우스 드래그와 휠로 카메라를 조정할 수 있습니다.")); });
    add(view, QStringLiteral("카메라 위치 클립보드 저장"), {}, [this] { m_view->saveCameraToClipboard(); setStatus(QStringLiteral("카메라 위치를 클립보드에 저장했습니다")); });
    add(view, QStringLiteral("카메라 위치 클립보드 복원"), {}, [this] { m_view->restoreCameraFromClipboard(); });
    add(view, QStringLiteral("고해상도 그림 출력"), QKeySequence(QStringLiteral("Ctrl+Q")), [this] { saveImage(); });
    auto *axes = add(view, QStringLiteral("축 표시"), {}, [this] { m_style.showAxes = !m_style.showAxes; applyStyle(m_style); }); axes->setCheckable(true); axes->setChecked(m_style.showAxes);
    auto *centerAxes = add(view, QStringLiteral("분자 중심으로 축 이동"), {}, [this] { m_style.centerAxes = !m_style.centerAxes; applyStyle(m_style); }); centerAxes->setCheckable(true); centerAxes->setChecked(m_style.centerAxes);
    auto *showHydrogen = view->addAction(QStringLiteral("수소 표시"));
    showHydrogen->setCheckable(true);
    showHydrogen->setChecked(m_style.showHydrogen);
    connect(showHydrogen, &QAction::triggered, this, [this](bool checked) {
        m_style.showHydrogen = checked;
        applyStyle(m_style);
    });
    QMenu *labels = view->addMenu(QStringLiteral("원자/결합 라벨"));
    m_atomLabelsAction = labels->addAction(QStringLiteral("원자 번호"));
    m_atomLabelsAction->setObjectName(QStringLiteral("ShowAtomNumbers"));
    m_atomLabelsAction->setIconText(QStringLiteral("Atomic number"));
    m_atomLabelsAction->setToolTip(QStringLiteral("모든 표시 원자의 구 중심에 번호 표시 · 다시 누르면 숨기기"));
    m_atomLabelsAction->setCheckable(true);
    m_atomLabelsAction->setChecked(m_style.showLabels);
    connect(m_atomLabelsAction, &QAction::triggered, this, [this](bool checked) {
        m_style.showLabels = checked;
        applyStyle(m_style);
    });
    auto *propertyLabels = add(labels, QStringLiteral("전하/스핀 밀도 표시"), {}, [this] { m_style.showProperties = !m_style.showProperties; applyStyle(m_style); }); propertyLabels->setCheckable(true); propertyLabels->setChecked(m_style.showProperties);
    m_geometryAction=add(view, QStringLiteral("구조 파라미터(거리/각/이면각) 표시"), {}, [this] { toggleGeometryMode(); });m_geometryAction->setCheckable(true);
    auto *vdw = add(view, QStringLiteral("반데르발스 구 표시"), {}, [this] { m_style.showVdwSpheres = !m_style.showVdwSpheres; applyStyle(m_style); }); vdw->setCheckable(true); vdw->setChecked(m_style.showVdwSpheres);
    add(view, QStringLiteral("현재 caption/object를 복제하여 다중 객체 그림 제작"), {}, [this] { setStatus(QStringLiteral("다중 객체 그림 제작")); });
    add(view, QStringLiteral("벡터/등가면을 고정/복제하여 다중 객체 그림 제작"), {}, [this] { setStatus(QStringLiteral("벡터/등가면 객체 복제")); });
    QMenu *hideAtoms = view->addMenu(QStringLiteral("특정 원자 숨기기"));
    add(hideAtoms, QStringLiteral("모든 수소 숨기기"), {}, [this, showHydrogen] {
        m_style.showHydrogen = false;
        applyStyle(m_style);
        showHydrogen->setChecked(false);
    });
    add(hideAtoms, QStringLiteral("탄소에 결합된 수소 숨기기"), {}, [this] {
        setCarbonHydrogensVisible(false);
    });
    add(hideAtoms, QStringLiteral("더미 원자 숨기기"), {}, [this] {
        for (Atom &atom : m_data.molecule.atoms)
            if (atom.atomicNumber <= 0) atom.visible = false;
        updateAtomVisibility();
    });
    add(hideAtoms, QStringLiteral("선택 원자 숨기기"), {}, [this] {
        auto selected = selectedStructureAtoms();
        const int picked = m_view->selectedAtom();
        if (selected.isEmpty() && picked >= 0 && picked < m_data.molecule.atoms.size()) selected.insert(picked);
        if (selected.isEmpty()) {
            setStatus(QStringLiteral("선택된 원자가 없습니다"));
            return;
        }
        for (int index : selected) m_data.molecule.atoms[index].visible = false;
        updateAtomVisibility();
    });
    hideAtoms->addSeparator();
    add(hideAtoms, QStringLiteral("숨긴 원자 모두 복원"), {}, [this, showHydrogen] {
        for (Atom &atom : m_data.molecule.atoms) atom.visible = true;
        m_style.showHydrogen = true;
        updateAtomVisibility();
        applyStyle(m_style);
        showHydrogen->setChecked(true);
    });
    add(view, QStringLiteral("도구 모음 표시 설정"), {}, [this] {
        setStatus(QStringLiteral("도구 모음 표시 설정은 현재 창 상태로 저장됩니다."));
    });

    QMenu *display = menuBar()->addMenu(QStringLiteral("표시 스타일"));
    display->setObjectName(QStringLiteral("DisplayPresets"));
    auto *presetGroup = new QActionGroup(display);
    for (const QString &name : StyleManager::presetNames()) {
        auto *action = add(display, name, {}, [this, name] { choosePreset(name); });
        action->setCheckable(true);
        presetGroup->addAction(action);
    }
    connect(display, &QMenu::aboutToShow, this, [this, display] {
        for (QAction *action : display->actions()) action->setChecked(action->text() == m_style.name);
    });

    QMenu *tools = menuBar()->addMenu(QStringLiteral("도구"));
    add(tools, QStringLiteral("빠른 양자화학 계산 (xTB)..."), {}, [this] { startXtbCalculation(); });
    add(tools, QStringLiteral("구조 최적화..."), QKeySequence(QStringLiteral("Ctrl+Alt+O")), [this] { startMMOptimization(); });
    add(tools, QStringLiteral("사용자 지정 기하학 파라미터(거리/각도/이면각) 적용"), {}, [this] { if(!m_geometryMode)toggleGeometryMode();else setStatus(QStringLiteral("구조 파라미터 편집: 원자를 순서대로 2–4개 선택하세요.")); });
    add(tools, QStringLiteral("데이터 탐색기"), {}, [this] { if (m_explorerDock) { m_explorerDock->show(); m_explorerDock->raise(); } });
    add(tools, QStringLiteral("Z-matrix 생성"), {}, [this] { showZMatrix(); })->setObjectName("BuildZMatrix");
    add(tools, QStringLiteral("GED 도구의 기본 골격"), {}, [this] { setStatus(QStringLiteral("GED 도구 기본 골격")); });
    add(tools, QStringLiteral("결정학 도구의 기본 골격"), {}, [this] { setStatus(QStringLiteral("결정학 도구 기본 골격")); });
    add(tools, QStringLiteral("애니메이션 생성"), {}, [this] { exportAnimation(); });
    add(tools, QStringLiteral("데이터 요약"), {}, [this] { showToolMessage(QStringLiteral("데이터 요약"), AnalysisEngine::summary(m_data)); });
    add(tools, QStringLiteral("계산 입력파일 구역 생성"), {}, [this] { createGaussianInput(); });
    add(tools, QStringLiteral("분자 조각 추출기"), {}, [this] { extractStructureFragments(); });
    add(tools, QStringLiteral("오비탈"), {}, [this] { m_explorerDock->show(); m_analysis->setCurrentTab(6); setStatus(QStringLiteral("오비탈 목록을 Context Sidebar에 표시합니다")); });
    tools->addSeparator();
    add(tools, QStringLiteral("구조 비교"), {}, [this] { setStatus(QStringLiteral("구조 비교")); });
    add(tools, QStringLiteral("구조 결합"), {}, [this] { setStatus(QStringLiteral("구조 결합")); });
    add(tools, QStringLiteral("분자량 계산"), {}, [this] { setStatus(QStringLiteral("분자량: %1 Da").arg(AnalysisEngine::molecularMass(m_data.molecule), 0, 'f', 4)); });
    add(tools, QStringLiteral("관성모멘트 계산"), {}, [this] { const QVector3D i = AnalysisEngine::principalMomentApproximation(m_data.molecule); setStatus(QStringLiteral("관성모멘트 근사: %1 / %2 / %3").arg(i.x(), 0, 'f', 3).arg(i.y(), 0, 'f', 3).arg(i.z(), 0, 'f', 3)); });
    add(tools, QStringLiteral("충돌 직경 계산"), {}, [this] { setStatus(QStringLiteral("충돌 직경 근사: %1 Å").arg(AnalysisEngine::collisionDiameter(m_data.molecule), 0, 'f', 3)); });
    add(tools, QStringLiteral("분자 면적 계산"), {}, [this] { setStatus(QStringLiteral("분자 면적 근사: %1 Å²").arg(AnalysisEngine::molecularAreaApproximation(m_data.molecule), 0, 'f', 3)); });
    add(tools, QStringLiteral("반데르발스 반지름 표"), {}, [this] {
        QString table;
        for (int i = 1; i <= 36; ++i) table += QStringLiteral("%1  %2 Å\n").arg(elementSymbol(i), -3).arg(vanDerWaalsRadius(i), 0, 'f', 2);
        showToolMessage(QStringLiteral("반데르발스 반지름 표"), table);
    });
    tools->addSeparator();
    add(tools, QStringLiteral("간단 유틸리티"), {}, [this] { setStatus(QStringLiteral("간단 유틸리티")); });
    add(tools, QStringLiteral("스크립트"), {}, [this] { setStatus(QStringLiteral("스크립트")); });
    add(tools, QStringLiteral("지원/도움말"), {}, [this] { showToolMessage(QStringLiteral("지원/도움말"), QStringLiteral("Youngcraft: Gaussian 결과를 빠르게 탐색하는 경량 분자 뷰어\n단축키: Ctrl+O 열기, Ctrl+I 이미지 복사, Enter 최적 보기, P 재생")); });
    add(tools, QStringLiteral("잘못 인식된 결합 수정 도구"), {}, [this] { setStatus(QStringLiteral("결합 cutoff는 표시 스타일/환경설정에서 조정할 수 있습니다")); });
    add(tools, QStringLiteral("파일 연결 설정"), {}, [this] { setStatus(QStringLiteral("Windows 파일 연결 설정은 휴대용 배포 정책상 직접 변경하지 않습니다")); });
    add(tools, QStringLiteral("사용자 그래프 유틸리티"), {}, [this] { setStatus(QStringLiteral("사용자 그래프 유틸리티")); });
    add(tools, QStringLiteral("작업 기록/보관"), {}, [this] { setStatus(QStringLiteral("작업 기록/보관")); });
    add(tools, QStringLiteral("환경설정"), {}, [this] { openStyleEditor(); });

    auto *orbitals = menuBar()->addMenu(QStringLiteral("Orbitals"));
    orbitals->addAction(QStringLiteral("FMO / MEP / NBO / Wiberg"), this, [this] {
        m_analysis->setCurrentTab(6); m_explorerDock->show(); m_explorerDock->raise();
    });
    orbitals->addAction(QStringLiteral("CHK 변환 / FCHK / cube 열기…"), this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Orbitals"), {},
            QStringLiteral("Checkpoint / cube (*.chk *.fchk *.fch *.cube *.cub);;Gaussian output (*.out *.log)"));
        if (!path.isEmpty()) openFile(path);
    });
    QMenu *help = menuBar()->addMenu(QStringLiteral("도움말"));
    add(help, QStringLiteral("Youngcraft 정보"), {}, [this] { showToolMessage(QStringLiteral("Youngcraft 정보"), QStringLiteral("Made by Youngseob Lee, Since 2026, only for JBNU and BIOCC")); });
}

} // namespace youngcraft
