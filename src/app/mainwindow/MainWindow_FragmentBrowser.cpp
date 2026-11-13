#include "app/MainWindow.h"
#include "render/MoleculeView.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace youngcraft {
void MainWindow::openFragmentBrowser(bool labOnly) {
    QString errors;
    const auto library=FragmentLibrary::load(&errors);
    if(library.isEmpty()){setStatus(errors);return;}
    QDialog dialog(this);dialog.setObjectName("FragmentBrowser");
    dialog.setWindowTitle(labOnly?QStringLiteral("Ligands for BIOCC"):QStringLiteral("Fragment 삽입"));dialog.resize(900,660);
    QVBoxLayout outer(&dialog);
    QLineEdit search;search.setObjectName("FragmentSearch");
    search.setPlaceholderText(QStringLiteral("이름 검색: MTP, MDAP, peroxo, hydroperoxo, MeCN, H2O…"));outer.addWidget(&search);
    QComboBox category;category.setObjectName("FragmentCategory");
    category.addItem(QStringLiteral("All Templates"),QString());
    category.addItem(QStringLiteral("Ligands for BIOCC"),"lab-ligand");
    category.addItem(QStringLiteral("Lab Oxygen · 산소 배위 조각"),"lab-adduct");
    category.addItem(QStringLiteral("Lab Solvents · MeCN / H₂O 보조 리간드"),"lab-coligand");
    QStringList categories;
    for(const auto &f:library)if(f.category!="lab-ligand"&&f.category!="lab-adduct"&&f.category!="lab-coligand"&&!categories.contains(f.category))categories<<f.category;
    categories.sort();for(const auto &c:categories)category.addItem(c,c);
    if(labOnly)category.setCurrentIndex(1);outer.addWidget(&category);
    QHBoxLayout middle;QListWidget list;list.setObjectName("FragmentList");
    MoleculeView preview;preview.setStyle(m_style);preview.setMinimumSize(360,280);
    middle.addWidget(&list);middle.addWidget(&preview,1);outer.addLayout(&middle,1);
    QLabel description;description.setWordWrap(true);outer.addWidget(&description);
    QFormLayout form;
    QComboBox mode;mode.setObjectName("FragmentInsertionMode");
    mode.addItem(QStringLiteral("리간드 / 조각만 배치 또는 기존 금속에 연결"),0);
    mode.addItem(QStringLiteral("선택한 금속과 배위한 구조를 새 탭에 생성"),1);
    QComboBox metal;metal.setObjectName("FragmentMetal");
    for(int z=1;z<=118;++z)if(isMetal(z))metal.addItem(elementSymbol(z),z);
    metal.setCurrentIndex(metal.findData(26));
    QSpinBox oxidation,charge,multiplicity;
    oxidation.setObjectName("FragmentOxidation");oxidation.setRange(-4,4);oxidation.setValue(3);
    charge.setObjectName("FragmentTotalCharge");charge.setRange(-20,20);
    multiplicity.setObjectName("FragmentMultiplicity");multiplicity.setRange(1,15);multiplicity.setValue(1);
    form.addRow(QStringLiteral("삽입 방식"),&mode);form.addRow(QStringLiteral("배위 금속"),&metal);
    form.addRow(QStringLiteral("금속 형식 전하"),&oxidation);form.addRow(QStringLiteral("구조 총전하"),&charge);
    form.addRow(QStringLiteral("Multiplicity (계산 입력용)"),&multiplicity);outer.addLayout(&form);
    QLabel note(QStringLiteral("금속과 함께 생성하면 주개 원자의 배위 결합을 자동 연결합니다.\n"
        "저장된 리간드 모양과 기준 배위 거리를 사용하는 시작 구조입니다. 전하와 spin은 계산에 맞게 지정하세요."));
    note.setWordWrap(true);outer.addWidget(&note);
    QDialogButtonBox buttons(QDialogButtonBox::Cancel);
    buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    auto *insert=buttons.addButton(QStringLiteral("삽입 / 연결"),QDialogButtonBox::AcceptRole);
    outer.addWidget(&buttons);
    auto selected=[&]() -> const Fragment * {auto *item=list.currentItem();return item?&library[item->data(Qt::UserRole).toInt()]:nullptr;};
    auto suggestMultiplicity=[&]{
        if(const auto *f=selected()){
            int electrons=metal.currentData().toInt()-charge.value();for(const auto &atom:f->atoms)electrons+=atom.atomicNumber;
            if((electrons+multiplicity.value())%2==0)multiplicity.setValue(electrons%2?2:1);
        }
    };
    auto refresh=[&]{
        const auto *f=selected();insert->setEnabled(f!=nullptr);
        if(!f){preview.setMolecule({});description.clear();mode.setEnabled(false);return;}
        const bool ligand=f->ligand||f->category=="lab-ligand";
        if(!ligand)mode.setCurrentIndex(0);mode.setEnabled(ligand);
        const bool withMetal=ligand&&mode.currentData().toInt()==1;
        for(QWidget *widget:QVector<QWidget *>{&metal,&oxidation,&charge,&multiplicity})widget->setEnabled(withMetal);
        Molecule molecule;QString error;
        bool ok=withMetal?FragmentLibrary::metalComplex(*f,metal.currentData().toInt(),oxidation.value(),charge.value(),multiplicity.value(),molecule,&error)
                         :FragmentLibrary::insert(molecule,*f,-1,{},f->direction,&error);
        preview.setMolecule(molecule);
        const QString group=f->category=="lab-ligand"?"Lab Ligands":f->category=="lab-adduct"?"Lab Oxygen":f->category;
        description.setText(ok?f->name+" · "+group+QStringLiteral(" · 주개 %1개 · 리간드 전하 %2").arg(f->attachments.size()).arg(f->totalCharge):error);
        insert->setEnabled(ok);insert->setText(withMetal?QStringLiteral("금속–리간드 구조 생성"):QStringLiteral("삽입 / 연결"));
    };
    auto updateCharge=[&]{if(const auto *f=selected())charge.setValue(oxidation.value()+f->totalCharge);};
    auto filter=[&]{
        list.clear();
        for(int i=0;i<library.size();++i){const auto &f=library[i];
            if(!category.currentData().toString().isEmpty()&&f.category!=category.currentData().toString())continue;
            if(!f.name.contains(search.text(),Qt::CaseInsensitive)&&!f.category.contains(search.text(),Qt::CaseInsensitive))continue;
            auto *item=new QListWidgetItem(f.name,&list);item->setData(Qt::UserRole,i);item->setToolTip(f.resource);
        }
        if(list.count())list.setCurrentRow(0);else refresh();
    };
    connect(&list,&QListWidget::currentItemChanged,&dialog,[&]{updateCharge();suggestMultiplicity();refresh();});
    connect(&category,&QComboBox::currentIndexChanged,&dialog,[&]{filter();});
    connect(&search,&QLineEdit::textChanged,&dialog,[&]{filter();});
    connect(&mode,&QComboBox::currentIndexChanged,&dialog,[&]{refresh();});
    connect(&metal,&QComboBox::currentIndexChanged,&dialog,[&]{suggestMultiplicity();refresh();});
    connect(&oxidation,&QSpinBox::valueChanged,&dialog,[&]{updateCharge();refresh();});
    connect(&charge,&QSpinBox::valueChanged,&dialog,[&]{suggestMultiplicity();refresh();});
    connect(&multiplicity,&QSpinBox::valueChanged,&dialog,[&]{refresh();});
    connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    filter();if(labOnly)mode.setCurrentIndex(1);
    if(dialog.exec()!=QDialog::Accepted||!selected())return;
    const auto fragment=*selected();
    if(mode.currentData().toInt()==1){
        Molecule molecule;QString error;
        if(!FragmentLibrary::metalComplex(fragment,metal.currentData().toInt(),oxidation.value(),charge.value(),multiplicity.value(),molecule,&error)){setStatus(error,8000);return;}
        newDrawingDocument(molecule);setStatus(QStringLiteral("%1–%2 배위 구조를 생성했습니다.").arg(elementSymbol(metal.currentData().toInt()),fragment.name));
        return;
    }
    setDrawingMode(true);m_view->endDrawingChain();m_view->setProperty("pendingCenter",0);
    m_view->setProperty("pendingFragment",fragment.resource);m_view->setFragmentPreview(fragment);
    setStatus(QStringLiteral("빈 곳: 조각 배치 · 기존 금속/배위 자리: 연결 · Esc: 취소"),0);
}
}
