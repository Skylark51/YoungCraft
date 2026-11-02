#include "compute/GaussianConnection.h"
#include <QTimer>
#include "compute/GaussianRemote.h"
#include "ui/OrbitalStudyWidget.h"
#include "render/MoleculeView.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <cmath>

namespace youngcraft {
namespace {
QTableWidget *table(const QStringList &headers) {
    auto *t = new QTableWidget(0,headers.size()); t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection); t->horizontalHeader()->setStretchLastSection(true);
    t->setMinimumHeight(150); return t;
}
void append(QTableWidget *t, const QStringList &values) {
    const int row=t->rowCount(); t->insertRow(row);
    for(int i=0;i<values.size();++i) t->setItem(row,i,new QTableWidgetItem(values[i]));
}
QString num(double x) { return std::isfinite(x)?QString::number(x,'f',5):QStringLiteral("—"); }
}
OrbitalStudyWidget::OrbitalStudyWidget(QWidget *parent) : QWidget(parent), m_runner(new GaussianTools(this)) {
    setObjectName("OrbitalStudyWidget");
    auto *layout=new QVBoxLayout(this); layout->setContentsMargins(2,2,2,2);
    m_source=new QLabel; m_source->setWordWrap(true); layout->addWidget(m_source);
    auto *open=new QPushButton(QStringLiteral("CHK / FCHK / cube 열기…")); layout->addWidget(open);
    connect(open,&QPushButton::clicked,this,[this] {
        const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("Orbitals 파일"),{},"Checkpoint / cube (*.chk *.fchk *.fch *.cube *.cub);;Gaussian output (*.out *.log)");
        if(!path.isEmpty()) emit openRequested(path);
    });
    auto *save=new QPushButton(QStringLiteral("변환된 FCHK 저장…")); layout->addWidget(save);
    connect(save,&QPushButton::clicked,this,[this] {
        if(m_data.format!="FCHK") { m_status->setText(QStringLiteral("CHK 또는 FCHK를 먼저 여세요. CHK는 formchk로 자동 변환합니다.")); return; }
        const auto path=QFileDialog::getSaveFileName(this,QStringLiteral("FCHK 저장"),"orbitals.fchk","Formatted checkpoint (*.fchk)");
        if(path.isEmpty())return;
        QFile input(m_data.filePath); QSaveFile output(path);
        if(!input.open(QIODevice::ReadOnly)||!output.open(QIODevice::WriteOnly)) {m_status->setText(QStringLiteral("FCHK 저장 파일을 열 수 없습니다."));return;}
        while(!input.atEnd()) { const auto bytes=input.read(1024*1024); if(bytes.isEmpty()&&input.error()!=QFile::NoError) {output.cancelWriting();break;} if(output.write(bytes)!=bytes.size()){output.cancelWriting();break;} }
        m_status->setText(output.commit()?QStringLiteral("FCHK 저장 완료: ")+path:QStringLiteral("FCHK 저장 실패"));
    });
    auto *quick=new QHBoxLayout;
    m_homo=new QPushButton("HOMO");m_homo->setObjectName("ShowHomo");m_lumo=new QPushButton("LUMO");m_lumo->setObjectName("ShowLumo");
    m_generateMep=new QPushButton(QStringLiteral("MEP 생성"));m_generateMep->setObjectName("GenerateMep");
    quick->addWidget(m_homo);quick->addWidget(m_lumo);quick->addWidget(m_generateMep);layout->addLayout(quick);
    connect(m_homo,&QPushButton::clicked,this,[this]{if(chooseFrontier(false))generate(false);else m_status->setText(QStringLiteral("HOMO 궤도를 찾지 못했습니다."));});
    connect(m_lumo,&QPushButton::clicked,this,[this]{if(chooseFrontier(true))generate(false);else m_status->setText(QStringLiteral("LUMO 궤도를 찾지 못했습니다."));});
    connect(m_generateMep,&QPushButton::clicked,this,[this]{generate(true);});
    auto *remote=new QGroupBox(QStringLiteral("계산 서버 · 자동 연결"));
    remote->setObjectName("RemoteGaussianEnabled");remote->setCheckable(true);
    remote->setChecked(QSettings().value("orbitals/remoteEnabled",false).toBool());
    auto *connection=GaussianConnection::instance();
    QTimer::singleShot(0,connection,[connection]{connection->connectNow();});
    auto *remoteLayout=new QVBoxLayout(remote);
    auto *remoteSummary=new QLabel(connection->text());remoteSummary->setObjectName("GaussianConnectionStatus");remoteSummary->setProperty("connected",connection->connected());remoteSummary->setWordWrap(true);remoteLayout->addWidget(remoteSummary);
    connect(connection,&GaussianConnection::changed,remoteSummary,[remoteSummary](const QString &text,bool connected){remoteSummary->setText(text);remoteSummary->setProperty("connected",connected);});
    auto *reconnect=new QPushButton(QStringLiteral("자동 재연결"));reconnect->setObjectName("ReconnectGaussianServer");remoteLayout->addWidget(reconnect);
    connect(reconnect,&QPushButton::clicked,connection,[connection]{connection->connectNow(true);});
    auto *remoteOptions=new QPushButton(QStringLiteral("서버 설정…"));remoteOptions->setObjectName("RemoteGaussianSettingsButton");remoteLayout->addWidget(remoteOptions);
    auto *remoteDialog=new QDialog(this);remoteDialog->setObjectName("RemoteGaussianSettings");remoteDialog->setWindowTitle(QStringLiteral("계산 서버 · Gaussian 설정"));
    auto *remoteForm=new QFormLayout(remoteDialog);
    connect(remoteOptions,&QPushButton::clicked,this,[remoteDialog,connection]{remoteDialog->exec();connection->connectNow(true);});
    auto *remoteHelp=new QLabel(QStringLiteral("기존 SSH 키 인증으로 CHK 변환과 MO / MEP 생성을 서버에서 실행하고 결과를 가져옵니다."));
    remoteHelp->setWordWrap(true);remoteForm->addRow(remoteHelp);
    auto remoteText=[this,remoteForm](const QString &label,const QString &key,const QString &fallback,bool browse) {
        auto *row=new QWidget;auto *h=new QHBoxLayout(row);h->setContentsMargins(0,0,0,0);
        auto *value=new QLineEdit(QSettings().value("orbitals/"+key,fallback).toString());value->setObjectName(key);h->addWidget(value);
        connect(value,&QLineEdit::editingFinished,this,[key,value]{QSettings().setValue("orbitals/"+key,value->text().trimmed());});
        if(browse){auto *button=new QPushButton("…");button->setMaximumWidth(30);h->addWidget(button);
            connect(button,&QPushButton::clicked,this,[this,key,value]{const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("SSH 파일 선택"),{},"All files (*)");if(!path.isEmpty()){value->setText(path);QSettings().setValue("orbitals/"+key,path);}});}
        remoteForm->addRow(label,row);
    };
    remoteText(QStringLiteral("서버 (사용자@호스트 / SSH 별칭)"),"remoteHost",{},false);
    auto *port=new QSpinBox;port->setObjectName("remotePort");port->setRange(1,65535);port->setValue(QSettings().value("orbitals/remotePort",22).toInt());
    remoteForm->addRow(QStringLiteral("SSH 포트"),port);connect(port,qOverload<int>(&QSpinBox::valueChanged),this,[](int value){QSettings().setValue("orbitals/remotePort",value);});
    remoteText(QStringLiteral("SSH 설정 파일 (선택)"),"remoteConfig",{},true);
    remoteText(QStringLiteral("SSH 키 파일 (선택)"),"remoteIdentity",{},true);
    remoteText(QStringLiteral("서버 Gaussian 폴더"),"remoteGaussianDirectory","/opt/gaussian/g16",false);
    remoteText(QStringLiteral("서버 CHK 폴더 (동일 파일 재사용)"),"remoteCheckpointDirectory",{},false);
    auto *remoteClose=new QDialogButtonBox(QDialogButtonBox::Close);remoteForm->addRow(remoteClose);
    connect(remoteClose,&QDialogButtonBox::rejected,remoteDialog,&QDialog::reject);
    connect(remote,&QGroupBox::toggled,this,[this](bool checked){
        QSettings().setValue("orbitals/remoteEnabled",checked);
        GaussianConnection::instance()->connectNow(true);
        m_status->setText(checked?QStringLiteral("서버 자동 연결을 사용합니다. CHK를 열면 HOMO를 자동 생성합니다."):QStringLiteral("로컬 Gaussian 실행을 선택했습니다."));
    });
    layout->addWidget(remote);
    auto *toolGroup=new QGroupBox(QStringLiteral("로컬 Gaussian 도구 경로 (서버 실행 해제 시 사용)"));
    toolGroup->setCheckable(true);toolGroup->setChecked(false); auto *toolForm=new QFormLayout(toolGroup);
    for(const auto &name:{QStringLiteral("formchk"),QStringLiteral("cubegen")}) {
        auto *row=new QWidget;auto *h=new QHBoxLayout(row);h->setContentsMargins(0,0,0,0);
        auto *path=new QLineEdit(GaussianTools::executable(name));path->setObjectName(name+"Path");path->setToolTip(QStringLiteral("Gaussian 실행 파일 경로. 설치 환경변수를 상속합니다."));
        auto *browse=new QPushButton("…");browse->setMaximumWidth(30);h->addWidget(path);h->addWidget(browse);toolForm->addRow(name,row);
        connect(path,&QLineEdit::editingFinished,this,[name,path]{QSettings().setValue("orbitals/"+name,path->text().trimmed());});
        connect(browse,&QPushButton::clicked,this,[this,path,name]{const auto f=QFileDialog::getOpenFileName(this,name,{},"Executable (*.exe);;All files (*)");if(!f.isEmpty()){path->setText(f);QSettings().setValue("orbitals/"+name,f);}});
    }
    remoteForm->insertRow(remoteForm->rowCount()-1,toolGroup);
    m_tables=new QTabWidget; m_tables->setObjectName("OrbitalAnalysisTables");
    m_orbitals=table({"MO","Spin","Energy (eV)","Occupation","Frontier"});m_orbitals->setObjectName("OrbitalTable");
    m_tables->addTab(m_orbitals,"FMO"); layout->addWidget(m_tables);
    auto *frontier=new QHBoxLayout;
    auto *exportButton=new QPushButton(QStringLiteral("표 저장…"));frontier->addWidget(exportButton);connect(exportButton,&QPushButton::clicked,this,&OrbitalStudyWidget::exportTable);layout->addLayout(frontier);
    auto *filter=new QLineEdit;filter->setPlaceholderText(QStringLiteral("분석 표 검색 (원자 / donor / acceptor 등)"));layout->addWidget(filter);
    connect(filter,&QLineEdit::textChanged,this,[this](const QString &text){for(auto *t:m_tables->findChildren<QTableWidget*>())for(int r=0;r<t->rowCount();++r){bool found=text.isEmpty();for(int c=0;c<t->columnCount();++c)if(t->item(r,c))found|=t->item(r,c)->text().contains(text,Qt::CaseInsensitive);t->setRowHidden(r,!found);}});
    auto *form=new QFormLayout;
    m_resolution=new QComboBox; m_resolution->setObjectName("OrbitalGridResolution");m_resolution->addItems({"40 (빠르게)","80 (표준)","120 (정밀)"});m_resolution->setCurrentIndex(1);form->addRow(QStringLiteral("격자 해상도"),m_resolution);
    m_density=new QComboBox;m_density->addItems({"SCF","MP2","CI","QCI"});m_density->setToolTip(QStringLiteral("FCHK에 저장된 밀도 유형을 선택하세요. 없는 밀도는 cubegen이 오류를 보고합니다."));form->addRow(QStringLiteral("MEP 밀도"),m_density);
    layout->addLayout(form);
    auto *actions=new QHBoxLayout; m_generateMo=new QPushButton(QStringLiteral("선택 MO 생성"));m_generateMo->setObjectName("GenerateMo");
    actions->addWidget(m_generateMo);layout->addLayout(actions);
    connect(m_generateMo,&QPushButton::clicked,this,[this]{generate(false);});
    auto *map=new QPushButton(QStringLiteral("현재 density cube에 potential cube 매핑…"));layout->addWidget(map);
    connect(map,&QPushButton::clicked,this,[this]{
        if(!m_surface) {m_status->setText(QStringLiteral("전자밀도 cube 파일을 먼저 여세요."));return;}
        const auto f=QFileDialog::getOpenFileName(this,QStringLiteral("Potential cube"),{},"Cube (*.cube *.cub)");if(f.isEmpty())return;
        auto potential=std::make_shared<CubeGrid>(CubeParser::parse(f));if(!potential->valid){m_status->setText(potential->error);return;}
        if(potential->molecule.atoms.size()!=m_surface->molecule.atoms.size()){m_status->setText(QStringLiteral("두 cube의 분자가 다릅니다."));return;}
        for(int i=0;i<potential->molecule.atoms.size();++i)if(potential->molecule.atoms[i].atomicNumber!=m_surface->molecule.atoms[i].atomicNumber||(potential->molecule.atoms[i].position-m_surface->molecule.atoms[i].position).length()>0.001f){m_status->setText(QStringLiteral("두 cube의 원자/좌표가 일치하지 않습니다."));return;}
        for(int x:{0,m_surface->nx-1})for(int y:{0,m_surface->ny-1})for(int z:{0,m_surface->nz-1})if(!std::isfinite(potential->sample(m_surface->origin+x*m_surface->axisX+y*m_surface->axisY+z*m_surface->axisZ))){m_status->setText(QStringLiteral("Potential 격자가 density 격자 전체를 포함해야 합니다."));return;}
        showSurface(m_surface,potential);
    });
    auto *surfaceGroup=new QGroupBox(QStringLiteral("Surfaces · 등가면 / 색상"));auto *surfaceForm=new QFormLayout(surfaceGroup);
    auto spin=[surfaceForm](const QString &label,double lo,double hi,double value,int decimals){auto *s=new QDoubleSpinBox;s->setDecimals(decimals);s->setRange(lo,hi);s->setValue(value);s->setSingleStep(value>0?value/10:0.01);surfaceForm->addRow(label,s);return s;};
    m_iso=spin("Isovalue (a.u.)",0.000001,10,0.02,6);m_iso->setObjectName("OrbitalIsovalue");m_opacity=spin(QStringLiteral("불투명도"),0,1,0.75,2);
    m_positive=new QCheckBox(QStringLiteral("양의 위상 / 밀도"));m_positive->setChecked(true);m_negative=new QCheckBox(QStringLiteral("음의 위상"));m_negative->setChecked(true);surfaceForm->addRow(m_positive,m_negative);
    auto colorButton=[this,surfaceForm](const QString &label,QColor *color){auto *b=new QPushButton(color->name());b->setStyleSheet("border-left:12px solid "+color->name());surfaceForm->addRow(label,b);connect(b,&QPushButton::clicked,this,[this,b,color]{const auto c=QColorDialog::getColor(*color,this);if(c.isValid()){*color=c;b->setText(c.name());b->setStyleSheet("border-left:12px solid "+c.name());applyStyle();}});};
    colorButton(QStringLiteral("MO 양의 색"),&m_plusColor);colorButton(QStringLiteral("MO 음의 색"),&m_minusColor);
    m_min=spin("MEP min (a.u.)",-100,100,-0.05,4);m_max=spin("MEP max (a.u.)",-100,100,0.05,4);
    colorButton(QStringLiteral("MEP 낮은 값"),&m_lowColor);colorButton(QStringLiteral("MEP 높은 값"),&m_highColor);
    layout->addWidget(surfaceGroup);
    m_legend=new QLabel;m_legend->setWordWrap(true);layout->addWidget(m_legend);
    connect(m_iso,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this](double value){
        (m_mep ? m_mepIsovalue : m_moIsovalue) = value;
        applyStyle();
    });
    for(auto *s:{m_opacity,m_min,m_max})connect(s,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{applyStyle();});
    for(auto *b:{m_positive,m_negative})connect(b,&QCheckBox::toggled,this,[this]{applyStyle();});
    m_status=new QLabel(QStringLiteral("Gaussian formchk / cubegen 또는 기존 cube 파일을 사용합니다."));m_status->setObjectName("OrbitalStatus");m_status->setWordWrap(true);m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->insertWidget(4,m_status);
    m_cancel=new QPushButton(QStringLiteral("계산 취소"));m_cancel->setEnabled(false);layout->insertWidget(5,m_cancel);connect(m_cancel,&QPushButton::clicked,m_runner,&GaussianTools::cancel);
    connect(m_runner,&GaussianTools::status,m_status,&QLabel::setText);
    connect(m_runner,&GaussianTools::failed,this,[this](const QString &message){m_status->setText(message);updateGenerationButtons();});
    connect(m_runner,&GaussianTools::ready,this,&OrbitalStudyWidget::showSurface);
    setData(CalculationData{});
}
void OrbitalStudyWidget::setView(MoleculeView *view) {m_view=view;}
void OrbitalStudyWidget::setData(const CalculationData &data) {
    if(m_runner->busy())m_runner->cancel();
    m_data=data;m_surface.reset();m_potential.reset();m_mep=false;
    {QSignalBlocker block(m_iso);m_iso->setValue(m_moIsovalue);}
    m_source->setText(data.filePath.isEmpty()?QStringLiteral("Orbitals · CHK 변환 / FMO / MEP / NBO / Wiberg"):QFileInfo(data.filePath).fileName());m_source->setToolTip(data.filePath);
    m_orbitals->setRowCount(0);
    QMap<QString,int> homos;
    for(const auto &o:data.orbitals) if(o.occupation>0) homos[o.spin]=qMax(homos.value(o.spin),o.number);
    for(const auto &o:data.orbitals) {
        const int homo=homos.value(o.spin);
        append(m_orbitals,{QString::number(o.number),o.spin,num(o.energyEv),num(o.occupation),homo>0?(o.number==homo?"HOMO":o.number==homo+1?"LUMO":""):""});
    }
    while(m_tables->count()>1){auto *w=m_tables->widget(1);m_tables->removeTab(1);delete w;}
    auto add=[this](const QString &title,const QStringList &headers,const QVector<NboRow> &rows){auto *t=table(QStringList{"Section"}+headers);for(const auto &r:rows)append(t,QStringList{QString::number(r.section)}+r.values);m_tables->addTab(t,title+QString(" (%1)").arg(rows.size()));};
    add("NPA",{"Atom","Element","Charge (e)","Core (e)","Valence (e)","Rydberg (e)","Total (e)"},data.nbo.charges);
    add("NBO",{"NBO","Occupation (e)","Description"},data.nbo.orbitals);
    add("E(2)",{"Donor","Acceptor","E(2) kcal/mol","ΔE a.u.","F(i,j) a.u."},data.nbo.interactions);
    add("Wiberg",{"Atom 1","Atom 2","Bond index"},data.nbo.wiberg);
    auto *raw=new QPlainTextEdit;raw->setReadOnly(true);raw->setPlainText(data.nbo.excerpt.isEmpty()?QStringLiteral("이 파일에서 지원되는 NBO/NPA/Wiberg 표를 찾지 못했습니다. Gaussian output에 해당 분석이 출력되어 있어야 합니다."):data.nbo.excerpt);m_tables->addTab(raw,QStringLiteral("원문"));
    updateGenerationButtons();
    if(data.format=="CUBE"){auto grid=std::make_shared<CubeGrid>(CubeParser::parse(data.filePath));if(grid->valid)m_surface=grid;}
    if (data.format=="FCHK") {
        if (QSettings().value("orbitals/remoteEnabled",false).toBool()) {
            const auto error=GaussianRemoteSettings::load().error();
            m_status->setText(error.isEmpty()
                ? QStringLiteral("FCHK 읽기 완료. MO를 선택하고 ‘선택 MO 생성’ 또는 ‘MEP 생성’을 누르면 계산 서버에서 실행합니다.")
                : QStringLiteral("서버 설정을 확인하세요: ")+error);
        } else {
            m_status->setText(!GaussianTools::isAvailable("cubegen")
                ? QStringLiteral("FCHK를 읽었지만 Gaussian cubegen 실행 파일이 없어 MO / MEP를 생성할 수 없습니다. 로컬 Gaussian 도구 경로를 지정하거나 계산 서버 실행을 선택하세요.")
                : data.orbitals.isEmpty()
                    ? QStringLiteral("구조를 읽었지만 MO 에너지 표가 없습니다. MEP 생성은 밀도 데이터가 있는 FCHK에서 사용할 수 있습니다.")
                    : QStringLiteral("FCHK 읽기 완료. HOMO / LUMO 또는 표의 MO를 선택한 뒤 ‘선택 MO 생성’을 누르세요. MEP는 ‘MEP 생성’을 누르세요."));
        }
    } else if (data.format!="CUBE") {
        m_status->setText(QStringLiteral("MO / MEP 생성에는 FCHK가 필요합니다. CHK는 formchk로 변환하며, 등가면 생성에는 cubegen이 필요합니다. CHK / FCHK / cube 열기로 파일을 선택하세요."));
    }
    chooseFrontier(false);
}
bool OrbitalStudyWidget::chooseFrontier(bool lumo) {
    QString spin="Alpha";if(m_orbitals->currentRow()>=0)spin=m_orbitals->item(m_orbitals->currentRow(),1)->text();
    for(int r=0;r<m_orbitals->rowCount();++r)if(m_orbitals->item(r,1)->text()==spin&&m_orbitals->item(r,4)->text()==(lumo?"LUMO":"HOMO")){m_orbitals->selectRow(r);m_orbitals->scrollToItem(m_orbitals->item(r,0));return true;}
    return false;
}
void OrbitalStudyWidget::updateGenerationButtons() {
    const bool idle=!m_runner->busy(), formatted=m_data.format=="FCHK", mo=formatted&&!m_data.orbitals.isEmpty();
    m_generateMo->setEnabled(idle&&mo);m_homo->setEnabled(idle&&mo);m_lumo->setEnabled(idle&&mo);
    m_generateMep->setEnabled(idle&&formatted);m_cancel->setEnabled(!idle);
}
void OrbitalStudyWidget::startDefaultOrbital() {
    if(m_data.format!="FCHK"||m_surface||m_runner->busy())return;
    if(!QSettings().value("orbitals/remoteEnabled",false).toBool()||!QSettings().value("orbitals/autoGenerateHomo",true).toBool())return;
    if(chooseFrontier(false))generate(false);
    else m_status->setText(QStringLiteral("FCHK를 읽었지만 HOMO를 판별할 수 없습니다. MO 표에서 궤도를 선택하세요."));
}
void OrbitalStudyWidget::generate(bool mep) {
    if(m_runner->busy())return;
    QString kind="MEP";
    if(!mep){int r=m_orbitals->currentRow();if(r<0){m_status->setText(QStringLiteral("궤도를 선택하세요."));return;}kind=(m_orbitals->item(r,1)->text()=="Beta"?"BMO=":"AMO=")+m_orbitals->item(r,0)->text();}
    m_homo->setEnabled(false);m_lumo->setEnabled(false);m_generateMo->setEnabled(false);m_generateMep->setEnabled(false);m_cancel->setEnabled(true);
    m_runner->generate(m_data.filePath,kind,QList<int>{40,80,120}[m_resolution->currentIndex()],m_density->currentText());
    updateGenerationButtons();
}
void OrbitalStudyWidget::showSurface(std::shared_ptr<const CubeGrid> surface,std::shared_ptr<const CubeGrid> potential) {
    m_surface=surface;m_potential=potential;m_mep=bool(potential);
    updateGenerationButtons();
    {QSignalBlocker block(m_iso);m_iso->setValue(m_mep?m_mepIsovalue:m_moIsovalue);}
    if(m_view)m_view->setCubeGrid(m_surface);
    applyStyle();
    m_status->setToolTip(QStringLiteral("생성된 cube: %1\n%2").arg(m_surface->filePath,m_potential?m_potential->filePath:QString()));
    m_status->setText(m_mep?QStringLiteral("MEP: 전자밀도 등가면에 정전기 퍼텐셜 매핑. 이미지 복사/저장으로 내보낼 수 있습니다."):QStringLiteral("MO 등가면 생성 완료. Isovalue와 위상 색상을 조절하고 이미지 복사/저장으로 내보내세요."));
}
void OrbitalStudyWidget::applyStyle() {
    if(!m_view)return;
    if(m_mep && m_min->value()>=m_max->value()){m_status->setText(QStringLiteral("MEP min은 max보다 작아야 합니다."));return;}
    auto style=m_view->style();style.isoValue=float(m_iso->value());style.isoOpacity=float(m_opacity->value());style.showPositiveSurface=m_positive->isChecked();style.showNegativeSurface=!m_mep&&m_negative->isChecked();style.positiveSurfaceColor=m_plusColor;style.negativeSurfaceColor=m_minusColor;m_view->setStyle(style);emit surfaceStyleChanged();
    m_view->setPotentialGrid(m_potential,m_min->value(),m_max->value(),m_lowColor,m_highColor);
    m_legend->setText(m_mep?QStringLiteral("MEP %1 → %2 a.u. · %3 → 흰색 → %4\n회색: 격자 범위 밖").arg(m_min->value()).arg(m_max->value()).arg(m_lowColor.name(),m_highColor.name()):QStringLiteral("MO: + / − 위상 · isovalue %1 a.u.").arg(m_iso->value()));
}
void OrbitalStudyWidget::exportTable() {
    auto *t=qobject_cast<QTableWidget*>(m_tables->currentWidget());if(!t){m_status->setText(QStringLiteral("저장할 분석 표를 선택하세요."));return;}
    const auto path=QFileDialog::getSaveFileName(this,QStringLiteral("분석 표 저장"),"orbitals.tsv","TSV (*.tsv)");if(path.isEmpty())return;
    QByteArray bytes("\xef\xbb\xbf");for(int r=-1;r<t->rowCount();++r){if(r>=0&&t->isRowHidden(r))continue;QStringList fields;for(int c=0;c<t->columnCount();++c){auto *item=r<0?t->horizontalHeaderItem(c):t->item(r,c);fields<< (item?item->text():QString()).replace('\t',' ').replace('\n',' ');}bytes+=(fields.join('\t')+'\n').toUtf8();}
    QSaveFile out(path);const bool ok=out.open(QIODevice::WriteOnly)&&out.write(bytes)==bytes.size()&&out.commit();m_status->setText(ok?QStringLiteral("분석 표 저장 완료"):QStringLiteral("분석 표 저장 실패"));
}
}
