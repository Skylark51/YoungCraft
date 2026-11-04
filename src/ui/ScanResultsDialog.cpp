#include "ui/ScanResultsDialog.h"
#include "ui/PlotWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <limits>

namespace youngcraft {
namespace {
QString number(double value, int precision = 6) {
    return std::isfinite(value) ? QString::number(value, 'g', precision) : QStringLiteral("—");
}
QDoubleSpinBox *realInput(double value, double minimum, double maximum, int decimals = 3) {
    auto *input = new QDoubleSpinBox;
    input->setDecimals(decimals);
    input->setRange(minimum,maximum);
    input->setValue(value);
    return input;
}
QPushButton *colorInput(const QColor &initial, QWidget *parent) {
    auto *button = new QPushButton(parent);
    auto setColor = [button](const QColor &color) {
        button->setProperty("color",color);
        button->setText(color.name());
        button->setStyleSheet(QStringLiteral("background-color:%1;color:%2;padding:5px;")
            .arg(color.name(),color.lightness()>128?QStringLiteral("black"):QStringLiteral("white")));
    };
    setColor(initial);
    QObject::connect(button,&QPushButton::clicked,parent,[button,parent,setColor] {
        const QColor color=QColorDialog::getColor(button->property("color").value<QColor>(),parent);
        if(color.isValid()) setColor(color);
    });
    return button;
}
struct FontControls {
    QComboBox *family;
    QSpinBox *size;
    QCheckBox *bold, *italic;
    QPushButton *color;
    QWidget *widget;
    FontControls(const QFont &font, const QColor &c, QWidget *parent) {
        widget=new QWidget(parent);
        auto *layout=new QHBoxLayout(widget);
        layout->setContentsMargins(0,0,0,0);
        family=new QComboBox; family->addItems({QStringLiteral("Arial"),QStringLiteral("Times New Roman")});
        family->setCurrentText(font.family());
        size=new QSpinBox; size->setRange(6,48); size->setValue(font.pointSize()); size->setSuffix(QStringLiteral(" pt"));
        bold=new QCheckBox(QStringLiteral("굵게")); bold->setChecked(font.bold());
        italic=new QCheckBox(QStringLiteral("기울임")); italic->setChecked(font.italic());
        color=colorInput(c,widget);
        layout->addWidget(family); layout->addWidget(size); layout->addWidget(bold); layout->addWidget(italic); layout->addWidget(color);
    }
    QFont font() const { QFont f(family->currentText(),size->value()); f.setBold(bold->isChecked()); f.setItalic(italic->isChecked()); return f; }
};
}

ScanResultsDialog::ScanResultsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("스캔 결과"));
    setObjectName(QStringLiteral("ScanResultsDialog"));
    resize(1100,850);
    auto *layout=new QVBoxLayout(this);
    auto *splitter=new QSplitter;
    m_plot=new PlotWidget;
    m_plot->setObjectName(QStringLiteral("ScanResultsPlot"));
    m_plot->setMinimumSize(440,360);
    PlotWidget::StyleOptions options;
    options.lineColor=Qt::black;
    options.borderColor=Qt::black;
    options.lineStyle=Qt::DashLine;
    options.thickness=2.4;
    options.titleFont=QFont(QStringLiteral("Arial"),14,QFont::Bold);
    options.axisFont=QFont(QStringLiteral("Arial"),13);
    options.tickFont=QFont(QStringLiteral("Arial"),11);
    options.yTitle=QStringLiteral("ΔE (kcal mol⁻¹)");
    m_plot->setStyleOptions(options);
    splitter->addWidget(m_plot);
    auto *side=new QWidget;
    auto *sideLayout=new QVBoxLayout(side);
    m_table=new QTableWidget(0,2);
    m_table->setObjectName(QStringLiteral("ScanResultsTable"));
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    sideLayout->addWidget(m_table,1);
    m_table->installEventFilter(this);
    m_plot->installEventFilter(this);
    auto *copy=new QPushButton(QStringLiteral("복사")); sideLayout->addWidget(copy);
    m_highlight=new QCheckBox(QStringLiteral("고에너지 영역 강조 (표)")); sideLayout->addWidget(m_highlight);
    auto *form=new QFormLayout;
    m_threshold=realInput(20,0,1e9); m_threshold->setSuffix(QStringLiteral(" kcal/mol"));
    m_reference=new QComboBox; m_reference->addItems({QStringLiteral("전체 최솟값"),QStringLiteral("선택 지점")});
    form->addRow(QStringLiteral("기준값 (≥)"),m_threshold); form->addRow(QStringLiteral("기준"),m_reference);
    sideLayout->addLayout(form);
    auto *unit=new QLabel(QStringLiteral("에너지: kcal/mol")); sideLayout->addWidget(unit);
    splitter->addWidget(side); splitter->setStretchFactor(0,3); splitter->setStretchFactor(1,1); splitter->setSizes({760,320});
    layout->addWidget(splitter,1);
    m_optimizationStatus=new QLabel; m_optimizationStatus->setObjectName("ScanOptimizationStatus");
    layout->addWidget(m_optimizationStatus);
    m_optimizationPlot=new PlotWidget; m_optimizationPlot->setObjectName("ScanOptimizationEnergyPlot");
    m_optimizationPlot->setMinimumHeight(130); m_optimizationPlot->setMaximumHeight(180);
    layout->addWidget(m_optimizationPlot);
    m_history=new QListWidget; m_history->setObjectName("ScanOptimizationFrames");
    m_history->setFlow(QListView::LeftToRight); m_history->setMaximumHeight(60);
    m_history->setStyleSheet("QListWidget::item { padding: 6px; }");
    layout->addWidget(m_history);
    connect(m_optimizationPlot,&PlotWidget::pointClicked,this,[this](int index){
        if(index>=0 && index<m_energyFrames.size()){setSelectedFrame(m_energyFrames[index]);emit frameSelected(m_energyFrames[index]);}
    });
    connect(m_history,&QListWidget::currentRowChanged,this,[this](int row){
        if(row>=0 && row<m_historyFrames.size()){setSelectedFrame(m_historyFrames[row]);emit frameSelected(m_historyFrames[row]);}
    });
    m_status=new QLabel; m_status->setWordWrap(true); layout->addWidget(m_status);
    auto *buttons=new QHBoxLayout;
    auto *frame=new QPushButton(QStringLiteral("축 범위 설정"));
    auto *style=new QPushButton(QStringLiteral("스타일"));
    style->setObjectName(QStringLiteral("EditScanStyle"));
    auto *axisTitles=new QPushButton(QStringLiteral("축 제목"));
    axisTitles->setObjectName(QStringLiteral("EditScanAxisTitles"));
    m_mechanism=new QPushButton(QStringLiteral("Mechanism 단계"));
    m_mechanism->setObjectName(QStringLiteral("EditMechanismStages"));
    auto *picture=new QPushButton(QStringLiteral("그림 복사"));
    auto *exportFigure=new QPushButton(QStringLiteral("논문용 내보내기…"));
    exportFigure->setObjectName(QStringLiteral("ExportPublicationFigure"));
    auto *close=new QPushButton(QStringLiteral("닫기"));
    for(auto *button:{frame,axisTitles,style,m_mechanism,picture,exportFigure,close}) { button->setAutoDefault(false); buttons->addWidget(button); }
    layout->addLayout(buttons);
    connect(close,&QPushButton::clicked,this,&QDialog::close);
    connect(style,&QPushButton::clicked,this,&ScanResultsDialog::editStyle);
    connect(frame,&QPushButton::clicked,this,&ScanResultsDialog::editFrame);
    connect(axisTitles,&QPushButton::clicked,this,&ScanResultsDialog::editAxisTitles);
    connect(m_mechanism,&QPushButton::clicked,this,&ScanResultsDialog::editMechanism);
    connect(picture,&QPushButton::clicked,this,[this]{ QApplication::clipboard()->setImage(m_plot->publicationImage(PlotWidget::ExportOptions{})); });
    connect(exportFigure,&QPushButton::clicked,m_plot,&PlotWidget::showExportDialog);
    connect(copy,&QPushButton::clicked,this,[this]{
        QString text;
        if(m_scanDimensions==2) {
            text=m_scanAxisLabel+QLatin1Char('\t')+m_scanAxisLabel2+QStringLiteral("\tEnergy (kcal/mol)\n");
            for(const auto &p:m_points) text+=number(p.coordinate,12)+QLatin1Char('\t')+number(p.coordinate2,12)
                +QLatin1Char('\t')+number(p.relativeKcal,12)+QLatin1Char('\n');
        } else {
            text=m_table->horizontalHeaderItem(0)->text()+QStringLiteral("\tEnergy (kcal/mol)\n");
            for(const auto &p:m_points) text+=number(p.coordinate,12)+QLatin1Char('\t')+number(p.relativeKcal,12)+QLatin1Char('\n');
        }
        QApplication::clipboard()->setText(text);
    });
    connect(m_plot,&PlotWidget::pointClicked,this,[this](int index){ setSelectedPoint(index); emit pointSelected(index); });
    connect(m_table,&QTableWidget::currentCellChanged,this,[this](int row,int,int,int){
        if(row<0) return;
        setSelectedPoint(row); emit pointSelected(row);
    });
    connect(m_highlight,&QCheckBox::toggled,this,&ScanResultsDialog::updateHighlight);
    connect(m_threshold,&QDoubleSpinBox::valueChanged,this,&ScanResultsDialog::updateHighlight);
    connect(m_reference,&QComboBox::currentIndexChanged,this,&ScanResultsDialog::updateHighlight);
}

