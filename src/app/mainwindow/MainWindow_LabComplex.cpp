#include "app/MainWindow.h"
#include "model/LabComplexBuilder.h"
#include "ui/XtbDialog.h"
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
namespace youngcraft {
void MainWindow::buildLabComplex() {
    if(auto *calculation=findChild<XtbDialog *>();calculation&&calculation->calculationActive()) {
        setStatus(QStringLiteral("진행 중인 계산을 완료하거나 중단한 뒤 조합하세요."));return;
    }
    QString errors;
    const auto fragments=FragmentLibrary::load(&errors);
    QDialog dialog(this);dialog.setWindowTitle(QStringLiteral("Complex 템플릿 적용"));
    dialog.setObjectName("LabComplexDialog");dialog.setMinimumWidth(540);
    QFormLayout form(&dialog);
    QLabel note(QStringLiteral("금속 + 저장된 ligand + 산소 fragment로 편집 가능한 시작 구조를 만듭니다.\n"
                              "End-on (η¹) / Side-on (η²)은 금속–산소 연결 방식입니다.\n"
                              "MeCN/H₂O를 고르면 빈 배위 방향에 실제 배위결합으로 연결합니다.\n"
                              "산소 fragment의 전하·결합 차수는 시작 모델이며 전자 상태를 확정하지 않습니다."));
    note.setWordWrap(true);form.addRow(&note);
    QComboBox metal,ligand,adduct,coligand;
    coligand.setObjectName("LabColigand");coligand.addItem(QStringLiteral("없음"),-1);
    metal.setObjectName("LabMetal");ligand.setObjectName("LabLigand");adduct.setObjectName("LabAdduct");
    for(int z:{26,25,27,28,29,24,23,22,30,44,45,46})metal.addItem(elementSymbol(z),z);
    for(int i=0;i<fragments.size();++i) {
        const auto &f=fragments[i];
        if(f.category=="lab-ligand")ligand.addItem(f.name,i);
        if(f.category=="lab-adduct")adduct.addItem(f.name,i);
        if(f.category=="lab-coligand")coligand.addItem(f.name,i);
    }
    if(ligand.count()==0||adduct.count()==0) {
        QMessageBox::warning(this,QStringLiteral("실험실 fragment"),
            QStringLiteral("lab_fragments에 ligand와 산소 fragment 파일이 필요합니다.\n")+errors);return;
    }
    QSpinBox oxidation,charge,spin;oxidation.setRange(1,4);oxidation.setValue(3);
    charge.setRange(-20,20);spin.setRange(1,15);
    charge.setObjectName("LabCharge");spin.setObjectName("LabMultiplicity");
    auto updateCharge=[&]{charge.setValue(oxidation.value()+fragments[ligand.currentData().toInt()].totalCharge+
                                       fragments[adduct.currentData().toInt()].totalCharge+
                                       (coligand.currentData().toInt()<0?0:fragments[coligand.currentData().toInt()].totalCharge));};
    connect(&oxidation,&QSpinBox::valueChanged,&dialog,updateCharge);
    connect(&ligand,&QComboBox::currentIndexChanged,&dialog,updateCharge);
    connect(&adduct,&QComboBox::currentIndexChanged,&dialog,updateCharge);
    connect(&coligand,&QComboBox::currentIndexChanged,&dialog,updateCharge);
    connect(&metal,&QComboBox::currentIndexChanged,&dialog,[&]{spin.setValue(metal.currentData().toInt()==26?6:5);});
    spin.setValue(6);updateCharge();
    form.addRow(QStringLiteral("금속"),&metal);form.addRow(QStringLiteral("Ligand"),&ligand);
    form.addRow(QStringLiteral("산소 fragment"),&adduct);form.addRow(QStringLiteral("금속 산화수 (형식 전하)"),&oxidation);
    form.addRow(QStringLiteral("보조 ligand (빈 배위 방향)"),&coligand);
    form.addRow(QStringLiteral("착물 총전하"),&charge);
    form.addRow(QStringLiteral("후속 양자계산용 multiplicity"),&spin);
    QLabel spinNote(QStringLiteral("Multiplicity는 초기 메타데이터입니다. GFN-FF MM은 스핀을 계산하지 않습니다."));
    spinNote.setWordWrap(true);form.addRow(&spinNote);
    QLabel referenceNote;referenceNote.setObjectName("LabOptimizedReference");referenceNote.setWordWrap(true);form.addRow(&referenceNote);
    auto referenceStatus=[&]{
        const auto *reference=LabComplexBuilder::referenceFor(fragments[ligand.currentData().toInt()],fragments[adduct.currentData().toInt()],
            metal.currentData().toInt(),oxidation.value(),charge.value(),spin.value(),coligand.currentData().toInt()<0?nullptr:&fragments[coligand.currentData().toInt()]);
        referenceNote.setText(reference?QStringLiteral("서버 최적화 좌표 사용: %1\n금속·전하·multiplicity·배위 조합이 일치하는 참조 구조입니다.").arg(reference->source):QStringLiteral("현재 조합은 저장된 ligand / fragment로 시작 구조를 조합합니다."));
    };
    for(auto *box:{&metal,&ligand,&adduct,&coligand})connect(box,&QComboBox::currentIndexChanged,&dialog,referenceStatus);
    for(auto *box:{&oxidation,&charge,&spin})connect(box,&QSpinBox::valueChanged,&dialog,referenceStatus);
    referenceStatus();
    QCheckBox optimize(QStringLiteral("조합 후 GFN-FF MM 실행 (배위/O–O 거리 restraint)"));
    optimize.setChecked(false);form.addRow(&optimize);
    QDialogButtonBox buttons(QDialogButtonBox::Cancel);
    buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    buttons.addButton(QStringLiteral("구조 만들기"),QDialogButtonBox::AcceptRole);
    form.addRow(&buttons);
    connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    Molecule assembled;QString error;
    if(!LabComplexBuilder::assemble(fragments[ligand.currentData().toInt()],fragments[adduct.currentData().toInt()],
                                   metal.currentData().toInt(),oxidation.value(),charge.value(),spin.value(),assembled,&error,
                                   coligand.currentData().toInt()<0?nullptr:&fragments[coligand.currentData().toInt()])) {
        QMessageBox::warning(this,QStringLiteral("착물 조합"),error);return;
    }
    newDrawingDocument(assembled);
    if(optimize.isChecked())startXtbCalculation(0,true);
}
}
