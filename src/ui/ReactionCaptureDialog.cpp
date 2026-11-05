#include "ui/ReactionCaptureDialog.h"
#include "ui/CaptureRotationControls.h"
#include "animation/AnimationController.h"
#include "export/ReactionCapture.h"
#include "export/ImageExporter.h"
#include "io/FileDispatcher.h"
#include "render/MoleculeView.h"
#include <QBuffer>
#include <QApplication>
#include <QCheckBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QFormLayout>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUuid>
#include <QUrl>
#include <QVBoxLayout>
#include <cmath>
#include <memory>

namespace youngcraft {
namespace {
QVector3D boundsCenter(const Molecule &molecule) {
    if(molecule.isEmpty())return {};
    auto minimum=molecule.atoms.first().position,maximum=minimum;
    for(const auto &atom:molecule.atoms)for(int j=0;j<3;++j){minimum[j]=qMin(minimum[j],atom.position[j]);maximum[j]=qMax(maximum[j],atom.position[j]);}
    return (minimum+maximum)*0.5f;
}
}
ReactionCaptureDialog::ReactionCaptureDialog(const CalculationData &data, MoleculeView *source,
        double amplitude, double cycle, bool recount, QWidget *parent, bool si)
    : QDialog(parent),m_data(data),m_amplitude(amplitude),m_cycle(cycle),m_recount(recount) {
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("ReactionCaptureDialog"));
    setProperty("tsPath",data.filePath);
    setWindowTitle(QStringLiteral("맞춤 캡처 — IRC · TS · IRC")); resize(1160,590);
    auto *layout=new QVBoxLayout(this);
    auto *help=new QLabel(QStringLiteral("가운데 TS를 회전·확대하면 양쪽도 따라갑니다. 배경은 내보낼 때 투명하며 세 그림의 너비와 높이는 같습니다."));
    help->setWordWrap(true);layout->addWidget(help);
    auto *purpose=new QHBoxLayout;layout->addLayout(purpose);
    m_si=new QCheckBox(QStringLiteral("SI용으로 만드나요? (정지 PNG · 투명 배경)"));m_si->setObjectName("CaptureForSI");m_si->setChecked(si);purpose->addWidget(m_si);
    m_includeTs=new QCheckBox(QStringLiteral("TS 포함 — 3장 (해제하면 양쪽 IRC 2장)"));m_includeTs->setObjectName("CaptureIncludeTS");m_includeTs->setChecked(true);purpose->addWidget(m_includeTs);
    // Share label and viewport rows: wrapped filenames must not shift one view.
    auto *row=m_previewGrid=new QGridLayout;layout->addLayout(row,1);row->setRowStretch(1,1);
    for(int i=0;i<3;++i) {
        row->setColumnStretch(i,1);
        m_names[i]=new QLabel(i==1 ? QFileInfo(data.filePath).fileName() : QStringLiteral("IRC 읽는 중…"));
        m_names[i]->setWordWrap(true);m_names[i]->setAlignment(Qt::AlignLeft|Qt::AlignTop);auto policy=m_names[i]->sizePolicy();policy.setHorizontalPolicy(QSizePolicy::Ignored);m_names[i]->setSizePolicy(policy);row->addWidget(m_names[i],0,i);
        m_views[i]=new MoleculeView; m_views[i]->setMinimumSize(180,260);
        m_views[i]->setStyle(source->style());row->addWidget(m_views[i],1,i);
    }
    // Use the final TS geometry even when the source is in a displaced frame.
    if(!data.frames.isEmpty()) for(int i=0;i<m_data.molecule.atoms.size();++i)
        m_data.molecule.atoms[i].position=data.frames.last().coordinates.value(i,m_data.molecule.atoms[i].position);
    for(int i=0;i<m_data.molecule.atoms.size();++i) {
        const auto atom=source->molecule().atoms.value(i);
        m_data.molecule.atoms[i].visible=atom.visible;
        m_data.molecule.atoms[i].displayOpacity=atom.displayOpacity;
        m_data.molecule.atoms[i].colorOverride=atom.colorOverride;
        m_data.molecule.atoms[i].radiusScale=atom.radiusScale;
        m_initialVisibility.push_back(atom.visible);
    }
    for(const auto &bond:m_data.molecule.bonds) {
        if(bond.first<0 || bond.second<0 || bond.first>=m_data.molecule.atoms.size() || bond.second>=m_data.molecule.atoms.size())continue;
        const int a=m_data.molecule.atoms[bond.first].atomicNumber,b=m_data.molecule.atoms[bond.second].atomicNumber;
        if(a==1 && b==6)m_carbonHydrogens.insert(bond.first);
        if(a==6 && b==1)m_carbonHydrogens.insert(bond.second);
    }
    m_views[1]->setMolecule(m_data.molecule);
    auto sourceCamera=source->cameraState();sourceCamera.centerOffset+=boundsCenter(source->molecule())-boundsCenter(m_data.molecule);
    synchronize(sourceCamera);
    connect(source,&MoleculeView::cameraChanged,this,[this,source]{
        auto camera=source->cameraState();camera.centerOffset+=boundsCenter(source->molecule())-boundsCenter(m_data.molecule);synchronize(camera);
    });
    for(auto *view:m_views) connect(view,&MoleculeView::cameraChanged,this,[this,view]{
        auto camera=view->cameraState();
        if(view==m_views[1])camera.centerOffset+=boundsCenter(view->molecule())-boundsCenter(m_data.molecule);
        synchronize(camera);
    });
    for(int i=0;i<data.frequencies.size();++i)
        if(data.frequencies[i].imaginary || data.frequencies[i].wavenumber<0) {m_mode=i;break;}
    m_animation=new AnimationController(this);
    m_animation->setAmplitude(amplitude);m_animation->setCycleTime(cycle);
    if(m_mode>=0) {
        QVector<QVector3D> base;for(const auto &atom:m_data.molecule.atoms) base.push_back(atom.position);
        m_animation->setFrequency(base,m_data.frequencies[m_mode]);
        connect(m_animation,&AnimationController::coordinatesReady,this,[this](const QVector<QVector3D> &coords){
            m_views[1]->setCoordinates(coords);
            if(m_recount) m_views[1]->rebuildBonds(m_views[1]->style().recountCutoffScale,m_views[1]->style().recountHydrogen,m_views[1]->style().recountMetalCoordination);
        });
        if(!m_si->isChecked())m_animation->startFrequency();
    }
    const auto initialCamera=m_camera;
    layout->addWidget(captureRotationControls({m_views[0],m_views[1],m_views[2]},this,[this,initialCamera]{synchronize(initialCamera);}));
    auto *controls=new QHBoxLayout;layout->addLayout(controls);
    auto *swap=new QPushButton(QStringLiteral("IRC 좌우 교환"));swap->setObjectName("SwapReactionSides");controls->addWidget(swap);
    connect(swap,&QPushButton::clicked,this,[this]{
        if(!m_ready)return;
        const auto l=m_views[0]->molecule(),r=m_views[2]->molecule();
        const auto camera=m_camera;
        m_views[0]->setMolecule(r);m_views[2]->setMolecule(l);synchronize(camera);
        const auto name=m_names[0]->text();m_names[0]->setText(m_names[2]->text());m_names[2]->setText(name);
    });
    auto *preset=new QComboBox; preset->setObjectName("CaptureOfficePreset");preset->addItems({QStringLiteral("Word (각 5 cm)"),QStringLiteral("PowerPoint (각 10 cm)")});controls->addWidget(preset);
    m_width=new QDoubleSpinBox;m_width->setRange(1,30);m_width->setDecimals(1);m_width->setSuffix(QStringLiteral(" cm / 그림"));controls->addWidget(m_width);
    auto *settings=new QSettings(this);
    preset->setCurrentIndex(settings->value("reactionCapture/preset",0).toInt()==1?1:0);
    m_ppt=preset->currentIndex()==1;
    auto loadWidth=[this,settings,preset]{m_width->setValue(settings->value(preset->currentIndex()==0?"reactionCapture/wordCm":"reactionCapture/pptCm",preset->currentIndex()==0?5.0:10.0).toDouble());};
    loadWidth();
    connect(preset,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,settings,preset,loadWidth]{m_ppt=preset->currentIndex()==1;settings->setValue("reactionCapture/preset",preset->currentIndex());loadWidth();});
    connect(m_width,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[=](double v){settings->setValue(preset->currentIndex()==0?"reactionCapture/wordCm":"reactionCapture/pptCm",v);});
    auto *hydrogenControls=new QHBoxLayout;layout->addLayout(hydrogenControls);
    auto *hideHydrogens=new QPushButton(QStringLiteral("수소 숨기기 (C–H)"));hideHydrogens->setObjectName("HideCaptureCarbonHydrogens");hydrogenControls->addWidget(hideHydrogens);
    auto *keepHydrogens=new QPushButton(QStringLiteral("선택 수소 제외 숨기기"));keepHydrogens->setObjectName("HideCaptureUnselectedHydrogens");hydrogenControls->addWidget(keepHydrogens);
    keepHydrogens->setToolTip(QStringLiteral("체크한 수소만 남기고 나머지 모든 수소를 숨깁니다. 같은 원자 번호에 세 구조 모두 적용합니다."));
    auto *restoreHydrogens=new QPushButton(QStringLiteral("수소 복원"));restoreHydrogens->setObjectName("RestoreCaptureHydrogens");hydrogenControls->addWidget(restoreHydrogens);
    connect(hideHydrogens,&QPushButton::clicked,this,[this]{m_hydrogenVisibility=HydrogenVisibility::HideCarbon;updateHydrogenVisibility();});
    connect(keepHydrogens,&QPushButton::clicked,this,[this]{
        if(m_keptHydrogens.isEmpty()){m_status->setText(QStringLiteral("먼저 남길 수소를 구조에서 클릭하거나 아래 목록에서 체크하세요."));return;}
        m_hydrogenVisibility=HydrogenVisibility::SelectedOnly;updateHydrogenVisibility();
    });
    connect(restoreHydrogens,&QPushButton::clicked,this,[this]{m_hydrogenVisibility=HydrogenVisibility::ShowAll;updateHydrogenVisibility();});
    auto *hydrogenHelp=new QLabel(QStringLiteral("남길 수소: 구조의 H를 클릭하거나 번호를 체크하세요. 선택 수소 제외 숨기기는 체크한 H만 남깁니다."));hydrogenHelp->setWordWrap(true);layout->addWidget(hydrogenHelp);
    m_hydrogens=new QListWidget;m_hydrogens->setObjectName("CaptureKeptHydrogens");
    m_hydrogens->setFlow(QListView::LeftToRight);m_hydrogens->setWrapping(true);m_hydrogens->setResizeMode(QListView::Adjust);
    m_hydrogens->setGridSize(QSize(110,27));m_hydrogens->setMaximumHeight(65);m_hydrogens->setSelectionMode(QAbstractItemView::NoSelection);layout->addWidget(m_hydrogens);
    for(int i=0;i<m_data.molecule.atoms.size();++i) {
        const auto &atom=m_data.molecule.atoms[i];if(atom.atomicNumber!=1)continue;
        auto *item=new QListWidgetItem(QStringLiteral("H #%1").arg(atom.index>0?atom.index:i+1),m_hydrogens);
        item->setData(Qt::UserRole,i);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(Qt::Unchecked);
        item->setToolTip(m_carbonHydrogens.contains(i)?QStringLiteral("TS에서 탄소에 결합된 수소"):QStringLiteral("TS에서 C–H 이외의 수소"));
    }
    connect(m_hydrogens,&QListWidget::itemChanged,this,[this](QListWidgetItem *item){
        const int index=item->data(Qt::UserRole).toInt();
        if(item->checkState()==Qt::Checked)m_keptHydrogens.insert(index);else m_keptHydrogens.remove(index);
        updateHydrogenVisibility();
    });
    for(auto *view:m_views)connect(view,&MoleculeView::atomPicked,this,[this](int index){
        if(index<0 || index>=m_data.molecule.atoms.size() || m_data.molecule.atoms[index].atomicNumber!=1)return;
        for(int row=0;row<m_hydrogens->count();++row) {
            auto *item=m_hydrogens->item(row);if(item->data(Qt::UserRole).toInt()!=index)continue;
            item->setCheckState(item->checkState()==Qt::Checked?Qt::Unchecked:Qt::Checked);break;
        }
    });
    auto *buttons=new QHBoxLayout;layout->addLayout(buttons);
    const QStringList texts={QStringLiteral("한 줄 복사"),QStringLiteral("왼쪽 복사"),QStringLiteral("TS GIF 복사"),QStringLiteral("오른쪽 복사"),QStringLiteral("PNG·GIF 3개 저장")};
    for(int i=0;i<texts.size();++i) {
        auto *button=new QPushButton(texts[i]);button->setEnabled(false);buttons->addWidget(button);m_exports.push_back(button);
        connect(button,&QPushButton::clicked,this,[this,i]{exportPanels(i);});
    }
    auto *captionForm=new QFormLayout;layout->addLayout(captionForm);
    m_figureNumber=new QLineEdit(QStringLiteral("SX"));m_figureNumber->setObjectName("CaptureFigureNumber");captionForm->addRow(QStringLiteral("Figure 번호"),m_figureNumber);
    m_structureName=new QLineEdit;m_structureName->setObjectName("CaptureStructureName");m_structureName->setPlaceholderText(QStringLiteral("XX — 원하는 구조 이름"));captionForm->addRow(QStringLiteral("구조 이름"),m_structureName);
    m_caption=new QPlainTextEdit;m_caption->setObjectName("CaptureCaption");m_caption->setMaximumHeight(72);captionForm->addRow(QStringLiteral("캡션 (직접 편집 가능)"),m_caption);
    auto *copyCaption=new QPushButton(QStringLiteral("캡션 복사"));copyCaption->setObjectName("CopyCaptureCaption");captionForm->addRow(copyCaption);
    connect(copyCaption,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(m_caption->toPlainText());});
    connect(m_figureNumber,&QLineEdit::textChanged,this,[this]{updateCaption();});
    connect(m_structureName,&QLineEdit::textChanged,this,[this]{updateCaption();});
    connect(m_si,&QCheckBox::toggled,this,[this]{updateCaptureMode();});
    connect(m_includeTs,&QCheckBox::toggled,this,[this]{updateCaptureMode();});
    updateCaptureMode();updateCaption();
    m_status=new QLabel(QStringLiteral("같은 폴더에서 IRC 최종 구조를 자동으로 찾고 있습니다…"));m_status->setWordWrap(true);layout->addWidget(m_status);
    const auto paths=ReactionCapture::companionPaths(data.filePath);
    if(paths.size()!=2 || m_mode<0 || data.frequencies.value(m_mode).displacement.size()!=data.molecule.atoms.size()) {
        m_status->setText(QStringLiteral("-freq-ts-freq.out 파일과 유효한 허수 진동 모드가 필요합니다."));return;
    }
    // Parsing large output files must not block rotation or the main window.
    auto results=std::make_shared<QVector<ParseResult>>();
    auto *worker=QThread::create([paths,results]{for(const auto &path:paths) results->push_back(FileDispatcher::load(path).parseResult);});
    connect(worker,&QThread::finished,worker,&QObject::deleteLater);
    connect(worker,&QThread::finished,this,[this,paths,results]{
        for(int side=0;side<2;++side) {
            const int index=side==0?0:2;auto parsed=results->at(side);
            if(!parsed.success || parsed.data.molecule.isEmpty()) {
                m_status->setText(QStringLiteral("IRC 파일을 읽을 수 없습니다: %1\n%2").arg(paths[side],parsed.error));return;
            }
            auto molecule=parsed.data.molecule;
            if(!parsed.data.frames.isEmpty() && parsed.data.frames.last().coordinates.size()==molecule.atoms.size())
                for(int i=0;i<molecule.atoms.size();++i) molecule.atoms[i].position=parsed.data.frames.last().coordinates[i];
            if(!ReactionCapture::alignTo(molecule,m_data.molecule)) {
                m_status->setText(QStringLiteral("TS와 IRC의 원자 수 또는 원자 순서가 다릅니다: %1").arg(paths[side]));return;
            }
            const auto style=m_views[1]->style();
            molecule.rebuildBonds(style.recountCutoffScale,style.recountHydrogen,style.recountMetalCoordination,false);
            for(int i=0;i<molecule.atoms.size();++i)
                molecule.atoms[i].displayOpacity=m_data.molecule.atoms[i].displayOpacity;
            m_views[index]->setMolecule(molecule);m_names[index]->setText(QFileInfo(paths[side]).fileName());
        }
        synchronize(m_camera);m_ready=true;updateHydrogenVisibility();
        for(auto *button:m_exports) button->setEnabled(true);
        updateCaption();
        if(m_si->isChecked()) {m_status->setText(QStringLiteral("SI 준비 완료 · 투명 배경 정지 PNG와 Figure 캡션을 복사·저장할 수 있습니다."));return;}
        m_status->setText(QStringLiteral("준비 완료 · IRC는 원자 순서로 TS에 회전 정렬했습니다. 한 줄 복사는 Office용 그림 3개와 합성 GIF를 제공합니다. 앱의 붙여넣기 형식에 따라 애니메이션 재생 여부가 달라질 수 있습니다."));
    });
    worker->start();
}

