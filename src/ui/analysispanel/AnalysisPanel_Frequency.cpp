#include "ui/SidebarButton.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

namespace youngcraft {
namespace {
QString numberOrDash(double value, int decimals=4) {
    return std::isfinite(value)?QString::number(value,'f',decimals):QStringLiteral("—");
}
constexpr int frequencyPageSize=5;
constexpr int frequencyRowHeight=26;
}

QWidget *AnalysisPanel::makeFrequencyTab() {
    auto *page=new QWidget;
    page->setObjectName("FrequencyPage");
    auto *layout=new QVBoxLayout(page);
    layout->setContentsMargins(4,4,4,4);layout->setSpacing(6);
    auto *toolbar=new QWidget(page);toolbar->setObjectName("FrequencyToolbar");
    auto *playback=new QHBoxLayout(toolbar);playback->setContentsMargins(0,0,0,0);playback->setSpacing(4);
    const auto button=[&](const QString &name,const QString &label,QStyle::StandardPixmap icon){
        auto *result=new QPushButton(label,toolbar);result->setObjectName(name);
        makeSidebarIconButton(result,icon);playback->addWidget(result);return result;
    };
    auto *play=button("FrequencyPlay",QStringLiteral("재생 / 일시정지"),QStyle::SP_MediaPlay);
    auto *restore=button("FrequencyRestore",QStringLiteral("원래 구조"),QStyle::SP_MediaStop);
    restore->setToolTip(QStringLiteral("정지 · 원래 구조"));
    auto *save=button("SaveFrequencyAnimation",QStringLiteral("애니메이션 저장"),QStyle::SP_DialogSaveButton);
    save->setIcon(sidebarSaveIcon());
    m_nextFrequencyPage=button("NextFrequencyPage",QStringLiteral("다음 진동수 5개"),QStyle::SP_MediaSkipForward);
    auto *details=button("OpenFrequencyDetails",QStringLiteral("진동 상세 정보"),QStyle::SP_FileDialogDetailedView);
    playback->addStretch();layout->addWidget(toolbar);
    m_frequencyPageLabel=new QLabel(page);m_frequencyPageLabel->setObjectName("FrequencyPageRange");
    layout->addWidget(m_frequencyPageLabel);
    m_frequencyList=new QListWidget(page);m_frequencyList->setObjectName("FrequencyList");
    m_frequencyList->setMinimumWidth(0);m_frequencyList->setUniformItemSizes(true);
    m_frequencyList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_frequencyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_frequencyList->setFixedHeight(frequencyPageSize*frequencyRowHeight+2);
    layout->addWidget(m_frequencyList);
    auto *form=new QFormLayout;form->setVerticalSpacing(6);
    m_amplitude=new QDoubleSpinBox(page);m_amplitude->setObjectName("FrequencyAmplitude");
    m_amplitude->setRange(0,3);m_amplitude->setSingleStep(.05);m_amplitude->setValue(.5);
    m_cycleTime=new QDoubleSpinBox(page);m_cycleTime->setObjectName("FrequencyCycleTime");
    m_cycleTime->setRange(.2,20);m_cycleTime->setSingleStep(.1);m_cycleTime->setValue(2);m_cycleTime->setSuffix(QStringLiteral(" 초"));
    form->addRow(QStringLiteral("진동 스케일"),m_amplitude);form->addRow(QStringLiteral("주기"),m_cycleTime);
    m_recountBonds=new QCheckBox(QStringLiteral("결합 다시 계산"),page);m_recountBonds->setChecked(true);
    m_vectors=new QCheckBox(QStringLiteral("변위 벡터 표시"),page);
    m_scaledVectors=new QCheckBox(QStringLiteral("스케일 벡터"),page);m_scaledVectors->setEnabled(false);m_scaledVectors->setObjectName("FrequencyScaledVectors");
    form->addRow(m_recountBonds);form->addRow(m_vectors);form->addRow(m_scaledVectors);
    layout->addLayout(form);layout->addStretch();

    m_frequencyDetails=new QDialog(page);m_frequencyDetails->setObjectName("FrequencyDetailsDialog");
    m_frequencyDetails->setWindowTitle(QStringLiteral("진동 상세 정보"));m_frequencyDetails->resize(760,600);
    auto *detailLayout=new QVBoxLayout(m_frequencyDetails);
    m_frequencyInfo=new QLabel(QStringLiteral("진동 모드를 선택하세요"),m_frequencyDetails);m_frequencyInfo->setWordWrap(true);
    detailLayout->addWidget(m_frequencyInfo);
    m_frequencyDetailsTable=new QTableWidget(0,5,m_frequencyDetails);m_frequencyDetailsTable->setObjectName("FrequencyDetailsTable");
    m_frequencyDetailsTable->setHorizontalHeaderLabels({QStringLiteral("모드"),QStringLiteral("파수 (cm⁻¹)"),QStringLiteral("IR 세기"),QStringLiteral("Raman 활성도"),QStringLiteral("상태")});
    m_frequencyDetailsTable->verticalHeader()->hide();
    m_frequencyDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_frequencyDetailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);m_frequencyDetailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_frequencyDetailsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    detailLayout->addWidget(m_frequencyDetailsTable,2);
    m_frequencyPlot=new PlotWidget(m_frequencyDetails);detailLayout->addWidget(m_frequencyPlot,1);
    m_copyFrequencyAnimation=new QPushButton(QStringLiteral("TS 애니메이션 복사"),m_frequencyDetails);
    m_copyFrequencyAnimation->setObjectName("CopyFrequencyAnimation");
    m_copyFrequencyAnimation->setToolTip(QStringLiteral("선택한 진동 모드를 투명 배경 GIF 애니메이션으로 복사합니다."));
    detailLayout->addWidget(m_copyFrequencyAnimation);m_copyFrequencyAnimation->hide();
    auto *close=new QDialogButtonBox(QDialogButtonBox::Close,m_frequencyDetails);
    close->button(QDialogButtonBox::Close)->setText(QStringLiteral("닫기"));detailLayout->addWidget(close);
    connect(close,&QDialogButtonBox::rejected,m_frequencyDetails,&QDialog::hide);
    connect(details,&QPushButton::clicked,this,[this]{
        m_frequencyDetails->show();m_frequencyDetails->raise();m_frequencyDetails->activateWindow();
    });
    connect(m_nextFrequencyPage,&QPushButton::clicked,this,[this]{
        const int next=m_frequencyPage+frequencyPageSize>=m_data.frequencies.size()?0:m_frequencyPage+frequencyPageSize;
        updateFrequencyPage(next);m_frequencyList->setCurrentRow(next);
    });
    connect(m_frequencyDetailsTable,&QTableWidget::currentCellChanged,this,[this](int row,int,int,int){
        if(row>=0 && row<m_data.frequencies.size()){setCurrentFrequency(row);emit frequencyRequested(row);}
    });
    connect(m_frequencyList,&QListWidget::currentRowChanged,this,[this](int row){
        if(row<0)return;
        setCurrentFrequency(row);m_frequencySelectionChanged=true;emit frequencyRequested(row);
        QTimer::singleShot(0,this,[this]{m_frequencySelectionChanged=false;});
    });
    connect(m_frequencyList,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){
        if(!m_frequencySelectionChanged)emit frequencyRequested(m_frequencyList->row(item));
    });
    connect(play,&QPushButton::clicked,this,&AnalysisPanel::frequencyPlayRequested);
    connect(restore,&QPushButton::clicked,this,&AnalysisPanel::frequencyRestoreRequested);
    connect(save,&QPushButton::clicked,this,&AnalysisPanel::animationSaveRequested);
    connect(m_copyFrequencyAnimation,&QPushButton::clicked,this,&AnalysisPanel::animationCopyRequested);
    connect(m_amplitude,qOverload<double>(&QDoubleSpinBox::valueChanged),this,&AnalysisPanel::frequencyAmplitudeChanged);
    connect(m_cycleTime,qOverload<double>(&QDoubleSpinBox::valueChanged),this,&AnalysisPanel::frequencyCycleTimeChanged);
    connect(m_recountBonds,&QCheckBox::toggled,this,&AnalysisPanel::recountBondsChanged);
    const auto vectorChanged=[this]{emit vibrationVectorsChanged(m_vectors->isChecked(),m_scaledVectors->isChecked());};
    connect(m_vectors,&QCheckBox::toggled,this,[this,vectorChanged](bool enabled){m_scaledVectors->setEnabled(enabled);vectorChanged();});
    connect(m_scaledVectors,&QCheckBox::toggled,this,vectorChanged);
    updateFrequencyPage(0);
    return page;
}

