#include "app/MainWindow.h"

#include "animation/AnimationController.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/GeometryMeasureBar.h"

#include <QAction>
#include <QStackedWidget>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <cmath>

namespace youngcraft {

void MainWindow::arrangeScanApproach() {
    const auto ids=m_geometrySelection;
    if(ids.size()!=2) { setStatus(QStringLiteral("접근시킬 반응 원자 두 개를 순서대로 클릭하세요."),8000); return; }
    QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("Scan 접근 배치"));
    QFormLayout form(&dialog);
    QLabel note(QStringLiteral("원자 %1 → %2\n각 조각의 내부 구조를 유지하며 반응 자리가 마주 보게 회전·이동합니다.\n축 주위 회전을 탐색하여 비결합 접촉을 줄입니다. 전체 구조 최적화는 별도로 실행하세요.")
        .arg(ids[0]+1).arg(ids[1]+1)); note.setWordWrap(true); form.addRow(&note);
    QDoubleSpinBox separation; separation.setRange(0.5,100); separation.setDecimals(3);
    separation.setSuffix(QStringLiteral(" Å")); separation.setValue(3.0);
    separation.setObjectName(QStringLiteral("ApproachDistance")); form.addRow(QStringLiteral("목표 반응 원자 거리"),&separation);
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&buttons);
    connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    Molecule edited=m_view->molecule();QString error;
    if(!GeometryEditor::arrangeApproach(edited,ids[0],ids[1],separation.value(),&error)){setStatus(error,10000);return;}
    ensureEditableDocument();m_animation->stop();
    syncEditedGeometry(edited, {});
    updateGeometryReadout();
    m_view->bestView();
    setStatus(error.isEmpty()?QStringLiteral("Scan 시작 배치를 적용했습니다. Ctrl+Z로 되돌릴 수 있습니다."):error,10000);
}

void MainWindow::toggleGeometryMode(){
    if(!m_geometryMode) ensureEditableDocument();
    m_geometryMode=!m_geometryMode;
    if(m_geometryAction)m_geometryAction->setChecked(m_geometryMode);
    if(m_geometryMode){if(m_bondEditMode){m_bondEditMode=false;m_firstBondAtom=-1;}m_animation->stop();m_workspace->setCurrentWidget(m_moleculeWorkspace);m_geometryBar->show();setStatus(QStringLiteral("구조 파라미터 편집: 원자를 순서대로 2–4개 선택하세요."),0);}else{clearGeometrySelection();m_geometryBar->hide();for(int i=0;i<3;++i)m_view->setMeasurementVisible(i,false);setStatus(QStringLiteral("구조 파라미터 편집 종료"));}
}

void MainWindow::handleGeometryPick(int index){
    if(index<0){clearGeometrySelection();return;}
    if(index>=m_view->molecule().atoms.size())return;
    if(m_geometrySelection.removeOne(index)){
        m_view->selection().atoms.remove(index);
        m_view->setSelectedAtom(-1);
        m_view->setGeometrySelection(m_geometrySelection);
        m_geometryBar->setSelection(m_view->molecule(),m_geometrySelection);
        updateGeometryReadout();
        setStatus(QStringLiteral("측정: 원자 %1 선택 취소 (%2개 선택)").arg(m_view->molecule().atoms[index].index).arg(m_geometrySelection.size()));
        return;
    }
    if(m_geometrySelection.size()>=4)m_geometrySelection.clear();
    m_geometrySelection.push_back(index);m_view->setGeometrySelection(m_geometrySelection);m_geometryBar->setSelection(m_view->molecule(),m_geometrySelection);
    updateGeometryReadout();
    setStatus(QStringLiteral("측정: %1번째 원자 선택 (%2 %3)").arg(m_geometrySelection.size()).arg(m_view->molecule().atoms[index].element).arg(m_view->molecule().atoms[index].index));
}

void MainWindow::clearGeometrySelection(){m_geometrySelection.clear();m_view->setGeometrySelection({});if(m_geometryBar)m_geometryBar->setSelection(m_view->molecule(),{});updateGeometryReadout();}

