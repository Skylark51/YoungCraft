#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/StyleEditorDialog.h"

#include <QActionGroup>
#include <QDateTime>
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
#include <QPlainTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>

namespace youngcraft {

void MainWindow::setStatus(const QString &message, int timeout) {
    m_statusLabel->setText(message);
    statusBar()->showMessage(message, timeout);
}

void MainWindow::showToolMessage(const QString &title, const QString &message) {
    QMessageBox::information(this, title, message.isEmpty() ? QStringLiteral("현재 데이터가 없습니다.") : message);
}

bool MainWindow::isTextInputFocused() const {
    QWidget *widget = QApplication::focusWidget();
    return qobject_cast<QLineEdit *>(widget) || qobject_cast<QTextEdit *>(widget) || qobject_cast<QPlainTextEdit *>(widget) ||
           qobject_cast<QAbstractSpinBox *>(widget);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/sidebarPanelState"), saveState());
    StyleManager::save(m_style);
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    const bool input = isTextInputFocused();
    if(!input && m_drawingMode && !(event->modifiers() & (Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier))) {
        if(event->key()==Qt::Key_Escape){m_view->endDrawingChain();m_view->setProperty("pendingFragment",QString());m_view->setProperty("pendingCenter",0);m_view->clearInsertionPreview();event->accept();return;}
        if(event->key()>=Qt::Key_1&&event->key()<=Qt::Key_3){m_drawBondOrder->setCurrentIndex(m_drawBondOrder->findData(double(event->key()-Qt::Key_0)));event->accept();return;}
        if(!event->text().isEmpty()&&event->text()[0].isLetter()){
            qint64 now=QDateTime::currentMSecsSinceEpoch();QString buffer=m_drawBar->property("elementBuffer").toString();
            if(now-m_drawBar->property("elementBufferTime").toLongLong()>700||buffer.size()>=2)buffer.clear();
            QString candidate=buffer+event->text();int z=atomicNumberFor(candidate);
            if(z<1){candidate=event->text();z=atomicNumberFor(candidate);}
            if(z>0)m_drawElement->setCurrentText(elementSymbol(z));
            else if(candidate.compare("A",Qt::CaseInsensitive)==0)m_drawBondOrder->setCurrentIndex(m_drawBondOrder->findData(0.0));
            m_drawBar->setProperty("elementBuffer",candidate);m_drawBar->setProperty("elementBufferTime",now);event->accept();return;
        }
    }
    if (!input && event->key() == Qt::Key_Space) { m_view->centerOnMolecule(); event->accept(); return; }
    if (!input && event->key() == Qt::Key_Return) { m_view->bestView(); event->accept(); return; }
    if (!input && event->key() == Qt::Key_Enter) { m_view->bestView(); event->accept(); return; }
    if (!input && event->key() == Qt::Key_5 && (event->modifiers() & Qt::KeypadModifier)) { m_view->updateRotationCenter(); event->accept(); return; }
    if (!input && event->key() == Qt::Key_Left) { m_analysis->selectPrevious(); event->accept(); return; }
    if (!input && event->key() == Qt::Key_Right) { m_analysis->selectNext(); event->accept(); return; }
    if (!input && event->key() == Qt::Key_P) { m_analysis->currentTab() == 1 ? toggleFrequencyAnimation() : toggleFrameAnimation(); event->accept(); return; }
    if (!input && event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_I) { copyImage(); event->accept(); return; }
    if (!input && event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_D) { setAtomDragMode(!m_dragAtoms); event->accept(); return; }
    QMainWindow::keyPressEvent(event);
}

} // namespace youngcraft