double AnalysisPanel::frequencyAmplitude() const { return m_amplitude?m_amplitude->value():.5; }
double AnalysisPanel::frequencyCycleTime() const { return m_cycleTime?m_cycleTime->value():2; }
bool AnalysisPanel::recountBondsEnabled() const { return m_recountBonds&&m_recountBonds->isChecked(); }
bool AnalysisPanel::scaledVectorsEnabled() const { return m_scaledVectors&&m_scaledVectors->isChecked(); }

void AnalysisPanel::updateFrequencyPage(int first) {
    const int total=m_frequencyList->count();
    m_frequencyPage=total?qBound(0,first,total-1)/frequencyPageSize*frequencyPageSize:0;
    for(int i=0;i<total;++i)m_frequencyList->item(i)->setHidden(i<m_frequencyPage||i>=m_frequencyPage+frequencyPageSize);
    m_frequencyPageLabel->setText(total?QStringLiteral("%1–%2 / %3").arg(m_frequencyPage+1).arg(qMin(total,m_frequencyPage+frequencyPageSize)).arg(total):QStringLiteral("진동수 없음"));
    m_nextFrequencyPage->setEnabled(total>frequencyPageSize);
    const bool wrap=m_frequencyPage+frequencyPageSize>=total;
    m_nextFrequencyPage->setToolTip(wrap?QStringLiteral("처음 진동수 5개로 돌아가기"):QStringLiteral("다음 진동수 5개"));
    m_frequencyList->scrollToTop();
}