void MainWindow::updateGeometryReadout() {
    if (!m_measurementReadout) return;
    if (m_geometryBar) m_geometryBar->setVisible(m_geometryMode || m_geometrySelection.size() >= 2);
    const auto &molecule = m_view->molecule();
    for (int id : m_geometrySelection) {
        if (id < 0 || id >= molecule.atoms.size()) { clearGeometrySelection(); return; }
    }
    if (m_geometrySelection.isEmpty()) {
        m_measurementReadout->setText(QStringLiteral("원자를 순서대로 클릭하세요: 2개 거리 · 3개 각도 · 4개 이면각"));
        return;
    }
    QStringList atoms, values;
    for (int id : m_geometrySelection)
        atoms << QString::number(molecule.atoms[id].index > 0 ? molecule.atoms[id].index : id + 1);
    const auto &ids = m_geometrySelection;
    auto format = [](double value, int precision, const QString &unit) {
        return std::isfinite(value) ? QString::number(value, 'f', precision) + unit : QStringLiteral("정의 불가");
    };
    if (ids.size() >= 2) values << QStringLiteral("거리 %1").arg(format(GeometryEditor::distance(molecule, ids[0], ids[1]), 2, QStringLiteral(" Å")));
    if (ids.size() >= 3) values << QStringLiteral("각도 %1").arg(format(GeometryEditor::angleDegrees(molecule, ids[0], ids[1], ids[2]), 2, QStringLiteral("°")));
    if (ids.size() >= 4) values << QStringLiteral("이면각 %1").arg(format(GeometryEditor::dihedralDegrees(molecule, ids[0], ids[1], ids[2], ids[3]), 2, QStringLiteral("°")));
    m_measurementReadout->setText(QStringLiteral("선택 %1  |  %2").arg(atoms.join(QStringLiteral(" → ")),
        values.isEmpty() ? QStringLiteral("다음 원자를 클릭하세요") : values.join(QStringLiteral("  ·  "))));
    if (m_geometryBar) m_geometryBar->setSelection(molecule, ids);
}

void MainWindow::applyGeometryMeasurement(int kind,double value){
    const int required=kind+2;if(kind<0||kind>2||!std::isfinite(value)||m_geometrySelection.size()<required)return;m_animation->stop();m_view->setFrequencyMode(nullptr);Molecule edited=m_view->molecule();QString warning;bool ok=false;const GeometryMoveMode mode=m_geometryBar->moveMode();
    auto ids=m_geometrySelection;
    QSet<int> left,right;
    if(kind==0){left=MoleculeEditController::side(edited,ids[0],ids[0],ids[1]);right=MoleculeEditController::side(edited,ids[1],ids[0],ids[1]);}
    else if(kind==1){left=MoleculeEditController::side(edited,ids[0],ids[0],ids[1]);right=MoleculeEditController::side(edited,ids[2],ids[1],ids[2]);}
    else{left=MoleculeEditController::side(edited,ids[0],ids[1],ids[2]);right=MoleculeEditController::side(edited,ids[3],ids[1],ids[2]);}
    bool overlap=false;for(int i:left)if(right.contains(i)){overlap=true;break;}
    if(overlap){setStatus(QStringLiteral("고리 구조에서는 fragment를 안전하게 분리할 수 없습니다."));return;}
    if(mode==GeometryMoveMode::OneSide && m_geometryBar->moveFirstSide()){
        if(kind==0)std::swap(ids[0],ids[1]);else if(kind==1)std::swap(ids[0],ids[2]);else{std::swap(ids[0],ids[3]);std::swap(ids[1],ids[2]);}
    }
    if(kind==0)ok=GeometryEditor::setDistance(edited,ids[0],ids[1],value,mode,&warning);
    else if(kind==1)ok=GeometryEditor::setAngle(edited,ids[0],ids[1],ids[2],value,mode,&warning);
    else ok=GeometryEditor::setDihedral(edited,ids[0],ids[1],ids[2],ids[3],value,mode,&warning);
    if(!ok){setStatus(QStringLiteral("선택한 구조에서는 요청한 기하학 값을 적용할 수 없습니다."));m_geometryBar->setSelection(m_view->molecule(),m_geometrySelection);return;}
    // Validate the transform before creating an editable copy of a calculation.
    const auto selection = m_geometrySelection;
    const bool firstSide = m_geometryBar->moveFirstSide();
    const bool copied = !m_currentFile.isEmpty() || !m_data.frames.isEmpty();
    const auto camera = m_view->cameraState();
    ensureEditableDocument();
    if (copied) {
        setDrawingMode(false);
        m_view->setCameraState(camera);
        m_geometrySelection = selection;
        m_view->setGeometrySelection(selection);
        m_geometryBar->setMovement(mode, firstSide);
    }
    syncEditedGeometry(edited,warning);
}

void MainWindow::syncEditedGeometry(const Molecule&molecule,const QString&warning){
    commitEditedMolecule(molecule,false);
    QVector<QVector3D> coordinates;coordinates.reserve(molecule.atoms.size());for(const Atom&atom:molecule.atoms)coordinates.push_back(atom.position);m_view->setCoordinates(coordinates);
    if(m_data.molecule.atoms.size()==coordinates.size())for(int i=0;i<coordinates.size();++i)m_data.molecule.atoms[i].position=coordinates[i];

    if(m_analysis->coordinateViewActive()) m_analysis->setCoordinateMolecule(m_view->molecule());
    updateGeometryReadout();
    setStatus(warning.isEmpty()?QStringLiteral("구조 파라미터를 적용했습니다. Ctrl+Z로 되돌릴 수 있습니다."):warning,warning.isEmpty()?4000:8000);
}

} // namespace youngcraft