bool ScanResultsDialog::navigatePoint(QKeyEvent *event) {
    if (event->modifiers() != Qt::NoModifier ||
        (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right)) return false;
    if (!m_points.isEmpty()) {
        const int index = qBound(0, m_selected + (event->key() == Qt::Key_Right ? 1 : -1), int(m_points.size()) - 1);
        if (index != m_selected) {
            setSelectedPoint(index);
            emit pointSelected(index);
        }
    }
    event->accept();
    return true;
}

bool ScanResultsDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress && navigatePoint(static_cast<QKeyEvent *>(event))) return true;
    return QDialog::eventFilter(watched, event);
}

void ScanResultsDialog::keyPressEvent(QKeyEvent *event) {
    if (!navigatePoint(event)) QDialog::keyPressEvent(event);
}

void ScanResultsDialog::setData(const CalculationData &data,int selected) {
    const QSignalBlocker blocker(m_table);
    m_points=data.scanPoints;
    m_frames=data.frames;
    m_scanDimensions=data.scanDimensions;
    m_mechanism->setEnabled(m_scanDimensions != 2);
    m_scanAxisLabel=data.scanAxisLabel;
    m_scanAxisLabel2=data.scanAxisLabel2;
    m_selected=-1;
    m_table->setColumnCount(m_scanDimensions==2?3:2);
    m_table->setRowCount(int(m_points.size()));
    m_table->setHorizontalHeaderLabels(m_scanDimensions==2
        ? QStringList{data.scanAxisLabel,data.scanAxisLabel2,QStringLiteral("에너지")}
        : QStringList{data.scanAxisLabel,QStringLiteral("에너지")});
    QVector<QPointF> series;
    QVector<PlotWidget::HeatmapPoint> heatmap;
    for(int i=0;i<m_points.size();++i) {
        const auto &p=m_points[i]; series.push_back({p.coordinate,p.relativeKcal});
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(QStringLiteral("점 %1%2").arg(p.index)
            .arg(p.inProgress ? QStringLiteral(" · 진행 중") : QString())));
        if(m_scanDimensions==2) heatmap.push_back({p.coordinate,p.coordinate2,p.relativeKcal,i,p.energyHartree});
        const int columns=m_scanDimensions==2?3:2;
        for(int col=0;col<columns;++col) {
            const double value=col==0?p.coordinate:(m_scanDimensions==2&&col==1?p.coordinate2:p.relativeKcal);
            auto *item=new QTableWidgetItem(number(value,9));
            item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            item->setToolTip(data.format == QStringLiteral("GSM")
                ? QStringLiteral("ΔE = %1 kcal/mol").arg(number(p.relativeKcal,9))
                : QStringLiteral("E = %1 Hartree").arg(number(p.energyHartree,14)));
            m_table->setItem(i,col,item);
        }
    }
    auto style=m_plot->styleOptions();
    QSettings settings;
    settings.beginGroup(m_scanDimensions==2?QStringLiteral("scan/axisTitles/2d"):QStringLiteral("scan/axisTitles/1d"));
    style.xTitle=settings.value(QStringLiteral("x"),data.scanAxisLabel).toString();
    style.yTitle=settings.value(QStringLiteral("y"),m_scanDimensions==2?data.scanAxisLabel2:QStringLiteral("ΔE (kcal mol⁻¹)")).toString();
    m_plot->setStyleOptions(style);
    if(m_scanDimensions==2)
        m_plot->setHeatmapSeries(heatmap,style.xTitle,style.yTitle,QStringLiteral("ΔE\n(kcal mol⁻¹)"));
    else
        m_plot->setScanSeries(series,style.xTitle,style.yTitle);
    if (m_scanDimensions != 2 && !m_mechanismInitialized && !series.isEmpty()) {
        double minimum=std::numeric_limits<double>::infinity(), maximum=-minimum;
        for (const auto &point:series) if (std::isfinite(point.x()) && std::isfinite(point.y())) {
            minimum=std::min(minimum,point.x()); maximum=std::max(maximum,point.x());
        }
        if (minimum==maximum) { const auto a=m_plot->autoRange(); minimum=a.xMin; maximum=a.xMax; }
        style.mechanismStages=PlotWidget::defaultMechanismStages(minimum,maximum);
        style.showMechanism=false;
        m_plot->setStyleOptions(style);
        m_mechanismInitialized=!style.mechanismStages.isEmpty();
    }
    m_plot->setEmptyMessage(data.scanStatusText());
    m_status->setText(data.scanStatusText());
    if(!m_points.isEmpty()) setSelectedPoint(selected>=0&&selected<m_points.size()?selected:0);
    if(m_points.isEmpty()) updateOptimization();
    updateHighlight();
}

