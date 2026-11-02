#include "ui/GeometryMeasureBar.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <cmath>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace youngcraft {

GeometryMeasureBar::GeometryMeasureBar(QWidget *parent):QWidget(parent){
    setObjectName(QStringLiteral("GeometryMeasureBar"));auto *layout=new QVBoxLayout(this);layout->setContentsMargins(6,4,6,4);layout->setSpacing(3);
    m_distance=createRow(MeasurementKind::Distance,QStringLiteral("Å"),2,0.1,100.0,0.01);
    m_angle=createRow(MeasurementKind::Angle,QStringLiteral("°"),2,0.0,180.0,0.1);
    m_dihedral=createRow(MeasurementKind::Dihedral,QStringLiteral("°"),2,-180.0,180.0,0.1);
    layout->addWidget(m_distance.widget);layout->addWidget(m_angle.widget);layout->addWidget(m_dihedral.widget);
    auto *bottom=new QHBoxLayout;m_moveMode=new QComboBox(this);m_moveMode->setObjectName(QStringLiteral("GeometryMoveMode"));m_moveMode->addItem(QStringLiteral("→ 끝 원자 쪽 이동"),int(GeometryMoveMode::OneSide));m_moveMode->addItem(QStringLiteral("← 첫 원자 쪽 이동"),int(GeometryMoveMode::OneSide));m_moveMode->addItem(QStringLiteral("↔ 양쪽 이동"),int(GeometryMoveMode::BothSides));m_moveMode->setToolTip(QStringLiteral("선택 순서 기준으로 움직일 쪽을 지정합니다. 연결된 원자들은 함께 이동합니다."));auto *clear=new QPushButton(QStringLiteral("선택 초기화"),this);clear->setObjectName(QStringLiteral("GeometryClearSelection"));bottom->addWidget(new QLabel(QStringLiteral("이동 방식:"),this));bottom->addWidget(m_moveMode);bottom->addStretch();bottom->addWidget(clear);layout->addLayout(bottom);
    connect(m_moveMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{emit moveModeChanged(moveMode());});connect(clear,&QPushButton::clicked,this,&GeometryMeasureBar::clearSelectionRequested);
    m_distanceStyle.fontFamily=font().family();m_angleStyle=m_distanceStyle;m_dihedralStyle=m_distanceStyle;
}

GeometryMeasureBar::Row GeometryMeasureBar::createRow(MeasurementKind kind,const QString&unit,int decimals,double minimum,double maximum,double step){
    Row result;result.widget=new QWidget(this);const QString key=kind==MeasurementKind::Distance?QStringLiteral("Distance"):kind==MeasurementKind::Angle?QStringLiteral("Angle"):QStringLiteral("Dihedral");result.widget->setObjectName(QStringLiteral("Geometry%1Row").arg(key));auto *layout=new QHBoxLayout(result.widget);layout->setContentsMargins(0,0,0,0);result.atoms=new QLabel(result.widget);result.atoms->setMinimumWidth(210);result.value=new QDoubleSpinBox(result.widget);result.value->setObjectName(QStringLiteral("Geometry%1Value").arg(key));result.value->setRange(minimum,maximum);result.value->setDecimals(decimals);result.value->setSingleStep(step);result.value->setKeyboardTracking(false);result.value->setMinimumWidth(120);result.value->setSuffix(QStringLiteral(" ")+unit);auto *apply=new QPushButton(QStringLiteral("적용"),result.widget);apply->setObjectName(QStringLiteral("Geometry%1Apply").arg(key));result.show=new QPushButton(QStringLiteral("표시"),result.widget);result.show->setObjectName(QStringLiteral("Geometry%1Show").arg(key));result.show->setCheckable(true);result.style=new QPushButton(QStringLiteral("스타일"),result.widget);layout->addWidget(result.atoms,1);layout->addWidget(result.value);layout->addWidget(apply);layout->addWidget(result.show);layout->addWidget(result.style);
    auto *valueBox=result.value;auto applyValue=[this,kind,valueBox]{const double value=valueBox->value();if(kind==MeasurementKind::Distance)emit distanceApplyRequested(value);else if(kind==MeasurementKind::Angle)emit angleApplyRequested(value);else emit dihedralApplyRequested(value);};connect(apply,&QPushButton::clicked,this,applyValue);connect(result.value->findChild<QLineEdit*>(),&QLineEdit::returnPressed,this,applyValue);connect(result.show,&QPushButton::toggled,this,[this,kind,button=result.show](bool on){button->setText(on?QStringLiteral("숨기기"):QStringLiteral("표시"));if(kind==MeasurementKind::Distance)emit distanceVisibilityChanged(on);else if(kind==MeasurementKind::Angle)emit angleVisibilityChanged(on);else emit dihedralVisibilityChanged(on);});connect(result.style,&QPushButton::clicked,this,[this,kind]{editStyle(kind);});result.widget->setEnabled(false);return result;
}

