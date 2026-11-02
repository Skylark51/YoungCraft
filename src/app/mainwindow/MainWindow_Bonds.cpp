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
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>

namespace youngcraft {

void MainWindow::toggleSelectedBond() {
    if (isTextInputFocused()) return;
    auto selected = m_view->selection().atoms;
    const auto measured = m_geometrySelection;
    // Normal consecutive clicks populate the measurement pair; Shift/Ctrl clicks
    // also support the explicit multi-selection used while drawing.
    if(selected.size()<2 && measured.size()==2)selected=QSet<int>(measured.begin(),measured.end());
    if (selected.size() != 2) { toggleBondEditMode(); return; }
    const auto ids = selected.values();
    Molecule edited = m_view->molecule();
    const int a = ids[0], b = ids[1];
    if (a < 0 || b < 0 || a >= edited.atoms.size() || b >= edited.atoms.size()) return;
    const bool remove = edited.bondIndex(a,b) >= 0;
    if (remove) {
        edited.removeBond(a,b);
        for(auto &site:edited.coordinationSites)
            if((site.center==a&&site.donor==b)||(site.center==b&&site.donor==a)){site.occupied=false;site.donor=-1;}
    }
    else {
        const bool coordination = isMetal(edited.atoms[a].atomicNumber) || isMetal(edited.atoms[b].atomicNumber);
        edited.setBond(a,b,1.0,coordination,coordination ? BondStyle::Dashed : BondStyle::Solid,true);
    }
    if(!remove)for(const auto &pair:QVector<QPair<int,int>>{{a,b},{b,a}})if(isMetal(edited.atoms[pair.first].atomicNumber)) {
        const auto direction=(edited.atoms[pair.second].position-edited.atoms[pair.first].position).normalized();
        int best=-1;float score=-2;
        for(int i=0;i<edited.coordinationSites.size();++i){const auto &site=edited.coordinationSites[i];
            if(site.center!=pair.first||site.occupied)continue;
            float dot=QVector3D::dotProduct(direction,site.direction);if(dot>score){score=dot;best=i;}}
        if(best>=0){edited.coordinationSites[best].occupied=true;edited.coordinationSites[best].donor=pair.second;}
    }
    // A connectivity toggle keeps atom identities and coordinates, including explicit H.
    commitEditedMolecule(edited,true,false);
    m_bondEditMode = false; m_firstBondAtom = -1;
    m_view->selection().atoms = selected;
    if(measured.size()==2 && selected.contains(measured[0]) && selected.contains(measured[1])) {
        handleGeometryPick(measured[0]);handleGeometryPick(measured[1]);
    }
    m_view->update();
    setStatus(remove ? QStringLiteral("선택 원자 사이의 실제 결합을 삭제했습니다 (Ctrl+B: 다시 연결).")
                     : QStringLiteral("선택 원자 사이에 실제 결합을 추가했습니다 (Ctrl+B: 삭제)."));
}

void MainWindow::toggleBondEditMode() {
    if (!m_bondEditMode) { ensureEditableDocument(); if(m_drawingMode)setDrawingMode(false); }
    if (!m_bondEditMode && m_geometryMode) toggleGeometryMode();
    m_bondEditMode = !m_bondEditMode;
    m_firstBondAtom = -1;
    setStatus(m_bondEditMode ? QStringLiteral("결합 편집 모드: 첫 번째 원자를 선택하세요")
                             : QStringLiteral("결합 편집 모드 종료"));
}

void MainWindow::handleBondPick(int index) {
    if (!m_bondEditMode || index < 0) return;
    if (m_firstBondAtom < 0) {
        m_firstBondAtom = index;
        setStatus(QStringLiteral("결합 편집: 두 번째 원자를 선택하세요"));
        return;
    }
    if (index == m_firstBondAtom) { setStatus(QStringLiteral("다른 원자를 선택하세요")); return; }
    QMenu menu(this);
    struct Choice { QString text; double order; bool coordination; BondStyle style; };
    const QVector<Choice> choices{{QStringLiteral("단일결합"), 1.0, false, BondStyle::Solid},
                                  {QStringLiteral("이중결합"), 2.0, false, BondStyle::Solid},
                                  {QStringLiteral("삼중결합"), 3.0, false, BondStyle::Solid},
                                  {QStringLiteral("1.5 결합"), 1.5, false, BondStyle::Dashed},
                                  {QStringLiteral("배위결합"), 1.0, true, BondStyle::Dashed},
                                  {QStringLiteral("점선 결합"), 1.0, false, BondStyle::Dashed}};
    for (const Choice &choice : choices) {
        QAction *action = menu.addAction(choice.text);
        connect(action, &QAction::triggered, this, [this, index, choice] {
            Molecule edited = m_view->molecule();
            edited.setBond(m_firstBondAtom, index, choice.order, choice.coordination, choice.style, true);
            commitEditedMolecule(edited, true); m_firstBondAtom = -1;
            setStatus(QStringLiteral("결합을 변경했습니다"));
        });
    }
    menu.addSeparator();
    QAction *remove = menu.addAction(QStringLiteral("결합 삭제"));
    connect(remove, &QAction::triggered, this, [this, index] {
        Molecule edited = m_view->molecule();
        edited.removeBond(m_firstBondAtom, index); commitEditedMolecule(edited, true);
        m_firstBondAtom = -1; setStatus(QStringLiteral("결합을 삭제했습니다"));
    });
    menu.exec(QCursor::pos());
    if (m_firstBondAtom >= 0) m_firstBondAtom = -1;
}

void MainWindow::recountBonds(bool clearManual) {
    if (m_data.molecule.isEmpty()) return;
    QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("결합 재계산 (Recount Bonds)"));
    QFormLayout form(&dialog);
    QDoubleSpinBox cutoff; cutoff.setRange(0.5, 3.0); cutoff.setSingleStep(0.05); cutoff.setValue(m_style.recountCutoffScale);
    QCheckBox hydrogen(QStringLiteral("수소 포함")); hydrogen.setChecked(m_style.recountHydrogen);
    QCheckBox metals(QStringLiteral("금속 배위결합 포함")); metals.setChecked(m_style.recountMetalCoordination);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    form.addRow(QStringLiteral("결합 인식 거리 배율"), &cutoff); form.addRow(&hydrogen); form.addRow(&metals); form.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    m_style.recountCutoffScale = float(cutoff.value()); m_style.recountHydrogen = hydrogen.isChecked();
    m_style.recountMetalCoordination = metals.isChecked(); StyleManager::save(m_style);
    Molecule edited = m_view->molecule();
    edited.rebuildBonds(cutoff.value(), hydrogen.isChecked(), metals.isChecked(), !clearManual);
    commitEditedMolecule(edited, true);
    setStatus(QStringLiteral("결합 %1개를 재계산했습니다").arg(m_data.molecule.bonds.size()));
}

} // namespace youngcraft