void ScanResultsDialog::setSelectedPoint(int index) {
    if(index<0||index>=m_points.size()) return;
    const bool changed=m_selected!=index;
    m_selected=index;
    const QSignalBlocker blocker(m_table);
    m_table->setCurrentCell(index,0);
    m_plot->setSelectedPoint(index);
    if(m_scanAxisLabel == QStringLiteral("GSM node")) {
        m_status->setText(QStringLiteral("GSM 노드 %1 | ΔE = %2 kcal/mol")
            .arg(m_points[index].index).arg(number(m_points[index].relativeKcal,10)));
    } else if(m_scanDimensions==2) {
        m_status->setText(QStringLiteral("지점 %1 | %2 = %3 | %4 = %5 | E = %6 Hartree | ΔE = %7 kcal/mol")
            .arg(m_points[index].index).arg(m_scanAxisLabel,number(m_points[index].coordinate,9),
                 m_scanAxisLabel2,number(m_points[index].coordinate2,9),number(m_points[index].energyHartree,14),
                 number(m_points[index].relativeKcal,10)));
    } else {
        m_status->setText(QStringLiteral("지점 %1 | E = %2 Hartree | ΔE = %3 kcal/mol")
            .arg(m_points[index].index).arg(number(m_points[index].energyHartree,14),number(m_points[index].relativeKcal,10)));
    }
    if (m_points[index].inProgress) m_status->setText(m_status->text() + QStringLiteral(" | 최적화 진행 중 (파일 기준)"));
    if(changed) updateOptimization();
    updateHighlight();
}

