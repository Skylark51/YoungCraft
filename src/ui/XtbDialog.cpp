#include "ui/XtbDialog.h"
#include "ui/SidebarButton.h"
#include "ui/SidebarFlowLayout.h"
#include "ui/ConstraintEditor.h"
#include "ui/PlotWidget.h"
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>
#include <QScrollArea>
#include <QSplitter>
#include <cmath>
#include <algorithm>
namespace youngcraft {
XtbDialog::XtbDialog(const Molecule &m,const QString &path,QWidget *parent,int initialMethod,bool autoRun,const QVector<int> &selection)
    :QDialog(parent),original(m),runtime(path),runner(this),automatic(autoRun) {
    setWindowTitle(QStringLiteral("xTB 빠른 양자화학 계산"));setObjectName("XtbDialog");setAttribute(Qt::WA_DeleteOnClose);resize(960,720);setMinimumSize(860,620);
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(12,12,12,12);layout->setSpacing(10);
    auto *notice=new QLabel(QStringLiteral("빠른 구조 relaxation / DFT 시작 구조 / single-point inspection용 반경험적 방법입니다.\n고정밀 DFT를 대체하지 않으며, open-shell spin-state 상대 에너지에는 제한이 있습니다."),this);notice->setWordWrap(true);layout->addWidget(notice);
    auto *columns=new QSplitter(Qt::Horizontal,this);columns->setObjectName("XtbColumns");columns->setChildrenCollapsible(false);
    layout->addWidget(columns,1);
    auto *settingsScroll=new QScrollArea(columns);settingsScroll->setObjectName("XtbSettingsScroll");settingsScroll->setWidgetResizable(true);settingsScroll->setFrameShape(QFrame::NoFrame);settingsScroll->setMinimumWidth(360);
    settings=new QWidget(settingsScroll);settings->setObjectName("XtbSettingsPanel");auto *form=new QFormLayout(settings);
    form->setContentsMargins(0,0,8,0);form->setRowWrapPolicy(QFormLayout::WrapLongRows);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    settingsScroll->setWidget(settings);settings->setAutoFillBackground(false);settingsScroll->viewport()->setAutoFillBackground(false);
    auto *resultsScroll=new QScrollArea(columns);resultsScroll->setObjectName("XtbResultsScroll");resultsScroll->setWidgetResizable(true);resultsScroll->setFrameShape(QFrame::NoFrame);resultsScroll->setMinimumWidth(380);
    auto *resultsPanel=new QWidget(resultsScroll);resultsPanel->setObjectName("XtbResultsPanel");
    auto *resultsLayout=new QVBoxLayout(resultsPanel);resultsLayout->setContentsMargins(8,0,0,0);resultsLayout->setSpacing(8);resultsScroll->setWidget(resultsPanel);resultsPanel->setAutoFillBackground(false);resultsScroll->viewport()->setAutoFillBackground(false);
    columns->setSizes({400,540});columns->setStretchFactor(0,4);columns->setStretchFactor(1,5);
    kind=new QComboBox(settings);kind->addItems({"Single Point","Geometry Optimization"});kind->setCurrentIndex(1);kind->setObjectName("XtbKind");
    method=new QComboBox(settings);method->setObjectName("XtbMethod");
    method->addItem("GFN2-xTB",2);method->addItem("GFN1-xTB",1);method->addItem("GFN-FF (MM)",0);
    charge=new QSpinBox(settings);charge->setRange(-100,100);charge->setValue(m.charge);charge->setObjectName("XtbCharge");
    multiplicity=new QSpinBox(settings);multiplicity->setRange(1,100);multiplicity->setValue(m.multiplicity);multiplicity->setObjectName("XtbMultiplicity");
    solvation=new QComboBox(settings);solvation->addItems({"Gas phase","ALPB"});
    solvent=new QComboBox(settings);solvent->addItems(XtbRunner::solvents());solvent->setEnabled(false);
    level=new QComboBox(settings);level->addItems({"loose","normal","tight"});level->setCurrentIndex(1);
    cycles=new QSpinBox(settings);cycles->setRange(10,5000);cycles->setValue(500);
    threads=new QComboBox(settings);for(int n:{1,2,4,8})threads->addItem(QString::number(n),n);threads->addItem("Auto (최대 4)",0);threads->setCurrentIndex(1);
    form->addRow(QStringLiteral("계산 종류"),kind);form->addRow("Method",method);form->addRow("Charge",charge);form->addRow("Multiplicity (UHF = M − 1)",multiplicity);form->addRow("Solvent model",solvation);form->addRow("ALPB solvent",solvent);form->addRow("Optimization level",level);form->addRow(QStringLiteral("최대 cycles"),cycles);form->addRow("CPU threads",threads);
    preserveFramework=new QCheckBox(QStringLiteral("배위 결합·peroxo O–O 거리 유지 (조화 restraint)"),settings);
    preserveFramework->setObjectName("XtbPreserveFramework");
    const bool hasMetal=std::any_of(m.atoms.cbegin(),m.atoms.cend(),[](const Atom &a){return isMetal(a.atomicNumber);});
    preserveFramework->setChecked(hasMetal);form->addRow(preserveFramework);
    preserveBonds=new QCheckBox(QStringLiteral("초기 결합 길이 유지 (조화 restraint · 구조 정리)"),settings);
    preserveBonds->setObjectName("XtbPreserveBonds");preserveBonds->setChecked(hasMetal);
    preserveBonds->setToolTip(QStringLiteral("현재 결합의 초기 거리를 유지하며 구조를 정리합니다. 사용자 거리 constraint가 우선합니다. 자유 최적화에서는 해제하세요."));
    form->addRow(preserveBonds);
    constraints=new ConstraintEditor(m,selection,resultsPanel,true);resultsLayout->addWidget(constraints);
    connect(solvation,&QComboBox::currentIndexChanged,this,[this](int i){solvent->setEnabled(i==1);});
    auto updateMethod=[this,notice]{const bool ff=method->currentData().toInt()==0;
        multiplicity->setEnabled(!ff);solvation->setEnabled(!ff);
        if(ff){solvation->setCurrentIndex(0);solvent->setEnabled(false);}
        notice->setText(ff?QStringLiteral("GFN-FF MM으로 초기 구조를 정리합니다. 스핀 상태와 반응성은 계산하지 않습니다.\n금속–ligand 결합과 O–O 결합은 계산 후 확인하고 DFT 시작 구조로 사용하세요."):
            QStringLiteral("빠른 구조 relaxation / DFT 시작 구조용 반경험적 방법입니다. 스핀 상태 상대 에너지에는 제한이 있습니다."));};
    connect(method,&QComboBox::currentIndexChanged,this,updateMethod);
    method->setCurrentIndex(method->findData(initialMethod));updateMethod();
    connect(kind,&QComboBox::currentIndexChanged,this,[this](int i){level->setEnabled(i==1);cycles->setEnabled(i==1);});
    status=new QLabel(QStringLiteral("계산 준비"),this);status->setObjectName("XtbStatus");status->setWordWrap(true);status->setTextInteractionFlags(Qt::TextSelectableByMouse);status->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);resultsLayout->addWidget(status);
    energyMode=new QComboBox(this);energyMode->addItems({"Absolute energy (Eh)","Relative energy (kcal/mol, 첫 frame 기준)"});energyMode->setObjectName("XtbEnergyMode");resultsLayout->addWidget(energyMode);
    plot=new PlotWidget(this);plot->setObjectName("XtbEnergyPlot");plot->setMinimumHeight(180);resultsLayout->addWidget(plot,1);
    connect(energyMode,&QComboBox::currentIndexChanged,this,[this]{refreshPlot();});
    auto *buttons=new QHBoxLayout;layout->addLayout(buttons);
    start=new QPushButton(QStringLiteral("계산 시작"),this);start->setObjectName("XtbStart");cancel=new QPushButton(QStringLiteral("중단"),this);cancel->setObjectName("XtbCancel");cancel->setEnabled(false);
    auto *outputButton=new QPushButton(QStringLiteral("출력 보기"),this);outputButton->setObjectName("XtbOutput");buttons->addWidget(start);buttons->addWidget(cancel);buttons->addWidget(outputButton);buttons->addSpacing(12);
    auto *results=buttons;
    apply=new QPushButton(QStringLiteral("현재 구조에 적용"),this);apply->setObjectName("XtbApply");open=new QPushButton(QStringLiteral("새 구조로 열기"),this);open->setObjectName("XtbOpen");save=new QPushButton(QStringLiteral("XYZ 저장"),this);save->setObjectName("XtbSave");
    for(auto *button:{apply,open,save}){results->addWidget(button);button->setEnabled(false);}
    connect(start,&QPushButton::clicked,this,&XtbDialog::startCalculation);
    connect(cancel,&QPushButton::clicked,&runner,&XtbRunner::cancel);
    connect(&runner,&XtbRunner::frameReady,this,[this](const XtbFrame &f){
        emit previewCoordinates(f.coordinates);if(std::isfinite(f.energy))series.push_back(QPointF(f.step,f.energy));refreshPlot();
        status->setText(QStringLiteral("진행 중 · step %1 · E %2 Eh · ∇ %3").arg(f.step).arg(f.energy,0,'f',9).arg(f.gradient,0,'g',6));
    });
    connect(&runner,&XtbRunner::metricsChanged,this,[this]{if(!kind->currentIndex())status->setText(QStringLiteral("Single Point · E %1 Eh · ∇ %2 · gap %3 eV").arg(runner.energy,0,'f',9).arg(runner.gradient).arg(runner.gap));});
    connect(&runner,&XtbRunner::completed,this,&XtbDialog::finishCalculation);
    connect(apply,&QPushButton::clicked,this,[this]{applied=true;original=runner.result;emit applyGeometry(runner.result);});
    connect(open,&QPushButton::clicked,this,[this]{emit openGeometry(runner.result);});
    connect(save,&QPushButton::clicked,this,[this]{QString path=QFileDialog::getSaveFileName(this,QStringLiteral("XYZ 저장"),"xtbopt.xyz","XYZ (*.xyz)");if(path.isEmpty())return;QFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(XtbRunner::xyz(runner.result).toUtf8())<0)QMessageBox::warning(this,"xTB",QStringLiteral("XYZ 저장 실패"));});
    connect(outputButton,&QPushButton::clicked,this,[this]{auto *d=new QDialog(this);d->setAttribute(Qt::WA_DeleteOnClose);d->setWindowTitle(QStringLiteral("xTB 출력 — ")+runner.directory);d->resize(800,600);auto *l=new QVBoxLayout(d);auto *text=new QPlainTextEdit(d);text->setReadOnly(true);text->setPlainText(runner.output);l->addWidget(text);d->show();});
    refreshPlot();
    if(automatic)QTimer::singleShot(0,this,&XtbDialog::startCalculation);
}
void XtbDialog::setSelectionProvider(std::function<QVector<int>()> provider) { constraints->setSelectionProvider(std::move(provider)); }
void XtbDialog::embedInSidebar() {
    setWindowFlags(Qt::Widget);setMinimumSize(0,0);setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    auto *columns=findChild<QSplitter *>("XtbColumns");columns->setOrientation(Qt::Vertical);
    if(auto *form=qobject_cast<QFormLayout *>(settings->layout())){
        form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        if(auto *label=qobject_cast<QLabel *>(form->labelForField(multiplicity)))label->setText("Multiplicity");
        if(auto *label=qobject_cast<QLabel *>(form->labelForField(solvation)))label->setText("Solvation");
        if(auto *label=qobject_cast<QLabel *>(form->labelForField(level)))label->setText("Opt level");
    }
    multiplicity->setToolTip("UHF = multiplicity − 1");
    preserveFramework->setToolTip(preserveFramework->text());preserveFramework->setText(QStringLiteral("배위 골격 거리 유지"));
    preserveBonds->setToolTip(preserveBonds->text());preserveBonds->setText(QStringLiteral("초기 결합 거리 유지"));
    for(auto *label:findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    for(auto *box:findChildren<QComboBox *>()){box->setMinimumWidth(0);box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);box->setMinimumContentsLength(6);}
    for(auto *scroll:columns->findChildren<QScrollArea *>(QString(),Qt::FindDirectChildrenOnly))scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    for(auto *scroll:columns->findChildren<QScrollArea *>(QString(),Qt::FindDirectChildrenOnly))scroll->setMinimumWidth(0);
    for(auto *button:findChildren<QPushButton *>()) {
        const auto name=button->objectName();
        if(name=="XtbStart")makeSidebarIconButton(button,QStyle::SP_MediaPlay);
        else if(name=="XtbCancel")makeSidebarIconButton(button,QStyle::SP_MediaStop);
        else if(name=="XtbApply")makeSidebarIconButton(button,QStyle::SP_DialogApplyButton);
        else if(name=="XtbOpen")makeSidebarIconButton(button,QStyle::SP_DirOpenIcon);
        else if(name=="XtbSave")makeSidebarIconButton(button,QStyle::SP_DialogSaveButton);
        else if(name=="XtbOutput")makeSidebarIconButton(button,QStyle::SP_FileDialogDetailedView);
    }
    auto *root=qobject_cast<QVBoxLayout *>(layout());
    auto *buttons=root->takeAt(root->count()-1);
    auto *flow=new SidebarFlowLayout;flow->setSpacing(4);
    if(buttons && buttons->layout()){
        while(auto *item=buttons->layout()->takeAt(0)){if(item->widget())flow->addWidget(item->widget());delete item;}
        delete buttons;
    }
    root->addLayout(flow);columns->setSizes({340,320});
}
void XtbDialog::setBondRetention(bool enabled) {
    preserveBonds->setChecked(enabled);preserveFramework->setChecked(enabled);
}
void XtbDialog::configureOptimization(const QVector<GeometryConstraint> &entries,int maximum) {
    constraints->setEntries(entries);cycles->setValue(qBound(10,maximum,5000));
    // Keep the user-visible framework/bond retention defaults across MM fallback.
}
void XtbDialog::startCalculation() {
    if(inputValidator && !inputValidator())return;
    QVector<GeometryConstraint> entries; QString error;
    if(!constraints->read(entries,&error)){QMessageBox::warning(this,"Constraint",error);return;}
    emit restorePreview();applied=false;series.clear();refreshPlot();
    XtbOptions o;o.optimize=kind->currentIndex()==1;o.method=method->currentData().toInt();o.charge=charge->value();o.multiplicity=multiplicity->value();o.cycles=cycles->value();o.level=level->currentText();o.threads=threads->currentData().toInt();if(solvation->currentIndex())o.solvent=solvent->currentText();
    o.constraints=entries;
    o.preserveCoordination=preserveFramework->isChecked();
    o.preserveBondLengths=preserveBonds->isChecked();
    settings->setEnabled(false);constraints->setEnabled(false);start->setEnabled(false);cancel->setEnabled(true);for(auto *b:{apply,open,save})b->setEnabled(false);
    status->setText(QStringLiteral("시작 중..."));runner.start(original,o,runtime);
}
void XtbDialog::refreshPlot() {
    auto values=series;if(energyMode->currentIndex()&&!values.isEmpty()){double ref=values.first().y();for(auto &p:values)p.setY((p.y()-ref)*627.509474);}
    plot->setSeries(values,"Optimization step",energyMode->currentIndex()?QStringLiteral("ΔE (kcal mol⁻¹)"):QStringLiteral("Energy (Eh)"));
}
void XtbDialog::finishCalculation() {
    settings->setEnabled(true);constraints->setEnabled(true);start->setEnabled(true);cancel->setEnabled(false);
    QString label;
    switch(runner.state){case XtbRunner::State::Finished:label=kind->currentIndex()?QStringLiteral("최적화 수렴"):QStringLiteral("Single Point 완료");break;case XtbRunner::State::NotConverged:label=QStringLiteral("최적화 미수렴 — 마지막 geometry 확인 가능");break;case XtbRunner::State::Cancelled:label=QStringLiteral("중단 — 마지막 geometry 확인 가능");break;default:label=runner.error;break;}
    auto value=[](double v){return std::isfinite(v)?QString::number(v,'g',12):QStringLiteral("제공되지 않음");};
    status->setText(QStringLiteral("%1\nStep %2 · Total Energy %3 Eh · Gradient norm %4 · HOMO-LUMO gap %5 eV\n%6").arg(label).arg(runner.cycle).arg(value(runner.energy),value(runner.gradient),value(runner.gap),runner.directory));
    if(kind->currentIndex() && (preserveBonds->isChecked() || preserveFramework->isChecked()))status->setText(status->text()+QStringLiteral("\n초기 결합 거리 restraint 적용"));
    bool usable=runner.state==XtbRunner::State::Finished||runner.state==XtbRunner::State::NotConverged||runner.state==XtbRunner::State::Cancelled;
    for(auto *b:{apply,open,save})b->setEnabled(usable);
    if(usable&&kind->currentIndex()){QVector<QVector3D> coords;for(const auto &a:runner.result.atoms)coords<<a.position;emit previewCoordinates(coords);}
    if(!runner.geometryWarning.isEmpty())status->setText(status->text()+"\n"+runner.geometryWarning);
    if(automatic&&!runner.geometryWarning.isEmpty()){
        emit restorePreview();
        status->setText(status->text()+QStringLiteral("\n원본 구조를 유지했습니다. 결과는 새 구조로 열어 확인할 수 있습니다."));
    }
    if(automatic&&runner.geometryWarning.isEmpty()&&runner.state==XtbRunner::State::Finished&&kind->currentIndex()){
        applied=true;original=runner.result;emit applyGeometry(runner.result);
        status->setText(status->text()+QStringLiteral("\n수렴한 구조를 자동으로 적용했습니다."));
    }
    if(closing)close();
}
void XtbDialog::closeEvent(QCloseEvent *event) {
    if(runner.state==XtbRunner::State::Starting||runner.state==XtbRunner::State::Running||runner.state==XtbRunner::State::Stopping){closing=true;runner.cancel();event->ignore();return;}
    if(!applied)emit restorePreview();QDialog::closeEvent(event);
}
}
