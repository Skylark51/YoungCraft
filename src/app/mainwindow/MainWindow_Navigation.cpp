#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include "ui/StyleEditorDialog.h"

#include <QActionGroup>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDockWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalBlocker>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>
#include <cmath>
#include <limits>

namespace youngcraft {

void MainWindow::populateExplorer() {
    updateOutputSource();
    m_explorer->clear();
    if (m_data.filePath.isEmpty() && m_data.molecule.atoms.isEmpty()) {
        m_explorer->addTopLevelItem(new QTreeWidgetItem({QStringLiteral("파일을 열어 주세요")}));
        return;
    }
    auto *root = new QTreeWidgetItem({m_data.filePath.isEmpty() ? QStringLiteral("새 구조") : QFileInfo(m_data.filePath).fileName()});
    m_explorer->addTopLevelItem(root);
    auto *geometry = new QTreeWidgetItem({QStringLiteral("구조 / 최적화 (%1)").arg(m_data.frames.size())}); geometry->setData(0, Qt::UserRole, QStringLiteral("section")); geometry->setData(0, Qt::UserRole + 1, 0); root->addChild(geometry);
    for (int i = 0; i < m_data.frames.size(); ++i) {
        auto *child = new QTreeWidgetItem({QStringLiteral("프레임 %1").arg(i + 1)}); child->setData(0, Qt::UserRole, QStringLiteral("frame")); child->setData(0, Qt::UserRole + 1, i); geometry->addChild(child);
    }
    auto *frequency = new QTreeWidgetItem({QStringLiteral("진동 (%1)").arg(m_data.frequencies.size())}); frequency->setData(0, Qt::UserRole, QStringLiteral("section")); frequency->setData(0, Qt::UserRole + 1, 1); root->addChild(frequency);
    for (int i = 0; i < m_data.frequencies.size(); ++i) { auto *child = new QTreeWidgetItem({QStringLiteral("%1: %2 cm⁻¹").arg(i + 1).arg(m_data.frequencies[i].wavenumber)}); child->setData(0, Qt::UserRole, QStringLiteral("frequency")); child->setData(0, Qt::UserRole + 1, i); frequency->addChild(child); }
    auto *scan = new QTreeWidgetItem({(m_data.format == QStringLiteral("GSM")
        ? QStringLiteral("GSM 경로 (%1)") : QStringLiteral("퍼텐셜 에너지 스캔 (%1)"))
        .arg(m_data.scanPoints.size())}); scan->setData(0, Qt::UserRole, QStringLiteral("section")); scan->setData(0, Qt::UserRole + 1, 2); root->addChild(scan);
    for (int i = 0; i < m_data.scanPoints.size(); ++i) {
        const auto &point = m_data.scanPoints[i];
        auto *child = new QTreeWidgetItem({QStringLiteral("점 %1%2").arg(point.index)
            .arg(point.inProgress ? QStringLiteral(" · 진행 중") : QString())});
        child->setData(0, Qt::UserRole, QStringLiteral("scan"));
        child->setData(0, Qt::UserRole + 1, i); scan->addChild(child);
    }
    auto *irc = new QTreeWidgetItem({QStringLiteral("IRC (%1)").arg(m_data.ircPoints.size())}); irc->setData(0, Qt::UserRole, QStringLiteral("section")); irc->setData(0, Qt::UserRole + 1, 3); root->addChild(irc);
    for (int i = 0; i < m_data.ircPoints.size(); ++i) { auto *child = new QTreeWidgetItem({QStringLiteral("점 %1").arg(i + 1)}); child->setData(0, Qt::UserRole, QStringLiteral("irc")); child->setData(0, Qt::UserRole + 1, i); irc->addChild(child); }
    auto *td = new QTreeWidgetItem({QStringLiteral("TD-DFT (%1)").arg(m_data.excitedStates.size())}); td->setData(0, Qt::UserRole, QStringLiteral("tddft")); root->addChild(td);
    auto *atoms = new QTreeWidgetItem({QStringLiteral("원자 속성")}); atoms->setData(0, Qt::UserRole, QStringLiteral("section")); atoms->setData(0, Qt::UserRole + 1, 5); root->addChild(atoms);
    for (int i = 0; i < m_data.molecule.atoms.size(); ++i) { auto *child = new QTreeWidgetItem({QStringLiteral("%1 %2").arg(m_data.molecule.atoms[i].index).arg(m_data.molecule.atoms[i].element)}); child->setData(0, Qt::UserRole, QStringLiteral("atom")); child->setData(0, Qt::UserRole + 1, i); atoms->addChild(child); }
    root->addChild(new QTreeWidgetItem({QStringLiteral("오비탈 (%1)").arg(m_data.orbitals.size())}));
    root->setExpanded(true); geometry->setExpanded(true); frequency->setExpanded(true);
}

void MainWindow::showFrame(int index) {
    if (index < 0 || index >= m_data.frames.size()) return;
    m_animation->stop();
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    m_currentFrame = index;
    m_view->setCoordinates(m_data.frames[index].coordinates);
    updateGeometryReadout();
    m_view->setFrequencyMode(nullptr);
    m_analysis->setCurrentFrame(index);
    if(m_outputDock->isVisible())updateOutputSource("frame_geometry",index);
    setStatus(QStringLiteral("프레임 %1 표시").arg(index + 1));
}

void MainWindow::showFrequency(int index) {
    if (index < 0 || index >= m_data.frequencies.size()) return;
    m_animation->stop();
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    m_currentFrequency = index;
    if(m_outputDock->isVisible())updateOutputSource("frequency_modes",index);
    const FrequencyMode &mode = m_data.frequencies[index];
    const QVector<QVector3D> base = m_data.frames.isEmpty() ? [&] { QVector<QVector3D> values; for (const Atom &a : m_data.molecule.atoms) values.push_back(a.position); return values; }() : m_data.frames.last().coordinates;
    bool hasMotion = false;
    for (const QVector3D &vector : mode.displacement) {
        if (vector.lengthSquared() > 1.0e-12f) { hasMotion = true; break; }
    }
    if (!hasMotion) {
        m_view->setCoordinates(base);
        m_view->setFrequencyMode(&mode);
        m_analysis->setCurrentFrequency(index);
        setStatus(QStringLiteral("선택한 진동 모드에 변위 벡터가 없습니다."));
        return;
    }
    m_animation->setFrequency(base, mode);
    m_animation->setAmplitude(m_analysis->frequencyAmplitude());
    m_animation->setCycleTime(m_analysis->frequencyCycleTime());
    m_view->setCoordinates(base);
    m_view->setFrequencyMode(&mode);
    m_view->rebuildBonds(m_style.recountCutoffScale, m_style.recountHydrogen, m_style.recountMetalCoordination);
    const QVector<QVector3D> firstCoordinates = AnimationController::coordinatesAtPhase(
        base, mode, m_analysis->frequencyAmplitude(), 0.5);
    m_view->setCoordinates(firstCoordinates);
    m_view->rebuildBonds(m_style.recountCutoffScale, m_style.recountHydrogen, m_style.recountMetalCoordination);
    m_analysis->setCurrentFrequency(index);
    if (m_analysis->coordinateViewActive()) m_analysis->setCoordinateMolecule(m_view->molecule());
    m_animation->startFrequency();
    setStatus(QStringLiteral("진동 모드 %1: %2 cm⁻¹").arg(mode.number).arg(mode.wavenumber));
}

void MainWindow::showScanPoint(int index) {
    if (index < 0 || index >= m_data.scanPoints.size()) return;
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    const int frame = m_data.scanPoints[index].frameIndex;
    if (frame >= 0 && frame < m_data.frames.size()) showFrame(frame);
    m_analysis->setCurrentScanPoint(index);
    const auto &point = m_data.scanPoints[index];
    if (m_data.format == QStringLiteral("GSM"))
        setStatus(QStringLiteral("GSM 노드 %1 | ΔE = %2 kcal/mol").arg(point.index)
                  .arg(point.relativeKcal, 0, 'f', 3));
    else
        setStatus(QStringLiteral("PES Scan 점 %1 | E = %2 Hartree%3").arg(point.index)
                  .arg(point.energyHartree, 0, 'f', 9).arg(frame < 0 ? QStringLiteral(" | 대응 구조 없음") : QString()));
    QTreeWidgetItemIterator it(m_explorer);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toString() == QStringLiteral("scan") &&
            (*it)->data(0, Qt::UserRole + 1).toInt() == index) {
            m_explorer->setCurrentItem(*it); break;
        }
        ++it;
    }
}

