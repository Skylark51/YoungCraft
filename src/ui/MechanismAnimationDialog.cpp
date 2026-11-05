#include "ui/MechanismAnimationDialog.h"
#include "ui/ReactionCoordinateWidget.h"
#include "render/MoleculeView.h"
#include "io/FileDispatcher.h"
#include "export/ImageExporter.h"
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QSlider>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <QtMath>
#include <QVBoxLayout>
#include <cmath>
#include <memory>

namespace youngcraft {
MechanismAnimationDialog::MechanismAnimationDialog(const RenderStyle &style,const QVector<MechanismStep> &openSteps,QWidget *parent)
    :QDialog(parent),m_openSteps(openSteps) {
    setObjectName("MechanismAnimationDialog");setWindowTitle(QStringLiteral("전체 메커니즘 애니메이션"));
    setAttribute(Qt::WA_DeleteOnClose);resize(1280,820);
    auto *layout=new QVBoxLayout(this);
    auto *help=new QLabel(QStringLiteral("반응 경로 파일을 순서대로 넣으세요. 공통 원자를 따라 부드럽게 움직이고 이탈 조각은 위로 이동합니다.\n"
        "IRC·다중 XYZ의 경로 프레임을 사용할 수 있습니다. 원자 수·순서가 달라지는 구간은 ‘원자 대응’에서 확인하세요. 구조 사이 보간은 분자동역학 계산이 아닙니다."));
    help->setWordWrap(true);layout->addWidget(help);
    auto *splitter=new QSplitter;layout->addWidget(splitter,1);
    m_controls=new QWidget;auto *left=new QVBoxLayout(m_controls);left->setContentsMargins(0,0,0,0);splitter->addWidget(m_controls);
    auto *files=new QHBoxLayout;left->addLayout(files);
    auto *add=new QPushButton(QStringLiteral("파일 추가…"));add->setObjectName("MechanismAddFiles");files->addWidget(add);
    auto *opened=new QPushButton(QStringLiteral("열린 탭 가져오기"));opened->setEnabled(!openSteps.isEmpty());files->addWidget(opened);
    connect(add,&QPushButton::clicked,this,[this]{addFiles(QFileDialog::getOpenFileNames(this,QStringLiteral("반응 경로 파일 — 추가 후 순서를 조절하세요"),{},FileDispatcher::openFilter()));});
    connect(opened,&QPushButton::clicked,this,[this]{m_steps+=m_openSteps;populate();rebuild();});
    auto *projectRow=new QHBoxLayout;left->addLayout(projectRow);
    auto *load=new QPushButton(QStringLiteral("프로젝트 열기…")),*save=new QPushButton(QStringLiteral("프로젝트 저장…"));projectRow->addWidget(load);projectRow->addWidget(save);
    connect(load,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("메커니즘 프로젝트"),{},"Youngcraft Mechanism (*.ycmechanism)");if(!path.isEmpty())openProject(path);});
    connect(save,&QPushButton::clicked,this,[this]{auto path=QFileDialog::getSaveFileName(this,QStringLiteral("메커니즘 프로젝트 저장"),"mechanism.ycmechanism","Youngcraft Mechanism (*.ycmechanism)");if(!path.isEmpty()){if(!path.endsWith(".ycmechanism",Qt::CaseInsensitive))path+=".ycmechanism";saveProject(path);}});
    m_table=new QTableWidget(0,5);m_table->setObjectName("MechanismSteps");
    m_table->setHorizontalHeaderLabels({QStringLiteral("단계 이름"),QStringLiteral("원자 / 경로"),QStringLiteral("구간(초)"),QStringLiteral("역방향"),QStringLiteral("대응 결과")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);m_table->horizontalHeader()->setSectionResizeMode(4,QHeaderView::Stretch);
    m_table->setColumnWidth(1,85);m_table->setColumnWidth(2,82);m_table->setColumnWidth(3,64);m_table->setMinimumWidth(460);left->addWidget(m_table,1);
    connect(m_table,&QTableWidget::itemChanged,this,[this](QTableWidgetItem *item){if(item->column()==0&&item->row()<m_steps.size()){m_steps[item->row()].name=item->text();rebuild(false);}});
    auto *stepButtons=new QHBoxLayout;left->addLayout(stepButtons);
    for(const auto &choice:QVector<QPair<QString,int>>{{QStringLiteral("↑"),-1},{QStringLiteral("↓"),1}}){auto *button=new QPushButton(choice.first);stepButtons->addWidget(button);connect(button,&QPushButton::clicked,this,[this,choice]{moveStep(choice.second);});}
    auto *remove=new QPushButton(QStringLiteral("제거")),*mapping=new QPushButton(QStringLiteral("원자 대응…"));stepButtons->addWidget(remove);stepButtons->addWidget(mapping);
    connect(remove,&QPushButton::clicked,this,[this]{const int row=m_table->currentRow();if(row<0)return;m_steps.removeAt(row);for(auto &s:m_steps)s.mapping.clear();m_focusMode=-1;m_parked.clear();populate();rebuild();});
    connect(mapping,&QPushButton::clicked,this,&MechanismAnimationDialog::editMapping);
    m_path=new QCheckBox(QStringLiteral("추가할 파일의 IRC·다중 XYZ 경로 사용"));m_path->setChecked(true);left->addWidget(m_path);
    m_align=new QCheckBox(QStringLiteral("공통 원자로 단계 간 회전·위치 정렬"));m_align->setChecked(true);left->addWidget(m_align);
    m_preserve=new QCheckBox(QStringLiteral("변하지 않는 결합 길이 유지"));m_preserve->setChecked(true);left->addWidget(m_preserve);
    m_keep=new QCheckBox(QStringLiteral("이탈 조각을 위에 반투명하게 남기기"));m_keep->setChecked(true);left->addWidget(m_keep);
    auto *liftRow=new QHBoxLayout;left->addLayout(liftRow);liftRow->addWidget(new QLabel(QStringLiteral("조각 이동 높이")));
    m_lift=new QDoubleSpinBox;m_lift->setRange(0,30);m_lift->setValue(6);m_lift->setSuffix(QStringLiteral(" Å"));liftRow->addWidget(m_lift);
    for(auto *box:{m_align,m_preserve,m_keep})connect(box,&QCheckBox::toggled,this,[this]{rebuild();});
    connect(m_lift,&QDoubleSpinBox::valueChanged,this,[this]{rebuild();});
    auto *right=new QWidget;auto *rightLayout=new QVBoxLayout(right);rightLayout->setContentsMargins(0,0,0,0);splitter->addWidget(right);
    m_view=new MoleculeView;m_view->setObjectName("MechanismView");m_view->setMinimumSize(400,300);
    auto presentation=style;presentation.showPositiveSurface=false;presentation.showNegativeSurface=false;presentation.showAxes=false;presentation.showLabels=false;presentation.showProperties=false;presentation.showVibrationVectors=false;presentation.showHydrogen=true;
    m_view->setStyle(presentation);rightLayout->addWidget(m_view,1);
    auto *coordinateRow=new QHBoxLayout;rightLayout->addLayout(coordinateRow);
    m_coordinateEdit=new QPushButton(QStringLiteral("반응 좌표 · 2D / 에너지 삽입…"));m_coordinateEdit->setObjectName("ReactionCoordinateEdit");coordinateRow->addWidget(m_coordinateEdit);
    m_coordinateMode=new QPushButton(QStringLiteral("2D · 에너지 동기화"));m_coordinateMode->setObjectName("ReactionCoordinateMode");m_coordinateMode->setCheckable(true);coordinateRow->addWidget(m_coordinateMode);
    m_playerOpen=new QPushButton(QStringLiteral("재생 화면 새 창…"));m_playerOpen->setObjectName("ReactionPlaybackOpen");coordinateRow->addWidget(m_playerOpen);
    m_player=new QDialog(this,Qt::Window);m_player->setObjectName("ReactionPlaybackWindow");
    m_player->setWindowTitle(QStringLiteral("반응 좌표 애니메이션 · 재생 화면"));m_player->setWindowFlag(Qt::WindowMinMaxButtonsHint);m_player->resize(1280,760);
    auto *playerLayout=new QVBoxLayout(m_player);playerLayout->setContentsMargins(8,8,8,8);
    m_coordinate=new ReactionCoordinateWidget(&m_steps,m_player);playerLayout->addWidget(m_coordinate,1);
    m_playerCaption=new QLabel;playerLayout->addWidget(m_playerCaption);
    auto *playerControls=new QHBoxLayout;playerLayout->addLayout(playerControls);
    m_playerPlay=new QPushButton(QStringLiteral("▶ 재생"));m_playerPlay->setObjectName("ReactionPlaybackPlay");playerControls->addWidget(m_playerPlay);
    auto *playerStart=new QPushButton(QStringLiteral("처음"));playerStart->setObjectName("ReactionPlaybackStart");playerControls->addWidget(playerStart);
    m_playerSlider=new QSlider(Qt::Horizontal);m_playerSlider->setObjectName("ReactionPlaybackTimeline");m_playerSlider->setRange(0,10000);playerControls->addWidget(m_playerSlider,1);
    m_playerLoop=new QCheckBox(QStringLiteral("반복"));playerControls->addWidget(m_playerLoop);
    auto *fullscreen=new QPushButton(QStringLiteral("전체 화면 (F11)"));playerControls->addWidget(fullscreen);
    auto toggleFullscreen=[this]{if(m_player->isFullScreen())m_player->showNormal();else m_player->showFullScreen();};
    connect(fullscreen,&QPushButton::clicked,this,toggleFullscreen);
    auto *shortcut=new QShortcut(QKeySequence(Qt::Key_F11),m_player);connect(shortcut,&QShortcut::activated,this,toggleFullscreen);
    connect(m_playerOpen,&QPushButton::clicked,this,&MechanismAnimationDialog::showPlaybackWindow);
    connect(m_playerPlay,&QPushButton::clicked,this,[this]{play(!m_timer.isActive());});
    connect(playerStart,&QPushButton::clicked,this,[this]{play(false);seek(0);});
    connect(m_playerSlider,&QSlider::sliderPressed,this,[this]{play(false);});
    connect(m_playerSlider,&QSlider::valueChanged,this,[this](int value){if(!m_exporting)seek(m_timeline.duration()*value/10000.);});
    connect(m_player,&QDialog::finished,this,[this]{play(false);});
    connect(m_coordinateEdit,&QPushButton::clicked,this,[this]{play(false);m_coordinate->edit();});
    connect(m_coordinateMode,&QPushButton::toggled,this,[this](bool on){play(false);m_coordinate->setEnabledPresentation(on);});
    connect(m_coordinate,&ReactionCoordinateWidget::configurationChanged,this,[this]{updateControls();seek(m_time);if(m_coordinate->active())showPlaybackWindow();});
    m_caption=new QLabel(QStringLiteral("반응 경로를 추가하세요"));m_caption->setWordWrap(true);rightLayout->addWidget(m_caption);
    auto *playback=new QHBoxLayout;rightLayout->addLayout(playback);
    m_play=new QPushButton(QStringLiteral("▶ 재생"));m_play->setObjectName("MechanismPlay");playback->addWidget(m_play);
    auto *start=new QPushButton(QStringLiteral("처음"));playback->addWidget(start);
    m_slider=new QSlider(Qt::Horizontal);m_slider->setObjectName("MechanismTimeline");m_slider->setRange(0,10000);playback->addWidget(m_slider,1);
    m_loop=new QCheckBox(QStringLiteral("반복"));playback->addWidget(m_loop);
    connect(m_loop,&QCheckBox::toggled,m_playerLoop,&QCheckBox::setChecked);
    connect(m_playerLoop,&QCheckBox::toggled,m_loop,&QCheckBox::setChecked);
    connect(m_play,&QPushButton::clicked,this,[this]{play(!m_timer.isActive());});
    connect(start,&QPushButton::clicked,this,[this]{play(false);seek(0);});
    connect(m_slider,&QSlider::sliderPressed,this,[this]{play(false);});
    connect(m_slider,&QSlider::valueChanged,this,[this](int value){if(!m_exporting)seek(m_timeline.duration()*value/10000.);});
    m_timer.setTimerType(Qt::PreciseTimer);m_timer.setInterval(16);
    connect(&m_timer,&QTimer::timeout,this,[this]{double t=m_startedAt+m_clock.elapsed()/1000.;if(t>=m_timeline.duration()){if(m_loop->isChecked()){t=std::fmod(t,m_timeline.duration());m_startedAt=t;m_clock.restart();}else{play(false);t=m_timeline.duration();}}seek(t);});
    auto *focus=new QHBoxLayout;rightLayout->addLayout(focus);
    for(const auto &choice:QVector<QPair<QString,int>>{{QStringLiteral("선택만 표시"),0},{QStringLiteral("나머지 반투명"),1},{QStringLiteral("전체 복원"),2}}) {
        auto *button=new QPushButton(choice.first);button->setObjectName(QStringLiteral("MechanismFocus%1").arg(choice.second));focus->addWidget(button);
        connect(button,&QPushButton::clicked,this,[this,choice]{emphasize(choice.second);});
    }
    m_labels=new QCheckBox(QStringLiteral("원자 번호"));focus->addWidget(m_labels);
    connect(m_labels,&QCheckBox::toggled,this,[this](bool on){auto s=m_view->style();s.showLabels=on;m_view->setStyle(s);});
    auto *fit=new QPushButton(QStringLiteral("전체 경로 맞춤"));focus->addWidget(fit);
    connect(fit,&QPushButton::clicked,this,[this]{if(!m_valid)return;play(false);fitTimeline();seek(m_time);});
    auto *fragments=new QHBoxLayout;rightLayout->addLayout(fragments);
    auto *park=new QPushButton(QStringLiteral("선택 조각 위로 이동")),*unpark=new QPushButton(QStringLiteral("조각 이동 복원"));fragments->addWidget(park);fragments->addWidget(unpark);
    park->setToolTip(QStringLiteral("OTf 등 이동할 조각의 모든 원자를 선택하세요. 현재 시점부터 1.2초 동안 위로 움직이고 반투명해집니다."));
    connect(park,&QPushButton::clicked,this,[this]{if(!m_valid||m_exporting)return;const auto selected=m_view->selection().atoms;if(selected.isEmpty()){m_status->setText(QStringLiteral("먼저 이동할 조각 전체의 원자를 선택하세요."));return;}play(false);m_parked=selected;m_parkStart=m_time;m_timeline.park(m_parked,m_parkStart);seek(m_time);m_status->setText(QStringLiteral("선택 조각은 현재 시점부터 위로 이동합니다. ‘전체 경로 맞춤’으로 이동 영역까지 볼 수 있습니다."));});
    connect(unpark,&QPushButton::clicked,this,[this]{m_parked.clear();m_timeline.park({},0);seek(m_time);});
    auto *exportSettings=new QHBoxLayout;rightLayout->addLayout(exportSettings);
    m_resolution=new QComboBox;for(int width:{640,960,1280,1920})m_resolution->addItem(QStringLiteral("%1 × %2").arg(width).arg(width*9/16),width);m_resolution->setCurrentIndex(1);exportSettings->addWidget(m_resolution);
    m_fps=new QSpinBox;m_fps->setRange(10,60);m_fps->setValue(25);m_fps->setSuffix(" fps");exportSettings->addWidget(m_fps);
    m_transparent=new QCheckBox(QStringLiteral("PNG 투명 배경"));m_transparent->setToolTip(QStringLiteral("GIF·MP4는 현재 배경색에 합성하여 원자 반투명 효과를 유지합니다."));exportSettings->addWidget(m_transparent);
    m_captions=new QCheckBox(QStringLiteral("단계 이름"));m_captions->setChecked(true);exportSettings->addWidget(m_captions);
    auto *exports=new QHBoxLayout;rightLayout->addLayout(exports);
    m_gif=new QPushButton(QStringLiteral("GIF 저장…"));m_mp4=new QPushButton(QStringLiteral("MP4 저장…"));m_png=new QPushButton(QStringLiteral("PNG 시퀀스…"));
    for(auto *button:{m_gif,m_mp4,m_png})exports->addWidget(button);
    connect(m_gif,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getSaveFileName(this,QStringLiteral("메커니즘 GIF"),"mechanism.gif","GIF (*.gif)");if(!p.isEmpty())exportAnimation(p,"gif",m_resolution->currentData().toInt(),m_fps->value());});
    connect(m_mp4,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getSaveFileName(this,QStringLiteral("메커니즘 MP4"),"mechanism.mp4","MP4 (*.mp4)");if(!p.isEmpty())exportAnimation(p,"mp4",m_resolution->currentData().toInt(),m_fps->value());});
    connect(m_png,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getExistingDirectory(this,QStringLiteral("PNG 시퀀스 저장 폴더"));if(!p.isEmpty())exportAnimation(p,"png",m_resolution->currentData().toInt(),m_fps->value());});
    splitter->setSizes({500,760});
    m_status=new QLabel;m_status->setWordWrap(true);m_status->setObjectName("MechanismStatus");layout->addWidget(m_status);
    updateControls();
}

