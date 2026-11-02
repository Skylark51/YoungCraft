#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
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
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>

namespace youngcraft {

void MainWindow::openStyleEditor() {
    auto *dialog = new StyleEditorDialog(m_style, &m_data.molecule, m_view->selectedAtom(), this);
    RenderStyle committedStyle = m_style;
    connect(dialog, &StyleEditorDialog::styleApplied, this, [this](const RenderStyle &style) {
        m_style = style;
        m_view->setStyle(style);
        m_atomLabelsAction->setChecked(style.showLabels);
    });
    connect(dialog, &StyleEditorDialog::styleSaved, this, [&committedStyle](const RenderStyle &style) {
        committedStyle = style;
    });
    connect(dialog, &StyleEditorDialog::atomColorApplied, this, [this](int atomIndex, const QColor &color) {
        if (atomIndex < 0 || atomIndex >= m_data.molecule.atoms.size()) return;
        m_data.molecule.atoms[atomIndex].colorOverride = color;
        m_view->setMolecule(m_data.molecule);
    });
    if (dialog->exec() == QDialog::Accepted) StyleManager::save(m_style);
    else { m_style = committedStyle; m_view->setStyle(m_style); }
    m_atomLabelsAction->setChecked(m_style.showLabels);
    dialog->deleteLater();
}

void MainWindow::applyStyle(const RenderStyle &style) {
    m_style = style;
    m_view->setStyle(m_style);
    m_atomLabelsAction->setChecked(m_style.showLabels);
    StyleManager::save(m_style);
}

void MainWindow::choosePreset(const QString &name) {
    if (name == QStringLiteral("사용자 지정")) { openStyleEditor(); return; }
    applyStyle(StyleManager::preset(name));
    setStatus(QStringLiteral("표시 스타일: %1").arg(name));
}

void MainWindow::setPerspective(bool perspective) {
    m_style.perspective = perspective;
    applyStyle(m_style);
}

} // namespace youngcraft