void ReactionCaptureDialog::updateCaptureMode() {
    const bool si=m_si->isChecked();
    m_includeTs->setEnabled(si);
    m_views[1]->setVisible(!si || m_includeTs->isChecked());
    m_names[1]->setVisible(!si || m_includeTs->isChecked());
    m_previewGrid->setColumnStretch(1,(!si || m_includeTs->isChecked())?1:0);
    m_exports[2]->setText(si?QStringLiteral("TS PNG 복사"):QStringLiteral("TS GIF 복사"));
    m_exports[2]->setVisible(!si || m_includeTs->isChecked());
    m_exports[4]->setText(si?QStringLiteral("PNG %1개 저장").arg(m_includeTs->isChecked()?3:2):QStringLiteral("PNG·GIF 3개 저장"));
    if(si) {
        m_animation->stop();
        QVector<QVector3D> base;for(const auto &a:m_data.molecule.atoms)base.push_back(a.position);
        m_views[1]->setCoordinates(base);m_views[1]->setBonds(m_data.molecule.bonds);synchronize(m_camera);
    } else if(m_mode>=0)m_animation->startFrequency();
}

void ReactionCaptureDialog::updateCaption() {
    if(!m_caption)return;
    // Pair each displayed element with its actual configured or per-atom color.
    QMap<QString,QStringList> legend;
    const auto style=m_views[1]->style();
    for(const auto &atom:m_data.molecule.atoms) {
        if(!atom.visible || (atom.atomicNumber==1 && !style.showHydrogen))continue;
        const QColor c=style.monochrome?elementColor(atom.atomicNumber,true):atom.colorOverride.isValid()?atom.colorOverride:style.elementColors.value(atom.atomicNumber,elementColor(atom.atomicNumber));
        QString color;
        const int h=c.hsvHue(),s=c.hsvSaturation(),v=c.value();
        if(s<45)color=v>225?"White":v<65?"Black":"Gray";
        else if(h<15 || h>=345)color="Red";
        else if(h<45)color="Orange";
        else if(h<70)color="Yellow";
        else if(h<170)color="Green";
        else if(h<200)color="Cyan";
        else if(h<260)color="Blue";
        else if(h<295)color="Purple";
        else color="Pink";

        if(!legend[color].contains(atom.element))legend[color]<<atom.element;
    }
    QStringList colors,elements;
    for(auto it=legend.cbegin();it!=legend.cend();++it) {
        colors<<it.key().toLower();elements<<it.value().join(QStringLiteral("/"));
    }
    QString description;
    if(colors.size()==1)description=QStringLiteral("%1 represents %2 atoms.").arg(colors.first(),elements.first());
    else if(!colors.isEmpty())description=QStringLiteral("%1 represent %2 atoms, respectively.").arg(colors.join(QStringLiteral(", ")),elements.join(QStringLiteral(", ")));
    if(!description.isEmpty())description[0]=description[0].toUpper();
    const auto number=m_figureNumber->text().trimmed(),name=m_structureName->text().trimmed();
    m_caption->setPlainText(QStringLiteral("Figure %1. Optimized structures of %2. %3")
        .arg(number.isEmpty()?QStringLiteral("SX"):number,name.isEmpty()?QStringLiteral("XX"):name,description));
}