void MechanismAnimationDialog::updateControls() {
    const bool ready=m_valid&&!m_loading&&!m_exporting;
    m_controls->setEnabled(!m_loading&&!m_exporting);m_play->setEnabled(ready);m_slider->setEnabled(ready);
    m_coordinateEdit->setEnabled(ready);m_coordinateMode->setEnabled(ready&&m_coordinate->ready());
    {const QSignalBlocker block(m_coordinateMode);m_coordinateMode->setChecked(m_coordinate->active());}
    m_playerOpen->setEnabled(ready&&m_coordinate->active());
    m_player->setEnabled(!m_loading&&!m_exporting);
    m_playerPlay->setEnabled(ready);m_playerSlider->setEnabled(ready);m_playerLoop->setEnabled(ready);
    if(!m_coordinate->active())m_player->hide();
    m_coordinateMode->setToolTip(QStringLiteral("2개 이상의 단계에서 2D 그림 또는 에너지 중 하나라도 입력하면 활성화됩니다. 입력한 항목만 표시하며, 좌표는 단계 순서대로 증가해야 합니다."));
    m_gif->setEnabled(ready);m_png->setEnabled(ready);m_mp4->setEnabled(ready&&ImageExporter::ffmpegAvailable());
    m_mp4->setToolTip(ImageExporter::ffmpegAvailable()?QString():QStringLiteral("MP4 저장에는 ffmpeg가 필요합니다. GIF·PNG 시퀀스는 바로 저장할 수 있습니다."));
}
void MechanismAnimationDialog::populate() {
    const QSignalBlocker block(m_table);m_table->setRowCount(m_steps.size());
    for(int row=0;row<m_steps.size();++row) {
        const auto &step=m_steps[row];auto *name=new QTableWidgetItem(step.name);name->setToolTip(step.path);m_table->setItem(row,0,name);
        auto *info=new QTableWidgetItem(QStringLiteral("%1 / %2").arg(step.molecule.atoms.size()).arg(qMax(1,int(step.frames.size()))));info->setFlags(info->flags()&~Qt::ItemIsEditable);m_table->setItem(row,1,info);
        auto *seconds=new QDoubleSpinBox;seconds->setRange(.1,120);seconds->setValue(step.seconds);seconds->setSingleStep(.5);m_table->setCellWidget(row,2,seconds);
        connect(seconds,&QDoubleSpinBox::valueChanged,this,[this,row](double value){m_steps[row].seconds=value;rebuild(false);});
        auto *reverse=new QCheckBox;reverse->setChecked(step.reversed);m_table->setCellWidget(row,3,reverse);
        connect(reverse,&QCheckBox::toggled,this,[this,row](bool on){m_steps[row].reversed=on;rebuild();});
        auto *summary=new QTableWidgetItem;summary->setFlags(summary->flags()&~Qt::ItemIsEditable);m_table->setItem(row,4,summary);
    }
}
bool MechanismAnimationDialog::setSteps(const QVector<MechanismStep> &steps) {
    if(m_loading||m_exporting)return false;m_steps=steps;m_focusMode=-1;m_focus.clear();m_parked.clear();populate();return rebuild();
}
bool MechanismAnimationDialog::rebuild(bool fit) {
    play(false);m_valid=false;
    m_coordinate->refresh();
    m_options.align=m_align->isChecked();m_options.preserveBonds=m_preserve->isChecked();m_options.keepDeparted=m_keep->isChecked();m_options.lift=float(m_lift->value());
    QString error;
    if(!m_timeline.build(m_steps,m_options,&error)){m_status->setText(error);updateControls();return false;}
    if(m_focusMode>=0)m_timeline.emphasize(m_focus,m_focusMode);
    m_timeline.park(m_parked,m_parkStart);
    const auto summaries=m_timeline.summaries();
    {const QSignalBlocker block(m_table);for(int row=0;row<summaries.size();++row){m_table->item(row,4)->setText(summaries[row]);m_table->item(row,4)->setToolTip(summaries[row]);}}
    m_view->selection().clear();m_view->setGeometrySelection({});m_view->setSelectedAtom(-1);
    if(fit)fitTimeline();
    m_valid=true;seek(qMin(m_time,m_timeline.duration()));
    m_status->setText(QStringLiteral("%1단계 · 원자 트랙 %2개 · %3초 · Shift/Ctrl로 원자 선택 · 우클릭 회전 · 휠 확대").arg(m_steps.size()).arg(m_timeline.trackCount()).arg(m_timeline.duration(),0,'f',1));
    updateControls();return true;
}
void MechanismAnimationDialog::addFiles(const QStringList &paths) {
    if(paths.isEmpty()||m_loading||m_exporting)return;
    play(false);m_loading=true;updateControls();m_status->setText(QStringLiteral("반응 경로 파일 %1개 읽는 중…").arg(paths.size()));
    struct Results { QVector<MechanismStep> steps;QStringList errors; };
    const auto results=std::make_shared<Results>();const bool usePath=m_path->isChecked();
    auto *worker=QThread::create([paths,usePath,results]{for(const auto &path:paths){const auto file=FileDispatcher::load(path);if(file.parseResult.success&&!file.parseResult.data.molecule.isEmpty())results->steps.push_back(MechanismStep::fromCalculation(file.parseResult.data,usePath));else results->errors.push_back(QFileInfo(path).fileName()+": "+file.parseResult.error);}});
    connect(worker,&QThread::finished,worker,&QObject::deleteLater);
    connect(worker,&QThread::finished,this,[this,results]{m_loading=false;m_steps+=results->steps;populate();rebuild();if(!results->errors.isEmpty())m_status->setText(QStringLiteral("읽지 못한 파일:\n")+results->errors.join('\n'));updateControls();});
    worker->start();
}
void MechanismAnimationDialog::fitTimeline() {
    const auto envelope=m_timeline.envelope();
    m_fittedCenter={};
    if(!envelope.isEmpty()){auto minimum=envelope.atoms.first().position,maximum=minimum;for(const auto &atom:envelope.atoms)for(int axis=0;axis<3;++axis){minimum[axis]=qMin(minimum[axis],atom.position[axis]);maximum[axis]=qMax(maximum[axis],atom.position[axis]);}m_fittedCenter=(minimum+maximum)*.5f;}
    m_view->setMolecule(envelope);
}
void MechanismAnimationDialog::seek(double time) {
    if(!m_valid)return;m_time=qBound(0.,time,m_timeline.duration());m_view->setAnimationMolecule(m_timeline.sample(m_coordinate->mappedTime(m_time)));m_coordinate->setTime(m_time);
    {const QSignalBlocker block(m_slider);m_slider->setValue(qRound(10000*m_time/m_timeline.duration()));}
    {const QSignalBlocker block(m_playerSlider);m_playerSlider->setValue(m_slider->value());}
    m_caption->setText(QStringLiteral("%1   ·   %2 / %3 s").arg(m_timeline.caption(m_time)).arg(m_time,0,'f',2).arg(m_timeline.duration(),0,'f',2));
    m_playerCaption->setText(m_caption->text());
}
void MechanismAnimationDialog::showPlaybackWindow() {
    if(!m_valid||m_loading||m_exporting||!m_coordinate->active())return;
    m_coordinate->setTime(m_time);m_player->show();m_player->raise();m_player->activateWindow();
}
void MechanismAnimationDialog::play(bool on) {
    if(on&&(!m_valid||m_loading||m_exporting))return;
    if(on&&m_coordinate->active())showPlaybackWindow();
    if(on){if(m_time>=m_timeline.duration())seek(0);m_startedAt=m_time;m_clock.restart();m_timer.start();m_play->setText(QStringLiteral("Ⅱ 일시정지"));}
    else{m_timer.stop();m_play->setText(QStringLiteral("▶ 재생"));}
    m_playerPlay->setText(m_play->text());
}
void MechanismAnimationDialog::moveStep(int delta) {
    const int row=m_table->currentRow(),next=row+delta;if(row<0||next<0||next>=m_steps.size())return;
    m_steps.swapItemsAt(row,next);for(auto &s:m_steps)s.mapping.clear();m_focusMode=-1;m_parked.clear();populate();m_table->setCurrentCell(next,0);rebuild();
    m_status->setText(m_status->text()+QStringLiteral(" · 순서를 바꾸어 자동 대응을 다시 계산했습니다."));
}
void MechanismAnimationDialog::editMapping() {
    const int row=m_table->currentRow();if(row<=0||row>=m_steps.size()){m_status->setText(QStringLiteral("첫 단계는 기준 번호입니다. 두 번째 이후 단계를 선택하세요."));return;}
    MechanismTimeline autoMap;auto candidate=m_steps;for(int s=row;s<candidate.size();++s)candidate[s].mapping.clear();QString error;
    if(!autoMap.build(candidate,m_options,&error)){m_status->setText(error);return;}
    int previousCount=0;for(int s=0;s<row;++s)for(int id:autoMap.mappings()[s])previousCount=qMax(previousCount,id+1);
    const auto existing=m_steps[row].mapping.isEmpty()?autoMap.mappings()[row]:m_steps[row].mapping;
    QDialog dialog(this);dialog.setWindowTitle(QStringLiteral("원자 대응 — 단계 %1").arg(row+1));dialog.resize(530,620);
    auto *layout=new QVBoxLayout(&dialog);auto *note=new QLabel(QStringLiteral("현재 파일의 각 원자를 앞 단계에서 사용한 전체 원자 번호에 대응시키세요.\n0은 새로 등장하는 원자입니다. 같은 원소·중복 없는 번호만 허용합니다. 첫 단계의 원자 번호가 기준입니다."));note->setWordWrap(true);layout->addWidget(note);
    auto *table=new QTableWidget(m_steps[row].molecule.atoms.size(),3);table->setHorizontalHeaderLabels({QStringLiteral("파일 원자"),QStringLiteral("원소"),QStringLiteral("전체 원자 번호 (0=신규)")});table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);layout->addWidget(table,1);
    for(int i=0;i<table->rowCount();++i){auto *atom=new QTableWidgetItem(QString::number(i+1)),*element=new QTableWidgetItem(m_steps[row].molecule.atoms[i].element);atom->setFlags(atom->flags()&~Qt::ItemIsEditable);element->setFlags(element->flags()&~Qt::ItemIsEditable);table->setItem(i,0,atom);table->setItem(i,1,element);auto *id=new QSpinBox;id->setRange(0,previousCount);id->setValue(existing[i]>=0&&existing[i]<previousCount?existing[i]+1:0);table->setCellWidget(i,2,id);}
    auto *message=new QLabel;message->setWordWrap(true);layout->addWidget(message);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{auto updated=m_steps;updated[row].mapping.clear();for(int i=0;i<table->rowCount();++i)updated[row].mapping.push_back(qobject_cast<QSpinBox*>(table->cellWidget(i,2))->value()-1);for(int s=row+1;s<updated.size();++s)updated[s].mapping.clear();MechanismTimeline check;QString error;if(!check.build(updated,m_options,&error)){message->setText(error);return;}m_steps=updated;dialog.accept();});
    if(dialog.exec()==QDialog::Accepted){m_focusMode=-1;m_parked.clear();rebuild();}
}
void MechanismAnimationDialog::emphasize(int mode) {
    if(!m_valid||m_exporting)return;const auto selected=m_view->selection().atoms;
    if(mode!=2&&selected.isEmpty()){m_status->setText(QStringLiteral("먼저 강조할 원자를 Shift/Ctrl로 선택하세요. 이 선택은 전체 영상에서 같은 원자를 따라갑니다."));return;}
    m_focus=selected;m_focusMode=mode;m_timeline.emphasize(selected,mode);seek(m_time);
}

