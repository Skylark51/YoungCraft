#include "ui/AnalysisPanel.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"
#include "ui/TransitionAnalysisDialog.h"
#include "ui/TransitionAnalysisIcon.h"
#include "ui/SidebarFlowLayout.h"

#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace youngcraft {

namespace {

QString numberOrDash(double value, int decimals = 4) {
    return std::isfinite(value) ? QString::number(value, 'f', decimals) : QStringLiteral("—");
}

QTableWidgetItem *item(const QString &text) { return new QTableWidgetItem(text); }

void normalize(QVector<QPointF> &values) {
    double maximum = 0.0;
    for (const QPointF &value : values) maximum = std::max(maximum, value.y());
    if (maximum <= 0.0) return;
    for (QPointF &value : values) value.setY(value.y() / maximum);
}

} // namespace

QWidget *AnalysisPanel::makeTddftTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *form = new QFormLayout;
    m_broadeningShape = new QComboBox;
    m_broadeningShape->setObjectName(QStringLiteral("TddftBroadeningShape"));
    m_broadeningShape->addItem(QStringLiteral("가우스"), int(SpectrumLineShape::Gaussian));
    m_broadeningShape->addItem(QStringLiteral("로렌츠"), int(SpectrumLineShape::Lorentzian));
    m_broadeningShape->addItem(QStringLiteral("없음"), -1);
    m_broadening = new QDoubleSpinBox;
    m_broadening->setObjectName(QStringLiteral("TddftBroadeningWidth"));
    m_broadening->setRange(1.0, 100.0); m_broadening->setSingleStep(1.0); m_broadening->setValue(18.0);
    m_broadening->setSuffix(QStringLiteral(" nm"));
    m_broadening->setToolTip(QStringLiteral("가우스: 표준편차 σ · Lorentzian: 반치반폭 HWHM (nm)"));
    m_normalizeSpectrum = new QCheckBox(QStringLiteral("정규화"));
    m_normalizeSpectrum->setObjectName(QStringLiteral("TddftNormalize"));
    m_normalizeSpectrum->setChecked(true);
    m_showSpectrumSticks = new QCheckBox(QStringLiteral("막대 표시"));
    m_showSpectrumSticks->setObjectName(QStringLiteral("TddftSticks"));
    m_showSpectrumSticks->setChecked(true);
    form->addRow(QStringLiteral("선폭 확장"), m_broadeningShape);
    form->addRow(QStringLiteral("폭"), m_broadening);
    form->addRow(m_normalizeSpectrum);
    form->addRow(m_showSpectrumSticks);
    auto *actions=new QWidget(page);actions->setObjectName(QStringLiteral("TddftSelectionActions"));
    auto *actionLayout=new SidebarFlowLayout(actions);
    auto *nto=new QPushButton(transitionAnalysisIcon(),QStringLiteral("NTO 분석"));
    nto->setObjectName(QStringLiteral("TddftTransitionAnalysis"));
    nto->setToolTip(QStringLiteral("선택한 TD 상태의 NTO 쌍과 hole / particle 기여도 및 3D 분석"));
    connect(nto,&QPushButton::clicked,this,[this]{showTransitionAnalysis();});
    actionLayout->addWidget(nto);
    form->addRow(actions);
    layout->addLayout(form);
    m_tddftTable = new QTableWidget;
    m_tddftTable->setObjectName(QStringLiteral("TddftStatesTable"));
    m_tddftTable->setColumnCount(6);
    m_tddftTable->setHorizontalHeaderLabels({QStringLiteral("상태"), QStringLiteral("에너지 (eV)"), QStringLiteral("파장 (nm)"), QStringLiteral("진동자 세기"), QStringLiteral("주요 전이"),QStringLiteral("NTO / h–p")});
    m_tddftTable->horizontalHeader()->setSectionResizeMode(4,QHeaderView::Stretch);
    m_tddftTable->horizontalHeader()->setSectionResizeMode(5,QHeaderView::ResizeToContents);
    m_tddftTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tddftTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tddftTable->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tddftTable, 2);
    connect(m_broadening, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        fillTddft();
        emit broadeningChanged(value);
    });
    connect(m_broadeningShape, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        m_broadening->setEnabled(m_broadeningShape->currentData().toInt() >= 0);
        fillTddft();
    });
    connect(m_normalizeSpectrum, &QCheckBox::toggled, this, [this] { fillTddft(); });
    connect(m_showSpectrumSticks, &QCheckBox::toggled, this, [this] { fillTddft(); });
    connect(m_tddftTable, &QTableWidget::currentCellChanged, this,
            [this]{updateTddftSelection();});
    return page;
}