void ScanResultsDialog::setSelectedFrame(int index) {
    const QSignalBlocker blocker(m_history);
    m_history->setCurrentRow(m_historyFrames.indexOf(index));
    m_optimizationPlot->setSelectedPoint(m_energyFrames.indexOf(index));
}

void ScanResultsDialog::updateOptimization() {
    const QSignalBlocker blocker(m_history);
    m_history->clear(); m_historyFrames.clear(); m_energyFrames.clear();
    QVector<QPointF> series;
    double reference=kNaN;
    if(m_selected>=0 && m_selected<m_points.size()) {
        const auto &point=m_points[m_selected];
        auto frames=point.optimizationFrames;
        if(frames.isEmpty() && point.frameIndex>=0)frames<<point.frameIndex;
        for(int frame:frames) {
            if(frame<0 || frame>=m_frames.size())continue;
            const int step=m_historyFrames.size()+1;
            m_historyFrames<<frame;
            const double energy=m_frames[frame].metrics.energyHartree;
            m_history->addItem(QStringLiteral("단계 %1 · %2").arg(step).arg(std::isfinite(energy)?QString::number(energy,'f',8)+" Eh":QStringLiteral("SCF 이전")));
            if(std::isfinite(energy)) {
                if(!std::isfinite(reference))reference=energy;
                m_energyFrames<<frame; series<<QPointF(step,(energy-reference)*627.509474);
            }
        }
        m_optimizationStatus->setText(QStringLiteral("Scan 점 %1 · %2 · 최적화 %3단계 · 첫 단계 기준 ΔE · 클릭하여 구조 보기")
            .arg(point.index).arg(point.inProgress?QStringLiteral("진행 중"):QStringLiteral("완료")).arg(m_historyFrames.size()));
    } else m_optimizationStatus->setText(QStringLiteral("최적화 이력 없음"));
    m_optimizationPlot->setSeries(series,"Optimization step",QStringLiteral("ΔE (kcal mol⁻¹)"));
    m_optimizationPlot->setMarkers(series);
    m_optimizationPlot->setEmptyMessage(QStringLiteral("최적화 단계 에너지 데이터 없음"));
    setSelectedFrame(m_selected>=0?m_points[m_selected].frameIndex:-1);
}