void ReactionCaptureDialog::exportStaticPanels(int kind, const QString &baseDirectory) {
    auto panels=renderPanels();
    if(panels.size()!=3 || panels[0].isNull() || panels[1].isNull() || panels[2].isNull()) {m_status->setText(QStringLiteral("정지 PNG 캡처에 실패했습니다."));return;}
    if(kind==1 || kind==2 || kind==3) {ImageExporter::copyToClipboard(panels[kind==1?0:kind==2?1:2]);m_status->setText(QStringLiteral("투명 배경 정지 PNG를 복사했습니다."));return;}
    if(!m_includeTs->isChecked())panels.removeAt(1);
    const double cm=m_width->value(),height=panels[0].height()*cm/panels[0].width();
    QString html=QStringLiteral("<html><body><p style=\"margin:0;white-space:nowrap\">");
    for(const auto &panel:panels) {
        QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);panel.save(&buffer,"PNG");
        html+=QStringLiteral("<img src=\"data:image/png;base64,%1\" width=\"%2\" height=\"%3\" style=\"width:%4cm;height:%5cm\">").arg(QString::fromLatin1(bytes.toBase64())).arg(qRound(cm*96/2.54)).arg(qRound(height*96/2.54)).arg(cm,0,'f',4).arg(height,0,'f',4);
    }
    html+=QStringLiteral("</p><p>%1</p></body></html>").arg(m_caption->toPlainText().toHtmlEscaped());
    if(kind==4) {
        const QString directory=baseDirectory+QStringLiteral("/Youngcraft-SI-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        if(!QDir().mkpath(directory)){m_status->setText(QStringLiteral("저장 폴더를 만들 수 없습니다."));return;}
        const QStringList names=m_includeTs->isChecked()?QStringList{"1-left.png","2-TS.png","3-right.png"}:QStringList{"1-left.png","2-right.png"};
        for(int i=0;i<panels.size();++i)if(!panels[i].save(directory+"/"+names[i])){m_status->setText(QStringLiteral("PNG 저장 실패"));return;}
        QFile caption(directory+"/caption.txt"),paste(directory+"/paste.html");
        if(!caption.open(QIODevice::WriteOnly) || !paste.open(QIODevice::WriteOnly) || caption.write(m_caption->toPlainText().toUtf8())<0 || paste.write(html.toUtf8())<0){m_status->setText(QStringLiteral("캡션/HTML 저장 실패"));return;}
        m_status->setText(QStringLiteral("SI 저장 완료: %1").arg(directory));return;
    }
    QImage row(panels[0].width()*panels.size(),panels[0].height(),QImage::Format_ARGB32);row.fill(Qt::transparent);
    QPainter painter(&row);for(int i=0;i<panels.size();++i)painter.drawImage(i*panels[0].width(),0,panels[i]);painter.end();
    row.setDotsPerMeterX(panels[0].dotsPerMeterX());row.setDotsPerMeterY(panels[0].dotsPerMeterY());
    ImageExporter::copyToClipboard(row);
    auto *mime=new QMimeData;const auto *original=QApplication::clipboard()->mimeData();
    for(const auto &format:original->formats())mime->setData(format,original->data(format));
    mime->setHtml(html);mime->setText(m_caption->toPlainText());QApplication::clipboard()->setMimeData(mime);
    m_status->setText(QStringLiteral("SI 정지 PNG %1장과 캡션을 복사했습니다 (각 %2 cm). ").arg(panels.size()).arg(cm));
}

