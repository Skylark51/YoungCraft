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
#include <QVBoxLayout>
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
#include <QSaveFile>
#include <QImageWriter>
#include <cmath>
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

void MainWindow::saveStructure() {
    if(m_view->molecule().isEmpty()){setStatus(QStringLiteral("저장할 구조가 없습니다"));return;}
    QDialog choice(this);choice.setObjectName("SaveStructureChoice");choice.setWindowTitle(QStringLiteral("저장 형식 선택"));
    auto *layout=new QVBoxLayout(&choice);layout->addWidget(new QLabel(QStringLiteral("무엇을 저장할까요?"),&choice));
    auto *picture=new QPushButton(QStringLiteral("그림 저장"),&choice);picture->setObjectName("SavePictureChoice");
    auto *xyz=new QPushButton(QStringLiteral("XYZ 좌표 저장"),&choice);xyz->setObjectName("SaveXYZChoice");
    layout->addWidget(picture);layout->addWidget(xyz);
    auto *cancel=new QDialogButtonBox(QDialogButtonBox::Cancel,&choice);cancel->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));layout->addWidget(cancel);
    bool coordinates=false;
    connect(picture,&QPushButton::clicked,&choice,&QDialog::accept);
    connect(xyz,&QPushButton::clicked,&choice,[&]{coordinates=true;choice.accept();});
    connect(cancel,&QDialogButtonBox::rejected,&choice,&QDialog::reject);
    if(choice.exec()!=QDialog::Accepted)return;
    if(coordinates)saveXYZ();else saveImage();
}

void MainWindow::saveXYZ() {
    const auto molecule=m_view->molecule();
    if(molecule.isEmpty()){setStatus(QStringLiteral("저장할 구조가 없습니다"));return;}
    QString text=QStringLiteral("%1\n%2\n").arg(molecule.atoms.size()).arg(molecule.title.simplified());
    for(const auto &atom:molecule.atoms){
        if(!std::isfinite(atom.position.x())||!std::isfinite(atom.position.y())||!std::isfinite(atom.position.z())){
            QMessageBox::warning(this,QStringLiteral("XYZ 저장 실패"),QStringLiteral("유효하지 않은 원자 좌표가 있습니다."));return;
        }
        text+=QStringLiteral("%1 %2 %3 %4\n").arg(atom.element.isEmpty()?elementSymbol(atom.atomicNumber):atom.element)
            .arg(atom.position.x(),0,'f',8).arg(atom.position.y(),0,'f',8).arg(atom.position.z(),0,'f',8);
    }
    QString path=QFileDialog::getSaveFileName(this,QStringLiteral("XYZ 좌표 저장"),QStringLiteral("structure.xyz"),QStringLiteral("XYZ 좌표 (*.xyz)"));
    if(path.isEmpty())return;
    if(QFileInfo(path).suffix().compare(QStringLiteral("xyz"),Qt::CaseInsensitive)!=0)path+=QStringLiteral(".xyz");
    QSaveFile file(path);const auto bytes=text.toUtf8();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){
        QMessageBox::warning(this,QStringLiteral("XYZ 저장 실패"),file.errorString());return;
    }
    setStatus(QStringLiteral("XYZ 저장 완료: %1 (%2개 원자)").arg(path).arg(molecule.atoms.size()));
}

void MainWindow::saveImage() {
    if (m_view->molecule().isEmpty()) { setStatus(QStringLiteral("저장할 구조가 없습니다")); return; }
    QString filters=QStringLiteral("PNG 그림 (*.png);;GIF 그림 (*.gif);;JPEG 그림 (*.jpg *.jpeg);;TIFF 그림 (*.tif *.tiff);;BMP 그림 (*.bmp)");
    if(QImageWriter::supportedImageFormats().contains("webp"))filters+=QStringLiteral(";;WebP 그림 (*.webp)");
    QString selected=QStringLiteral("PNG 그림 (*.png)");
    QString path=QFileDialog::getSaveFileName(this,QStringLiteral("그림 저장"),{},filters,&selected);
    if (path.isEmpty()) return;
    if(QFileInfo(path).suffix().isEmpty()){
        const QString extension=selected.contains("TIFF")?"tif":selected.contains("JPEG")?"jpg":selected.contains("GIF")?"gif":selected.contains("BMP")?"bmp":selected.contains("WebP")?"webp":"png";
        path+=QStringLiteral(".")+extension;
    }
    const auto suffix=QFileInfo(path).suffix().toLower();
    bool sizeAccepted = false;
    const int longEdge = QInputDialog::getInt(this, QStringLiteral("그림 해상도"),
        suffix=="gif"?QStringLiteral("긴 변의 픽셀 수"):QStringLiteral("긴 변의 픽셀 수 (600 dpi 정보 포함)"), 3000, 512, 8192, 256, &sizeAccepted);
    if (!sizeAccepted) return;
    bool transparent = false;
    if (suffix=="png"||suffix=="gif"||suffix=="tif"||suffix=="tiff"||suffix=="webp") {
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
    if (ImageExporter::saveImage(image, path, 1, transparent, &error)) setStatus((suffix=="gif"?QStringLiteral("그림 저장 완료: %1 (%2 × %3 px)"):QStringLiteral("그림 저장 완료: %1 (%2 × %3 px, 600 dpi)")).arg(path).arg(image.width()).arg(image.height()));
    else QMessageBox::warning(this, QStringLiteral("그림 저장 실패"), error);
}

} // namespace youngcraft