void MainWindow::showIrcPoint(int index) {
    if (index < 0 || index >= m_data.ircPoints.size()) return;
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    if(m_outputDock->isVisible())updateOutputSource("irc",index);
    const int frame = m_data.ircPoints[index].frameIndex;
    if (frame >= 0 && frame < m_data.frames.size()) showFrame(frame);
    setStatus(QStringLiteral("IRC 점 %1 표시").arg(index + 1));
}

void MainWindow::setCarbonHydrogensVisible(bool visible) {
    auto &molecule = m_data.molecule;
    for (const Bond &bond : molecule.bonds) {
        if (bond.first < 0 || bond.second < 0 ||
            bond.first >= molecule.atoms.size() || bond.second >= molecule.atoms.size()) continue;
        Atom &first = molecule.atoms[bond.first];
        Atom &second = molecule.atoms[bond.second];
        if (first.atomicNumber == 1 && second.atomicNumber == 6) first.visible = visible;
        if (second.atomicNumber == 1 && first.atomicNumber == 6) second.visible = visible;
    }
    // Update flags only: keep the camera, live animation coordinates and selection.
    updateAtomVisibility();
    setStatus(visible ? QStringLiteral("탄소에 결합된 수소 표시 복원")
                      : QStringLiteral("탄소에 결합된 수소 숨기기"));
}

void MainWindow::updateAtomVisibility() {
    QVector<bool> visibility;
    visibility.reserve(m_data.molecule.atoms.size());
    for (const Atom &atom : m_data.molecule.atoms) visibility.push_back(atom.visible);
    m_view->setAtomVisibility(visibility);
}

void MainWindow::showAtom(int index) {
    if (index < 0 || index >= m_data.molecule.atoms.size()) return;
    m_workspace->setCurrentWidget(m_moleculeWorkspace);
    m_view->setSelectedAtom(index);
    setStatus(QStringLiteral("원자 %1 (%2) 선택 | charge %3 | spin %4")
                  .arg(m_data.molecule.atoms[index].index).arg(m_data.molecule.atoms[index].element)
                  .arg(m_data.molecule.atoms[index].charge, 0, 'f', 6).arg(m_data.molecule.atoms[index].spinDensity, 0, 'f', 6));
}

} // namespace youngcraft