void ScanResultsDialog::updateHighlight() {
    double reference=std::numeric_limits<double>::infinity();
    if(m_reference->currentIndex()==1) {
        if(m_selected>=0) reference=m_points[m_selected].relativeKcal;
    } else for(const auto &p:m_points) if(std::isfinite(p.relativeKcal)) reference=std::min(reference,p.relativeKcal);
    // Subtract the chosen reference in display space; never mutate parsed scan data.
    const double cutoff=reference+m_threshold->value();
    const bool enabled=m_highlight->isChecked()&&std::isfinite(reference);
    m_plot->setHighlightRule(false,cutoff); // Red on the graph is reserved for the TS candidate.
    for(int i=0;i<m_points.size();++i) for(int col=0;col<m_table->columnCount();++col)
        m_table->item(i,col)->setForeground(enabled&&std::isfinite(m_points[i].relativeKcal)&&m_points[i].relativeKcal>=cutoff
            ? QBrush(Qt::red) : QBrush());
}

void ScanResultsDialog::saveAxisTitles() {
    QSettings settings;
    settings.beginGroup(m_scanDimensions==2?QStringLiteral("scan/axisTitles/2d"):QStringLiteral("scan/axisTitles/1d"));
    settings.setValue(QStringLiteral("x"),m_plot->styleOptions().xTitle);
    settings.setValue(QStringLiteral("y"),m_plot->styleOptions().yTitle);
    settings.sync();
}

void ScanResultsDialog::editAxisTitles() {
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("ScanAxisTitlesDialog"));
    dialog.setWindowTitle(QStringLiteral("축 제목"));
    auto *layout=new QVBoxLayout(&dialog);
    auto *form=new QFormLayout;
    auto *x=new QLineEdit(m_plot->styleOptions().xTitle);
    auto *y=new QLineEdit(m_plot->styleOptions().yTitle);
    x->setObjectName(QStringLiteral("ScanXAxisTitle"));
    y->setObjectName(QStringLiteral("ScanYAxisTitle"));
    x->setMinimumWidth(320);
    form->addRow(QStringLiteral("X축 제목"),x);
    form->addRow(QStringLiteral("Y축 제목"),y);
    layout->addLayout(form);
    auto *help=new QLabel(QStringLiteral("저장한 제목은 프로그램을 다시 실행해도 유지됩니다."));
    layout->addWidget(help);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("저장"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if (dialog.exec()==QDialog::Accepted) {
        auto style=m_plot->styleOptions();
        style.xTitle=x->text(); style.yTitle=y->text();
        m_plot->setStyleOptions(style);
        saveAxisTitles();
    }
}

void ScanResultsDialog::editFrame() {
    QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("축 범위 설정"));
    auto *layout=new QVBoxLayout(&dialog); auto *form=new QFormLayout;
    const auto a=m_plot->axisRange();
    auto *xmin=realInput(a.xMin,-1e100,1e100,9), *xmax=realInput(a.xMax,-1e100,1e100,9);
    auto *ymin=realInput(a.yMin,-1e100,1e100,9), *ymax=realInput(a.yMax,-1e100,1e100,9);
    form->addRow(QStringLiteral("X 최솟값"),xmin); form->addRow(QStringLiteral("X 최댓값"),xmax);
    form->addRow(QStringLiteral("Y 최솟값"),ymin); form->addRow(QStringLiteral("Y 최댓값"),ymax); layout->addLayout(form);
    auto *error=new QLabel; error->setText(QStringLiteral("두 축 모두 최솟값이 최댓값보다 작아야 합니다.")); layout->addWidget(error);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    auto *automatic=buttons->addButton(QStringLiteral("자동"),QDialogButtonBox::ResetRole); layout->addWidget(buttons);
    bool autoRequested=false;
    auto validate=[&]{
        autoRequested=false;
        const bool valid=xmin->value()<xmax->value()&&ymin->value()<ymax->value();
        buttons->button(QDialogButtonBox::Ok)->setEnabled(valid); error->setVisible(!valid);
    };
    for(auto *input:{xmin,xmax,ymin,ymax}) connect(input,&QDoubleSpinBox::valueChanged,&dialog,validate);
    validate();
    connect(automatic,&QPushButton::clicked,&dialog,[&]{
        const auto bounds=m_plot->autoRange();
        xmin->setValue(bounds.xMin); xmax->setValue(bounds.xMax); ymin->setValue(bounds.yMin); ymax->setValue(bounds.yMax);
        autoRequested=true;
    });
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted) {
        if(autoRequested) m_plot->clearManualRange();
        else m_plot->setManualRange({xmin->value(),xmax->value(),ymin->value(),ymax->value()});
    }
}

