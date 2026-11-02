#include "ui/MultiCaptureDialog.h"
#include "ui/CaptureRotationControls.h"
#include "export/ImageExporter.h"
#include <QBuffer>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QUuid>
#include <QVBoxLayout>
#include <cmath>

namespace youngcraft {
MultiCaptureDialog::MultiCaptureDialog(const QVector<CaptureSource> &sources,QWidget *parent)
    :QDialog(parent),m_sources(sources) {
    setAttribute(Qt::WA_DeleteOnClose);setObjectName("MultiCaptureDialog");setWindowTitle(QStringLiteral("맞춤 캡처 — 열린 창·탭 다중 캡처"));resize(1200,800);
    auto *layout=new QVBoxLayout(this);
    auto *help=new QLabel(QStringLiteral("함께 캡처할 구조를 체크하세요. 원자 수가 달라도 선택한 구조의 뷰를 함께 조절합니다. 우클릭: 회전 · Ctrl+우클릭/가운데 버튼: 이동 · 휠: 확대/축소. 열린 시점의 구조·표시 설정을 캡처합니다."));help->setWordWrap(true);layout->addWidget(help);
    m_list=new QListWidget;m_list->setObjectName("CaptureSources");m_list->setMaximumHeight(105);layout->addWidget(m_list);
    auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);auto *canvas=new QWidget;canvas->setProperty("sidebarPatternPaper",true);m_grid=new QGridLayout(canvas);scroll->setWidget(canvas);layout->addWidget(scroll,1);
    for(int i=0;i<sources.size();++i) {
        auto *item=new QListWidgetItem(sources[i].name,m_list);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(Qt::Checked);
        auto *name=new QLabel(sources[i].name,canvas);name->setWordWrap(true);name->setAlignment(Qt::AlignLeft|Qt::AlignTop);auto policy=name->sizePolicy();policy.setHorizontalPolicy(QSizePolicy::Ignored);name->setSizePolicy(policy);m_names.push_back(name);
        auto *view=new MoleculeView(canvas);view->setMinimumSize(260,240);view->setStyle(sources[i].style);view->setMolecule(sources[i].molecule);view->setCameraState(sources[i].camera);
        m_views.push_back(view);
        connect(view,&MoleculeView::cameraChanged,this,[this,view]{synchronize(view);});
    }
    layout->addWidget(captureRotationControls(m_views,this,[this]{for(int i=0;i<m_views.size();++i)m_views[i]->setCameraState(m_sources[i].camera);}));
    auto *buttons=new QHBoxLayout;layout->addLayout(buttons);
    m_width=new QDoubleSpinBox;m_width->setRange(1,30);m_width->setValue(5);m_width->setSuffix(QStringLiteral(" cm / 그림"));buttons->addWidget(m_width);
    for(bool save:{false,true}) {auto *button=new QPushButton(save?QStringLiteral("선택한 PNG 모두 저장"):QStringLiteral("선택한 그림 한 줄 복사"));button->setObjectName(save?"SaveMultiCapture":"CopyMultiCapture");buttons->addWidget(button);connect(button,&QPushButton::clicked,this,[this,save]{exportPanels(save);});}
    m_status=new QLabel;layout->addWidget(m_status);
    connect(m_list,&QListWidget::itemChanged,this,[this]{arrange();});arrange();
}
void MultiCaptureDialog::arrange() {
    while(auto *item=m_grid->takeAt(0))delete item;
    for(int row=0;row<m_grid->rowCount();++row)m_grid->setRowStretch(row,0);
    int n=0;for(int i=0;i<m_views.size();++i) {const bool checked=m_list->item(i)->checkState()==Qt::Checked;m_names[i]->setVisible(checked);m_views[i]->setVisible(checked);if(checked){m_grid->addWidget(m_names[i],2*(n/3),n%3);m_grid->addWidget(m_views[i],2*(n/3)+1,n%3);++n;}}
    for(int column=0;column<3;++column)m_grid->setColumnStretch(column,column<qMin(3,n)?1:0);
    for(int row=0;row<(n+2)/3;++row)m_grid->setRowStretch(2*row+1,1);
    m_status->setText(QStringLiteral("%1개 선택 · 투명 PNG · 원본 구조는 변경하지 않습니다.").arg(n));
}
void MultiCaptureDialog::synchronize(MoleculeView *source) {
    const auto camera=source->cameraState();
    for(int i=0;i<m_views.size();++i)if(m_list->item(i)->checkState()==Qt::Checked)m_views[i]->setCameraState(camera);
}
QVector<QImage> MultiCaptureDialog::renderPanels() {
    QVector<QImage> images;QSize common;
    for(int i=0;i<m_views.size();++i)if(m_list->item(i)->checkState()==Qt::Checked) {
        auto image=m_views[i]->imageAtSize({1400,1400},true,true);
        if(image.isNull())return {};
        common=common.expandedTo(ImageExporter::contentBounds(image).size());images.push_back(image);
    }
    if(images.isEmpty() || common.isEmpty())return {};
    common+=QSize(24,24);
    const int pixels=qMin(qRound(m_width->value()*600/2.54),qMax(1,int(std::sqrt(16000000.0*common.width()/common.height()))));
    for(auto &image:images) {
        auto cropped=image.copy(ImageExporter::contentBounds(image));QImage panel(common,QImage::Format_ARGB32);panel.fill(Qt::transparent);QPainter p(&panel);p.drawImage((common.width()-cropped.width())/2,(common.height()-cropped.height())/2,cropped);p.end();
        image=panel.scaledToWidth(pixels,Qt::SmoothTransformation);const int dpm=qRound(pixels*100/m_width->value());image.setDotsPerMeterX(dpm);image.setDotsPerMeterY(dpm);
    }
    return images;
}
void MultiCaptureDialog::exportPanels(bool save,const QString &targetDirectory) {
    if(m_list->count()==0)return;
    QString directory=targetDirectory;
    if(save && directory.isEmpty()){directory=QFileDialog::getExistingDirectory(this,QStringLiteral("다중 캡처 저장 위치"));if(directory.isEmpty())return;}
    const auto images=renderPanels();if(images.isEmpty()){m_status->setText(QStringLiteral("구조를 선택하거나 렌더링 상태를 확인하세요."));return;}
    if(save) {
        directory+="/Youngcraft-Multi-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        if(!QDir().mkpath(directory)){m_status->setText(QStringLiteral("폴더 생성 실패"));return;}
        int n=0;for(int i=0;i<m_views.size();++i)if(m_list->item(i)->checkState()==Qt::Checked) {
            const auto path=directory+QStringLiteral("/%1.png").arg(++n,3,10,QChar('0'));
            if(!images[n-1].save(path)){m_status->setText(QStringLiteral("PNG 저장 실패: ")+path);return;}
        }
        m_status->setText(QStringLiteral("저장 완료: ")+directory);return;
    }
    // Bound the composite allocation for large selections.
    const int cell=qMin(images.first().width(),qMax(1,16000/int(images.size())));
    const int height=qMax(1,qRound(double(images.first().height())*cell/images.first().width()));
    QImage row(cell*images.size(),height,QImage::Format_ARGB32);row.fill(Qt::transparent);const int dpm=qRound(cell*100/m_width->value());row.setDotsPerMeterX(dpm);row.setDotsPerMeterY(dpm);QPainter p(&row);
    QString html=QStringLiteral("<html><body><p style='white-space:nowrap'>");
    for(int i=0;i<images.size();++i){p.drawImage(QRect(i*cell,0,cell,height),images[i]);QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);images[i].save(&buffer,"PNG");html+=QStringLiteral("<img src='data:image/png;base64,%1' style='width:%2cm;height:%3cm'>").arg(QString::fromLatin1(bytes.toBase64())).arg(m_width->value(),0,'f',4).arg(images[i].height()*m_width->value()/images[i].width(),0,'f',4);}
    p.end();html+="</p></body></html>";
    ImageExporter::copyToClipboard(row);auto *mime=new QMimeData;const auto *original=QApplication::clipboard()->mimeData();for(const auto &format:original->formats())mime->setData(format,original->data(format));mime->setHtml(html);QApplication::clipboard()->setMimeData(mime);
    m_status->setText(QStringLiteral("%1개 그림 복사 완료 · 각 %2 cm").arg(images.size()).arg(m_width->value()));
}
}
