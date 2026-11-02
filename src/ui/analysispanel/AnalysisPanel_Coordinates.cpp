#include "ui/AnalysisPanel.h"
#include "io/XYZParser.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"

#include <QDoubleSpinBox>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QCheckBox>
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
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

namespace youngcraft {

bool AnalysisPanel::coordinateViewActive() const { return m_tabs && m_tabs->currentIndex() == m_coordinateTabIndex; }

void AnalysisPanel::setCoordinateMolecule(const Molecule &molecule) {
    if (!m_coordinateText || !m_coordinateTitle) return;
    m_coordinateTitle->setText(molecule.title.isEmpty() ? QStringLiteral("현재 구조") : molecule.title);
    QString text = QStringLiteral("번호  원소               X               Y               Z\n");
    m_coordinateClipboardText.clear();
    for (const Atom &atom : molecule.atoms) {
        m_coordinateClipboardText += QStringLiteral("%1  %2  %3  %4\n")
                    .arg(atom.element, -4)
                    .arg(atom.position.x(), 15, 'f', 8)
                    .arg(atom.position.y(), 15, 'f', 8)
                    .arg(atom.position.z(), 15, 'f', 8);
        text += QStringLiteral("%1  %2  %3  %4  %5\n")
                    .arg(atom.index, 4)
                    .arg(atom.element, -4)
                    .arg(atom.position.x(), 15, 'f', 8)
                    .arg(atom.position.y(), 15, 'f', 8)
                    .arg(atom.position.z(), 15, 'f', 8);
    }
    m_coordinateText->setPlainText(text);
    m_coordinateCopy->setEnabled(!m_coordinateClipboardText.isEmpty());
}

QWidget *AnalysisPanel::makeCoordinateTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_coordinateTitle = new QLabel(QStringLiteral("현재 구조"));
    m_coordinateTitle->setAlignment(Qt::AlignCenter);
    auto *header = new QHBoxLayout;
    header->addWidget(m_coordinateTitle, 1);
    m_coordinateCopy = new QPushButton(QStringLiteral("XYZ 복사"));
    m_coordinateCopy->setObjectName(QStringLiteral("CopyCoordinateButton"));
    m_coordinateCopy->setToolTip(QStringLiteral("현재 구조의 원소와 X, Y, Z 좌표를 복사합니다 (번호·제목 제외)."));
    m_coordinateCopy->setEnabled(false);
    connect(m_coordinateCopy, &QPushButton::clicked, this, [this] {
        emit coordinateViewRequested();
        if (!m_coordinateClipboardText.isEmpty())
            QApplication::clipboard()->setText(m_coordinateClipboardText);
    });
    header->addWidget(m_coordinateCopy);
    layout->addLayout(header);
    m_coordinateText = new QPlainTextEdit;
    m_coordinateText->setObjectName(QStringLiteral("CoordinateText"));
    m_coordinateText->setReadOnly(true);
    m_coordinateText->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_coordinateText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_coordinateText, 1);
    auto *input=new QPlainTextEdit;input->setObjectName("CoordinatePasteInput");
    input->setPlaceholderText(QStringLiteral("새 XYZ 붙여넣기\n원자 수·제목이 있는 XYZ 또는 원소 X Y Z 좌표를 넣으세요."));
    input->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));input->setMinimumHeight(100);input->setMaximumHeight(140);layout->insertWidget(1,input);
    auto *render=new QPushButton(QStringLiteral("붙여넣은 XYZ 렌더링 (새 탭)"));render->setObjectName("RenderPastedXYZ");layout->insertWidget(2,render);
    auto *message=new QLabel;message->setObjectName("CoordinatePasteStatus");message->setWordWrap(true);layout->insertWidget(3,message);
    connect(render,&QPushButton::clicked,this,[this,input,message]{
        const auto parsed=XYZParser::parseText(input->toPlainText());
        if(!parsed.success){message->setText(parsed.error);return;}
        QString xyz=QStringLiteral("%1\n%2\n").arg(parsed.data.molecule.atoms.size()).arg(parsed.data.molecule.title);
        for(const auto &a:parsed.data.molecule.atoms)xyz+=QStringLiteral("%1 %2 %3 %4\n").arg(a.element).arg(a.position.x(),0,'g',9).arg(a.position.y(),0,'g',9).arg(a.position.z(),0,'g',9);
        message->setText(QStringLiteral("XYZ 확인 완료 · %1개 원자").arg(parsed.data.molecule.atoms.size()));
        emit coordinateImportRequested(xyz);
    });
    return page;
}

} // namespace youngcraft