bool MechanismAnimationDialog::saveProject(const QString &path) {
    if(!m_valid||m_loading||m_exporting)return false;
    auto root=QJsonDocument::fromJson(MechanismTimeline::saveProject(m_steps,m_options)).object();
    root["reactionCoordinate"]=m_coordinate->settings();
    const auto c=m_view->cameraState();QJsonArray keep;for(int id:m_focus)keep.append(id);
    QJsonArray parked;for(int id:m_parked)parked.append(id);root["parked"]=parked;root["parkStart"]=m_parkStart;
    root["cameraCenter"]=QJsonArray{m_fittedCenter.x(),m_fittedCenter.y(),m_fittedCenter.z()};
    root["focusMode"]=m_focusMode;root["focus"]=keep;root["labels"]=m_labels->isChecked();root["captions"]=m_captions->isChecked();root["transparent"]=m_transparent->isChecked();
    root["camera"]=QJsonArray{c.centerOffset.x(),c.centerOffset.y(),c.centerOffset.z(),c.rotation.scalar(),c.rotation.x(),c.rotation.y(),c.rotation.z(),c.distance,c.orthoScale,c.perspective,c.perspectiveFov};
    QSaveFile file(path);const auto bytes=QJsonDocument(root).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){m_status->setText(QStringLiteral("프로젝트 저장 실패: ")+file.errorString());return false;}
    m_status->setText(QStringLiteral("프로젝트 저장 완료: ")+path);return true;
}
bool MechanismAnimationDialog::openProject(const QString &path) {
    if(m_loading||m_exporting)return false;
    QFile file(path);QString error;QVector<MechanismStep> steps;MechanismOptions options;
    if(!file.open(QIODevice::ReadOnly)||file.size()>128*1024*1024){m_status->setText(QStringLiteral("프로젝트 파일을 읽을 수 없습니다."));return false;}
    const auto bytes=file.readAll();if(!MechanismTimeline::loadProject(bytes,steps,options,&error)){m_status->setText(error);return false;}
    {const QSignalBlocker a(m_align),b(m_preserve),c(m_keep),d(m_lift);m_align->setChecked(options.align);m_preserve->setChecked(options.preserveBonds);m_keep->setChecked(options.keepDeparted);m_lift->setValue(options.lift);}
    if(!setSteps(steps))return false;
    const auto root=QJsonDocument::fromJson(bytes).object();m_focusMode=root["focusMode"].toInt(-1);if(m_focusMode < -1||m_focusMode>2)m_focusMode=-1;
    m_coordinate->restoreSettings(root["reactionCoordinate"].toObject());updateControls();
    m_focus.clear();for(const auto &v:root["focus"].toArray())if(v.isDouble()&&v.toInt()>=0&&v.toInt()<m_timeline.trackCount())m_focus.insert(v.toInt());
    if(m_focusMode>=0)m_timeline.emphasize(m_focus,m_focusMode);
    m_parked.clear();for(const auto &v:root["parked"].toArray())if(v.isDouble()&&v.toInt()>=0&&v.toInt()<m_timeline.trackCount())m_parked.insert(v.toInt());
    m_parkStart=root["parkStart"].toDouble();m_timeline.park(m_parked,m_parkStart);
    m_labels->setChecked(root["labels"].toBool());m_captions->setChecked(root["captions"].toBool(true));m_transparent->setChecked(root["transparent"].toBool());
    const auto camera=root["camera"].toArray();
    if(camera.size()==11){auto c=m_view->cameraState();bool valid=true;for(int i=0;i<11;++i)if(i!=9)valid=valid&&camera[i].isDouble()&&std::isfinite(camera[i].toDouble());if(valid&&camera[7].toDouble()>0&&camera[8].toDouble()>0){c.centerOffset={float(camera[0].toDouble()),float(camera[1].toDouble()),float(camera[2].toDouble())};c.rotation=QQuaternion(float(camera[3].toDouble()),float(camera[4].toDouble()),float(camera[5].toDouble()),float(camera[6].toDouble())).normalized();c.distance=float(camera[7].toDouble());c.orthoScale=float(camera[8].toDouble());c.perspective=camera[9].toBool();c.perspectiveFov=qBound(20.f,float(camera[10].toDouble()),70.f);m_view->setCameraState(c);}}
    const auto center=root["cameraCenter"].toArray();
    if(center.size()==3){const QVector3D oldCenter(float(center[0].toDouble()),float(center[1].toDouble()),float(center[2].toDouble()));if(std::isfinite(oldCenter.x())&&std::isfinite(oldCenter.y())&&std::isfinite(oldCenter.z())){auto c=m_view->cameraState();c.centerOffset+=oldCenter-m_fittedCenter;m_view->setCameraState(c);}}
    seek(0);return true;
}

