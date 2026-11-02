from pathlib import Path
r=Path('.')
def edit(name,fn):
 p=r/name;s=p.read_text(encoding='utf-8');p.write_text(fn(s),encoding='utf-8')
edit('src/app/MainWindow.h',lambda s:s.replace('#include "model/Molecule.h"','#include "model/Molecule.h"\n#include "model/MoleculeEditController.h"\n#include "model/FragmentLibrary.h"').replace('void commitEditedMolecule(const Molecule &molecule, bool topologyChanged);','''void commitEditedMolecule(const Molecule &molecule, bool topologyChanged, bool adjustHydrogens = true);
    DrawState drawState() const;
    void setupEditorControls();
    void createBuildMenu();
    void ensureEditableDocument();
    void deleteSelection();
    void changeSelectionElement();
    void manipulateSelection();
    void openFragmentBrowser();
    void insertCoordinationDialog();
    bool consumeInsertion(int target, const QVector3D &position, const QVector3D &direction = {1,0,0});'''))
edit('src/app/mainwindow/MainWindow_MM.cpp',lambda s:s.replace('    if (enabled && m_geometryMode)', '    if (enabled) ensureEditableDocument();\n    if (enabled && m_geometryMode)',1).replace('void MainWindow::setAtomDragMode(bool enabled) {','void MainWindow::setAtomDragMode(bool enabled) {\n    if(enabled) ensureEditableDocument();').replace('void MainWindow::commitEditedMolecule(const Molecule &molecule, bool topologyChanged) {','void MainWindow::commitEditedMolecule(const Molecule &molecule, bool topologyChanged, bool adjustHydrogens) {').replace('''    if (topologyChanged) {
        if (m_currentFile.isEmpty()) {
            m_undoMolecules.push_back(m_data.molecule);
            if (m_undoMolecules.size() > 100) m_undoMolecules.removeFirst();
            m_redoMolecules.clear();
        }
        if (m_autoHButton && m_autoHButton->isChecked()) committed.reconcileAutoHydrogens();
    }''','''    ensureEditableDocument();
    if (topologyChanged && adjustHydrogens && m_autoHButton && m_autoHButton->isChecked()) committed.reconcileAutoHydrogens();
    QString error;
    if (!MoleculeEditController::validate(committed, &error)) { setStatus(error); m_view->setEditedMolecule(m_data.molecule); return; }
    m_undoMolecules.push_back(m_data.molecule);
    m_redoMolecules.clear();
    if(topologyChanged) { m_view->selection().clear(); m_view->setSelectedAtom(-1); clearGeometrySelection(); }''').replace('''    } else if (m_currentFrame >= 0 && m_currentFrame < m_data.frames.size() &&
               m_data.frames[m_currentFrame].coordinates.size() == committed.atoms.size()) {
        auto &coordinates = m_data.frames[m_currentFrame].coordinates;
        for (int i = 0; i < committed.atoms.size(); ++i) coordinates[i] = committed.atoms[i].position;
    }''','    }').replace('    atom.position = position;', '    atom.position = position;\n    atom.formalCharge = drawState().formalCharge;').replace('''        const double order = m_drawBondOrder->currentData().toDouble();
        edited.setBond(bondFrom, edited.atoms.size() - 1, order, false,
                       order == 1.5 ? BondStyle::Dashed : BondStyle::Solid, true);''','''        MoleculeEditController::drawBond(edited, bondFrom, edited.atoms.size()-1, drawState());'''))
