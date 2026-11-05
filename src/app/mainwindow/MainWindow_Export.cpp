#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "animation/AnimationController.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include <QtMath>
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/StyleEditorDialog.h"
#include "ui/ReactionCaptureDialog.h"
#include "ui/MultiCaptureDialog.h"
#include "ui/MechanismAnimationDialog.h"
#include <QTabWidget>
#include "export/ReactionCapture.h"
#include <QInputDialog>

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

void MainWindow::openMechanismAnimation() {
    for(auto *dialog:findChildren<MechanismAnimationDialog *>("MechanismAnimationDialog")) {
        dialog->show();dialog->raise();dialog->activateWindow();return;
    }
    QVector<MechanismStep> steps;
    auto append=[&](MainWindow *owner){
        for(int i=0;i<owner->m_documentTabs->count();++i){const auto document=owner->m_documents.value(owner->m_documentTabs->widget(i));
            if(!document||!document->view||document->view->molecule().isEmpty())continue;
            MechanismStep step;step.name=owner->m_documentTabs->tabText(i);step.path=document->path;step.molecule=document->view->molecule();steps.push_back(step);
        }
    };
    append(this);
    for(auto *widget:QApplication::topLevelWidgets())if(auto *owner=qobject_cast<MainWindow *>(widget);owner&&owner!=this)append(owner);
    (new MechanismAnimationDialog(m_view->style(),steps,this))->show();
}

void MainWindow::openReactionCapture() {
    if (ReactionCapture::companionPaths(m_currentFile).isEmpty()) {
        openMultiCapture();
        return;
    }
    for(auto *dialog:findChildren<QDialog *>(QStringLiteral("ReactionCaptureDialog"))) {
        if(dialog->property("tsPath").toString()==m_currentFile) {dialog->show();dialog->raise();dialog->activateWindow();return;}
    }
    auto *dialog=new ReactionCaptureDialog(m_data,m_view,m_analysis->frequencyAmplitude(),
        m_analysis->frequencyCycleTime(),m_analysis->recountBondsEnabled(),this);
    dialog->show();
    auto *button=new QPushButton(QStringLiteral("열린 창·탭 다중 캡처"),dialog);
    dialog->layout()->addWidget(button);
    connect(button,&QPushButton::clicked,this,&MainWindow::openMultiCapture);
}

void MainWindow::openMultiCapture() {
    QVector<CaptureSource> sources;
    int windowNumber=0;
    for(auto *widget:QApplication::topLevelWidgets()) {
        auto *window=qobject_cast<MainWindow *>(widget);if(!window)continue;
        ++windowNumber;
        for(int i=0;i<window->m_documentTabs->count();++i) {
            const auto document=window->m_documents.value(window->m_documentTabs->widget(i));
            if(!document || !document->view || document->view->molecule().isEmpty())continue;
            auto *view=document->view;
            sources.push_back({QStringLiteral("창 %1%2 · %3").arg(windowNumber).arg(window==this?QStringLiteral(" (현재 창)"):QString(),window->m_documentTabs->tabText(i)),view->molecule(),view->style(),view->cameraState()});
        }
    }
    if(sources.isEmpty()){setStatus(QStringLiteral("먼저 캡처할 구조를 열어주세요."));return;}
    (new MultiCaptureDialog(sources,this))->show();
}

void MainWindow::copyImage() {
    const auto pixels = m_view->size().scaled(3000, 3000, Qt::KeepAspectRatio);
    const auto rendered = m_view->imageAtSize(pixels, true);
    const QRect bounds = ImageExporter::contentBounds(rendered);
    const int padding = m_animation->isPlaying()
        ? qMax(2, qCeil(qMax(bounds.width(), bounds.height()) * 0.02)) : 0;
    const auto image = ImageExporter::cropToContent(rendered, padding);
    if (image.isNull()) { setStatus(QStringLiteral("고해상도 그림을 렌더링할 수 없습니다")); return; }
    ImageExporter::copyToClipboard(image);
    setStatus(QStringLiteral("투명 배경 고해상도 그림을 복사했습니다 (%1 × %2 px, 600 dpi)").arg(image.width()).arg(image.height()));
}

void MainWindow::saveImage() {
    if (m_data.molecule.isEmpty()) { setStatus(QStringLiteral("저장할 구조가 없습니다")); return; }
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("그림 저장"), {}, QStringLiteral("PNG 그림 (*.png);;JPEG 그림 (*.jpg *.jpeg)"));
    if (path.isEmpty()) return;
    bool sizeAccepted = false;
    const int longEdge = QInputDialog::getInt(this, QStringLiteral("그림 해상도"),
        QStringLiteral("긴 변의 픽셀 수 (600 dpi 정보 포함)"), 3000, 512, 8192, 256, &sizeAccepted);
    if (!sizeAccepted) return;
    bool transparent = false;
    if (QFileInfo(path).suffix().compare(QStringLiteral("png"), Qt::CaseInsensitive) == 0) {
        const auto answer = QMessageBox::question(this, QStringLiteral("투명 배경"),
                                                   QStringLiteral("배경을 투명하게 저장할까요?"),
                                                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                   QMessageBox::Yes);
        if (answer == QMessageBox::Cancel) return;
        transparent = answer == QMessageBox::Yes;
    }
    QString error;
    const auto pixels = m_view->size().scaled(longEdge, longEdge, Qt::KeepAspectRatio);
    const auto image = m_view->imageAtSize(pixels, transparent);
    if (image.isNull()) { QMessageBox::warning(this, QStringLiteral("그림 저장 실패"), QStringLiteral("이 해상도로 렌더링할 수 없습니다. 픽셀 수를 줄여주세요.")); return; }
    if (ImageExporter::saveImage(image, path, 1, transparent, &error)) setStatus(QStringLiteral("그림 저장 완료: %1 (%2 × %3 px, 600 dpi)").arg(path).arg(image.width()).arg(image.height()));
    else QMessageBox::warning(this, QStringLiteral("그림 저장 실패"), error);
}

} // namespace youngcraft
