#include "app/MainWindow.h"
#include "model/LigandReplacement.h"
#include "render/MoleculeView.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace youngcraft {
void MainWindow::replaceLigandTemplate() {
    const Molecule source=m_view->molecule();
    QDialog dialog(this);dialog.setObjectName("LigandReplacementDialog");
    dialog.setWindowTitle(QStringLiteral("ligand 자동 인식/교체"));dialog.resize(1040,720);
    QVBoxLayout layout(&dialog);QFormLayout form;
    QLabel note(QStringLiteral("교체할 ligand를 확인하세요. 금속·기질·산소 fragment 등 나머지 원자와 결합은 그대로 보존합니다.\n"
        "왼쪽: 교체 대상(주황색) / 보존 원자(청록색). 오른쪽: 교체 결과. 결과는 새 편집 탭에 생성합니다."));
    note.setWordWrap(true);layout.addWidget(&note);
    QComboBox metal,candidate,templ;
    metal.setObjectName("ReplacementMetal");candidate.setObjectName("ReplacementCandidate");templ.setObjectName("ReplacementTemplate");
    for(int i=0;i<source.atoms.size();++i)if(isMetal(source.atoms[i].atomicNumber))metal.addItem(QStringLiteral("%1 · 원자 %2").arg(source.atoms[i].element).arg(i+1),i);
    if(!metal.count()){setStatus(QStringLiteral("현재 구조에 ligand를 인식할 금속 중심이 없습니다."),8000);return;}
    if(m_view->selectedAtom()>=0){int row=metal.findData(m_view->selectedAtom());if(row>=0)metal.setCurrentIndex(row);}
    QString loadErrors;const auto library=FragmentLibrary::load(&loadErrors);
    for(int pass=0;pass<2;++pass)for(int i=0;i<library.size();++i){
        const auto &f=library[i];
        if((pass==0&&f.category=="lab-ligand")||(pass==1&&f.ligand&&f.category!="lab-ligand"))templ.addItem(f.name,i);
    }
    if(!templ.count()){setStatus(QStringLiteral("사용할 ligand 템플릿이 없습니다. ")+loadErrors,10000);return;}
    QSet<int> selection=m_view->selection().atoms;
    for(int i:m_geometrySelection)selection.insert(i);
    if(m_view->selectedAtom()>=0)selection.insert(m_view->selectedAtom());
    QCheckBox protect(QStringLiteral("현재 선택한 원자를 반응 중심으로 보호"));protect.setObjectName("ProtectReactionCore");protect.setChecked(true);
    QStringList protectedLabels;for(int i=0;i<source.atoms.size();++i)if(selection.contains(i))protectedLabels<<QString::number(i+1);
    protect.setText(QStringLiteral("현재 선택 원자를 반응 중심으로 보호 (%1)").arg(protectedLabels.isEmpty()?QStringLiteral("선택 없음"):protectedLabels.join(", ")));
    protect.setToolTip(QStringLiteral("추가로 보호할 반응 원자를 캔버스에서 선택한 뒤 이 창을 여세요. 고정 원자는 항상 보호됩니다."));
    QSpinBox charge,spin;charge.setObjectName("ReplacementCharge");spin.setObjectName("ReplacementMultiplicity");
    charge.setRange(-1000,1000);charge.setValue(source.charge);spin.setRange(1,1000);spin.setValue(source.multiplicity);
    form.addRow(QStringLiteral("금속 중심"),&metal);form.addRow(QStringLiteral("인식된 ligand"),&candidate);
    form.addRow(QStringLiteral("교체 템플릿"),&templ);form.addRow(&protect);
    form.addRow(QStringLiteral("결과 총전하"),&charge);form.addRow(QStringLiteral("결과 multiplicity"),&spin);layout.addLayout(&form);
    QHBoxLayout views;MoleculeView before,after;before.setObjectName("ReplacementBefore");after.setObjectName("ReplacementAfter");
    for(auto *view:{&before,&after}){view->setStyle(m_style);view->setMinimumSize(360,300);views.addWidget(view,1);}layout.addLayout(&views,1);
    QLabel status;status.setWordWrap(true);status.setObjectName("ReplacementStatus");layout.addWidget(&status);
    QLabel chargeNote(QStringLiteral("총전하와 multiplicity는 현재 값을 유지합니다. ligand의 전하가 달라지면 위 값을 직접 수정하세요. 배치는 시작 구조이며, 보존 원자를 고정한 최적화로 확인할 수 있습니다."));
    chargeNote.setWordWrap(true);layout.addWidget(&chargeNote);
    QDialogButtonBox buttons(QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    auto *apply=buttons.addButton(QStringLiteral("교체 구조를 새 탭에 생성"),QDialogButtonBox::AcceptRole);apply->setObjectName("ApplyLigandReplacement");layout.addWidget(&buttons);
    QVector<LigandCandidate> candidates;Molecule result;bool valid=false;
    auto refresh=[&]{
        valid=false;apply->setEnabled(false);after.setMolecule({});
        if(candidate.currentIndex()<0||candidate.currentIndex()>=candidates.size()){
            before.setMolecule(source);
            status.setText(candidates.isEmpty()?QStringLiteral("분리 가능한 ligand를 찾지 못했습니다. 금속–주개 결합과 결합 인식을 확인하세요."):
                QStringLiteral("교체 후보를 직접 선택하세요. 템플릿으로 이름을 확인하지 못한 fragment과 보호된 fragment은 자동 선택하지 않습니다."));return;
        }
        const auto &c=candidates[candidate.currentIndex()];Molecule highlighted=source;
        QStringList ids;for(int i=0;i<source.atoms.size();++i){highlighted.atoms[i].colorOverride=c.atoms.contains(i)?QColor("#ee9b37"):QColor("#3bb6b2");if(c.atoms.contains(i))ids<<QString::number(i+1);}
        before.setMolecule(highlighted);QString error;
        if(templ.currentIndex()<0)return;
        valid=LigandReplacement::replace(source,c,library[templ.currentData().toInt()],protect.isChecked()?selection:QSet<int>{},result,&error);
        if(valid){result.charge=charge.value();result.multiplicity=spin.value();after.setMolecule(result);
            status.setText(QStringLiteral("%1 → %2 · 교체 %3개 / 보존 %4개\n교체 원자 번호: %5")
                .arg(c.name,templ.currentText()).arg(c.atoms.size()).arg(source.atoms.size()-c.atoms.size()).arg(ids.join(", ")));}
        else status.setText(error);apply->setEnabled(valid);
    };
    auto recognize=[&]{
        candidate.blockSignals(true);candidate.clear();candidates=LigandReplacement::recognize(source,metal.currentData().toInt(),library,protect.isChecked()?selection:QSet<int>{});
        for(const auto &c:candidates)candidate.addItem(QStringLiteral("%1 · %2개 원자 / 주개 %3개%4").arg(c.name).arg(c.atoms.size()).arg(c.donors.size()).arg(c.protectedCore?QStringLiteral(" · 보호됨"):QString()));
        int best=-1;for(int i=0;i<candidates.size();++i)if(candidates[i].knownTemplate&&!candidates[i].protectedCore&&(best<0||candidates[i].atoms.size()>candidates[best].atoms.size()))best=i;
        candidate.setCurrentIndex(best);candidate.blockSignals(false);refresh();
    };
    connect(&metal,&QComboBox::currentIndexChanged,&dialog,[&]{recognize();});
    connect(&protect,&QCheckBox::toggled,&dialog,[&]{recognize();});
    connect(&candidate,&QComboBox::currentIndexChanged,&dialog,[&]{refresh();});
    connect(&templ,&QComboBox::currentIndexChanged,&dialog,[&]{refresh();});
    connect(&charge,&QSpinBox::valueChanged,&dialog,[&]{refresh();});connect(&spin,&QSpinBox::valueChanged,&dialog,[&]{refresh();});
    connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    recognize();if(dialog.exec()!=QDialog::Accepted||!valid)return;
    newDrawingDocument(result);setStatus(QStringLiteral("ligand를 교체했습니다. 원래 구조는 기존 탭에 보존했습니다."),10000);
}
}