void ReactionCaptureDialog::updateHydrogenVisibility() {
    QVector<bool> visibility=m_initialVisibility;
    for(int i=0;i<m_data.molecule.atoms.size();++i) {
        auto &atom=m_data.molecule.atoms[i];
        if(atom.atomicNumber==1) {
            if(m_hydrogenVisibility==HydrogenVisibility::HideCarbon)visibility[i]=!m_carbonHydrogens.contains(i);
            else if(m_hydrogenVisibility==HydrogenVisibility::SelectedOnly)visibility[i]=m_keptHydrogens.contains(i);
            else if(m_hydrogenVisibility==HydrogenVisibility::ShowAll)visibility[i]=true;
        }
        atom.visible=visibility[i];
    }
    for(auto *view:m_views) {
        if(m_hydrogenVisibility!=HydrogenVisibility::Original) {
            auto style=view->style();style.showHydrogen=true;view->setStyle(style);
        }
        view->selection().clear();
        for(int index:m_keptHydrogens)if(visibility[index])view->selection().atoms.insert(index);
        view->setSelectedAtom(-1);view->setAtomVisibility(visibility);
    }
    updateCaption();
    if(!m_status)return;
    if(m_hydrogenVisibility==HydrogenVisibility::HideCarbon)m_status->setText(QStringLiteral("세 구조의 C–H 수소를 숨겼습니다. O–H·N–H 등은 유지합니다."));
    else if(m_hydrogenVisibility==HydrogenVisibility::SelectedOnly)m_status->setText(QStringLiteral("선택한 수소 %1개만 남겼습니다. TS·양쪽 IRC·복사·저장에 동일하게 적용됩니다.").arg(m_keptHydrogens.size()));
    else if(m_hydrogenVisibility==HydrogenVisibility::ShowAll)m_status->setText(QStringLiteral("세 구조의 모든 수소를 복원했습니다."));
}