# All geometry commits share the existing transaction stack; do not overwrite calculation frames.
edit('src/app/mainwindow/MainWindow_Geometry.cpp',lambda s:s.replace('    m_geometryMode=!m_geometryMode;', '    if(!m_geometryMode) ensureEditableDocument();\n    m_geometryMode=!m_geometryMode;').replace('    if(m_mmProcess)stopMMOptimization(QStringLiteral("구조 편집으로 MM 최적화를 중단했습니다."));', '    commitEditedMolecule(molecule,false);').replace('    if(m_currentFrame>=0&&m_currentFrame<m_data.frames.size()&&m_data.frames[m_currentFrame].coordinates.size()==coordinates.size())m_data.frames[m_currentFrame].coordinates=coordinates;',''))
edit('src/app/mainwindow/MainWindow_Bonds.cpp',lambda s:s.replace('    if (!m_bondEditMode && m_geometryMode)', '    if (!m_bondEditMode) { ensureEditableDocument(); if(m_drawingMode)setDrawingMode(false); }\n    if (!m_bondEditMode && m_geometryMode)'))
edit('src/app/mainwindow/MainWindow_Ui.cpp',lambda s:s.replace('    m_drawBondOrder->setFixedWidth(76);','    m_drawBondOrder->addItem(QStringLiteral("Automatic"), 0.0);\n    m_drawBondOrder->setCurrentIndex(4);\n    m_drawBondOrder->setFixedWidth(100);').replace('    drawLayout->addStretch();\n    auto *finishDrawing', '    setupEditorControls();\n    drawLayout->addStretch();\n    auto *finishDrawing',1).replace('''        if (m_drawingMode) addDrawnAtom(position);''','''        if (m_drawingMode && !consumeInsertion(-1,position)) addDrawnAtom(position);''').replace('''            const double order = m_drawBondOrder->currentData().toDouble();
            edited.setBond(first, second, order, false,
                           order == 1.5 ? BondStyle::Dashed : BondStyle::Solid, true);''','''            MoleculeEditController::drawBond(edited, first, second, drawState());'''))
edit('src/app/mainwindow/MainWindow_Menus.cpp',lambda s:s.replace('    edit->addSeparator();\n    add(edit, QStringLiteral("새 구조 그리기")', '''    add(edit, QStringLiteral("다시 실행 (Ctrl+Shift+Z)"), QKeySequence(QStringLiteral("Ctrl+Shift+Z")), [this] { redoDrawingEdit(); });
    createBuildMenu();
    edit->addSeparator();
    add(edit, QStringLiteral("새 구조 그리기")''',1).replace('showToolMessage(QStringLiteral("조각(fragment) 추가"), QStringLiteral("fragment DB 연결 지점입니다."));','openFragmentBrowser();').replace('setStatus(QStringLiteral("선택 삭제"));','if(!isTextInputFocused())deleteSelection();').replace('setStatus(QStringLiteral("원자 좌표 평행이동"));','manipulateSelection();').replace('setStatus(QStringLiteral("선택 원자 일괄 작업"));','changeSelectionElement();').replace('m_view->setFocusAtoms({}); setStatus(QStringLiteral("전체 원자를 선택 대상으로 지정했습니다"));','m_view->selection().clear(); for(int i=0;i<m_view->molecule().atoms.size();++i)m_view->selection().atoms.insert(i);m_view->update();').replace('m_view->setSelectedAtom(-1); setStatus(QStringLiteral("선택을 해제했습니다"));','m_view->selection().clear(); m_view->setSelectedAtom(-1); m_view->update();'))
# Embedded resources have no dependency on a development-machine path.
files=list((r/'resources/fragments').rglob('*.cjson'))
xml='<RCC><qresource prefix="/fragments">\n'+''.join('<file alias="'+f.relative_to(r/'resources/fragments').as_posix()+'">'+f.relative_to(r/'resources').as_posix()+'</file>\n' for f in files)+'</qresource></RCC>\n'
(r/'resources/fragments.qrc').write_text(xml,encoding='utf-8')
edit('CMakeLists.txt',lambda s:s.replace('    src/model/Molecule.cpp','    src/model/MoleculeEditController.cpp\n    src/model/FragmentLibrary.cpp\n    src/model/Molecule.cpp',1).replace('    src/app/mainwindow/MainWindow_MM.cpp','    src/app/mainwindow/MainWindow_Editor.cpp\n    resources/fragments.qrc\n    src/app/mainwindow/MainWindow_MM.cpp').replace('install(TARGETS Youngcraft RUNTIME DESTINATION .)','install(TARGETS Youngcraft RUNTIME DESTINATION .)\ninstall(DIRECTORY resources/fragments DESTINATION resources)\ninstall(FILES licenses/Avogadro-fragments-BSD-3-Clause.txt DESTINATION licenses)'))