void GeometryMeasureBar::setSelection(const Molecule &m, const QVector<int> &ids) {
    bool valid = true;
    for (int id : ids) if (id < 0 || id >= m.atoms.size()) valid = false;
    for (int k = 0; k < 3; ++k) {
        auto &r = row(MeasurementKind(k));
        const int count = k + 2;
        const QString name = k == 0 ? QStringLiteral("거리") : k == 1 ? QStringLiteral("각도") : QStringLiteral("이면각");
        const bool available = valid && ids.size() >= count;
        r.widget->setVisible(available);
        r.widget->setEnabled(available);
        if (!available) continue;
        QStringList atoms;
        for (int i = 0; i < count; ++i)
            atoms << QString::number(m.atoms[ids[i]].index > 0 ? m.atoms[ids[i]].index : ids[i]+1);
        const double value = k == 0 ? GeometryEditor::distance(m,ids[0],ids[1])
            : k == 1 ? GeometryEditor::angleDegrees(m,ids[0],ids[1],ids[2])
            : GeometryEditor::dihedralDegrees(m,ids[0],ids[1],ids[2],ids[3]);
        r.atoms->setText(QStringLiteral("%1 (%2)%3").arg(name, atoms.join(QStringLiteral("–")),
            std::isfinite(value) ? QString() : QStringLiteral(" · 정의 불가")));
        r.value->setEnabled(std::isfinite(value));
        r.widget->findChild<QPushButton *>(QStringLiteral("Geometry%1Apply").arg(
            k == 0 ? "Distance" : k == 1 ? "Angle" : "Dihedral"))->setEnabled(std::isfinite(value));
        QSignalBlocker block(r.value);
        r.value->setValue(std::isfinite(value) ? value : 0);
    }
}

GeometryMoveMode GeometryMeasureBar::moveMode()const{return GeometryMoveMode(m_moveMode->currentData().toInt());}
bool GeometryMeasureBar::moveFirstSide() const { return m_moveMode->currentIndex() == 1; }
void GeometryMeasureBar::setMovement(GeometryMoveMode mode, bool firstSide) {
    m_moveMode->setCurrentIndex(mode == GeometryMoveMode::BothSides ? 2 : firstSide ? 1 : 0);
}
GeometryMeasureBar::Row&GeometryMeasureBar::row(MeasurementKind kind){return kind==MeasurementKind::Distance?m_distance:kind==MeasurementKind::Angle?m_angle:m_dihedral;}
const GeometryMeasureBar::Row&GeometryMeasureBar::row(MeasurementKind kind)const{return kind==MeasurementKind::Distance?m_distance:kind==MeasurementKind::Angle?m_angle:m_dihedral;}
MeasurementTextStyle GeometryMeasureBar::measurementStyle(MeasurementKind kind)const{return kind==MeasurementKind::Distance?m_distanceStyle:kind==MeasurementKind::Angle?m_angleStyle:m_dihedralStyle;}
void GeometryMeasureBar::setMeasurementStyle(MeasurementKind kind,const MeasurementTextStyle&style){if(kind==MeasurementKind::Distance)m_distanceStyle=style;else if(kind==MeasurementKind::Angle)m_angleStyle=style;else m_dihedralStyle=style;emit measurementStyleChanged(kind,style);}

void GeometryMeasureBar::editStyle(MeasurementKind kind){
    QDialog dialog(this);dialog.setWindowTitle(QStringLiteral("측정값 스타일"));QFormLayout form(&dialog);QFontComboBox family;QSpinBox size;size.setRange(6,48);QCheckBox bold(QStringLiteral("굵게"));QPushButton color(QStringLiteral("색상 선택"));QLabel preview(QStringLiteral("123.46 Å"));preview.setAlignment(Qt::AlignCenter);preview.setMinimumHeight(38);MeasurementTextStyle style=measurementStyle(kind);family.setCurrentFont(QFont(style.fontFamily));size.setValue(style.pointSize);bold.setChecked(style.bold);QColor chosen=style.color;auto refresh=[&]{QFont f(family.currentFont());f.setPointSize(size.value());f.setBold(bold.isChecked());preview.setFont(f);preview.setStyleSheet(QStringLiteral("color:%1").arg(chosen.name()));};connect(&family,&QFontComboBox::currentFontChanged,&dialog,[&](const QFont&){refresh();});connect(&size,qOverload<int>(&QSpinBox::valueChanged),&dialog,[&](int){refresh();});connect(&bold,&QCheckBox::toggled,&dialog,[&](bool){refresh();});connect(&color,&QPushButton::clicked,&dialog,[&]{const QColor selected=QColorDialog::getColor(chosen,&dialog,QStringLiteral("측정값 색상"));if(selected.isValid()){chosen=selected;refresh();}});QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);form.addRow(QStringLiteral("글꼴"),&family);form.addRow(QStringLiteral("크기"),&size);form.addRow(&bold);form.addRow(&color);form.addRow(QStringLiteral("미리보기"),&preview);form.addRow(&buttons);refresh();if(dialog.exec()!=QDialog::Accepted)return;style.fontFamily=family.currentFont().family();style.pointSize=size.value();style.bold=bold.isChecked();style.color=chosen;setMeasurementStyle(kind,style);
}

} // namespace youngcraft
