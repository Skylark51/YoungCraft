#include "ui/OrbitalStudyWidget.h"
#include "ui/AnalysisPanel.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"

#include <QDoubleSpinBox>
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

namespace {

QString numberOrDash(double value, int decimals = 4) {
    return std::isfinite(value) ? QString::number(value, 'f', decimals) : QStringLiteral("—");
}

QTableWidgetItem *item(const QString &text) { return new QTableWidgetItem(text); }

} // namespace

QWidget *AnalysisPanel::makeAtomsTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    m_atomTable = new QTableWidget;
    m_atomTable->setColumnCount(7);
    m_atomTable->setHorizontalHeaderLabels({QStringLiteral("번호"), QStringLiteral("원소"), QStringLiteral("전하"), QStringLiteral("스핀"), QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")});
    m_atomTable->horizontalHeader()->setStretchLastSection(true);
    m_atomTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_atomTable);
    connect(m_atomTable, &QTableWidget::cellClicked, this, [this](int row, int) { emit atomRequested(row); });
    return page;
}

QWidget *AnalysisPanel::makeOrbitalsTab() {
    m_orbitalStudy = new OrbitalStudyWidget;
    connect(m_orbitalStudy, &OrbitalStudyWidget::openRequested, this, &AnalysisPanel::orbitalFileRequested);
    connect(m_orbitalStudy, &OrbitalStudyWidget::surfaceStyleChanged, this, &AnalysisPanel::orbitalStyleChanged);
    return m_orbitalStudy;
}
void AnalysisPanel::setOrbitalView(MoleculeView *view) { m_orbitalStudy->setView(view); }

void AnalysisPanel::fillAtoms() {
    m_atomTable->setRowCount(0);
    for (const Atom &atom : m_data.molecule.atoms) {
        const int row = m_atomTable->rowCount();
        m_atomTable->insertRow(row);
        m_atomTable->setItem(row, 0, item(QString::number(atom.index)));
        m_atomTable->setItem(row, 1, item(atom.element));
        m_atomTable->setItem(row, 2, item(numberOrDash(atom.charge, 6)));
        m_atomTable->setItem(row, 3, item(numberOrDash(atom.spinDensity, 6)));
        m_atomTable->setItem(row, 4, item(numberOrDash(atom.position.x(), 5)));
        m_atomTable->setItem(row, 5, item(numberOrDash(atom.position.y(), 5)));
        m_atomTable->setItem(row, 6, item(numberOrDash(atom.position.z(), 5)));
    }
}

void AnalysisPanel::fillOrbitals() { m_orbitalStudy->setData(m_data); }

} // namespace youngcraft