void ReactionCaptureDialog::synchronize(const CameraState &camera) {
    m_camera=camera;
    for(auto *view:m_views) {
        auto applied=camera;
        if(view==m_views[1])applied.centerOffset+=boundsCenter(m_data.molecule)-boundsCenter(view->molecule());
        view->setCameraState(applied);
    }
}

QVector<QImage> ReactionCaptureDialog::renderPanels() {
    if(!m_ready)return {};
    m_animation->stop();
    QVector<QVector3D> original;for(const auto &atom:m_views[1]->molecule().atoms)original.push_back(atom.position);
    const auto originalBonds=m_views[1]->molecule().bonds;
    const auto restore=qScopeGuard([this,original,originalBonds]{m_views[1]->setCoordinates(original);m_views[1]->setBonds(originalBonds);synchronize(m_camera);if(!m_si->isChecked())m_animation->startFrequency();});
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const auto cursor=qScopeGuard([]{QApplication::restoreOverrideCursor();});
    const QSize size=m_views[1]->size().scaled(1400,1400,Qt::KeepAspectRatio);
    QVector<QVector3D> base;for(const auto &atom:m_data.molecule.atoms)base.push_back(atom.position);
    m_views[1]->setCoordinates(base);synchronize(m_camera);
    const QImage left=m_views[0]->imageAtSize(size,true,true),right=m_views[2]->imageAtSize(size,true,true);
    if(m_si->isChecked())return ReactionCapture::equalPanels(left,{m_views[1]->imageAtSize(size,true,true)},right,1000,m_width->value());
    QVector<QImage> frames;
    const int count=qBound(4,qRound(m_cycle*20),120);
    for(int i=0;i<count;++i) {
        m_views[1]->setCoordinates(AnimationController::coordinatesAtPhase(base,m_data.frequencies[m_mode],m_amplitude,2*3.141592653589793*i/count));
        if(m_recount)m_views[1]->rebuildBonds(m_views[1]->style().recountCutoffScale,m_views[1]->style().recountHydrogen,m_views[1]->style().recountMetalCoordination);
        // Keep the absolute camera target fixed through the entire vibration.
        auto image=m_views[1]->imageAtSize(size,true,true);
        if(image.isNull())return {};
        frames.push_back(image);
    }
    return ReactionCapture::equalPanels(left,frames,right,1000,m_width->value());
}

