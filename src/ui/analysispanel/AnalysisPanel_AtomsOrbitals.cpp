#include "ui/OrbitalStudyWidget.h"
#include "ui/AnalysisPanel.h"
#include "ui/SidebarButton.h"
#include "ui/SidebarFlowLayout.h"
#include "compute/OrbitalDiagnostics.h"

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
#include <QDialog>
#include <QJsonArray>
#include <QScrollArea>
#include <QScreen>
#include <QGuiApplication>

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
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *open = new QPushButton(QStringLiteral("오비탈 · 표시 설정…"), page);
    open->setObjectName("OpenOrbitalWindow");
    setSidebarButtonIcon(open,UiIcon::Orbital,open->text());
    auto *quickActions = new QWidget(page);
    quickActions->setObjectName(QStringLiteral("OrbitalQuickActions"));
    auto *quickLayout = new SidebarFlowLayout(quickActions);
    auto *homo = new QPushButton(QStringLiteral("HOMO"), quickActions);
    homo->setObjectName(QStringLiteral("QuickShowHomo"));
    setSidebarButtonIcon(homo,UiIcon::Orbital,homo->text());
    homo->setToolTip(QStringLiteral("선택한 스핀의 HOMO를 표시합니다. 처음 표시할 때는 Gaussian으로 생성합니다."));
    auto *lumo = new QPushButton(QStringLiteral("LUMO"), quickActions);
    lumo->setObjectName(QStringLiteral("QuickShowLumo"));
    setSidebarButtonIcon(lumo,UiIcon::Orbital,lumo->text());
    lumo->setToolTip(QStringLiteral("선택한 스핀의 LUMO를 표시합니다. 처음 표시할 때는 Gaussian으로 생성합니다."));
    homo->setFixedHeight(sidebarButtonSize);lumo->setFixedHeight(sidebarButtonSize);
    quickLayout->addWidget(homo);quickLayout->addWidget(lumo);
    auto *help = new QLabel(QStringLiteral("HOMO / LUMO를 바로 표시합니다. 다른 MO, MEP와 표시 설정은 오비탈 창에서 선택하세요."), page);
    help->setWordWrap(true);
    layout->addWidget(quickActions); layout->addWidget(open); layout->addWidget(help); layout->addStretch();
    auto *dialog = new QDialog(page, Qt::Window);
    dialog->setObjectName("OrbitalManagerWindow");
    dialog->setWindowTitle(QStringLiteral("영크래프트 · 오비탈 관리"));
    dialog->setModal(false);
    dialog->setProperty("solidWorkspace",true);
    dialog->setAutoFillBackground(true);
    dialog->setAttribute(Qt::WA_TranslucentBackground,false);
    dialog->setWindowOpacity(1.0);
    auto *dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->setContentsMargins(4,4,4,4);
    dialogLayout->setSizeConstraint(QLayout::SetNoConstraint);
    m_orbitalStudy = new OrbitalStudyWidget(dialog);
    dialogLayout->addWidget(m_orbitalStudy);
    connect(homo, &QPushButton::clicked, this, [this]{m_orbitalStudy->showFrontierOrbital(false);});
    connect(lumo, &QPushButton::clicked, this, [this]{m_orbitalStudy->showFrontierOrbital(true);});
    connect(m_orbitalStudy, &OrbitalStudyWidget::frontierAvailabilityChanged, this, [homo,lumo](bool occupied,bool virtualOrbital){
        homo->setEnabled(occupied);lumo->setEnabled(virtualOrbital);
    });
    homo->setEnabled(m_orbitalStudy->frontierAvailable(false));
    lumo->setEnabled(m_orbitalStudy->frontierAvailable(true));
    auto *status = new QLabel(page);
    status->setObjectName(QStringLiteral("OrbitalQuickStatus"));status->setWordWrap(true);
    // Share progress and errors with the shortcuts even while the manager is closed.
    if(auto *source=m_orbitalStudy->findChild<QLabel*>(QStringLiteral("OrbitalStatus"))){
        status->setText(source->text());
    }
    connect(m_orbitalStudy, &OrbitalStudyWidget::statusChanged, status, &QLabel::setText);
    layout->insertWidget(3,status);
    connect(open, &QPushButton::clicked, this, [page, dialog] {
        const bool first=!dialog->property("orbitalWindowPlaced").toBool();
        auto *screen = first ? page->window()->screen() : dialog->screen();
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (screen && first) {
            const QRect area=screen->availableGeometry();
            const QSize size(qMin(1120,area.width()*9/10),qMin(680,area.height()*8/10));
            dialog->resize(size);
            dialog->move(area.right()-size.width()-16,area.center().y()-size.height()/2);
            dialog->setProperty("orbitalWindowPlaced",true);
        }
        dialog->show(); dialog->raise(); dialog->activateWindow();
        QTimer::singleShot(0,dialog,[dialog] {
            auto *screen=dialog->screen();if(!screen)return;
            const auto area=screen->availableGeometry();
            if(!dialog->isMaximized()) {
                auto frame=dialog->frameGeometry();
                const QSize decoration=frame.size()-dialog->size();
                dialog->resize(qMin(dialog->width(),qMax(1,area.width()-decoration.width()-16)),
                    qMin(dialog->height(),qMax(1,area.height()-decoration.height()-16)));
                frame=dialog->frameGeometry();
                // move() uses the outer frame position; setGeometry() uses the client
                // position and would shift the title bar upward on each repeated open.
                dialog->move(qBound(area.left()+8,frame.left(),qMax(area.left()+8,area.right()-frame.width()-7)),
                    qBound(area.top()+8,frame.top(),qMax(area.top()+8,area.bottom()-frame.height()-7)));
            }
            const auto frame=dialog->frameGeometry();
            recordOrbitalDiagnostic({{"event","manager-window"},{"screen",screen->name()},
                {"frame",QJsonArray{frame.x(),frame.y(),frame.width(),frame.height()}},
                {"available",QJsonArray{area.x(),area.y(),area.width(),area.height()}},
                {"fits",area.contains(frame)}});
        });
    });
    connect(m_orbitalStudy, &OrbitalStudyWidget::openRequested, this, &AnalysisPanel::orbitalFileRequested);
    connect(m_orbitalStudy, &OrbitalStudyWidget::surfaceStyleChanged, this, &AnalysisPanel::orbitalStyleChanged);
    return page;
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
