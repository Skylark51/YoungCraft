#include "ui/TransitionAnalysisDialog.h"
#include "compute/GaussianTools.h"
#include "render/MoleculeView.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>

namespace youngcraft {
namespace {
QString combination(const QStringList &labels,const QVector<double> &coeff) {
    QStringList text;
    for(int i=0;i<labels.size();++i) if(std::abs(coeff[i])>=0.001)
        text.append(QStringLiteral("%1·MO %2").arg(coeff[i],0,'f',3).arg(labels[i]));
    return text.join(QStringLiteral("  "));
}
}
TransitionAnalysisDialog::TransitionAnalysisDialog(const CalculationData &data,int row,QWidget *parent)
    :QDialog(parent),m_data(data),m_runner(new GaussianTools(this)) {
    setAttribute(Qt::WA_DeleteOnClose); setObjectName("TransitionAnalysisDialog");
    setWindowTitle(QStringLiteral("NTO · Hole / Particle 분석")); resize(1120,800);
    auto *layout=new QVBoxLayout(this);
    const auto state=data.excitedStates.value(row);
    m_analysis=TransitionAnalysis::calculate(state);
    if(state.frameIndex>=0 && state.frameIndex<data.frames.size() && data.frames[state.frameIndex].coordinates.size()==m_data.molecule.atoms.size())
        for(int i=0;i<m_data.molecule.atoms.size();++i)m_data.molecule.atoms[i].position=data.frames[state.frameIndex].coordinates[i];
    auto *title=new QLabel(QStringLiteral("%1 · 상태 %2 · %3 eV")
        .arg(QFileInfo(data.filePath).fileName(),state.label.isEmpty()?QString::number(state.state):state.label)
        .arg(state.energyEv,0,'f',4)); layout->addWidget(title);
    auto *notice=new QLabel(QStringLiteral("출력된 정방향 전이 계수(X)의 SVD 근사입니다. 생략된 전이 및 역방향 계수(Y)는 포함하지 않습니다. "
        "기여도는 읽은 계수 내에서 정규화합니다. 전체 전이밀도 기반의 엄밀한 TD-DFT NTO와 다를 수 있습니다. "
        "Hole은 전자가 빠지는 쪽, Particle은 전자가 이동하는 쪽입니다."));
    notice->setWordWrap(true); layout->addWidget(notice);
    auto *table=new QTableWidget(int(m_analysis.pairs.size()),3,this); table->setObjectName("NtoPairsTable");
    table->setHorizontalHeaderLabels({QStringLiteral("NTO 쌍 기여도"),QStringLiteral("Hole (signed MO 조합)"),QStringLiteral("Particle (signed MO 조합)")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(int i=0;i<m_analysis.pairs.size();++i) {
        const auto &p=m_analysis.pairs[i];
        table->setItem(i,0,new QTableWidgetItem(QStringLiteral("%1 · %2 %").arg(i+1).arg(100*p.weight,0,'f',3)));
        table->setItem(i,1,new QTableWidgetItem(combination(m_analysis.occupied,p.hole)));
        table->setItem(i,2,new QTableWidgetItem(combination(m_analysis.virtuals,p.particle)));
    }
    layout->addWidget(table,1);
    auto *populations=new QTableWidget(std::max(m_analysis.occupied.size(),m_analysis.virtuals.size()),4,this);
    populations->setObjectName("HoleParticlePopulations"); populations->setEditTriggers(QAbstractItemView::NoEditTriggers);
    populations->setHorizontalHeaderLabels({"Hole MO","Hole (%)","Particle MO","Particle (%)"});
    populations->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(int side=0;side<2;++side) {
        const auto &labels=side?m_analysis.virtuals:m_analysis.occupied;
        const auto &weights=side?m_analysis.particlePopulation:m_analysis.holePopulation;
        for(int i=0;i<labels.size();++i) {
            populations->setItem(i,2*side,new QTableWidgetItem(labels[i]));
            populations->setItem(i,2*side+1,new QTableWidgetItem(QString::number(100*weights[i],'f',3)));
        }
    }
    layout->addWidget(populations,1);
    const QFileInfo input(data.filePath);
    for(const auto &extension:{QStringLiteral("fchk"),QStringLiteral("fch"),QStringLiteral("chk")}) {
        const auto candidate=input.dir().filePath(input.completeBaseName()+'.'+extension);
        if(QFileInfo::exists(candidate)) {m_source=candidate;break;}
    }
    m_sourceLabel=new QLabel; m_sourceLabel->setWordWrap(true); layout->addWidget(m_sourceLabel);
    m_sourceLabel->setText(m_source.isEmpty()?QStringLiteral("3D: 같은 이름의 FCHK/FCH/CHK가 없습니다. MO cube 또는 원래 canonical MO 체크포인트가 필요합니다."):m_source);
    auto *controls=new QHBoxLayout;
    auto *source=new QPushButton(QStringLiteral("체크포인트 선택…")); controls->addWidget(source);
    source->setToolTip(QStringLiteral("동일 TD 계산의 canonical MO 파일을 선택하세요. SaveNTO로 MO가 치환된 파일은 사용하지 마세요."));
    connect(source,&QPushButton::clicked,this,[this]{
        if(m_runner->busy())return;
        const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("동일 계산의 canonical MO 체크포인트"),QFileInfo(m_data.filePath).absolutePath(),"Gaussian (*.fchk *.fch *.chk)");
        if(!path.isEmpty()){m_source=path;m_sourceLabel->setText(path);m_grids.clear();m_hole->clearCubeGrid();m_particle->clearCubeGrid();}
    });
    m_mode=new QComboBox; m_mode->setObjectName("NtoSurfaceMode");
    m_mode->addItem(QStringLiteral("전체 Hole / Particle 밀도"),-1);
    for(int i=0;i<m_analysis.pairs.size();++i) m_mode->addItem(QStringLiteral("NTO 쌍 %1 (%2 %)").arg(i+1).arg(100*m_analysis.pairs[i].weight,0,'f',2),i);
    controls->addWidget(m_mode,1);
    m_iso=new QDoubleSpinBox; m_iso->setDecimals(5);m_iso->setRange(0.00001,1);m_iso->setSingleStep(0.001);m_iso->setValue(0.002);
    m_iso->setPrefix("Iso ");controls->addWidget(m_iso);
    m_generate=new QPushButton(QStringLiteral("3D 분석")); m_generate->setObjectName("GenerateNtoSurfaces");controls->addWidget(m_generate);
    m_generate->setEnabled(m_analysis.error.isEmpty());
    m_cancel=new QPushButton(QStringLiteral("취소"));m_cancel->setEnabled(false);controls->addWidget(m_cancel);
    layout->addLayout(controls);
    auto *views=new QHBoxLayout;
    m_hole=new MoleculeView; m_particle=new MoleculeView;
    for(auto *view:{m_hole,m_particle}) {
        auto *column=new QVBoxLayout; column->addWidget(new QLabel(view==m_hole?"Hole":"Particle"));
        view->setMinimumSize(200,220);view->setMolecule(data.molecule);column->addWidget(view,1);views->addLayout(column,1);
    }
    layout->addLayout(views,3);
    m_status=new QLabel(m_analysis.error.isEmpty()?QStringLiteral("정규화 전 ΣX² = %1 · NTO %2쌍. 3D 분석은 동일 계산의 MO 데이터가 필요합니다.")
        .arg(m_analysis.printedNorm,0,'g',8).arg(m_analysis.pairs.size()):m_analysis.error);
    m_status->setObjectName("NtoAnalysisStatus");m_status->setWordWrap(true);layout->addWidget(m_status);
    connect(m_generate,&QPushButton::clicked,this,[this]{generate();});
    connect(m_cancel,&QPushButton::clicked,this,[this]{m_pending.clear();m_runner->cancel();});
    connect(this,&QDialog::finished,this,[this]{m_pending.clear();m_runner->cancel();});
    connect(m_mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{m_iso->setValue(m_mode->currentData().toInt()<0?0.002:0.02);showGrids();});
    connect(m_iso,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{showGrids();});
    connect(m_runner,&GaussianTools::status,m_status,&QLabel::setText);
    connect(m_runner,&GaussianTools::failed,this,[this](const QString &error){m_status->setText(error);m_pending.clear();m_generate->setEnabled(true);m_cancel->setEnabled(false);});
    connect(m_runner,&GaussianTools::ready,this,[this](auto grid,auto){
        if(m_next<m_pending.size())m_grids[m_pending[m_next++]]=grid;
        QTimer::singleShot(0,this,[this]{nextGrid();});
    });
}
void TransitionAnalysisDialog::generate() {
    if(m_runner->busy())return;
    m_pending=m_analysis.occupied;
    for(const auto &label:m_analysis.virtuals) if(!m_pending.contains(label))m_pending.append(label);
    if(m_pending.size()>64) {m_status->setText(QStringLiteral("3D 생성은 최대 64 MO까지 지원합니다. NTO 계수 분석은 위 표에서 확인할 수 있습니다."));return;}
    // Deterministic companions only; never assign unrelated cubes by directory order.
    const QFileInfo input(m_data.filePath);
    for(const auto &label:m_pending) if(!m_grids.contains(label)) {
        const auto path=input.dir().filePath(input.completeBaseName()+".mo."+label+".cube");
        if(QFileInfo::exists(path)) {
            auto cube=std::make_shared<CubeGrid>(CubeParser::parse(path));
            if(!cube->valid){m_status->setText(cube->error);return;}m_grids[label]=cube;
        }
    }
    bool missing=false;for(const auto &label:m_pending)if(!m_grids.contains(label))missing=true;
    if(missing && (m_source.isEmpty() || m_data.format=="ORCA")) {
        m_status->setText(QStringLiteral("3D 데이터가 부족합니다. Gaussian은 같은 계산의 FCHK/CHK와 cubegen이 필요합니다. "
            "ORCA는 원본 MO 번호·위상을 유지한 cube를 %1.mo.<번호a/b>.cube 이름으로 두세요 (예: %1.mo.0a.cube). 전체 목록: %2")
            .arg(input.completeBaseName(),m_pending.join(", ")));return;
    }
    m_next=0;m_generate->setEnabled(false);m_cancel->setEnabled(true);nextGrid();
}
void TransitionAnalysisDialog::nextGrid() {
    while(m_next<m_pending.size() && m_grids.contains(m_pending[m_next]))++m_next;
    if(m_next>=m_pending.size()) {
        m_generate->setEnabled(true);m_cancel->setEnabled(false);
        if(!m_pending.isEmpty()) {
            m_sourceLabel->setText(QStringLiteral("3D: %1개 canonical MO 그리드 연결됨 · %2")
                .arg(m_grids.size()).arg(m_source.isEmpty()?QFileInfo(m_data.filePath).absolutePath():m_source));
            showGrids();
        }
        return;
    }
    QString label=m_pending[m_next];bool beta=label.endsWith('b');label.remove(QRegularExpression("[ab]$"));
    m_runner->generate(m_source,(beta?"BMO=":"AMO=")+label,40,"SCF");
}
void TransitionAnalysisDialog::showGrids() {
    QVector<std::shared_ptr<const CubeGrid>> holes,particles;
    for(const auto &label:m_analysis.occupied) {if(!m_grids.contains(label))return;holes.append(m_grids[label]);}
    for(const auto &label:m_analysis.virtuals) {if(!m_grids.contains(label))return;particles.append(m_grids[label]);}
    QString error;const int pair=m_mode->currentData().toInt();
    auto h=m_analysis.grid(holes,false,pair,&error);
    if(!h){m_status->setText(error);return;}
    auto p=m_analysis.grid(particles,true,pair,&error);
    if(!p){m_status->setText(error);return;}
    for(const auto &field:{h,p}) {
        const auto &expected=m_data.molecule.atoms.isEmpty()?h->molecule:m_data.molecule;
        if(field->molecule.atoms.size()!=expected.atoms.size()) {
            m_status->setText(QStringLiteral("MO와 TD 결과의 원자 수가 다릅니다. 동일 계산의 MO 파일을 선택하세요."));return;
        }
        for(int i=0;i<expected.atoms.size();++i) if(expected.atoms[i].atomicNumber!=field->molecule.atoms[i].atomicNumber ||
            (expected.atoms[i].position-field->molecule.atoms[i].position).length()>0.02f) {
            m_status->setText(QStringLiteral("MO와 TD 결과의 원자 순서 또는 구조가 다릅니다. 동일 계산의 MO 파일을 선택하세요."));return;
        }
    }
    // Check both fields share one coordinate system as well.
    if(h->nx!=p->nx || h->ny!=p->ny || h->nz!=p->nz || (h->origin-p->origin).length()>1e-6f
       || (h->axisX-p->axisX).length()>1e-6f || (h->axisY-p->axisY).length()>1e-6f || (h->axisZ-p->axisZ).length()>1e-6f) {
        m_status->setText(QStringLiteral("Hole과 Particle MO 그리드 좌표계가 일치하지 않습니다."));return;
    }
    for(int side=0;side<2;++side) {
        auto *view=side?m_particle:m_hole;const auto grid=side?p:h;
        view->setMolecule(grid->molecule);auto style=view->style();style.showPositiveSurface=true;style.showNegativeSurface=pair>=0;
        style.positiveSurfaceColor=side?QColor("#257fce"):QColor("#df7733");style.negativeSurfaceColor=QColor("#ad40a0");
        view->setStyle(style);view->setCubeGrid(grid,float(m_iso->value()));
    }
    m_status->setText(pair<0?QStringLiteral("Hole / Particle: Σ wₖ |NTOₖ(r)|² · 출력 X 계수 기준 근사 밀도"):
        QStringLiteral("선택 NTO 쌍 · 양/음 위상 등가면 · 출력 X 계수 기준 근사"));
}
}