bool MechanismAnimationDialog::exportAnimation(const QString &path,const QString &format,int width,int fps) {
    if(!m_valid||m_exporting||m_loading||!m_view->isValid()||width<160||width>3840||fps<1||fps>120)return false;
    if(format!="gif"&&format!="png"&&format!="mp4")return false;
    play(false);const double originalTime=m_time;m_exporting=true;updateControls();m_view->setEnabled(false);
    auto restore=qScopeGuard([&]{m_exporting=false;m_view->setEnabled(true);seek(originalTime);updateControls();});
    const int height=((width*9/16)+1)/2*2; width=(width+1)/2*2;
    const int intervals=qMax(1,qCeil(m_timeline.duration()*fps)),count=intervals+1;
    if(count>18000){m_status->setText(QStringLiteral("한 번에 18,000프레임까지 저장합니다. fps나 구간 시간을 줄이거나 영상을 나누어 저장하세요."));return false;}
    if(format=="mp4"&&!ImageExporter::ffmpegAvailable()){m_status->setText(QStringLiteral("MP4 저장에는 ffmpeg가 필요합니다."));return false;}
    QTemporaryDir temporary;if(!temporary.isValid()){m_status->setText(QStringLiteral("임시 폴더 생성 실패"));return false;}
    QString directory=temporary.path();
    if(format=="png"){directory=path+"/Youngcraft-Mechanism-"+QUuid::createUuid().toString(QUuid::WithoutBraces);if(!QDir().mkpath(directory)){m_status->setText(QStringLiteral("PNG 폴더 생성 실패"));return false;}}
    QProgressDialog progress(QStringLiteral("전체 메커니즘 렌더링…"),QStringLiteral("취소"),0,count,this);progress.setWindowModality(Qt::WindowModal);progress.setMinimumDuration(0);progress.setAutoClose(false);progress.setAutoReset(false);
    const bool transparent=format=="png"&&m_transparent->isChecked();
    auto renderFrame=[&](int frame)->QImage {
        progress.setValue(frame);QApplication::processEvents();
        if(progress.wasCanceled())return {};
        const double time=m_timeline.duration()*frame/intervals;m_view->setAnimationMolecule(m_timeline.sample(m_coordinate->mappedTime(time)));
        auto image=m_coordinate->active()?m_coordinate->imageAtTime({width,height},time):m_view->imageAtSize({width,height},transparent,true);
        if(image.isNull())return {};
        if(m_captions->isChecked()&&!m_coordinate->active()) {
            QPainter painter(&image);QFont font=painter.font();font.setPixelSize(qMax(14,height/30));painter.setFont(font);
            const QString text=m_timeline.caption(time);const QRect box(12,height-qMax(60,height/9),width-24,qMax(48,height/10));
            painter.fillRect(box,QColor(255,255,255,210));painter.setPen(QColor(35,45,55));painter.drawText(box.adjusted(10,2,-10,-2),Qt::AlignCenter|Qt::TextWordWrap,text);
        }
        return image;
    };
    QString error;
    if(format=="gif") {
        if(!ImageExporter::saveGifFrames(count,{width,height},path,renderFrame,qRound(1000./fps),false,m_loop->isChecked(),&error)){m_status->setText(error);return false;}
    } else {
        for(int frame=0;frame<count;++frame){const auto image=renderFrame(frame);if(image.isNull()){m_status->setText(progress.wasCanceled()?QStringLiteral("저장 취소 · PNG 시퀀스의 완료된 프레임은 보존됩니다."):QStringLiteral("프레임 렌더링 실패"));return false;}if(!image.save(directory+QStringLiteral("/frame_%1.png").arg(frame,6,10,QChar('0')))){m_status->setText(QStringLiteral("프레임 파일 저장 실패"));return false;}}
    }
    progress.setLabelText(QStringLiteral("영상 인코딩 중…"));progress.setValue(count);
    if(format=="mp4") {
        const auto movie=temporary.path()+"/mechanism.mp4";QProcess encoder;
        encoder.start("ffmpeg",{"-y","-framerate",QString::number(fps),"-i",directory+"/frame_%06d.png","-c:v","libx264","-pix_fmt","yuv420p","-crf","20","-movflags","+faststart",movie});
        QEventLoop loop;QTimer poll;poll.setInterval(50);connect(&poll,&QTimer::timeout,&loop,[&]{if(progress.wasCanceled())encoder.kill();if(encoder.state()==QProcess::NotRunning)loop.quit();});
        connect(&encoder,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),&loop,&QEventLoop::quit);poll.start();loop.exec();
        if(progress.wasCanceled()||encoder.exitStatus()!=QProcess::NormalExit||encoder.exitCode()!=0){m_status->setText(QStringLiteral("MP4 저장 실패/취소: ")+QString::fromLocal8Bit(encoder.readAllStandardError()).right(1200));return false;}
        QFile input(movie);QSaveFile output(path);if(!input.open(QIODevice::ReadOnly)||!output.open(QIODevice::WriteOnly)){m_status->setText(QStringLiteral("MP4 파일 저장 실패"));return false;}
        while(!input.atEnd()){const auto data=input.read(1024*1024);if(data.isEmpty()||output.write(data)!=data.size()){m_status->setText(QStringLiteral("MP4 파일 쓰기 실패"));return false;}}
        if(!output.commit()){m_status->setText(output.errorString());return false;}
    } else if(format=="png") {
        QSaveFile manifest(directory+"/timeline.json");const auto json=QJsonDocument(QJsonObject{{"fps",fps},{"frames",count},{"duration",m_timeline.duration()},{"project",QJsonDocument::fromJson(MechanismTimeline::saveProject(m_steps,m_options)).object()}}).toJson();
        if(!manifest.open(QIODevice::WriteOnly)||manifest.write(json)!=json.size()||!manifest.commit()){m_status->setText(QStringLiteral("타임라인 기록 저장 실패"));return false;}
    }
    m_status->setText(QStringLiteral("저장 완료 · %1 프레임: %2").arg(count).arg(format=="png"?directory:path));return true;
}
void MechanismAnimationDialog::closeEvent(QCloseEvent *event) {if(m_exporting){event->ignore();return;}play(false);m_player->hide();QDialog::closeEvent(event);}
void MechanismAnimationDialog::reject(){if(!m_exporting){play(false);m_player->hide();QDialog::reject();}}
}