void ScanResultsDialog::editMechanism() {
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("MechanismStagesDialog"));
    dialog.setWindowTitle(QStringLiteral("Mechanism 단계"));
    dialog.resize(740,480);
    const auto original=m_plot->styleOptions();
    auto *layout=new QVBoxLayout(&dialog);
    auto *enabled=new QCheckBox(QStringLiteral("단계 범위 표시"));
    enabled->setChecked(original.showMechanism); layout->addWidget(enabled);
    auto *help=new QLabel(QStringLiteral("← Reactant →    ← Transition state →    ← Product →\n"
        "텍스트 양옆에 화살표를 표시합니다. 시작·끝은 그래프의 실제 X 좌표입니다. 좁은 구간의 단계 이름은 두 줄로 표시합니다."));
    help->setWordWrap(true); layout->addWidget(help);
    auto *table=new QTableWidget(0,3);
    table->setObjectName(QStringLiteral("MechanismStagesTable"));
    table->setHorizontalHeaderLabels({QStringLiteral("단계 이름"),QStringLiteral("X 시작"),QStringLiteral("X 끝")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(table,1);
    auto *rowButtons=new QHBoxLayout;
    auto *add=new QPushButton(QStringLiteral("단계 추가"));
    auto *remove=new QPushButton(QStringLiteral("선택 단계 삭제"));
    auto *reset=new QPushButton(QStringLiteral("기본 3단계로 복원"));
    for (auto *button:{add,remove,reset}) { button->setAutoDefault(false); rowButtons->addWidget(button); }
    layout->addLayout(rowButtons);
    auto *form=new QFormLayout;
    FontControls font(original.mechanismFont,original.mechanismColor,&dialog);
    auto *offset=realInput(original.mechanismOffset,10,100,1);
    offset->setSuffix(QStringLiteral(" px"));
    form->addRow(QStringLiteral("글꼴·색상"),font.widget);
    form->addRow(QStringLiteral("단계 여백"),offset); layout->addLayout(form);
    auto *error=new QLabel(QStringLiteral("단계 이름을 입력하고 X 시작을 X 끝보다 작게 지정하세요."));
    error->setObjectName(QStringLiteral("MechanismStagesError"));
    layout->addWidget(error);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));
    buttons->button(QDialogButtonBox::Apply)->setText(QStringLiteral("적용"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소")); layout->addWidget(buttons);
    auto validate=[&] {
        bool valid=true;
        for (int row=0;row<table->rowCount();++row) {
            auto *start=qobject_cast<QDoubleSpinBox *>(table->cellWidget(row,1));
            auto *end=qobject_cast<QDoubleSpinBox *>(table->cellWidget(row,2));
            valid=valid && table->item(row,0) && !table->item(row,0)->text().trimmed().isEmpty()
                && start && end && start->value()<end->value();
        }
        error->setVisible(!valid);
        buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
        buttons->button(QDialogButtonBox::Apply)->setEnabled(valid);
        remove->setEnabled(table->currentRow()>=0);
    };
    auto append=[&](const PlotWidget::MechanismStage &stage) {
        const int row=table->rowCount(); table->insertRow(row);
        table->setItem(row,0,new QTableWidgetItem(stage.text));
        auto *start=realInput(stage.start,-1e100,1e100,9), *end=realInput(stage.end,-1e100,1e100,9);
        table->setCellWidget(row,1,start); table->setCellWidget(row,2,end);
        connect(start,&QDoubleSpinBox::valueChanged,&dialog,validate);
        connect(end,&QDoubleSpinBox::valueChanged,&dialog,validate);
        table->setCurrentCell(row,0); validate();
    };
    for (const auto &stage:original.mechanismStages) append(stage);
    connect(table,&QTableWidget::itemChanged,&dialog,validate);
    connect(table,&QTableWidget::itemSelectionChanged,&dialog,validate);
    connect(add,&QPushButton::clicked,&dialog,[&] {
        const auto a=m_plot->axisRange();
        append({QStringLiteral("Stage %1").arg(table->rowCount()+1),a.xMin,a.xMax});
    });
    connect(remove,&QPushButton::clicked,&dialog,[&] { table->removeRow(table->currentRow()); validate(); });
    connect(reset,&QPushButton::clicked,&dialog,[&] {
        const QSignalBlocker blocker(table);
        table->setRowCount(0); const auto a=m_plot->axisRange();
        for (const auto &stage:PlotWidget::defaultMechanismStages(a.xMin,a.xMax)) append(stage);
        enabled->setChecked(true); validate();
    });
    auto apply=[&] {
        auto style=original;
        style.showMechanism=enabled->isChecked(); style.mechanismStages.clear();
        style.mechanismFont=font.font(); style.mechanismColor=font.color->property("color").value<QColor>();
        style.mechanismOffset=offset->value();
        for (int row=0;row<table->rowCount();++row)
            style.mechanismStages.push_back({table->item(row,0)->text().trimmed(),
                qobject_cast<QDoubleSpinBox *>(table->cellWidget(row,1))->value(),
                qobject_cast<QDoubleSpinBox *>(table->cellWidget(row,2))->value()});
        m_plot->setStyleOptions(style);
    };
    connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,apply);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] { apply(); dialog.accept(); });
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    validate();
    if (dialog.exec()!=QDialog::Accepted) m_plot->setStyleOptions(original);
}