void ReactionCaptureDialog::exportPanels(int kind) {
    QString directory;
    if(kind==4) {directory=QFileDialog::getExistingDirectory(this,QStringLiteral("새 캡처 폴더를 만들 위치"));if(directory.isEmpty())return;}
    if(m_si->isChecked()){exportStaticPanels(kind,directory);return;}
    auto panels=renderPanels();
    if(panels.size()<3) {m_status->setText(QStringLiteral("캡처 실패: 렌더링 크기를 줄이거나 뷰를 다시 열어주세요."));return;}
    const auto left=panels.takeFirst(),right=panels.takeLast();
    const int delay=qMax(10,qRound(m_cycle*1000/panels.size()/10)*10);
    QString error;
    if(kind==1 || kind==3) {
        ImageExporter::copyToClipboard(kind==1?left:right);
        m_status->setText(QStringLiteral("투명 PNG 복사 완료 · 너비 %1 cm, 높이 %2 cm").arg(m_width->value()).arg(left.height()*m_width->value()/left.width(),0,'f',2));return;
    }
    const QString base=kind==4?directory:QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+QStringLiteral("/reaction-captures");
    directory=base+QStringLiteral("/Youngcraft-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if(!QDir().mkpath(directory)) {m_status->setText(QStringLiteral("캡처 폴더를 만들 수 없습니다."));return;}
    const auto lp=directory+"/1-left.png",tp=directory+"/2-TS.gif",rp=directory+"/3-right.png";
    QVector<QImage> gifFrames=panels;
    if(m_ppt) {
        // PowerPoint ignores HTML dimensions for animated GIFs and uses 96 dpi.
        // Match that intrinsic size while retaining high-resolution endpoint PNGs.
        const int pixels=qMax(1,qRound(m_width->value()*96.0/2.54));
        for(auto &frame:gifFrames) frame=frame.scaledToWidth(pixels,Qt::SmoothTransformation);
    }
    if(!left.save(lp)||!right.save(rp)||!ImageExporter::saveGif(gifFrames,tp,delay,true,true,&error)) {
        m_status->setText(QStringLiteral("저장 실패: ")+error);return;
    }
    const double cm=m_width->value(),height=left.height()*cm/left.width();
    auto tag=[&](const QString &path,double width){
        QFile file(path);if(!file.open(QIODevice::ReadOnly))return QString();
        const QString source=QStringLiteral("data:image/%1;base64,%2").arg(path.endsWith(".gif")?"gif":"png",QString::fromLatin1(file.readAll().toBase64()));
        return QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\" style=\"width:%4cm;height:%5cm\">")
        .arg(source).arg(qRound(width*96/2.54)).arg(qRound(height*96/2.54)).arg(width,0,'f',4).arg(height,0,'f',4);};
    const QString prefix=QStringLiteral("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head><body><!--StartFragment-->");
    const QString suffix=QStringLiteral("<!--EndFragment--></body></html>");
    const QString html=prefix+QStringLiteral("<p style=\"margin:0;white-space:nowrap\">")+tag(lp,cm)+tag(tp,cm)+tag(rp,cm)+QStringLiteral("</p>")+suffix;
    if(kind==4) {
        QFile info(directory+"/paste.html");if(info.open(QIODevice::WriteOnly))info.write(html.toUtf8());
        m_status->setText(QStringLiteral("저장 완료: %1 · PNG 2장 + 투명 TS GIF + 크기 지정 HTML").arg(directory));return;
    }
    if(kind==2) {
        if(!ImageExporter::copyAnimationToClipboard(gifFrames,delay,&error)){m_status->setText(error);return;}
        auto *mime=new QMimeData;const auto *original=QApplication::clipboard()->mimeData();
        mime->setHtml(prefix+tag(tp,cm)+suffix);
        for(const auto &format:original->formats())if(format!="text/uri-list")mime->setData(format,original->data(format));
        QApplication::clipboard()->setMimeData(mime);
    } else {
        QVector<QImage> row;row.reserve(panels.size());
        for(const auto &frame:panels) {
            QImage image(left.width()*3,left.height(),QImage::Format_ARGB32);image.fill(Qt::transparent);
            QPainter p(&image);p.drawImage(0,0,left);p.drawImage(left.width(),0,frame);p.drawImage(left.width()*2,0,right);p.end();row.push_back(image);
        }
        if(!ImageExporter::copyAnimationToClipboard(row,delay,&error)){m_status->setText(error);return;}
        auto *mime=new QMimeData;const auto *original=QApplication::clipboard()->mimeData();
        mime->setHtml(html);
        for(const auto &format:original->formats())if(format!="text/uri-list")mime->setData(format,original->data(format));
        QApplication::clipboard()->setMimeData(mime);
    }
    m_status->setText(QStringLiteral("복사 완료 · 각 %1 × %2 cm · Office에서 원본 서식으로 붙여넣으세요. GIF 재생은 대상 앱/보기 모드에 따라 달라집니다.").arg(cm).arg(height,0,'f',2));
}
}