void AnalysisPanel::fillTddft() {
    const int selectedRow = m_tddftTable->currentRow();
    const QSignalBlocker blocker(m_tddftTable);
    m_tddftTable->setRowCount(0);
    QVector<QPointF> bars;
    for (const ExcitedState &state : m_data.excitedStates) {
        const int row = m_tddftTable->rowCount();
        m_tddftTable->insertRow(row);
        QString transitions;
        for (const OrbitalTransition &transition : state.transitions) {
            if (!transitions.isEmpty()) transitions += QStringLiteral(", ");
            transitions += QStringLiteral("%1→%2 (%3)").arg(transition.from, transition.to).arg(numberOrDash(transition.coefficient, 3));
        }
        if(!state.nativeNtoPairs.isEmpty()) {
            transitions=QStringLiteral("저장 NTO %1쌍").arg(state.nativeNtoPairs.size());
        }
        m_tddftTable->setItem(row, 0, item(state.label.isEmpty() ? QString::number(state.state) : state.label));
        m_tddftTable->setItem(row, 1, item(numberOrDash(state.energyEv, 4)));
        m_tddftTable->setItem(row, 2, item(numberOrDash(state.wavelengthNm, 2)));
        m_tddftTable->setItem(row, 3, item(numberOrDash(state.oscillatorStrength, 5)));
        m_tddftTable->setItem(row, 4, item(transitions));
        auto *analyze=new QToolButton;
        analyze->setObjectName(QStringLiteral("AnalyzeTransition_%1").arg(row));
        analyze->setIcon(transitionAnalysisIcon());
        analyze->setAccessibleName(QStringLiteral("상태 %1 NTO · Hole / Particle 분석").arg(state.state));
        analyze->setToolTip(state.transitions.isEmpty() && state.nativeNtoPairs.isEmpty()?QStringLiteral("전이 계수가 없어 분석할 수 없습니다. 계수를 포함한 TD 출력이 필요합니다."):
                            QStringLiteral("이 상태의 NTO · Hole / Particle 분석"));
        analyze->setEnabled(!state.transitions.isEmpty() || !state.nativeNtoPairs.isEmpty());
        connect(analyze,&QToolButton::clicked,this,[this,row]{m_tddftTable->setCurrentCell(row,0);showTransitionAnalysis(row);});
        m_tddftTable->setCellWidget(row,5,analyze);
        if(std::isfinite(state.wavelengthNm)&&state.wavelengthNm>0&&std::isfinite(state.oscillatorStrength)&&state.oscillatorStrength>=0)
            bars.push_back({state.wavelengthNm, state.oscillatorStrength});
    }
    QVector<QPointF> spectrum;
    const int shapeValue = m_broadeningShape->currentData().toInt();
    if (shapeValue >= 0) {
        double first=900,last=180;bool found=false;
        for(const auto &state:m_data.excitedStates) {
            if(!std::isfinite(state.wavelengthNm) || state.wavelengthNm<=0)continue;
            if(!found){first=last=state.wavelengthNm;found=true;}
            else {first=std::min(first,state.wavelengthNm);last=std::max(last,state.wavelengthNm);}
        }
        const double margin=std::max(50.0,4*m_broadening->value());
        const double start=found?std::max(1.0,25*std::floor((first-margin)/25)):180;
        const double end=found?25*std::ceil((last+margin)/25):900;
        spectrum = AnalysisEngine::uvVisSpectrum(
            m_data.excitedStates, m_broadening->value(), start, end, std::max(0.25,(end-start)/4000),
            SpectrumLineShape(shapeValue));
    }
    if (m_normalizeSpectrum->isChecked()) {
        normalize(spectrum);
        normalize(bars);
    }
    if (!m_showSpectrumSticks->isChecked()) bars.clear();
    QString summary = QStringLiteral("TD-DFT 데이터 없음");
    if (!m_data.excitedStates.isEmpty()) {
        double strongest = 0.0;
        int strongestState = 0;
        for (const ExcitedState &state : m_data.excitedStates) {
            if (state.oscillatorStrength > strongest) {
                strongest = state.oscillatorStrength;
                strongestState = state.state;
            }
        }
        const QString shape = m_broadeningShape->currentText();
        summary = QStringLiteral("전이 %1개  ·  최대 세기 S%2: f = %3  ·  %4%5")
                      .arg(m_data.excitedStates.size()).arg(strongestState)
                      .arg(strongest, 0, 'f', 5).arg(shape)
                      .arg(shapeValue >= 0 ? QStringLiteral(" ·  폭 %1 nm").arg(m_broadening->value(), 0, 'f', 1) : QString());
        if(std::all_of(m_data.excitedStates.cbegin(),m_data.excitedStates.cend(),[](const auto &s){return !s.nativeNtoPairs.isEmpty() && !std::isfinite(s.wavelengthNm);}))
            summary=QStringLiteral("Gaussian 저장 NTO 결과 · 상태 옆 또는 그래프 상단의 주황·파랑 NTO 아이콘으로 분석하세요. 스펙트럼은 원래 TD 출력에서 확인합니다.");
    }
    if (selectedRow >= 0 && selectedRow < m_tddftTable->rowCount())
        m_tddftTable->setCurrentCell(selectedRow, 0);
    emit tddftSpectrumChanged(spectrum, bars, summary);
    updateTddftSelection();
}

void AnalysisPanel::updateTddftSelection() {
    const int row=m_tddftTable->currentRow();
    const int selected=row>=0?row:0;
    const bool valid=selected<m_data.excitedStates.size();
    if(auto *button=findChild<QPushButton*>(QStringLiteral("TddftTransitionAnalysis")))
        button->setEnabled(valid && (!m_data.excitedStates[selected].transitions.isEmpty()
            || !m_data.excitedStates[selected].nativeNtoPairs.isEmpty()));
    if(row<0 || !valid)return;
    const auto &state=m_data.excitedStates[row];
    emit tddftStateInfoChanged(
        QStringLiteral("선택 상태 %1  ·  %2 eV  ·  %3 nm  ·  f = %4")
            .arg(state.label.isEmpty()?QString::number(state.state):state.label)
            .arg(numberOrDash(state.energyEv,4)).arg(numberOrDash(state.wavelengthNm,2))
            .arg(numberOrDash(state.oscillatorStrength,5)));
}

void AnalysisPanel::showTransitionAnalysis(int row) {
    if(row<0)row=qMax(0,m_tddftTable->currentRow());
    if(row>=m_data.excitedStates.size())return;
    auto *dialog=new TransitionAnalysisDialog(m_data,row,this);
    dialog->show();
}

} // namespace youngcraft
