#include "ui/AnalysisPanel.h"

#include "analysis/AnalysisEngine.h"
#include "ui/PlotWidget.h"

#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include <QRegularExpression>
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
#include <QTabBar>
#include <QVBoxLayout>

#include <cmath>

namespace youngcraft {

AnalysisPanel::AnalysisPanel(QWidget *parent) : QWidget(parent) {
    m_tabs = new QTabWidget(this);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(m_tabs);
    m_tabs->addTab(makeGeometryTab(), QStringLiteral("구조 / 최적화"));
    m_tabs->addTab(makeFrequencyTab(), QStringLiteral("진동"));
    m_tabs->addTab(makeScanTab(), QStringLiteral("퍼텐셜 에너지 스캔"));
    m_tabs->addTab(makeIrcTab(), QStringLiteral("IRC"));
    m_tddftTabIndex = m_tabs->addTab(makeTddftTab(), QStringLiteral("TD-DFT"));
    m_tabs->addTab(makeAtomsTab(), QStringLiteral("원자 속성"));
    m_tabs->addTab(makeOrbitalsTab(), QStringLiteral("오비탈"));
    m_coordinateTabIndex = m_tabs->addTab(makeCoordinateTab(), QStringLiteral("좌표"));
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        emit currentTabChanged(index);
        if (index == m_coordinateTabIndex) emit coordinateViewRequested();
        emit workspaceModeRequested(index == m_tddftTabIndex);
    });
}

void AnalysisPanel::setData(const CalculationData &data) {
    m_data = data;
    fillFrameList(); fillFrequencyList(); fillScanPlot(); fillIrcPlot(); fillTddft(); fillAtoms(); fillOrbitals();
}

void AnalysisPanel::selectDefaultTab() {
    const auto hasKeyword = [this](const QString &keyword) {
        return QRegularExpression(QStringLiteral("(?:^|[^a-z0-9_])(?:%1)(?=$|[^a-z0-9_])").arg(keyword),
                                  QRegularExpression::CaseInsensitiveOption).match(m_data.route).hasMatch();
    };
    // Specialized paths take precedence over the opt/freq steps in a combined job.
    if (m_data.scanDetected || !m_data.scanPoints.isEmpty() || hasKeyword(QStringLiteral("scan"))) setCurrentTab(2);
    else if (!m_data.ircPoints.isEmpty() || hasKeyword(QStringLiteral("irc"))) setCurrentTab(3);
    else if (!m_data.excitedStates.isEmpty() || hasKeyword(QStringLiteral("td|tda"))) setCurrentTab(4);
    else if (!m_data.frequencies.isEmpty() || hasKeyword(QStringLiteral("freq"))) setCurrentTab(1);
    else if (m_data.format == "FCHK" || m_data.format == "CUBE") setCurrentTab(6);
    else setCurrentTab(0);
}

bool AnalysisPanel::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->modifiers() == Qt::NoModifier && (key->key() == Qt::Key_Left || key->key() == Qt::Key_Right)) {
            key->key() == Qt::Key_Left ? selectPrevious() : selectNext();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AnalysisPanel::selectNext() {
    QListWidget *list = currentTab() == 2 ? m_scanList : currentTab() == 3 ? m_ircList
                       : currentTab() == 1 ? m_frequencyList : m_frameList;
    if (list && list->currentRow() + 1 < list->count()) list->setCurrentRow(list->currentRow() + 1);
}

void AnalysisPanel::selectPrevious() {
    QListWidget *list = currentTab() == 2 ? m_scanList : currentTab() == 3 ? m_ircList
                       : currentTab() == 1 ? m_frequencyList : m_frameList;
    if (list && list->currentRow() > 0) list->setCurrentRow(list->currentRow() - 1);
}

QStringList AnalysisPanel::tabTitles() const {
    QStringList titles;
    for (int i = 0; i < m_tabs->count(); ++i) titles.push_back(m_tabs->tabText(i));
    return titles;
}

void AnalysisPanel::setTabBarVisible(bool visible) {
    m_tabs->tabBar()->setVisible(visible);
}

void AnalysisPanel::setCurrentTab(int index) {
    if (m_tabs && index >= 0 && index < m_tabs->count()) m_tabs->setCurrentIndex(index);
}

} // namespace youngcraft