void AnalysisPanel::fillFrequencyList() {
    const QSignalBlocker listBlock(m_frequencyList),tableBlock(m_frequencyDetailsTable);
    m_frequencyList->clear();m_frequencyDetailsTable->setRowCount(int(m_data.frequencies.size()));
    bool hasImaginary=false;QVector<QPointF> bars;
    for(int row=0;row<m_data.frequencies.size();++row){
        const auto &mode=m_data.frequencies[row];const bool imaginary=mode.imaginary||mode.wavenumber<0;
        hasImaginary|=imaginary;
        auto *entry=new QListWidgetItem(QStringLiteral("%1: %2 cm⁻¹%3").arg(mode.number).arg(numberOrDash(mode.wavenumber,3)).arg(imaginary?QStringLiteral(" [허수]"):QString()),m_frequencyList);
        entry->setSizeHint(QSize(0,frequencyRowHeight));
        const QStringList values={QString::number(mode.number),numberOrDash(mode.wavenumber),numberOrDash(mode.irIntensity),numberOrDash(mode.ramanActivity),imaginary?QStringLiteral("허수 진동"):QStringLiteral("정상 진동")};
        for(int col=0;col<values.size();++col)m_frequencyDetailsTable->setItem(row,col,new QTableWidgetItem(values[col]));
        bars.push_back({double(mode.number),mode.wavenumber});
    }
    m_copyFrequencyAnimation->setVisible(hasImaginary);
    m_frequencyPlot->setSeries(bars,QStringLiteral("Normal mode"),QStringLiteral("Wavenumber (cm⁻¹)"));
    const bool available=!m_data.frequencies.isEmpty();
    for(auto *button:findChild<QWidget*>("FrequencyToolbar")->findChildren<QPushButton*>())button->setEnabled(available);
    updateFrequencyPage(0);
    if(available)setCurrentFrequency(0);
    else m_frequencyInfo->setText(QStringLiteral("진동 모드를 선택하세요"));
}

void AnalysisPanel::setCurrentFrequency(int index) {
    if(!m_frequencyList||index<0||index>=m_data.frequencies.size())return;
    const QSignalBlocker listBlock(m_frequencyList),tableBlock(m_frequencyDetailsTable);
    if(index<m_frequencyPage||index>=m_frequencyPage+frequencyPageSize)updateFrequencyPage(index);
    m_frequencyList->setCurrentRow(index);m_frequencyDetailsTable->selectRow(index);
    const auto &mode=m_data.frequencies[index];
    m_frequencyInfo->setText(QStringLiteral("모드 %1 · 파수: %2 cm⁻¹ · IR 세기: %3 · Raman 활성도: %4 · %5")
        .arg(mode.number).arg(numberOrDash(mode.wavenumber)).arg(numberOrDash(mode.irIntensity)).arg(numberOrDash(mode.ramanActivity))
        .arg(mode.imaginary||mode.wavenumber<0?QStringLiteral("허수 진동"):QStringLiteral("정상 진동")));
}
} // namespace youngcraft
