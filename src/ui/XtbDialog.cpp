#include "ui/XtbDialog.h"
#include "ui/ConstraintEditor.h"
#include "ui/PlotWidget.h"
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
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
#include <cmath>
#include <algorithm>
namespace youngcraft {
XtbDialog::XtbDialog(const Molecule &m,const QString &path,QWidget *parent,int initialMethod,bool autoRun,const QVector<int> &selection)
    :QDialog(parent),original(m),runtime(path),runner(this),automatic(autoRun) {
    setWindowTitle(QStringLiteral("xTB 빠른 양자화학 계산"));setObjectName("XtbDialog");setAttribute(Qt::WA_DeleteOnClose);resize(660,760);
    auto *layout=new QVBoxLayout(this);
    auto *notice=new QLabel(QStringLiteral("빠른 구조 relaxation / DFT 시작 구조 / single-point inspection용 반경험적 방법입니다.\n고정밀 DFT를 대체하지 않으며, open-shell spin-state 상대 에너지에는 제한이 있습니다."),this);notice->setWordWrap(true);layout->addWidget(notice);
    settings=new QWidget(this);auto *form=new QFormLayout(settings);layout->addWidget(settings);
    kind=new QComboBox(settings);kind->addItems({"Single Point","Geometry Optimization","Relaxed PES Scan (TS 후보)"});kind->setCurrentIndex(1);kind->setObjectName("XtbKind");
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
    scanSettings=new QWidget(settings);
    auto *scanForm=new QFormLayout(scanSettings);
    scanType=new QComboBox(scanSettings);scanType->setObjectName("XtbScanType");
    scanType->addItem(QStringLiteral("Distance (Å)"),"distance");
    scanType->addItem(QStringLiteral("Angle (°)"),"angle");
    scanType->addItem(QStringLiteral("Dihedral (°)"),"dihedral");
    const int initialType=selection.size()>=4?2:(selection.size()==3?1:0);
    scanType->setCurrentIndex(initialType);
    scanForm->addRow(QStringLiteral("Reaction coordinate"),scanType);
    auto *atomInputs=new QWidget(scanSettings);auto *atomLayout=new QHBoxLayout(atomInputs);
    atomLayout->setContentsMargins(0,0,0,0);
    for(int i=0;i<4;++i){
        auto *spin=new QSpinBox(atomInputs);spin->setRange(1,qMax(1,m.atoms.size()));
        spin->setPrefix(QString::number(i+1)+": ");
        spin->setObjectName(QString("XtbScanAtom%1").arg(i+1));
        const int atom=i<selection.size()?selection[i]:i;
        spin->setValue(qBound(1,atom+1,qMax(1,m.atoms.size())));
        scanAtoms[i]=spin;atomLayout->addWidget(spin);
    }
    scanForm->addRow(QStringLiteral("Atom numbers"),atomInputs);
    scanFrom=new QDoubleSpinBox(scanSettings);scanTo=new QDoubleSpinBox(scanSettings);
    for(auto *spin:{scanFrom,scanTo}){spin->setRange(-720,720);spin->setDecimals(4);spin->setSingleStep(0.1);}
    scanFrom->setObjectName("XtbScanFrom");scanTo->setObjectName("XtbScanTo");
    scanFrom->setValue(1.5);scanTo->setValue(2.0);
    scanForm->addRow(QStringLiteral("Start"),scanFrom);scanForm->addRow(QStringLiteral("End"),scanTo);
    scanPoints=new QSpinBox(scanSettings);scanPoints->setRange(3,501);scanPoints->setValue(11);
    scanPoints->setObjectName("XtbScanPoints");scanForm->addRow(QStringLiteral("Points"),scanPoints);
    auto updateAtoms=[this](int type){
        const int count=type+2;
        QVector<int> ids;QSet<int> unique;
        for(int i=0;i<4;++i) {
            scanAtoms[i]->setVisible(i<count);
            if(i<count){ids<<scanAtoms[i]->value()-1;unique.insert(ids.last());}
        }
        const QString unit=type==0?QStringLiteral(" Å"):QStringLiteral("°");
        scanFrom->setSuffix(unit);scanTo->setSuffix(unit);
        if(unique.size()==count){
            double value=type==0?GeometryEditor::distance(original,ids[0],ids[1]):
                type==1?GeometryEditor::angleDegrees(original,ids[0],ids[1],ids[2]):
                GeometryEditor::dihedralDegrees(original,ids[0],ids[1],ids[2],ids[3]);
            if(std::isfinite(value)) {
                scanFrom->setValue(value);
                scanTo->setValue(value+(type==0?0.5:20));
            }
        }
    };
    connect(scanType,&QComboBox::currentIndexChanged,this,updateAtoms);
    updateAtoms(initialType);
    scanSettings->setVisible(false);
    form->addRow(scanSettings);
    preserveFramework=new QCheckBox(QStringLiteral("배위 결합·peroxo O–O 거리 유지 (조화 restraint)"),settings);
    preserveFramework->setObjectName("XtbPreserveFramework");
    const bool hasMetal=std::any_of(m.atoms.cbegin(),m.atoms.cend(),[](const Atom &a){return isMetal(a.atomicNumber);});
    preserveFramework->setChecked(hasMetal);form->addRow(preserveFramework);
    preserveBonds=new QCheckBox(QStringLiteral("초기 결합 길이 유지 (조화 restraint · 구조 정리)"),settings);
    preserveBonds->setObjectName("XtbPreserveBonds");preserveBonds->setChecked(hasMetal);
    preserveBonds->setToolTip(QStringLiteral("현재 결합의 초기 거리를 유지하며 구조를 정리합니다. 사용자 거리 constraint가 우선합니다. 자유 최적화에서는 해제하세요."));
    form->addRow(preserveBonds);
    constraints=new ConstraintEditor(m,selection,settings); form->addRow(constraints);
    connect(solvation,&QComboBox::currentIndexChanged,this,[this](int i){solvent->setEnabled(i==1);});
    auto updateMethod=[this,notice]{const bool ff=method->currentData().toInt()==0;
        multiplicity->setEnabled(!ff);solvation->setEnabled(!ff);
        if(ff){solvation->setCurrentIndex(0);solvent->setEnabled(false);}
        notice->setText(ff?QStringLiteral("GFN-FF 분자역학으로 초기 구조를 정리합니다. 스핀 상태와 반응성은 계산하지 않습니다.\n금속–리간드 결합과 O–O 결합은 계산 후 확인하고 DFT 시작 구조로 사용하세요."):
            QStringLiteral("빠른 구조 relaxation / DFT 시작 구조용 반경험적 방법입니다. 스핀 상태 상대 에너지에는 제한이 있습니다."));};
    connect(method,&QComboBox::currentIndexChanged,this,updateMethod);
    method->setCurrentIndex(method->findData(initialMethod));updateMethod();
    connect(kind,&QComboBox::currentIndexChanged,this,[this](int i){
        level->setEnabled(i!=0);cycles->setEnabled(i!=0);
        scanSettings->setVisible(i==2);
        if(i==2){preserveFramework->setChecked(false);preserveBonds->setChecked(false);}
    });
    status=new QLabel(QStringLiteral("계산 준비"),this);status->setObjectName("XtbStatus");status->setWordWrap(true);status->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(status);
    energyMode=new QComboBox(this);energyMode->addItems({"Absolute energy (Eh)","Relative energy (kcal/mol, 첫 frame 기준)"});layout->addWidget(energyMode);
    plot=new PlotWidget(this);plot->setObjectName("XtbEnergyPlot");plot->setMinimumHeight(160);layout->addWidget(plot);
    connect(energyMode,&QComboBox::currentIndexChanged,this,[this]{refreshPlot();});
    connect(plot,&PlotWidget::pointClicked,this,[this](int index){
        if(kind->currentIndex()!=2 || calculationActive() || !runner.selectScanFrame(index))return;
        plot->setSelectedPoint(index);
        QVector<QVector3D> coordinates;
        for(const auto &atom:runner.result.atoms)coordinates<<atom.position;
        emit previewCoordinates(coordinates);
        status->setText(QStringLiteral("Scan point %1/%2 · E %3 Eh%4")
            .arg(index+1).arg(runner.scanFrames.size()).arg(runner.energy,0,'f',9)
            .arg(index==runner.tsCandidateIndex?QStringLiteral(" · TS 후보 (미검증)"):QString()));
    });
    auto *buttons=new QHBoxLayout;layout->addLayout(buttons);
    start=new QPushButton(QStringLiteral("계산 시작"),this);start->setObjectName("XtbStart");cancel=new QPushButton(QStringLiteral("중단"),this);cancel->setObjectName("XtbCancel");cancel->setEnabled(false);
    auto *outputButton=new QPushButton(QStringLiteral("출력 보기"),this);buttons->addWidget(start);buttons->addWidget(cancel);buttons->addWidget(outputButton);
    auto *results=new QHBoxLayout;layout->addLayout(results);
    apply=new QPushButton(QStringLiteral("현재 구조에 적용"),this);apply->setObjectName("XtbApply");open=new QPushButton(QStringLiteral("새 구조로 열기"),this);open->setObjectName("XtbOpen");save=new QPushButton(QStringLiteral("XYZ 저장"),this);
    for(auto *button:{apply,open,save}){results->addWidget(button);button->setEnabled(false);}
    connect(start,&QPushButton::clicked,this,&XtbDialog::startCalculation);
    connect(cancel,&QPushButton::clicked,&runner,&XtbRunner::cancel);
    connect(&runner,&XtbRunner::frameReady,this,[this](const XtbFrame &f){
        emit previewCoordinates(f.coordinates);
        if(std::isfinite(f.energy)){
            double x=f.step;
            if(kind->currentIndex()==2 && scanPoints->value()>1)
                x=scanFrom->value()+(scanTo->value()-scanFrom->value())*(f.step-1)/double(scanPoints->value()-1);
            series.push_back(QPointF(x,f.energy));
        }
        refreshPlot();
        status->setText(QStringLiteral("진행 중 · %1 %2 · E %3 Eh · ∇ %4")
            .arg(kind->currentIndex()==2?QStringLiteral("Scan point"):QStringLiteral("step"))
            .arg(f.step).arg(f.energy,0,'f',9).arg(f.gradient,0,'g',6));
    });
    connect(&runner,&XtbRunner::metricsChanged,this,[this]{if(!kind->currentIndex())status->setText(QStringLiteral("Single Point · E %1 Eh · ∇ %2 · gap %3 eV").arg(runner.energy,0,'f',9).arg(runner.gradient).arg(runner.gap));});
    connect(&runner,&XtbRunner::completed,this,&XtbDialog::finishCalculation);
    connect(apply,&QPushButton::clicked,this,[this]{applied=true;original=runner.result;emit applyGeometry(runner.result);});
    connect(open,&QPushButton::clicked,this,[this]{emit openGeometry(runner.result);});
    connect(save,&QPushButton::clicked,this,[this]{QString path=QFileDialog::getSaveFileName(this,QStringLiteral("XYZ 저장"),kind->currentIndex()==2?QString("xtb_scan_point_%1.xyz").arg(runner.cycle):QStringLiteral("xtbopt.xyz"),"XYZ (*.xyz)");if(path.isEmpty())return;QFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(XtbRunner::xyz(runner.result).toUtf8())<0)QMessageBox::warning(this,"xTB",QStringLiteral("XYZ 저장 실패"));});
    connect(outputButton,&QPushButton::clicked,this,[this]{auto *d=new QDialog(this);d->setAttribute(Qt::WA_DeleteOnClose);d->setWindowTitle(QStringLiteral("xTB 출력 — ")+runner.directory);d->resize(800,600);auto *l=new QVBoxLayout(d);auto *text=new QPlainTextEdit(d);text->setReadOnly(true);text->setPlainText(runner.output);l->addWidget(text);d->show();});
    refreshPlot();
    if(automatic)QTimer::singleShot(0,this,&XtbDialog::startCalculation);
}
void XtbDialog::setBondRetention(bool enabled) {
    preserveBonds->setChecked(enabled);preserveFramework->setChecked(enabled);
}
void XtbDialog::configureOptimization(const QVector<GeometryConstraint> &entries,int maximum) {
    constraints->setEntries(entries);cycles->setValue(qBound(10,maximum,5000));
    // Keep the user-visible framework/bond retention defaults across MM fallback.
}
void XtbDialog::startCalculation() {
    QVector<GeometryConstraint> entries; QString error;
    if(!constraints->read(entries,&error)){QMessageBox::warning(this,"Constraint",error);return;}
    emit restorePreview();applied=false;series.clear();refreshPlot();
    XtbOptions o;o.optimize=kind->currentIndex()!=0;o.method=method->currentData().toInt();o.charge=charge->value();o.multiplicity=multiplicity->value();o.cycles=cycles->value();o.level=level->currentText();o.threads=threads->currentData().toInt();if(solvation->currentIndex())o.solvent=solvent->currentText();
    o.constraints=entries;
    if(kind->currentIndex()==2) {
        XtbScan scan;scan.type=scanType->currentData().toString();
        const int count=scanType->currentIndex()+2;
        for(int i=0;i<count;++i)scan.atoms<<scanAtoms[i]->value()-1;
        scan.from=scanFrom->value();scan.to=scanTo->value();scan.points=scanPoints->value();
        o.scan=scan;
    }
    o.preserveCoordination=preserveFramework->isChecked();
    o.preserveBondLengths=preserveBonds->isChecked();
    settings->setEnabled(false);start->setEnabled(false);cancel->setEnabled(true);for(auto *b:{apply,open,save})b->setEnabled(false);
    status->setText(QStringLiteral("시작 중..."));runner.start(original,o,runtime);
}
void XtbDialog::refreshPlot() {
    auto values=series;if(energyMode->currentIndex()&&!values.isEmpty()){double ref=values.first().y();for(auto &p:values)p.setY((p.y()-ref)*627.509474);}
    const QString y=energyMode->currentIndex()?QStringLiteral("ΔE (kcal mol⁻¹)"):QStringLiteral("Energy (Eh)");
    if(kind->currentIndex()==2)plot->setScanSeries(values,scanType->currentIndex()==0?QStringLiteral("Distance (Å)"):scanType->currentIndex()==1?QStringLiteral("Angle (°)"):QStringLiteral("Dihedral (°)"),y);
    else plot->setSeries(values,QStringLiteral("Optimization step"),y);
}
void XtbDialog::finishCalculation() {
    settings->setEnabled(true);start->setEnabled(true);cancel->setEnabled(false);
    QString label;
    switch(runner.state){case XtbRunner::State::Finished:
        label=kind->currentIndex()==2?QStringLiteral("Relaxed PES Scan 완료"):
            (kind->currentIndex()?QStringLiteral("최적화 수렴"):QStringLiteral("Single Point 완료"));break;case XtbRunner::State::NotConverged:label=kind->currentIndex()==2?QStringLiteral("Scan 일부 완료 (미수렴)"):QStringLiteral("최적화 미수렴 — 마지막 geometry 확인 가능");break;case XtbRunner::State::Cancelled:label=QStringLiteral("중단 — 마지막 geometry 확인 가능");break;default:label=runner.error;break;}
    auto value=[](double v){return std::isfinite(v)?QString::number(v,'g',12):QStringLiteral("제공되지 않음");};
    status->setText(QStringLiteral("%1\nStep %2 · Total Energy %3 Eh · Gradient norm %4 · HOMO-LUMO gap %5 eV\n%6").arg(label).arg(runner.cycle).arg(value(runner.energy),value(runner.gradient),value(runner.gap),runner.directory));
    if(kind->currentIndex() && (preserveBonds->isChecked() || preserveFramework->isChecked()))
        status->setText(status->text()+QStringLiteral("\n초기 결합 거리 restraint 적용"));
    if(kind->currentIndex()==2){
        const int count=runner.scanFrames.size();
        const QString candidate=runner.tsCandidateIndex>=0?
            QStringLiteral("point %1 (미검증)").arg(runner.tsCandidateIndex+1):
            QStringLiteral("없음 (내부 에너지 극댓값 미발견)");
        status->setText(status->text()+QStringLiteral("\nScan frame %1/%2 · TS 후보: %3\n")
            .arg(count).arg(scanPoints->value()).arg(candidate)+
            QStringLiteral("최댓값 구조는 TS 초기 추정치입니다. 진정한 TS 여부는 비제약 최적화·진동수·IRC로 확인해야 합니다."));
        if(runner.tsCandidateIndex>=0)plot->setSelectedPoint(runner.tsCandidateIndex);
    }
    bool usable=runner.state==XtbRunner::State::Finished||runner.state==XtbRunner::State::NotConverged||runner.state==XtbRunner::State::Cancelled;
    if(kind->currentIndex()==2)usable=usable&&!runner.scanFrames.isEmpty();
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
