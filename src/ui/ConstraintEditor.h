#pragma once
#include "compute/GeometryConstraints.h"
#include "model/GeometryEditor.h"
#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QRegularExpression>

namespace youngcraft {
class ConstraintEditor : public QWidget {
public:
    ConstraintEditor(const Molecule &m, const QVector<int> &selected, QWidget *parent = nullptr, bool compactButtons = false) : QWidget(parent), molecule(m), selection(selected) {
        setObjectName("ConstraintEditor");
        auto *layout = new QVBoxLayout(this); layout->setContentsMargins(0,0,0,0);
        auto *hint = new QLabel(QStringLiteral("Constraint · 원자 번호는 1부터, 거리 Å / 각도 °\n목표값을 유지하는 조화 restraint입니다. 선택 원자 2–4개를 추가하면 현재 값을 가져옵니다.\n고정 원자는 좌표가 변하지 않습니다. 기존 구조를 먼저 고정한 뒤 조각을 추가하세요."));
        hint->setWordWrap(true); layout->addWidget(hint);
        table = new QTableWidget(0,3,this); table->setObjectName("ConstraintTable");
        table->setHorizontalHeaderLabels({QStringLiteral("종류"),QStringLiteral("원자 번호 (순서대로)"),QStringLiteral("목표값")});
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setMinimumHeight(100); table->setMaximumHeight(160); layout->addWidget(table);
        auto *buttons = new QGridLayout; layout->addLayout(buttons);
        const int columns=compactButtons?2:4;
        auto *add = new QPushButton(QStringLiteral("추가 / 선택 원자 가져오기")); add->setObjectName("AddConstraint");
        auto *remove = new QPushButton(QStringLiteral("선택 행 삭제")); remove->setObjectName("RemoveConstraint");
        buttons->addWidget(add,0,0); buttons->addWidget(remove,0,1);
        auto *freeze = new QPushButton(QStringLiteral("선택 원자 고정")); freeze->setObjectName("FreezeSelectedAtoms");
        auto *all = new QPushButton(QStringLiteral("현재 전체 고정")); all->setObjectName("FreezeAllAtoms");
        buttons->addWidget(freeze,2/columns,2%columns); buttons->addWidget(all,3/columns,3%columns);
        auto appendFixed = [this](const QVector<int> &ids) {
            QVector<GeometryConstraint> entries; QString error;
            read(entries,&error);
            for(int i:ids) { bool exists=false; for(const auto &c:entries)if(c.type=="fixed"&&c.atoms.contains(i))exists=true; if(!exists)entries<<GeometryConstraint{"fixed",{i},0}; }
            setEntries(entries);
        };
        connect(freeze,&QPushButton::clicked,this,[this,appendFixed]{appendFixed(selection);});
        connect(all,&QPushButton::clicked,this,[this,appendFixed]{QVector<int> ids;for(int i=0;i<molecule.atoms.size();++i)ids<<i;appendFixed(ids);});
        QVector<GeometryConstraint> retained;for(int i=0;i<m.atoms.size();++i)if(m.atoms[i].optimizationFixed)retained<<GeometryConstraint{"fixed",{i},0};setEntries(retained);
        connect(add,&QPushButton::clicked,this,[this]{ addRow(); });
        connect(remove,&QPushButton::clicked,this,[this]{ if(table->currentRow()>=0) table->removeRow(table->currentRow()); });
    }
    void setEntries(const QVector<GeometryConstraint> &entries) {
        table->setRowCount(0);
        for(const auto &c:entries) {
            addRow();const int row=table->rowCount()-1;
            auto *kind=static_cast<QComboBox *>(table->cellWidget(row,0));
            kind->setCurrentIndex(kind->findData(c.type));
            QStringList ids;for(int i:c.atoms)ids<<QString::number(i+1);
            static_cast<QLineEdit *>(table->cellWidget(row,1))->setText(ids.join(", "));
            static_cast<QDoubleSpinBox *>(table->cellWidget(row,2))->setValue(c.target);
        }
    }
    bool read(QVector<GeometryConstraint> &result, QString *error) const {
        result.clear();
        for(int row=0;row<table->rowCount();++row) {
            GeometryConstraint c; c.type=static_cast<QComboBox *>(table->cellWidget(row,0))->currentData().toString();
            const auto parts=static_cast<QLineEdit *>(table->cellWidget(row,1))->text().split(QRegularExpression("[,;\\s]+"),Qt::SkipEmptyParts);
            for(const auto &part:parts) { bool ok; int id=part.toInt(&ok); c.atoms << (ok ? id-1 : -1); }
            c.target=static_cast<QDoubleSpinBox *>(table->cellWidget(row,2))->value(); result<<c;
        }
        if(!validateConstraints(result,molecule.atoms.size(),error))return false;
        QSet<int> fixed;for(const auto &c:result)if(c.type=="fixed")fixed.insert(c.atoms[0]);
        if(fixed.size()==molecule.atoms.size()&&!fixed.isEmpty()){if(error)*error=QStringLiteral("모든 원자가 고정되어 있습니다. 추가 원자를 넣거나 고정을 일부 해제하세요.");return false;}
        for(const auto &c:result)if(c.type=="distance"&&fixed.contains(c.atoms[0])&&fixed.contains(c.atoms[1])&&std::abs(GeometryEditor::distance(molecule,c.atoms[0],c.atoms[1])-c.target)>0.001){if(error)*error=QStringLiteral("둘 다 고정된 원자의 거리는 바꿀 수 없습니다. 한쪽 고정을 해제하세요.");return false;}
        return true;
    }
private:
    void addRow() {
        const int row=table->rowCount(); table->insertRow(row);
        auto *type=new QComboBox; type->addItem(QStringLiteral("거리"),"distance"); type->addItem(QStringLiteral("각도"),"angle"); type->addItem(QStringLiteral("이면각"),"dihedral"); type->addItem(QStringLiteral("원자 고정"),"fixed");
        auto *ids=new QLineEdit; ids->setPlaceholderText("1, 2");
        auto *target=new QDoubleSpinBox; target->setDecimals(4); target->setRange(0.0001,10000); target->setValue(1.5);
        table->setCellWidget(row,0,type); table->setCellWidget(row,1,ids); table->setCellWidget(row,2,target);
        connect(type,&QComboBox::currentIndexChanged,this,[target](int i){ target->setEnabled(i!=3); target->setRange(i==3?0:i==2?-180:0.0001,i==0?10000:i==1?179.9999:180); target->setSuffix(i==0?QStringLiteral(" Å"):QStringLiteral(" °")); });
        target->setSuffix(QStringLiteral(" Å"));
        if(selection.size()>=2 && selection.size()<=4) {
            type->setCurrentIndex(selection.size()-2); QStringList text; for(int i:selection)text<<QString::number(i+1); ids->setText(text.join(", "));
            double value=selection.size()==2?GeometryEditor::distance(molecule,selection[0],selection[1]):selection.size()==3?GeometryEditor::angleDegrees(molecule,selection[0],selection[1],selection[2]):GeometryEditor::dihedralDegrees(molecule,selection[0],selection[1],selection[2],selection[3]);
            if(std::isfinite(value))target->setValue(value);
        }
        table->setCurrentCell(row,1);
    }
    Molecule molecule;
    QVector<int> selection;
    QTableWidget *table;
};
}