void ScanResultsDialog::editStyle() {
    QDialog dialog(this); dialog.setObjectName(QStringLiteral("ScanStyleDialog")); dialog.setWindowTitle(QStringLiteral("스타일")); dialog.resize(720,570);
    auto *layout=new QVBoxLayout(&dialog); auto *tabs=new QTabWidget; layout->addWidget(tabs);
    auto page=[&](const QString &title){ auto *widget=new QWidget; auto *form=new QFormLayout(widget); form->setSpacing(12); tabs->addTab(widget,title); return form; };
    auto check=[](bool value){ auto *box=new QCheckBox; box->setChecked(value); return box; };
    const auto original=m_plot->styleOptions();
    auto *general=page(QStringLiteral("일반"));
    auto *background=colorInput(original.background,&dialog);
    auto *title=new QLineEdit(original.title);
    auto *showTs=check(original.showTsAnnotation);
    auto *tsLabel=new QLineEdit(original.tsLabel);
    general->addRow(QStringLiteral("TS 후보 주석 표시"),showTs);
    general->addRow(QStringLiteral("TS 후보 주석"),tsLabel);
    FontControls titleFont(original.titleFont,original.titleColor,&dialog);
    general->addRow(QStringLiteral("배경 색상"),background); general->addRow(QStringLiteral("그래프 제목"),title);
    general->addRow(QStringLiteral("제목 글꼴"),titleFont.widget);
    auto *lines=page(QStringLiteral("선"));
    auto *showLine=check(original.showLine); auto *lineStyle=new QComboBox;
    lineStyle->addItems({QStringLiteral("실선"),QStringLiteral("파선")}); lineStyle->setCurrentIndex(original.lineStyle==Qt::DashLine?1:0);
    auto *thickness=realInput(original.thickness,0.25,12,2); auto *lineColor=colorInput(original.lineColor,&dialog);
    auto *points=check(original.showPoints); auto *pointSize=realInput(original.pointSize,2,30,1);
    auto *border=check(original.pointBorder); auto *pointColor=colorInput(original.pointColor,&dialog); auto *borderColor=colorInput(original.borderColor,&dialog);
    lines->addRow(QStringLiteral("선 표시"),showLine); lines->addRow(QStringLiteral("선 스타일"),lineStyle);
    lines->addRow(QStringLiteral("굵기"),thickness); lines->addRow(QStringLiteral("색상"),lineColor);
    lines->addRow(QStringLiteral("점 표시"),points); lines->addRow(QStringLiteral("점 크기"),pointSize);
    lines->addRow(QStringLiteral("점 모양"),new QLabel(QStringLiteral("원")));
    lines->addRow(QStringLiteral("테두리"),border); lines->addRow(QStringLiteral("점 색상"),pointColor); lines->addRow(QStringLiteral("테두리 색상"),borderColor);
    auto *axes=page(QStringLiteral("축"));
    auto *showX=check(original.showX), *showY=check(original.showY), *labels=check(original.tickLabels);
    auto *xTitle=new QLineEdit(original.xTitle), *yTitle=new QLineEdit(original.yTitle);
    xTitle->setObjectName(QStringLiteral("ScanXAxisTitle")); yTitle->setObjectName(QStringLiteral("ScanYAxisTitle"));
    FontControls axisFont(original.axisFont,original.axisColor,&dialog), tickFont(original.tickFont,original.tickColor,&dialog);
    auto *major=new QSpinBox; major->setRange(2,20); major->setValue(original.majorTicks);
    auto *minor=new QSpinBox; minor->setRange(0,10); minor->setValue(original.minorTicks);
    auto *custom=check(original.customIncrement); auto *increment=realInput(original.increment,0.000001,1e9,6);
    auto *xIncrement=realInput(original.xIncrement,0,1e9,6);
    auto *yIncrement=realInput(original.yIncrement,0,1e9,6);
    xIncrement->setSpecialValueText(QStringLiteral("자동")); yIncrement->setSpecialValueText(QStringLiteral("자동"));
    increment->setEnabled(original.customIncrement); major->setEnabled(!original.customIncrement);
    increment->setToolTip(QStringLiteral("두 축의 눈금 간격입니다. 촘촘한 눈금은 축마다 최대 200개로 줄여 표시합니다."));
    connect(custom,&QCheckBox::toggled,increment,&QDoubleSpinBox::setEnabled);
    connect(custom,&QCheckBox::toggled,major,[major](bool on){major->setEnabled(!on);});
    auto *majorGrid=check(original.majorGrid), *minorGrid=check(original.minorGrid);
    axes->addRow(QStringLiteral("X축 표시"),showX); axes->addRow(QStringLiteral("Y축 표시"),showY);
    axes->addRow(QStringLiteral("X축 제목"),xTitle); axes->addRow(QStringLiteral("Y축 제목"),yTitle);
    axes->addRow(QStringLiteral("축 제목 글꼴"),axisFont.widget); axes->addRow(QStringLiteral("눈금 라벨"),labels); axes->addRow(QStringLiteral("눈금 라벨 글꼴"),tickFont.widget);
    axes->addRow(QStringLiteral("주 눈금 개수"),major); axes->addRow(QStringLiteral("구간당 보조 눈금 개수"),minor);
    axes->addRow(QStringLiteral("눈금 간격 직접 설정"),custom); axes->addRow(QStringLiteral("눈금 간격"),increment);
    axes->addRow(QStringLiteral("X 눈금 간격 (0: 자동)"),xIncrement);
    axes->addRow(QStringLiteral("Y 눈금 간격 (0: 자동)"),yIncrement);
    axes->addRow(QStringLiteral("주 격자 표시"),majorGrid); axes->addRow(QStringLiteral("보조 격자 표시"),minorGrid);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons->button(QDialogButtonBox::Apply)->setText(QStringLiteral("적용"));buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소")); layout->addWidget(buttons);
    auto apply=[&]{
        auto s=original;
        auto color=[](QPushButton *button){return button->property("color").value<QColor>();};
        s.showTsAnnotation=showTs->isChecked(); s.tsLabel=tsLabel->text();
        s.background=color(background); s.title=title->text(); s.titleFont=titleFont.font(); s.titleColor=color(titleFont.color);
        s.showLine=showLine->isChecked(); s.lineStyle=lineStyle->currentIndex()==0?Qt::SolidLine:Qt::DashLine;
        s.thickness=thickness->value(); s.lineColor=color(lineColor); s.showPoints=points->isChecked(); s.pointSize=pointSize->value();
        s.pointBorder=border->isChecked(); s.pointColor=color(pointColor); s.borderColor=color(borderColor);
        s.showX=showX->isChecked(); s.showY=showY->isChecked(); s.xTitle=xTitle->text(); s.yTitle=yTitle->text();
        s.axisFont=axisFont.font(); s.axisColor=color(axisFont.color); s.tickFont=tickFont.font(); s.tickColor=color(tickFont.color);
        s.tickLabels=labels->isChecked(); s.majorTicks=major->value(); s.minorTicks=minor->value();
        s.xIncrement=xIncrement->value(); s.yIncrement=yIncrement->value();
        s.customIncrement=custom->isChecked(); s.increment=increment->value(); s.majorGrid=majorGrid->isChecked(); s.minorGrid=minorGrid->isChecked();
        m_plot->setStyleOptions(s);
    };
    connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,apply);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{apply();dialog.accept();});
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted) m_plot->setStyleOptions(original);
    else if (m_plot->styleOptions().xTitle!=original.xTitle || m_plot->styleOptions().yTitle!=original.yTitle)
        saveAxisTitles();
}

} // namespace youngcraft
