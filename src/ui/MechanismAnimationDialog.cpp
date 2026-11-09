#include "ui/MechanismAnimationDialog.h"
#include "ui/ReactionCoordinateWidget.h"
#include "render/MoleculeView.h"
#include "io/FileDispatcher.h"
#include "export/ImageExporter.h"
#include "export/PowerPointVideo.h"
#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScreen>
#include <QSettings>
#include <QScrollArea>
#include <QShowEvent>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QSlider>
#include <QShortcut>
#include <QSpinBox>
#include <QStyleOptionSlider>
#include <QSplitter>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <QtMath>
#include <QVBoxLayout>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <memory>

namespace youngcraft {
namespace {
class TimelineSlider final : public QSlider {
public:
    TimelineSlider():QSlider(Qt::Horizontal) {setMinimumHeight(25);}
    void setMarks(QVector<QPair<double,QColor>> marks) {m_marks=std::move(marks);update();}
protected:
    void paintEvent(QPaintEvent *event) override {
        QSlider::paintEvent(event);
        QStyleOptionSlider option;initStyleOption(&option);
        const auto groove=style()->subControlRect(QStyle::CC_Slider,&option,QStyle::SC_SliderGroove,this);
        const auto handle=style()->subControlRect(QStyle::CC_Slider,&option,QStyle::SC_SliderHandle,this);
        const int start=groove.left()+handle.width()/2,span=qMax(0,groove.width()-handle.width());
        QPainter painter(this);
        for(const auto &[fraction,color]:m_marks) {
            const int x=start+qRound(span*(option.upsideDown?1-fraction:fraction));
            painter.setPen(QPen(color,2));
            painter.drawLine(x,height()-6,x,height()-2);
        }
    }
private:
    QVector<QPair<double,QColor>> m_marks;
};

constexpr int presentationArrangement=3;
constexpr auto gifDirectoryKey="mechanism/lastGifDirectory";

double shotAspect(int aspect) {
    return aspect==2?9./16:aspect==1?1.:16./9;
}

QSize outputSize(int width,int aspect) {
    const int height=aspect==2?width*16/9:aspect==1?width:width*9/16;
    return {(width+1)/2*2,(height+1)/2*2};
}

struct ShotRegions { QRect structure,graph,artwork; };

ShotRegions shotRegions(QSize size,int arrangement,bool combined,bool hasArtwork,bool forceGraph=false) {
    const int w=size.width(),h=size.height();
    if(!combined)return {{0,0,w,h},{},{}};
    if(arrangement==presentationArrangement) {
        const int top=h*58/100;
        const int inset=hasArtwork?w*24/100:0;
        return {{inset,0,w-2*inset,top},{0,top,w,h-top},
                hasArtwork?QRect(w-inset,top/20,inset,top*72/100):QRect()};
    }
    if(arrangement==1) {
        const int split=w*5/9;
        return {{0,0,split,h},{split,0,w-split,h},{}};
    }
    const int split=forceGraph?h/2:h*5/9;
    return {{0,0,w,split},{0,split,w,h-split},{}};
}

class ShotArtwork final : public QWidget {
public:
    ShotArtwork(ReactionCoordinateWidget *coordinate,QWidget *parent)
        :QWidget(parent),m_coordinate(coordinate) {}
    void setTime(double seconds) {m_time=seconds;update();}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        m_coordinate->drawArtwork(painter,rect(),m_time,false);
    }
private:
    ReactionCoordinateWidget *m_coordinate;
    double m_time=0;
};

class ShotViewport final : public QWidget {
public:
    QWidget body{this};
    std::function<void(QSize)> arrange;
    void setAspect(double aspect) {m_aspect=aspect;fit();}
protected:
    void resizeEvent(QResizeEvent *) override {fit();}
private:
    void fit() {
        QSize size=this->size();
        if(m_aspect>0&&size.height()>0) {
            if(size.width()>size.height()*m_aspect)size.setWidth(qRound(size.height()*m_aspect));
            else size.setHeight(qRound(size.width()/m_aspect));
        }
        body.setGeometry(QRect(QPoint((width()-size.width())/2,(height()-size.height())/2),size));
        if(arrange)arrange(size);
    }
    double m_aspect=0;
};

void fitWindowToScreen(QDialog *dialog,QScreen *screen) {
    if(!screen||dialog->isFullScreen()||dialog->isMaximized())return;
    const auto area=screen->availableGeometry().adjusted(8,8,-8,-8);
    const auto frame=dialog->frameGeometry();
    const auto margins=frame.size()-dialog->size();
    dialog->resize(dialog->size().boundedTo(area.size()-margins));
    const auto adjusted=dialog->frameGeometry();
    if(!area.contains(adjusted))dialog->move(area.center()-QPoint(adjusted.width()/2,adjusted.height()/2));
}
}
MechanismAnimationDialog::MechanismAnimationDialog(const RenderStyle &style,const QVector<MechanismStep> &openSteps,QWidget *parent)
    :QDialog(parent),m_openSteps(openSteps) {
    setObjectName("MechanismAnimationDialog");setWindowTitle(QStringLiteral("전체 메커니즘 애니메이션"));
    setAttribute(Qt::WA_DeleteOnClose);resize(1280,820);
    auto *outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,0);
    auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);
    auto *content=new QWidget;
    auto *layout=new QVBoxLayout(content);
    auto *help=new QLabel(QStringLiteral("반응 경로 파일을 순서대로 넣으세요. 공통 원자를 따라 부드럽게 움직이고 이탈 조각은 위로 이동합니다.\n"
        "IRC·다중 XYZ의 경로 프레임을 사용할 수 있습니다. 원자 수·순서가 달라지는 구간은 ‘원자 대응’에서 확인하세요. 구조 사이 보간은 분자동역학 계산이 아닙니다."));
    help->setWordWrap(true);layout->addWidget(help);
    auto *splitter=new QSplitter;layout->addWidget(splitter,1);
    m_controls=new QWidget;auto *left=new QVBoxLayout(m_controls);left->setContentsMargins(0,0,0,0);splitter->addWidget(m_controls);
    auto *files=new QHBoxLayout;left->addLayout(files);
    auto *add=new QPushButton(QStringLiteral("파일 추가…"));add->setObjectName("MechanismAddFiles");files->addWidget(add);
    auto *addByName = new QPushButton(QStringLiteral("폴더·파일명 추가…"));
    addByName->setObjectName("MechanismAddFilesByName");
    files->addWidget(addByName);
    auto *opened=new QPushButton(QStringLiteral("열린 탭 가져오기"));opened->setEnabled(!openSteps.isEmpty());files->addWidget(opened);
    connect(add,&QPushButton::clicked,this,[this]{addFiles(QFileDialog::getOpenFileNames(this,QStringLiteral("반응 경로 파일 — 추가 후 순서를 조절하세요"),{},FileDispatcher::openFilter()));});
    connect(addByName, &QPushButton::clicked, this, [this] {
        const QString folder = QFileDialog::getExistingDirectory(this, QStringLiteral("반응 경로 파일이 있는 폴더 선택"));
        if (folder.isEmpty()) return;

        bool accepted = false;
        const QString names = QInputDialog::getMultiLineText(
            this, QStringLiteral("파일명으로 추가"),
            QStringLiteral("폴더: %1\n확장자를 포함한 파일명을 한 줄에 하나씩 입력하거나 Excel 셀을 붙여넣으세요.\n위에서 아래로, 같은 줄은 왼쪽에서 오른쪽 순서로 추가합니다.")
                .arg(QDir::toNativeSeparators(folder)),
            {}, &accepted);
        if (accepted) addFilesByName(folder, names);
    });
    connect(opened,&QPushButton::clicked,this,[this]{m_steps+=m_openSteps;populate();rebuild();});
    auto *projectRow=new QHBoxLayout;left->addLayout(projectRow);
    auto *load=new QPushButton(QStringLiteral("프로젝트 열기…")),*save=new QPushButton(QStringLiteral("프로젝트 저장…"));projectRow->addWidget(load);projectRow->addWidget(save);
    connect(load,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("메커니즘 프로젝트"),{},"Youngcraft Mechanism (*.ycmechanism)");if(!path.isEmpty())openProject(path);});
    auto saveProjectAs=[this](QWidget *parent){
        auto path=QFileDialog::getSaveFileName(parent,QStringLiteral("메커니즘 프로젝트 저장"),"mechanism.ycmechanism","Youngcraft Mechanism (*.ycmechanism)");
        if(path.isEmpty())return;
        if(!path.endsWith(".ycmechanism",Qt::CaseInsensitive))path+=".ycmechanism";
        const bool saved=saveProject(path);
        if(parent==m_player)m_playerHint->setText(saved?QStringLiteral("프로젝트를 저장했습니다."):m_status->text());
    };
    connect(save,&QPushButton::clicked,this,[this,saveProjectAs]{saveProjectAs(this);});
    m_table=new QTableWidget(0,7);m_table->setObjectName("MechanismSteps");
    m_table->setHorizontalHeaderLabels({QStringLiteral("단계 이름"),QStringLiteral("원자 / 경로"),QStringLiteral("구간(초)"),QStringLiteral("역방향"),QStringLiteral("대응 결과"),QStringLiteral("광반응"),QStringLiteral("진입 충돌")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);m_table->horizontalHeader()->setSectionResizeMode(4,QHeaderView::Stretch);
    m_table->setColumnWidth(1,85);m_table->setColumnWidth(2,82);m_table->setColumnWidth(3,64);m_table->setColumnWidth(5,72);m_table->setColumnWidth(6,76);m_table->setMinimumWidth(535);left->addWidget(m_table,1);
    connect(m_table,&QTableWidget::itemChanged,this,[this](QTableWidgetItem *item){if(item->column()==0&&item->row()<m_steps.size()){m_steps[item->row()].name=item->text();rebuild(false);}});
    connect(m_table,&QTableWidget::currentCellChanged,this,[this](int row,int,int previous,int){
        if(row!=previous)selectStage(row);
    });
    connect(m_table,&QTableWidget::cellClicked,this,[this](int row,int column){if(column==0)selectStage(row);});
    auto *stepButtons=new QHBoxLayout;left->addLayout(stepButtons);
    for(const auto &choice:QVector<QPair<QString,int>>{{QStringLiteral("↑"),-1},{QStringLiteral("↓"),1}}){auto *button=new QPushButton(choice.first);stepButtons->addWidget(button);connect(button,&QPushButton::clicked,this,[this,choice]{moveStep(choice.second);});}
    auto *remove=new QPushButton(QStringLiteral("제거")),*mapping=new QPushButton(QStringLiteral("원자 대응…"));stepButtons->addWidget(remove);stepButtons->addWidget(mapping);
    connect(remove,&QPushButton::clicked,this,[this]{const int row=m_table->currentRow();if(row<0)return;m_steps.removeAt(row);for(auto &s:m_steps)s.mapping.clear();m_focusMode=-1;m_parked.clear();m_keptHydrogens.clear();m_shotAtoms.clear();populate();rebuild();});
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
    auto *playerBody=new QHBoxLayout(m_player);playerBody->setContentsMargins(8,8,8,8);
    auto *playerLayout=new QVBoxLayout;playerBody->addLayout(playerLayout,1);
    auto *toolsScroll=new QScrollArea;toolsScroll->setWidgetResizable(true);toolsScroll->setFixedWidth(218);
    toolsScroll->setFrameShape(QFrame::NoFrame);playerBody->addWidget(toolsScroll);
    auto *toolsPanel=new QWidget;toolsPanel->setObjectName("ReactionPlaybackTools");
    auto *tools=new QVBoxLayout(toolsPanel);
    tools->addWidget(new QLabel(QStringLiteral("재생 편집")));
    m_playerStages=new QComboBox;m_playerStages->setObjectName("ReactionPlaybackStages");
    m_playerStages->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);tools->addWidget(m_playerStages);
    connect(m_playerStages,&QComboBox::activated,this,&MechanismAnimationDialog::selectStage);
    auto *stageHint=new QLabel(QStringLiteral("광반응은 출발 단계, 충돌은 도착 단계를 선택하세요."));
    stageHint->setWordWrap(true);tools->addWidget(stageHint);
    m_playerPhoto=new QPushButton(QStringLiteral("광반응 결합…"));tools->addWidget(m_playerPhoto);
    connect(m_playerPhoto,&QPushButton::clicked,this,[this]{editPhotoCleavages(m_table->currentRow(),m_playerPhoto,m_playerView);});
    auto *photoSelection=new QPushButton(QStringLiteral("3D 선택 결합에 hν…"));tools->addWidget(photoSelection);
    connect(photoSelection,&QPushButton::clicked,this,[this]{editPhotoSelection(m_playerView);});
    auto *editSegmentButton=new QPushButton(QStringLiteral("구간·충돌 조절…"));tools->addWidget(editSegmentButton);
    connect(editSegmentButton,&QPushButton::clicked,this,&MechanismAnimationDialog::editSegment);
    auto *shot=new QPushButton(QStringLiteral("카메라·구도…"));tools->addWidget(shot);
    connect(shot,&QPushButton::clicked,this,&MechanismAnimationDialog::editShot);
    auto *playerAppearance=new QPushButton(QStringLiteral("불투명도·글자…"));tools->addWidget(playerAppearance);
    connect(playerAppearance,&QPushButton::clicked,this,[this]{editAppearance(m_player);});
    m_playerBarrier=new QCheckBox(QStringLiteral("이 단계 진입 시 충돌"));m_playerBarrier->setObjectName("ReactionPlaybackImpact");tools->addWidget(m_playerBarrier);
    connect(m_playerBarrier,&QCheckBox::toggled,this,[this](bool on){setImpactBarrier(m_table->currentRow(),on);});
    m_playerPreviewBarrier=new QPushButton(QStringLiteral("진입 구간 미리 보기"));tools->addWidget(m_playerPreviewBarrier);
    connect(m_playerPreviewBarrier,&QPushButton::clicked,this,[this]{
        const int stage=m_table->currentRow();
        if(!m_valid||stage<=0)return;
        play(false);
        m_previewStart=m_timeline.stageTime(stage-1)+m_steps[stage-1].holdSeconds();
        seek(m_previewStart);play(true);
        m_previewEnd=m_timeline.stageTime(stage)+m_steps[stage].holdSeconds();
    });
    auto *playerEdit=new QPushButton(QStringLiteral("2D / 에너지 설정…"));tools->addWidget(playerEdit);
    connect(playerEdit,&QPushButton::clicked,m_coordinateEdit,&QPushButton::click);
    auto *playerLabels=new QCheckBox(QStringLiteral("원자 번호"));tools->addWidget(playerLabels);
    auto *playerHide=new QCheckBox(QStringLiteral("수소 모두 숨기기"));tools->addWidget(playerHide);
    auto *playerKeepH=new QPushButton(QStringLiteral("선택 수소만 표시"));tools->addWidget(playerKeepH);
    playerKeepH->setObjectName("ReactionPlaybackKeepHydrogens");
    auto *playerRestoreH=new QPushButton(QStringLiteral("수소 전체 표시"));tools->addWidget(playerRestoreH);
    auto *playerFit=new QPushButton(QStringLiteral("전체 경로 맞춤"));tools->addWidget(playerFit);
    auto *playerExport=new QPushButton(QStringLiteral("영상 내보내기…"));tools->addWidget(playerExport);
    connect(playerExport,&QPushButton::clicked,this,[this,playerExport]{
        QMenu menu(m_player);
        for(auto *button:{m_gif,m_mp4,m_png,m_pptx}) {
            auto *action=menu.addAction(button->text());action->setEnabled(button->isEnabled());
            connect(action,&QAction::triggered,button,&QPushButton::click);
        }
        menu.exec(playerExport->mapToGlobal(playerExport->rect().bottomLeft()));
    });
    auto *playerSave=new QPushButton(QStringLiteral("프로젝트 저장…"));tools->addWidget(playerSave);
    connect(playerSave,&QPushButton::clicked,this,[this,saveProjectAs]{saveProjectAs(m_player);});
    m_playerHint=new QLabel;m_playerHint->setWordWrap(true);tools->addWidget(m_playerHint);
    tools->addStretch();toolsScroll->setWidget(toolsPanel);
    m_playerView=new MoleculeView(m_player);m_playerView->setObjectName("ReactionPlaybackStructure");
    m_playerView->setMinimumSize(0,0);m_playerView->setStyle(presentation);
    auto *viewport=new ShotViewport;m_shotViewport=viewport;playerLayout->addWidget(viewport,1);
    auto backdrop=viewport->body.palette();
    backdrop.setColor(QPalette::Window,presentation.background);
    viewport->body.setPalette(backdrop);
    viewport->body.setAutoFillBackground(true);
    m_playerView->setParent(&viewport->body);
    m_coordinate=new ReactionCoordinateWidget(&m_steps,&viewport->body);
    m_shotArtwork=new ShotArtwork(m_coordinate,&viewport->body);
    m_shotArtwork->setObjectName("ReactionPlaybackArtwork");
    viewport->arrange=[this](QSize size){
        const auto regions=shotRegions(size,m_shotArrangement,m_coordinate->active()&&m_shotArrangement!=2,m_coordinate->hasImages());
        m_playerView->setGeometry(regions.structure);
        m_coordinate->setGeometry(regions.graph);
        m_coordinate->setVisible(!regions.graph.isEmpty());
        m_shotArtwork->setGeometry(regions.artwork);
        m_shotArtwork->setVisible(!regions.artwork.isEmpty());
    };
    connect(m_view,&MoleculeView::cameraChanged,this,[this]{m_playerView->setCameraState(shotCamera(m_view->molecule(),m_view->cameraState()));});
    connect(m_playerView,&MoleculeView::cameraChanged,this,[this]{if(m_shotAtoms.isEmpty()&&!m_savedCameraEnabled)m_view->setCameraState(m_playerView->cameraState());});
    m_playerCaption=new QLabel;m_playerCaption->setWordWrap(true);
    m_playerCaption->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    playerLayout->addWidget(m_playerCaption);
    auto *playerControls=new QHBoxLayout;playerLayout->addLayout(playerControls);
    m_playerPlay=new QPushButton(QStringLiteral("▶ 재생"));m_playerPlay->setObjectName("ReactionPlaybackPlay");playerControls->addWidget(m_playerPlay);
    auto *playerStart=new QPushButton(QStringLiteral("처음"));playerStart->setObjectName("ReactionPlaybackStart");playerControls->addWidget(playerStart);
    m_playerSlider=new TimelineSlider;m_playerSlider->setObjectName("ReactionPlaybackTimeline");m_playerSlider->setRange(0,10000);playerControls->addWidget(m_playerSlider,1);
    auto *precision=new QHBoxLayout;playerLayout->addLayout(precision);
    m_timelineEvents=new QComboBox;m_timelineEvents->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_timelineEvents->setMinimumContentsLength(14);precision->addWidget(m_timelineEvents,1);
    connect(m_timelineEvents,&QComboBox::activated,this,[this](int index){
        if(index<=0||!m_valid)return;
        play(false);seek(m_timelineEvents->itemData(index).toDouble());
        m_timelineEvents->setCurrentIndex(0);
    });
    auto *stops=new QPushButton(QStringLiteral("발표 정지점…"));precision->addWidget(stops);
    stops->setObjectName("MechanismPresentationStops");
    connect(stops,&QPushButton::clicked,this,[this,stops]{editPresentationStops(stops);});
    m_exactTime=new QDoubleSpinBox;m_exactTime->setDecimals(3);m_exactTime->setSingleStep(.001);
    m_exactTime->setSuffix(" s");m_exactTime->setKeyboardTracking(false);precision->addWidget(m_exactTime);
    connect(m_exactTime,&QDoubleSpinBox::valueChanged,this,[this](double time){play(false);seek(time);});
    for(int direction:{-1,1}) {
        auto *frame=new QPushButton(direction<0?QStringLiteral("◀ 프레임"):QStringLiteral("프레임 ▶"));
        precision->addWidget(frame);
        connect(frame,&QPushButton::clicked,this,[this,direction]{
            if(!m_valid)return;
            const double duration=m_timeline.duration();
            const double interval=duration/qMax(1,qCeil(duration*m_fps->value()));
            const double index=m_time/interval;
            const double next=direction<0?std::ceil(index-1e-8)-1:std::floor(index+1e-8)+1;
            play(false);seek(next*interval);
        });
    }
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
    connect(m_coordinateEdit,&QPushButton::clicked,this,[this]{play(false);m_coordinate->edit(m_table->currentRow());});
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
    connect(&m_timer,&QTimer::timeout,this,[this]{
        double time=m_startedAt+m_clock.elapsed()/1000.;
        if(m_previewEnd>=0&&time>=m_previewEnd) {
            const double end=m_previewEnd;
            if(m_loop->isChecked()) {
                m_startedAt=m_previewStart;m_clock.restart();seek(m_previewStart);
            } else {play(false);seek(end);}
            return;
        }
        if(time>=m_timeline.duration()) {
            if(m_loop->isChecked()){time=std::fmod(time,m_timeline.duration());m_startedAt=time;m_clock.restart();}
            else {play(false);time=m_timeline.duration();}
        }
        seek(time);
    });
    auto *focus=new QHBoxLayout;rightLayout->addLayout(focus);
    for(const auto &choice:QVector<QPair<QString,int>>{{QStringLiteral("선택만 표시"),0},{QStringLiteral("나머지 반투명"),1},{QStringLiteral("전체 복원"),2}}) {
        auto *button=new QPushButton(choice.first);button->setObjectName(QStringLiteral("MechanismFocus%1").arg(choice.second));focus->addWidget(button);
        connect(button,&QPushButton::clicked,this,[this,choice]{emphasize(choice.second);});
    }
    m_labels=new QCheckBox(QStringLiteral("원자 번호"));focus->addWidget(m_labels);
    connect(m_labels,&QCheckBox::toggled,this,[this](bool on){
        for(auto *view:{m_view,m_playerView}){auto s=view->style();s.showLabels=on;view->setStyle(s);}
    });
    m_hideHydrogen=new QCheckBox(QStringLiteral("수소 숨기기"));m_hideHydrogen->setObjectName("MechanismHideHydrogen");
    connect(m_hideHydrogen,&QCheckBox::toggled,this,[this](bool hide){
        m_keptHydrogens.clear();
        for(auto *view:{m_view,m_playerView}){auto s=view->style();s.showHydrogen=!hide;view->setStyle(s);}
        seek(m_time);
    });
    connect(m_labels,&QCheckBox::toggled,playerLabels,&QCheckBox::setChecked);
    connect(playerLabels,&QCheckBox::toggled,m_labels,&QCheckBox::setChecked);
    connect(m_hideHydrogen,&QCheckBox::toggled,playerHide,&QCheckBox::setChecked);
    connect(playerHide,&QCheckBox::toggled,m_hideHydrogen,&QCheckBox::setChecked);
    auto restoreHydrogens=[this]{m_keptHydrogens.clear();m_hideHydrogen->setChecked(false);seek(m_time);};
    connect(playerRestoreH,&QPushButton::clicked,this,restoreHydrogens);
    connect(playerKeepH,&QPushButton::clicked,this,[this]{keepSelectedHydrogens(m_playerView);});
    auto *fit=new QPushButton(QStringLiteral("전체 경로 맞춤"));focus->addWidget(fit);
    connect(fit,&QPushButton::clicked,this,[this]{if(!m_valid)return;play(false);fitTimeline();seek(m_time);});
    connect(playerFit,&QPushButton::clicked,fit,&QPushButton::click);
    auto *fragments=new QHBoxLayout;rightLayout->addLayout(fragments);
    auto *park=new QPushButton(QStringLiteral("선택 조각 위로 이동")),*unpark=new QPushButton(QStringLiteral("조각 이동 복원"));fragments->addWidget(park);fragments->addWidget(unpark);fragments->addWidget(m_hideHydrogen);
    auto *hydrogens=new QHBoxLayout;rightLayout->addLayout(hydrogens);
    auto *keepH=new QPushButton(QStringLiteral("선택 수소만 표시"));hydrogens->addWidget(keepH);
    auto *restoreH=new QPushButton(QStringLiteral("수소 전체 표시"));hydrogens->addWidget(restoreH);
    connect(keepH,&QPushButton::clicked,this,[this]{keepSelectedHydrogens(m_view);});
    connect(restoreH,&QPushButton::clicked,this,restoreHydrogens);
    park->setToolTip(QStringLiteral("OTf 등 이동할 조각의 모든 원자를 선택하세요. 현재 시점부터 1.2초 동안 위로 움직이고 반투명해집니다."));
    connect(park,&QPushButton::clicked,this,[this]{if(!m_valid||m_exporting)return;const auto selected=m_view->selection().atoms;if(selected.isEmpty()){m_status->setText(QStringLiteral("먼저 이동할 조각 전체의 원자를 선택하세요."));return;}play(false);m_parked=selected;m_parkStart=m_time;m_timeline.park(m_parked,m_parkStart);seek(m_time);m_status->setText(QStringLiteral("선택 조각은 현재 시점부터 위로 이동합니다. ‘전체 경로 맞춤’으로 이동 영역까지 볼 수 있습니다."));});
    connect(unpark,&QPushButton::clicked,this,[this]{m_parked.clear();m_timeline.park({},0);seek(m_time);});
    auto *exportSettings=new QHBoxLayout;rightLayout->addLayout(exportSettings);
    m_resolution=new QComboBox;for(int width:{640,960,1280,1920})m_resolution->addItem(QStringLiteral("%1 × %2").arg(width).arg(width*9/16),width);m_resolution->setCurrentIndex(1);exportSettings->addWidget(m_resolution);
    m_fps=new QSpinBox;m_fps->setRange(10,60);m_fps->setValue(25);m_fps->setSuffix(" fps");exportSettings->addWidget(m_fps);
    m_captions=new QCheckBox(QStringLiteral("단계 이름"));m_captions->setChecked(true);exportSettings->addWidget(m_captions);
    connect(m_captions,&QCheckBox::toggled,this,&MechanismAnimationDialog::applyTextStyle);
    auto *appearance=new QPushButton(QStringLiteral("불투명도·글자…"));exportSettings->addWidget(appearance);
    appearance->setObjectName("MechanismAppearance");
    connect(appearance,&QPushButton::clicked,this,[this]{editAppearance(this);});
    auto *exports=new QHBoxLayout;rightLayout->addLayout(exports);
    m_gif=new QPushButton(QStringLiteral("GIF 저장…"));m_mp4=new QPushButton(QStringLiteral("MP4 저장…"));m_png=new QPushButton(QStringLiteral("PNG 시퀀스…"));
    m_pptx=new QPushButton(QStringLiteral("발표용 장면…"));m_pptx->setObjectName("MechanismPowerPointExport");
    for(auto *button:{m_gif,m_mp4,m_png,m_pptx})exports->addWidget(button);
    connect(m_pptx,&QPushButton::clicked,this,[this]{
        auto *parent=m_player->isActiveWindow()?m_player:static_cast<QDialog*>(this);
        QDialog settings(parent);settings.setWindowTitle(QStringLiteral("발표용 장면 내보내기"));
        auto *form=new QFormLayout(&settings);
        auto *format=new QComboBox;format->addItems({QStringLiteral("PowerPoint (.pptx)"),QStringLiteral("장면 묶음 (.zip · MP4 + PNG)")});
        form->addRow(QStringLiteral("형식"),format);
        auto *resolution=new QComboBox;
        const QVector<int> widths=m_shotAspect==2?QVector<int>{608,720,1080,1280}
            :m_shotAspect==1?QVector<int>{1080,1440,1920,2160}
            :QVector<int>{1280,1920,2560,3840};
        for(int width:widths)
            resolution->addItem(QStringLiteral("%1 × %2").arg(width).arg(outputSize(width,m_shotAspect).height()),width);
        resolution->setCurrentIndex(m_shotAspect==2?2:m_shotAspect==1?0:1);
        form->addRow(QStringLiteral("해상도"),resolution);
        auto *fps=new QSpinBox;fps->setRange(10,60);fps->setValue(qMax(30,m_fps->value()));fps->setSuffix(" fps");
        form->addRow(QStringLiteral("프레임 속도"),fps);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);form->addRow(buttons);
        connect(buttons,&QDialogButtonBox::accepted,&settings,&QDialog::accept);
        connect(buttons,&QDialogButtonBox::rejected,&settings,&QDialog::reject);
        if(settings.exec()!=QDialog::Accepted)return;
        const bool bundle=format->currentIndex()==1;
        const QString extension=bundle?".zip":".pptx";
        auto path=QFileDialog::getSaveFileName(parent,QStringLiteral("발표용 장면 저장"),"mechanism"+extension,
            bundle?"ZIP (*.zip)":"PowerPoint (*.pptx)");
        if(path.isEmpty())return;
        if(!path.endsWith(extension,Qt::CaseInsensitive))path+=extension;
        exportPowerPoint(path,resolution->currentData().toInt(),fps->value(),bundle);
    });
    connect(m_gif,&QPushButton::clicked,this,&MechanismAnimationDialog::saveGifAs);
    connect(m_mp4,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getSaveFileName(this,QStringLiteral("메커니즘 MP4"),"mechanism.mp4","MP4 (*.mp4)");if(!p.isEmpty())exportAnimation(p,"mp4",m_resolution->currentData().toInt(),m_fps->value());});
    connect(m_png,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getExistingDirectory(this,QStringLiteral("PNG 시퀀스 저장 폴더"));if(!p.isEmpty())exportAnimation(p,"png",m_resolution->currentData().toInt(),m_fps->value());});
    splitter->setSizes({500,760});
    m_status=new QLabel;m_status->setWordWrap(true);m_status->setObjectName("MechanismStatus");layout->addWidget(m_status);
    scroll->setWidget(content);
    applyTextStyle();
    updateControls();
}

void MechanismAnimationDialog::saveGifAs() {
    auto *parent=m_player->isActiveWindow()?m_player:static_cast<QDialog*>(this);
    QFileDialog dialog(parent,QStringLiteral("메커니즘 GIF 저장"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(QStringLiteral("GIF (*.gif)"));
    dialog.setDefaultSuffix(QStringLiteral("gif"));
    const QString folder=QSettings().value(gifDirectoryKey).toString();
    if (QFileInfo(folder).isDir()) dialog.setDirectory(folder);
    dialog.selectFile(QStringLiteral("mechanism.gif"));
    if (dialog.exec()==QDialog::Accepted)
        exportAnimation(dialog.selectedFiles().first(),"gif",m_resolution->currentData().toInt(),m_fps->value());
}

void MechanismAnimationDialog::editAppearance(QWidget *parent) {
    if (m_loading || m_exporting) return;
    play(false);
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("메커니즘 불투명도·글자"));
    auto *form=new QFormLayout(&dialog);
    auto addOpacity=[&](const QString &label, float opacity) {
        auto *spin=new QSpinBox(&dialog);
        spin->setRange(0,100);
        spin->setSuffix(QStringLiteral(" %"));
        spin->setValue(qRound(100*opacity));
        form->addRow(label,spin);
        return spin;
    };
    auto *departed=addOpacity(QStringLiteral("자동 이탈 조각"),m_options.departedOpacity);
    auto *parked=addOpacity(QStringLiteral("위로 이동한 선택 조각"),m_options.parkedOpacity);
    auto *dimmed=addOpacity(QStringLiteral("‘나머지 반투명’ 원자"),m_options.dimmedOpacity);
    auto *font=new QFontComboBox(&dialog);
    font->setCurrentFont(m_videoFont);
    form->addRow(QStringLiteral("영상 글꼴"),font);
    auto *color=new QPushButton(m_videoTextColor.name(),&dialog);
    QColor textColor=m_videoTextColor;
    connect(color,&QPushButton::clicked,&dialog,[&] {
        const QColor choice=QColorDialog::getColor(textColor,&dialog,QStringLiteral("영상 글자 색"));
        if (!choice.isValid()) return;
        textColor=choice;
        color->setText(textColor.name());
    });
    form->addRow(QStringLiteral("영상 글자 색"),color);
    auto *transparent=new QCheckBox(QStringLiteral("GIF 배경 투명"),&dialog);
    transparent->setChecked(m_transparentGif);
    form->addRow(transparent);
    auto *note=new QLabel(QStringLiteral("투명 GIF는 픽셀 패턴으로 반투명 효과를 표현합니다.\n체크를 끄면 현재 배경색에 합성해 저장합니다."),&dialog);
    form->addRow(note);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if (dialog.exec()!=QDialog::Accepted) return;
    const float departedOpacity=departed->value()/100.f;
    const float parkedOpacity=parked->value()/100.f;
    const float dimmedOpacity=dimmed->value()/100.f;
    const bool opacityChanged=departedOpacity!=m_options.departedOpacity ||
        parkedOpacity!=m_options.parkedOpacity || dimmedOpacity!=m_options.dimmedOpacity;
    m_options.departedOpacity=departedOpacity;
    m_options.parkedOpacity=parkedOpacity;
    m_options.dimmedOpacity=dimmedOpacity;
    m_videoFont.setFamily(font->currentFont().family());
    m_videoTextColor=textColor;
    m_transparentGif=transparent->isChecked();
    applyTextStyle();
    if (opacityChanged && m_valid) rebuild(false);
}

void MechanismAnimationDialog::applyTextStyle() {
    for (auto *label : {m_caption,m_playerCaption}) {
        label->setTextFormat(Qt::PlainText);
        label->setFont(m_videoFont);
        auto colors=label->palette();
        colors.setColor(QPalette::WindowText,m_videoTextColor);
        label->setPalette(colors);
    }
    m_coordinate->setTextStyle(m_videoFont,m_videoTextColor,m_captions->isChecked());
    m_shotArtwork->update();
}

void MechanismAnimationDialog::updateControls() {
    const bool ready=m_valid&&!m_loading&&!m_exporting;
    m_controls->setEnabled(!m_loading&&!m_exporting);m_play->setEnabled(ready);m_slider->setEnabled(ready);
    m_coordinateEdit->setEnabled(ready);m_coordinateMode->setEnabled(ready&&m_coordinate->ready());
    {const QSignalBlocker block(m_coordinateMode);m_coordinateMode->setChecked(m_coordinate->active());}
    m_playerOpen->setEnabled(ready);
    m_player->setEnabled(!m_loading&&!m_exporting);
    m_playerPlay->setEnabled(ready);m_playerSlider->setEnabled(ready);m_playerLoop->setEnabled(ready);
    if(!m_valid)m_player->hide();
    refreshStageTools();
    refreshTimelineEvents();
    applyShotLayout();
    m_coordinateMode->setToolTip(QStringLiteral("2개 이상의 단계에서 2D 그림 또는 에너지 중 하나라도 입력하면 활성화됩니다. 입력한 항목만 표시하며, 좌표는 단계 순서대로 증가해야 합니다."));
#ifdef Q_OS_WIN
    m_pptx->setEnabled(ready&&m_coordinate->ready()&&m_coordinate->hasEnergies());
#else
    m_pptx->setEnabled(false);
#endif
    m_pptx->setToolTip(QStringLiteral("Windows에서 PPTX 또는 MP4·PNG 묶음을 만듭니다. 두 형식 모두 3D 구조와 에너지 그래프를 포함합니다."));
    m_gif->setEnabled(ready);m_png->setEnabled(ready);
#ifdef Q_OS_WIN
    m_mp4->setEnabled(ready);
#else
    m_mp4->setEnabled(ready&&ImageExporter::ffmpegAvailable());
#endif
    m_mp4->setToolTip(QStringLiteral("발표 자료에 삽입할 수 있는 H.264 MP4 영상입니다. 배경색을 포함합니다."));
}
void MechanismAnimationDialog::populate() {
    const QSignalBlocker block(m_table),stageBlock(m_playerStages);
    m_table->setRowCount(m_steps.size());
    m_playerStages->clear();
    for(int row=0;row<m_steps.size();++row) {
        const auto &step=m_steps[row];
        m_playerStages->addItem(QStringLiteral("%1. %2").arg(row+1).arg(step.name));
        auto *name=new QTableWidgetItem(step.name);
        const auto hint=QStringLiteral("클릭하여 이 단계의 구조 보기 · 두 번 클릭하여 이름 편집");
        name->setToolTip(step.path.isEmpty()?hint:hint+QLatin1Char('\n')+step.path);
        m_table->setItem(row,0,name);
        auto *info=new QTableWidgetItem(QStringLiteral("%1 / %2").arg(step.molecule.atoms.size()).arg(qMax(1,int(step.frames.size()))));info->setFlags(info->flags()&~Qt::ItemIsEditable);m_table->setItem(row,1,info);
        auto *seconds=new QDoubleSpinBox;seconds->setRange(.1,120);seconds->setValue(step.seconds);seconds->setSingleStep(.5);m_table->setCellWidget(row,2,seconds);
        connect(seconds,&QDoubleSpinBox::valueChanged,this,[this,row](double value){m_steps[row].seconds=value;rebuild(false);});
        auto *reverse=new QCheckBox;reverse->setChecked(step.reversed);m_table->setCellWidget(row,3,reverse);
        connect(reverse,&QCheckBox::toggled,this,[this,row](bool on){m_steps[row].reversed=on;rebuild();});
        auto *summary=new QTableWidgetItem;summary->setFlags(summary->flags()&~Qt::ItemIsEditable);m_table->setItem(row,4,summary);
        auto *photo=new QPushButton;m_table->setCellWidget(row,5,photo);
        connect(photo,&QPushButton::clicked,this,[this,row,photo]{editPhotoCleavages(row,photo,m_view);});
        auto *impact=new QCheckBox;m_table->setCellWidget(row,6,impact);
        impact->setToolTip(QStringLiteral("이 단계로 진입할 때만 충돌과 반동을 연출합니다."));
        connect(impact,&QCheckBox::toggled,this,[this,row](bool on){setImpactBarrier(row,on);});
    }
    if(m_table->currentRow()<0&&!m_steps.isEmpty())m_table->setCurrentCell(0,0);
}
bool MechanismAnimationDialog::setSteps(const QVector<MechanismStep> &steps) {
    if(m_loading||m_exporting)return false;m_steps=steps;m_presentationStops.clear();m_keptHydrogens.clear();m_shotAtoms.clear();m_focusMode=-1;m_focus.clear();m_parked.clear();populate();return rebuild();
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
    for(auto *view:{m_view,m_playerView}) {
        view->selection().clear();view->setGeometrySelection({});view->setSelectedAtom(-1);
    }
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
void MechanismAnimationDialog::addFilesByName(const QString &folder, const QString &names) {
    static const QRegularExpression separator(QStringLiteral("[\\r\\n\\t]+"));
    const QDir directory(folder);
    QStringList paths;
    for (const QString &cell : names.split(separator, Qt::SkipEmptyParts)) {
        QString name=cell.trimmed();
        if (name.size()>=2 && name.startsWith(u'"') && name.endsWith(u'"'))
            name=name.mid(1,name.size()-2).replace(QStringLiteral("\"\""),QStringLiteral("\""));
        if (name.isEmpty()) continue;
        if (name.contains(u'/') || name.contains(u'\\') || name.contains(u':') ||
            name==QStringLiteral(".") || name==QStringLiteral("..")) {
            QMessageBox::warning(this,QStringLiteral("파일명 확인"),
                                 QStringLiteral("경로를 제외한 파일명만 입력하세요.\n%1").arg(name));
            return;
        }
        paths.append(directory.filePath(name));
    }
    addFiles(paths);
}
void MechanismAnimationDialog::fitTimeline() {
    const auto envelope=m_timeline.envelope();
    m_fittedCenter={};
    if(!envelope.isEmpty()){auto minimum=envelope.atoms.first().position,maximum=minimum;for(const auto &atom:envelope.atoms)for(int axis=0;axis<3;++axis){minimum[axis]=qMin(minimum[axis],atom.position[axis]);maximum[axis]=qMax(maximum[axis],atom.position[axis]);}m_fittedCenter=(minimum+maximum)*.5f;}
    m_view->setMolecule(envelope);
    m_playerView->setMolecule(envelope);
    m_playerView->setCameraState(m_view->cameraState());
}
void MechanismAnimationDialog::seek(double time) {
    if(!m_valid)return;
    m_time=qBound(0.,time,m_timeline.duration());
    const double mapped=m_coordinate->mappedTime(m_time);
    const auto molecule=playbackFrame(m_time);
    m_view->setAnimationMolecule(molecule);
    if(m_player->isVisible()) {
        m_playerView->setAnimationMolecule(molecule);
        if(!m_shotAtoms.isEmpty()||m_savedCameraEnabled)
            m_playerView->setCameraState(shotCamera(molecule,m_view->cameraState()));
    }
    {const QSignalBlocker block(m_exactTime);m_exactTime->setValue(m_time);}
    m_coordinate->setTime(m_time);
    static_cast<ShotArtwork*>(m_shotArtwork)->setTime(m_time);
    {const QSignalBlocker block(m_slider);m_slider->setValue(qRound(10000*m_time/m_timeline.duration()));}
    {const QSignalBlocker block(m_playerSlider);m_playerSlider->setValue(m_slider->value());}
    m_caption->setText(QStringLiteral("%1   ·   %2 / %3 s").arg(m_timeline.caption(mapped)).arg(m_time,0,'f',2).arg(m_timeline.duration(),0,'f',2));
    m_playerCaption->setText(m_coordinate->impactFlash(m_time)>0?m_caption->text()+QStringLiteral("  ·  장벽 충돌"):m_caption->text());
}
void MechanismAnimationDialog::showPlaybackWindow() {
    if(!m_valid||m_loading||m_exporting)return;
    if(!m_player->isVisible()) {
        const auto target=screen();
        if(target) {
            m_player->winId();
            m_player->windowHandle()->setScreen(target);
            fitWindowToScreen(m_player,target);
        }
        m_playerView->setCameraState(m_view->cameraState());
        m_playerView->setAnimationMolecule(m_view->molecule());
        m_player->show();
        seek(m_time);
        fitWindowToScreen(m_player,m_player->screen());
        if(!m_player->property("screenFitConnected").toBool()) {
            connect(m_player->windowHandle(),&QWindow::screenChanged,m_player,[this](QScreen *current){
                fitWindowToScreen(m_player,current);
            });
            m_player->setProperty("screenFitConnected",true);
        }
    }
    m_player->raise();m_player->activateWindow();
}
void MechanismAnimationDialog::play(bool on) {
    if(on&&(!m_valid||m_loading||m_exporting))return;
    if(on&&m_coordinate->active()&&!m_player->isVisible())showPlaybackWindow();
    if(on){if(m_time>=m_timeline.duration())seek(0);m_startedAt=m_time;m_clock.restart();m_timer.start();m_play->setText(QStringLiteral("Ⅱ 일시정지"));}
    else{m_timer.stop();m_previewEnd=-1;m_play->setText(QStringLiteral("▶ 재생"));}
    m_playerPlay->setText(m_play->text());
}
void MechanismAnimationDialog::moveStep(int delta) {
    const int row=m_table->currentRow(),next=row+delta;if(row<0||next<0||next>=m_steps.size())return;
    m_steps.swapItemsAt(row,next);for(auto &s:m_steps)s.mapping.clear();m_focusMode=-1;m_parked.clear();m_keptHydrogens.clear();m_shotAtoms.clear();populate();if(rebuild())selectStage(next);
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
    if(dialog.exec()==QDialog::Accepted){m_focusMode=-1;m_parked.clear();m_keptHydrogens.clear();m_shotAtoms.clear();rebuild();}
}
void MechanismAnimationDialog::selectStage(int stage) {
    if(!m_valid||m_loading||m_exporting||stage<0||stage>=m_steps.size())return;
    play(false);
    {const QSignalBlocker block(m_table);m_table->setCurrentCell(stage,0);}
    seek(m_timeline.stageTime(stage));
    refreshStageTools();
}

void MechanismAnimationDialog::editSegment() {
    const int stage=m_table->currentRow();
    if(!m_valid||m_exporting||stage<0||stage>=m_steps.size())return;
    play(false);
    auto &step=m_steps[stage];
    QDialog dialog(m_player);dialog.setWindowTitle(QStringLiteral("구간·충돌 · %1").arg(step.name));
    auto *form=new QFormLayout(&dialog);
    auto field=[&](const QString &label,double low,double high,double value,const QString &unit){
        auto *spin=new QDoubleSpinBox;spin->setRange(low,high);spin->setDecimals(2);spin->setValue(value);
        spin->setSuffix(unit);form->addRow(label,spin);return spin;
    };
    auto *seconds=field(QStringLiteral("기본 이동 시간"),.1,120,step.seconds," s");
    auto *speed=field(QStringLiteral("속도 배율"),.1,10,step.presentation["speed"].toDouble(1)," ×");
    auto *hold=field(QStringLiteral("도착 후 정지"),0,30,step.holdSeconds()," s");
    auto *impact=new QCheckBox(QStringLiteral("이 단계 진입 시 충돌"));
    impact->setEnabled(stage>0);impact->setChecked(stage>0&&step.presentation["impactBarrier"].toBool());form->addRow(impact);
    auto *at=field(QStringLiteral("충돌 위치"),10,70,100*step.presentation["impactAt"].toDouble(.38)," %");
    auto *recoil=field(QStringLiteral("반동 거리"),0,50,100*step.presentation["impactRecoil"].toDouble(.15)," %");
    auto *flash=field(QStringLiteral("섬광 최대 길이"),.05,3,step.presentation["impactFlash"].toDouble(.65)," s");
    for(auto *spin:{at,recoil,flash}) {
        spin->setEnabled(impact->isChecked());connect(impact,&QCheckBox::toggled,spin,&QWidget::setEnabled);
    }
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    step.seconds=seconds->value();step.presentation["speed"]=speed->value();step.presentation["hold"]=hold->value();
    step.presentation["impactBarrier"]=impact->isChecked();step.presentation["impactAt"]=at->value()/100;
    step.presentation["impactRecoil"]=recoil->value()/100;step.presentation["impactFlash"]=flash->value();
    populate();if(rebuild(false))selectStage(stage);
}

void MechanismAnimationDialog::refreshTimelineEvents() {
    const QSignalBlocker blocker(m_exactTime);
    m_exactTime->setRange(0,m_valid?m_timeline.duration():0);
    m_exactTime->setValue(m_valid?m_time:0);
    m_timelineEvents->clear();m_timelineEvents->addItem(QStringLiteral("단계·효과 위치로 이동…"));
    auto *slider=static_cast<TimelineSlider*>(m_playerSlider);
    if(!m_valid){slider->setMarks({});return;}
    struct Event { double time; QString name; QColor color; };
    QVector<Event> events;
    for(int stage=0;stage<m_steps.size();++stage) {
        const auto &step=m_steps[stage];
        events.push_back({m_timeline.stageTime(stage),step.name,QColor(58,115,179)});
        if(stage>0&&step.presentation["impactBarrier"].toBool())
            events.push_back({m_timeline.stageTime(stage-1)+m_steps[stage-1].holdSeconds()+
                qBound(.1,step.presentation["impactAt"].toDouble(.38),.7)*step.travelSeconds(),QStringLiteral("충돌 · ")+step.name,QColor(235,135,45)});
        for(const auto &value:step.presentation["photoCleavages"].toArray()) {
            const auto mark=value.toArray();
            if(mark.size()<2)continue;
            const auto times=m_timeline.cleavageTimes(stage,mark[0].toInt(-1),mark[1].toInt(-1));
            const int i=mark.size()==3?mark[2].toInt(-1):0;
            if(i>=0&&i<times.size())events.push_back({m_coordinate->timeForMapped(times[i]),QStringLiteral("hν · ")+step.name,QColor(142,82,203)});
        }
    }
    for(double time:m_presentationStops)
        if(time>0&&time<m_timeline.duration())events.push_back({time,QStringLiteral("발표 정지점"),QColor(196,53,105)});
    for(double time:m_timeline.topologyChangeTimes())
        events.push_back({m_coordinate->timeForMapped(time),QStringLiteral("결합 변화"),QColor(35,154,125)});
    std::stable_sort(events.begin(),events.end(),[](const auto &a,const auto &b){return a.time<b.time;});
    QVector<QPair<double,QColor>> marks;
    for(const auto &[time,name,color]:events) {
        m_timelineEvents->addItem(QStringLiteral("%1 s · %2").arg(time,0,'f',3).arg(name),time);
        marks.push_back({time/m_timeline.duration(),color});
    }
    slider->setMarks(std::move(marks));
}

void MechanismAnimationDialog::refreshStageTools() {
    const int stage=m_table->currentRow();
    const bool ready=m_valid&&!m_loading&&!m_exporting;
    const QSignalBlocker stageBlock(m_playerStages),barrierBlock(m_playerBarrier);
    m_playerStages->setCurrentIndex(stage);
    m_playerStages->setToolTip(stage>=0&&stage<m_steps.size()?m_steps[stage].name:QString());
    const bool incoming=ready&&stage>0;
    const bool outgoing=ready&&stage>=0&&stage+1<m_steps.size();
    m_playerBarrier->setEnabled(incoming);
    m_playerPreviewBarrier->setEnabled(incoming);
    m_playerPhoto->setEnabled(outgoing);
    for(int row=0;row<m_steps.size();++row) {
        const auto &step=m_steps[row];
        const auto count=step.presentation["photoCleavages"].toArray().size();
        m_playerStages->setItemText(row,QStringLiteral("%1. %2").arg(row+1).arg(step.name));
        auto *photo=qobject_cast<QPushButton*>(m_table->cellWidget(row,5));
        photo->setEnabled(ready&&row+1<m_steps.size());
        photo->setText(count?QStringLiteral("hν %1").arg(count):QStringLiteral("지정…"));
        auto *impact=qobject_cast<QCheckBox*>(m_table->cellWidget(row,6));
        const QSignalBlocker impactBlock(impact);
        impact->setEnabled(ready&&row>0);
        impact->setChecked(row>0&&step.presentation["impactBarrier"].toBool());
        if(row==stage)m_playerBarrier->setChecked(impact->isChecked());
    }
    if(stage<0)m_playerBarrier->setChecked(false);
}

void MechanismAnimationDialog::setImpactBarrier(int stage,bool enabled) {
    if(!m_valid||m_loading||m_exporting||stage<=0||stage>=m_steps.size())return;
    m_steps[stage].presentation["impactBarrier"]=enabled;
    refreshTimelineEvents();
    selectStage(stage);
    m_playerHint->setText(enabled?QStringLiteral("충돌을 켰습니다. ‘진입 구간 미리 보기’로 확인하세요."):QStringLiteral("이 구간의 충돌을 껐습니다."));
}

void MechanismAnimationDialog::editPhotoCleavages(int stage,QWidget *anchor,MoleculeView *view) {
    if(!m_valid||m_loading||m_exporting||stage<0||stage+1>=m_steps.size())return;
    selectStage(stage);
    const auto pairs=m_timeline.cleavableBonds(stage);
    QMenu menu(anchor->window());
    menu.setObjectName("MechanismPhotoBonds");
    menu.addSection(QStringLiteral("%1 → %2").arg(m_steps[stage].name,m_steps[stage+1].name));
    if(pairs.isEmpty()) {
        menu.addAction(QStringLiteral("이 구간에서 끊어지는 결합 없음"))->setEnabled(false);
        menu.exec(anchor->mapToGlobal(anchor->rect().bottomLeft()));
        return;
    }
    const auto marks=m_steps[stage].presentation["photoCleavages"].toArray();
    const auto &atoms=m_steps[stage].molecule.atoms;
    const auto &tracks=m_timeline.mappings()[stage];
    const auto previousSelection=view->geometrySelection();
    for(const auto &[first,second]:pairs) {
        const int a=tracks[first],b=tracks[second];
        auto *action=menu.addAction(QStringLiteral("hν   %1%2 – %3%4").arg(atoms[first].element).arg(a+1)
            .arg(atoms[second].element).arg(b+1));
        action->setCheckable(true);
        for(const auto &value:marks) {
            const auto mark=value.toArray();
            if(mark.size()>=2&&mark[0].toInt()==first&&mark[1].toInt()==second){action->setChecked(true);break;}
        }
        action->setData(QJsonArray{first,second}.toVariantList());
        connect(action,&QAction::hovered,view,[view,a,b]{view->setGeometrySelection({a,b});});
    }
    auto *selected=menu.exec(anchor->mapToGlobal(anchor->rect().bottomLeft()));
    view->setGeometrySelection(previousSelection);
    if(!selected)return;
    const auto pair=selected->data().toList();
    setPhotoMark(stage,pair[0].toInt(),pair[1].toInt(),selected->isChecked()?0:-1);
}

void MechanismAnimationDialog::setPhotoMark(int stage,int first,int second,int occurrence) {
    auto marks=m_steps[stage].presentation["photoCleavages"].toArray();
    for(int i=marks.size()-1;i>=0;--i) {
        const auto mark=marks.at(i).toArray();
        if(mark.size()>=2&&mark[0].toInt()==first&&mark[1].toInt()==second)marks.removeAt(i);
    }
    if(occurrence>=0)marks.append(QJsonArray{first,second,occurrence});
    m_steps[stage].presentation["photoCleavages"]=marks;
    rebuild(false);
    m_playerHint->setText(QStringLiteral("이 단계의 광반응 결합 %1개").arg(marks.size()));
}

void MechanismAnimationDialog::editPhotoSelection(MoleculeView *view) {
    const int stage=m_table->currentRow();
    if(!m_valid||m_exporting||stage<0||stage+1>=m_steps.size())return;
    const auto &source=m_timeline.mappings()[stage];
    QSet<int> selected=view->selection().atoms;
    const auto &bonds=view->molecule().bonds;
    if(view->selection().bonds.size()==1&&selected.isEmpty()) {
        const int id=*view->selection().bonds.begin();
        if(id>=0&&id<bonds.size())selected={bonds[id].first,bonds[id].second};
    }
    if(selected.size()!=2) {
        m_playerHint->setText(QStringLiteral("결합선 하나 또는 양쪽 원자 두 개를 선택하세요."));
        return;
    }
    auto it=selected.cbegin();
    const int a=source.indexOf(*it++),b=source.indexOf(*it);
    const int first=qMin(a,b),second=qMax(a,b);
    const auto times=m_timeline.cleavageTimes(stage,first,second);
    if(times.isEmpty()){m_playerHint->setText(QStringLiteral("선택한 출발 구간에서 이 결합이 끊어지지 않습니다."));return;}
    play(false);
    QStringList choices{QStringLiteral("hν 해제")};
    for(int i=0;i<times.size();++i)choices.push_back(QStringLiteral("%1번째 절단 · %2 s").arg(i+1).arg(times[i],0,'f',3));
    int initial=1;
    for(const auto &value:m_steps[stage].presentation["photoCleavages"].toArray()) {
        const auto mark=value.toArray();
        if(mark.size()>=2&&mark[0].toInt()==first&&mark[1].toInt()==second)
            initial=qBound(1,mark.size()==3?mark[2].toInt()+1:1,int(choices.size())-1);
    }
    bool ok=false;
    const auto choice=QInputDialog::getItem(m_player,QStringLiteral("3D 선택 결합의 광반응"),QStringLiteral("절단 시점"),choices,initial,false,&ok);
    if(!ok)return;
    const int occurrence=choices.indexOf(choice)-1;
    setPhotoMark(stage,first,second,occurrence);
    if(occurrence>=0)seek(m_coordinate->timeForMapped(times[occurrence]));
}

void MechanismAnimationDialog::editShot() {
    if(!m_valid||m_exporting)return;
    play(false);
    QDialog dialog(m_player);dialog.setWindowTitle(QStringLiteral("카메라·구도"));
    auto *form=new QFormLayout(&dialog);
    auto *layout=new QComboBox;layout->addItems({QStringLiteral("위아래"),QStringLiteral("나란히"),QStringLiteral("3D 구조만"),QStringLiteral("발표용 · 중앙 3D / 우상단 2D / 하단 그래프")});
    layout->setCurrentIndex(m_shotArrangement);form->addRow(QStringLiteral("화면 배치"),layout);
    auto *aspect=new QComboBox;aspect->addItems({"16:9","1:1","9:16"});aspect->setCurrentIndex(m_shotAspect);
    form->addRow(QStringLiteral("재생·저장 화면 비율"),aspect);
    auto *camera=new QComboBox;
    camera->addItems({QStringLiteral("편집 창 카메라"),QStringLiteral("저장한 카메라"),QStringLiteral("현재 재생 창 카메라 저장")});
    camera->setCurrentIndex(m_savedCameraEnabled?1:0);
    if(!m_savedCameraEnabled)camera->setItemData(1,0,Qt::UserRole-1);
    form->addRow(QStringLiteral("카메라 기준"),camera);
    auto *focus=new QComboBox;
    focus->addItems({QStringLiteral("전체 구도"),QStringLiteral("저장된 반응 중심 추적"),QStringLiteral("선택 원자를 반응 중심으로 추적")});
    focus->setCurrentIndex(m_shotAtoms.isEmpty()?0:1);
    if(m_shotAtoms.isEmpty())focus->setItemData(1,0,Qt::UserRole-1);
    if(m_playerView->selection().atoms.isEmpty())focus->setItemData(2,0,Qt::UserRole-1);
    form->addRow(QStringLiteral("클로즈업"),focus);
    auto *zoom=new QDoubleSpinBox;zoom->setRange(1,8);zoom->setValue(m_shotZoom);zoom->setSuffix(" ×");
    form->addRow(QStringLiteral("확대 배율"),zoom);
    auto *ad=new QPushButton(QStringLiteral("선택 원자로 세로 광고 구도"));form->addRow(ad);
    ad->setEnabled(!m_playerView->selection().atoms.isEmpty());
    connect(ad,&QPushButton::clicked,&dialog,[&]{layout->setCurrentIndex(2);aspect->setCurrentIndex(2);focus->setCurrentIndex(2);zoom->setValue(2.5);});
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    m_shotArrangement=layout->currentIndex();m_shotAspect=aspect->currentIndex();m_shotZoom=float(zoom->value());
    m_savedCameraEnabled=camera->currentIndex()!=0;
    if(camera->currentIndex()==2)m_savedCamera=m_shotAtoms.isEmpty()?m_playerView->cameraState():m_view->cameraState();
    if(focus->currentIndex()==0)m_shotAtoms.clear();
    if(focus->currentIndex()==2)m_shotAtoms=m_playerView->selection().atoms;
    applyShotLayout();
    m_playerView->setCameraState(shotCamera(m_playerView->molecule(),m_view->cameraState()));
    seek(m_time);
}

void MechanismAnimationDialog::applyShotLayout() {
    m_coordinate->setSeparateArtwork(m_shotArrangement==presentationArrangement);
    static_cast<ShotViewport*>(m_shotViewport)->setAspect(shotAspect(m_shotAspect));
    const auto ratio=m_shotAspect==2?QStringLiteral("9:16"):m_shotAspect==1?QStringLiteral("1:1"):QStringLiteral("16:9");
    for(int i=0;i<m_resolution->count();++i) {
        const int width=m_resolution->itemData(i).toInt();
        m_resolution->setItemText(i,QStringLiteral("%1 × %2 · %3").arg(width).arg(outputSize(width,m_shotAspect).height()).arg(ratio));
    }
}

CameraState MechanismAnimationDialog::shotCamera(const Molecule &molecule,CameraState base) const {
    if(m_savedCameraEnabled)base=m_savedCamera;
    QVector3D center;int count=0;
    for(int id:m_shotAtoms)if(id>=0&&id<molecule.atoms.size()&&molecule.atoms[id].visible) {
        center+=molecule.atoms[id].position;++count;
    }
    if(count>0) {
        base.perspective=false;
        base.centerOffset=center/float(count)-m_fittedCenter;
        base.distance=qMax(.1f,base.distance/m_shotZoom);
        base.orthoScale=qMax(.1f,base.orthoScale/m_shotZoom);
    }
    return base;
}

void MechanismAnimationDialog::keepSelectedHydrogens(MoleculeView *view) {
    if(!m_valid||m_loading||m_exporting)return;
    QSet<int> keep;
    const auto &atoms=view->molecule().atoms;
    for(int id:view->selection().atoms)
        if(id>=0&&id<atoms.size()&&atoms[id].atomicNumber==1)keep.insert(id);
    if(keep.isEmpty()) {
        const auto message=QStringLiteral("구조에서 남길 수소를 클릭하세요. Ctrl/Shift로 여러 개를 선택할 수 있습니다.");
        m_status->setText(message);m_playerHint->setText(message);
        return;
    }
    m_hideHydrogen->setChecked(false);
    m_keptHydrogens=keep;
    seek(m_time);
    m_playerHint->setText(QStringLiteral("선택한 수소 %1개를 표시합니다.").arg(keep.size()));
}

Molecule MechanismAnimationDialog::playbackFrame(double time) const {
    auto molecule=m_timeline.sample(m_coordinate->mappedTime(time));
    const float impact=m_coordinate->impactFlash(time);
    for(int i=0;i<molecule.atoms.size();++i) {
        auto &atom=molecule.atoms[i];
        if(!m_keptHydrogens.isEmpty()&&atom.atomicNumber==1&&!m_keptHydrogens.contains(i))atom.visible=false;
        atom.displayFlash=qMax(atom.displayFlash,impact);
    }
    if(impact>0)for(auto &bond:molecule.bonds)bond.displayFlash=qMax(bond.displayFlash,impact);
    return molecule;
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
    root["videoFontFamily"]=m_videoFont.family();
    root["videoTextColor"]=m_videoTextColor.name();
    root["transparentGif"]=m_transparentGif;
    QJsonArray stops;for(double time:m_presentationStops)stops.append(time);root["presentationStops"]=stops;
    const auto c=m_view->cameraState();QJsonArray keep;for(int id:m_focus)keep.append(id);
    QJsonArray parked;for(int id:m_parked)parked.append(id);root["parked"]=parked;root["parkStart"]=m_parkStart;
    root["cameraCenter"]=QJsonArray{m_fittedCenter.x(),m_fittedCenter.y(),m_fittedCenter.z()};
    root["focusMode"]=m_focusMode;root["focus"]=keep;root["labels"]=m_labels->isChecked();root["captions"]=m_captions->isChecked();root["hideHydrogen"]=m_hideHydrogen->isChecked();
    QJsonArray hydrogens;for(int id:m_keptHydrogens)hydrogens.append(id);root["keptHydrogens"]=hydrogens;
    QJsonArray shotAtoms;for(int id:m_shotAtoms)shotAtoms.append(id);
    root["shot"]=QJsonObject{{"arrangement",m_shotArrangement},{"aspect",m_shotAspect},{"zoom",m_shotZoom},{"atoms",shotAtoms}};
    if(m_savedCameraEnabled) {
        const auto &s=m_savedCamera;
        root["shotCamera"]=QJsonArray{s.centerOffset.x(),s.centerOffset.y(),s.centerOffset.z(),
            s.rotation.scalar(),s.rotation.x(),s.rotation.y(),s.rotation.z(),s.distance,s.orthoScale,s.perspective,s.perspectiveFov};
    }
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
    m_options=options;
    if(!setSteps(steps))return false;
    const auto root=QJsonDocument::fromJson(bytes).object();m_focusMode=root["focusMode"].toInt(-1);if(m_focusMode < -1||m_focusMode>2)m_focusMode=-1;
    m_videoFont.setFamily(root["videoFontFamily"].toString(QStringLiteral("Arial")));
    const QColor textColor(root["videoTextColor"].toString());
    m_videoTextColor=textColor.isValid()?textColor:QColor(35,45,55);
    m_transparentGif=root["transparentGif"].toBool(true);
    for(const auto &value:root["presentationStops"].toArray()) {
        const double time=value.toDouble(-1);
        if(time>0&&std::isfinite(time))m_presentationStops.push_back(time);
    }
    m_coordinate->restoreSettings(root["reactionCoordinate"].toObject());updateControls();
    m_focus.clear();for(const auto &v:root["focus"].toArray())if(v.isDouble()&&v.toInt()>=0&&v.toInt()<m_timeline.trackCount())m_focus.insert(v.toInt());
    if(m_focusMode>=0)m_timeline.emphasize(m_focus,m_focusMode);
    m_parked.clear();for(const auto &v:root["parked"].toArray())if(v.isDouble()&&v.toInt()>=0&&v.toInt()<m_timeline.trackCount())m_parked.insert(v.toInt());
    m_parkStart=root["parkStart"].toDouble();m_timeline.park(m_parked,m_parkStart);
    m_labels->setChecked(root["labels"].toBool());m_captions->setChecked(root["captions"].toBool(true));m_hideHydrogen->setChecked(root["hideHydrogen"].toBool());
    applyTextStyle();
    if(!m_hideHydrogen->isChecked()) {
        const auto atoms=m_timeline.sample(0).atoms;
        for(const auto &v:root["keptHydrogens"].toArray()) {
            const int id=v.toInt(-1);
            if(id>=0&&id<atoms.size()&&atoms[id].atomicNumber==1)m_keptHydrogens.insert(id);
        }
    }
    const auto camera=root["camera"].toArray();
    if(camera.size()==11){auto c=m_view->cameraState();bool valid=true;for(int i=0;i<11;++i)if(i!=9)valid=valid&&camera[i].isDouble()&&std::isfinite(camera[i].toDouble());if(valid&&camera[7].toDouble()>0&&camera[8].toDouble()>0){c.centerOffset={float(camera[0].toDouble()),float(camera[1].toDouble()),float(camera[2].toDouble())};c.rotation=QQuaternion(float(camera[3].toDouble()),float(camera[4].toDouble()),float(camera[5].toDouble()),float(camera[6].toDouble())).normalized();c.distance=float(camera[7].toDouble());c.orthoScale=float(camera[8].toDouble());c.perspective=camera[9].toBool();c.perspectiveFov=qBound(20.f,float(camera[10].toDouble()),70.f);m_view->setCameraState(c);}}
    const auto center=root["cameraCenter"].toArray();
    if(center.size()==3){const QVector3D oldCenter(float(center[0].toDouble()),float(center[1].toDouble()),float(center[2].toDouble()));if(std::isfinite(oldCenter.x())&&std::isfinite(oldCenter.y())&&std::isfinite(oldCenter.z())){auto c=m_view->cameraState();c.centerOffset+=oldCenter-m_fittedCenter;m_view->setCameraState(c);}}
    const auto shot=root["shot"].toObject();
    m_shotArrangement=qBound(0,shot["arrangement"].toInt(),presentationArrangement);
    m_shotAspect=qBound(0,shot["aspect"].toInt(),2);
    m_shotZoom=float(qBound(1.,shot["zoom"].toDouble(2),8.));
    for(const auto &value:shot["atoms"].toArray()) {
        const int id=value.toInt(-1);
        if(id>=0&&id<m_timeline.trackCount())m_shotAtoms.insert(id);
    }
    const auto saved=root["shotCamera"].toArray();
    m_savedCameraEnabled=saved.size()==11&&saved[7].toDouble()>0&&saved[8].toDouble()>0;
    if(m_savedCameraEnabled) {
        m_savedCamera={{float(saved[0].toDouble()),float(saved[1].toDouble()),float(saved[2].toDouble())},
            QQuaternion(float(saved[3].toDouble()),float(saved[4].toDouble()),float(saved[5].toDouble()),float(saved[6].toDouble())).normalized(),
            float(saved[7].toDouble()),float(saved[8].toDouble()),saved[9].toBool(),float(qBound(20.,saved[10].toDouble(45),70.))};
    }
    applyShotLayout();
    m_playerView->setCameraState(m_view->cameraState());
    seek(0);return true;
}

QVector<double> MechanismAnimationDialog::presentationBoundaries() const {
    QVector<double> times{0,m_timeline.duration()};
    for(int i=0;i<m_steps.size();++i)times.push_back(m_timeline.stageTime(i));
    constexpr double tolerance=.0005;
    for(double stop:m_presentationStops)
        if(stop>0&&stop<m_timeline.duration()&&std::none_of(times.begin(),times.end(),[&](double t){return std::abs(t-stop)<=tolerance;}))
            times.push_back(stop);
    std::sort(times.begin(),times.end());
    QVector<double> result{0};
    for(double time:times)
        if(time-result.last()>tolerance&&m_timeline.duration()-time>tolerance)result.push_back(time);
    result.push_back(m_timeline.duration());
    return result;
}

void MechanismAnimationDialog::editPresentationStops(QWidget *anchor) {
    if(!m_valid||m_loading||m_exporting)return;
    play(false);
    QMenu menu(anchor->window());
    menu.addSection(QStringLiteral("기존 단계 경계는 자동으로 사용합니다"));
    auto *add=menu.addAction(QStringLiteral("현재 %1 s에 정지점 추가").arg(m_time,0,'f',3));
    const auto times=presentationBoundaries();
    add->setEnabled(m_presentationStops.size()<200&&std::none_of(times.begin(),times.end(),[&](double t){return std::abs(t-m_time)<=.0005;}));
    auto *remove=menu.addMenu(QStringLiteral("추가 정지점 삭제"));
    remove->setEnabled(!m_presentationStops.isEmpty());
    for(int i=0;i<m_presentationStops.size();++i) {
        auto *action=remove->addAction(QStringLiteral("%1 s").arg(m_presentationStops[i],0,'f',3));
        action->setData(i);
    }
    auto *choice=menu.exec(anchor->mapToGlobal(anchor->rect().bottomLeft()));
    if(choice==add) {
        m_presentationStops.push_back(m_time);
        std::sort(m_presentationStops.begin(),m_presentationStops.end());
    } else if(choice&&choice->data().isValid()) {
        m_presentationStops.removeAt(choice->data().toInt());
    } else return;
    refreshTimelineEvents();
    m_playerHint->setText(QStringLiteral("PowerPoint 재생 구간 %1개").arg(presentationBoundaries().size()-1));
}

QImage MechanismAnimationDialog::renderOutputFrame(double time,QSize size,bool transparent,const CameraState &baseCamera,bool forceGraph) {
    const bool combined=forceGraph||(m_coordinate->active()&&m_shotArrangement!=2);
    const auto regions=shotRegions(size,m_shotArrangement,combined,m_coordinate->hasImages(),forceGraph);
    const int width=size.width(),height=size.height();
    const auto molecule=playbackFrame(time);
    m_view->setAnimationMolecule(molecule);
    auto camera=shotCamera(molecule,baseCamera);
    if(!m_shotAtoms.isEmpty()&&!camera.perspective&&m_view->height()>0) {
        const float screenAspect=float(m_view->width())/m_view->height();
        const float outputAspect=float(regions.structure.width())/regions.structure.height();
        camera.orthoScale/=qMax(1.f,screenAspect/outputAspect);
    }
    m_view->setCameraState(camera);
    auto image=m_view->imageAtSize(regions.structure.size(),transparent,true);
    if(image.isNull())return {};
    if(combined) {
        QImage full(size,QImage::Format_ARGB32_Premultiplied);
        full.fill(transparent?QColor(Qt::transparent):image.pixelColor(0,0));
        QPainter painter(&full);
        painter.drawImage(regions.structure.topLeft(),image);
        painter.drawImage(regions.graph.topLeft(),m_coordinate->imageAtTime(regions.graph.size(),time,transparent,forceGraph));
        if(!regions.artwork.isEmpty())m_coordinate->drawArtwork(painter,regions.artwork,time,transparent);
        return full;
    }
    if(m_captions->isChecked()) {
        QPainter painter(&image);QFont font=m_videoFont;font.setPixelSize(qMax(14,height/30));painter.setFont(font);
        const QString text=m_timeline.caption(m_coordinate->mappedTime(time));const QRect box(12,height-qMax(60,height/9),width-24,qMax(48,height/10));
        if(!transparent)painter.fillRect(box,QColor(255,255,255,210));
        painter.setPen(m_videoTextColor);painter.drawText(box.adjusted(10,2,-10,-2),Qt::AlignCenter|Qt::TextWordWrap,text);
    }
    return image;
}

bool MechanismAnimationDialog::exportPowerPoint(const QString &path,int width,int fps,bool bundle) {
    auto fail=[&](const QString &message){m_status->setText(message);m_playerHint->setText(message);return false;};
    if(!m_valid||m_loading||m_exporting||!m_view->isValid()||width<160||width>3840||fps<10||fps>60)return false;
    if(!m_coordinate->ready()||!m_coordinate->hasEnergies())return fail(QStringLiteral("PowerPoint 저장에는 단계별 에너지와 유효한 반응 좌표가 필요합니다."));
    const auto bounds=presentationBoundaries();
    QVector<int> intervals;
    int total=1;
    for(int i=1;i<bounds.size();++i) {
        const int count=qMax(1,qCeil((bounds[i]-bounds[i-1])*fps));
        intervals.push_back(count);total+=count+1;
    }
    if(total>18000)return fail(QStringLiteral("PowerPoint 저장은 한 번에 18,000프레임까지 지원합니다."));
    QTemporaryDir temp;
    if(!temp.isValid())return fail(QStringLiteral("PowerPoint 임시 폴더를 만들 수 없습니다."));
    const QString script=temp.filePath("export.ps1");
    if(!QFile::copy(":/youngcraft/export-mechanism-pptx.ps1",script))return fail(QStringLiteral("PowerPoint 저장 도구를 열 수 없습니다."));

    play(false);
    const double originalTime=m_time;
    const auto baseCamera=m_view->cameraState();
    m_exporting=true;updateControls();m_view->setEnabled(false);
    const auto restore=qScopeGuard([&]{
        m_view->setCameraState(baseCamera);
        m_exporting=false;m_view->setEnabled(true);seek(originalTime);updateControls();
    });
    const QSize size=outputSize(width,m_shotAspect);
    QProgressDialog progress(QStringLiteral("PowerPoint 장면 렌더링…"),QStringLiteral("취소"),0,total,this);
    progress.setWindowModality(Qt::WindowModal);progress.setMinimumDuration(0);progress.setAutoClose(false);progress.setAutoReset(false);
    if(!renderOutputFrame(0,size,false,baseCamera,true).save(temp.filePath("start.png")))
        return fail(QStringLiteral("PowerPoint 시작 프레임을 저장할 수 없습니다."));
    int completed=1;
    for(int scene=0;scene<intervals.size();++scene) {
        const int steps=intervals[scene];
        const double start=bounds[scene],end=bounds[scene+1];
        QString error;
        QImage lastFrame;
        if(!writePowerPointVideo(temp.filePath(QStringLiteral("scene-%1.mp4").arg(scene+1)),size,fps,steps+1,[&](int frame){
            progress.setValue(completed++);QApplication::processEvents();
            if(progress.wasCanceled())return QImage{};
            auto image=renderOutputFrame(start+(end-start)*frame/steps,size,false,baseCamera,true);
            if(frame==steps)lastFrame=image;
            return image;
        },error))return fail(progress.wasCanceled()?QStringLiteral("PowerPoint 저장을 취소했습니다."):error);
        if(!lastFrame.save(temp.filePath(QStringLiteral("end-%1.png").arg(scene+1))))
            return fail(QStringLiteral("구간의 마지막 프레임을 저장할 수 없습니다."));
    }
    progress.setLabelText(QStringLiteral("영상과 정지 이미지를 묶는 중…"));progress.setValue(total);
    QProcess office;
    QEventLoop loop;
    QTimer poll;poll.setInterval(50);
    bool cancellationSent=false;
    connect(&poll,&QTimer::timeout,&loop,[&]{
        if(progress.wasCanceled()&&!cancellationSent) {
            QFile marker(temp.filePath("cancel"));
            cancellationSent=marker.open(QIODevice::WriteOnly);
        }
        if(office.state()==QProcess::NotRunning)loop.quit();
    });
    connect(&office,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),&loop,&QEventLoop::quit);
    const auto powerShell=QDir(qEnvironmentVariable("SystemRoot")).filePath("System32/WindowsPowerShell/v1.0/powershell.exe");
    office.start(powerShell,{"-NoProfile","-NonInteractive","-STA","-ExecutionPolicy","Bypass","-File",script,
        temp.path(),QString::number(intervals.size()),QString::number(size.width()),QString::number(size.height()),bundle?"zip":"pptx"});
    poll.start();loop.exec();
    if(progress.wasCanceled())return fail(QStringLiteral("PowerPoint 저장을 취소했습니다."));
    if(office.error()==QProcess::FailedToStart||office.exitStatus()!=QProcess::NormalExit||office.exitCode()!=0)
        return fail(QStringLiteral("발표 파일을 묶는 중 오류가 발생했습니다.\n")+
            QString::fromLocal8Bit(office.readAllStandardError()).right(750));
    QFile input(temp.filePath(bundle?"presentation.zip":"presentation.pptx"));
    QSaveFile output(path);
    if(!input.open(QIODevice::ReadOnly)||input.size()==0||!output.open(QIODevice::WriteOnly))
        return fail(QStringLiteral("PPTX 파일을 저장할 수 없습니다."));
    while(!input.atEnd()) {
        const auto bytes=input.read(1024*1024);
        if(bytes.isEmpty()||output.write(bytes)!=bytes.size())return fail(QStringLiteral("PPTX 파일 쓰기 실패"));
    }
    if(!output.commit())return fail(output.errorString());
    const auto message=QStringLiteral("%1 저장 완료 · 재생 구간 %2개").arg(bundle?"ZIP":"PPTX").arg(intervals.size());
    m_status->setText(message+QStringLiteral(" · ")+path);m_playerHint->setText(message);
    return true;
}

bool MechanismAnimationDialog::exportAnimation(const QString &path,const QString &format,int width,int fps) {
    if(!m_valid||m_exporting||m_loading||!m_view->isValid()||width<160||width>3840||fps<1||fps>120)return false;
    if(format!="gif"&&format!="png"&&format!="mp4")return false;
    play(false);const double originalTime=m_time;m_exporting=true;updateControls();m_view->setEnabled(false);
    auto restore=qScopeGuard([&]{m_exporting=false;m_view->setEnabled(true);seek(originalTime);updateControls();});
    const QSize size=outputSize(width,m_shotAspect);
    const auto baseCamera=m_view->cameraState();
    auto restoreCamera=qScopeGuard([&]{m_view->setCameraState(baseCamera);});
    const int intervals=qMax(1,qCeil(m_timeline.duration()*fps)),count=intervals+1;
    if(count>18000){m_status->setText(QStringLiteral("한 번에 18,000프레임까지 저장합니다. fps나 구간 시간을 줄이거나 영상을 나누어 저장하세요."));return false;}
    const bool hasFfmpeg=format=="mp4"&&ImageExporter::ffmpegAvailable();
#ifndef Q_OS_WIN
    if(format=="mp4"&&!hasFfmpeg){m_status->setText(QStringLiteral("MP4 저장에는 ffmpeg가 필요합니다."));return false;}
#endif
    QTemporaryDir temporary;if(!temporary.isValid()){m_status->setText(QStringLiteral("임시 폴더 생성 실패"));return false;}
    QString directory=temporary.path();
    if(format=="png"){directory=path+"/Youngcraft-Mechanism-"+QUuid::createUuid().toString(QUuid::WithoutBraces);if(!QDir().mkpath(directory)){m_status->setText(QStringLiteral("PNG 폴더 생성 실패"));return false;}}
    QProgressDialog progress(QStringLiteral("전체 메커니즘 렌더링…"),QStringLiteral("취소"),0,count,this);progress.setWindowModality(Qt::WindowModal);progress.setMinimumDuration(0);progress.setAutoClose(false);progress.setAutoReset(false);
    const bool transparent=format=="png" || (format=="gif" && m_transparentGif);
    auto renderFrame=[&](int frame)->QImage {
        progress.setValue(frame);QApplication::processEvents();
        if(progress.wasCanceled())return {};
        return renderOutputFrame(m_timeline.duration()*frame/intervals,size,transparent,baseCamera);
    };
    QString error;
    const auto movie=temporary.filePath("mechanism.mp4");
    if(format=="gif") {
        if(!ImageExporter::saveGifFrames(count,size,path,renderFrame,qRound(1000./fps),transparent,m_loop->isChecked(),&error,true)){m_status->setText(error);return false;}
        QSettings().setValue(gifDirectoryKey,QFileInfo(path).absolutePath());
    } else if(format=="mp4"&&!hasFfmpeg) {
        if(!writePowerPointVideo(movie,size,fps,count,renderFrame,error)) {
            m_status->setText(progress.wasCanceled()?QStringLiteral("MP4 저장을 취소했습니다."):error);
            return false;
        }
    } else {
        for(int frame=0;frame<count;++frame){const auto image=renderFrame(frame);if(image.isNull()){m_status->setText(progress.wasCanceled()?QStringLiteral("저장 취소 · PNG 시퀀스의 완료된 프레임은 보존됩니다."):QStringLiteral("프레임 렌더링 실패"));return false;}if(!image.save(directory+QStringLiteral("/frame_%1.png").arg(frame,6,10,QChar('0')))){m_status->setText(QStringLiteral("프레임 파일 저장 실패"));return false;}}
    }
    progress.setLabelText(QStringLiteral("영상 인코딩 중…"));progress.setValue(count);
    if(format=="mp4"&&hasFfmpeg) {
        QProcess encoder;
        encoder.start("ffmpeg",{"-y","-framerate",QString::number(fps),"-i",directory+"/frame_%06d.png","-c:v","libx264","-pix_fmt","yuv420p","-crf","20","-movflags","+faststart",movie});
        QEventLoop loop;QTimer poll;poll.setInterval(50);connect(&poll,&QTimer::timeout,&loop,[&]{if(progress.wasCanceled())encoder.kill();if(encoder.state()==QProcess::NotRunning)loop.quit();});
        connect(&encoder,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),&loop,&QEventLoop::quit);poll.start();loop.exec();
        if(progress.wasCanceled()||encoder.error()==QProcess::FailedToStart||encoder.exitStatus()!=QProcess::NormalExit||encoder.exitCode()!=0){m_status->setText(QStringLiteral("MP4 저장 실패/취소: ")+QString::fromLocal8Bit(encoder.readAllStandardError()).right(1200));return false;}
    }
    if(format=="mp4") {
        QFile input(movie);QSaveFile output(path);if(!input.open(QIODevice::ReadOnly)||!output.open(QIODevice::WriteOnly)){m_status->setText(QStringLiteral("MP4 파일 저장 실패"));return false;}
        while(!input.atEnd()){const auto data=input.read(1024*1024);if(data.isEmpty()||output.write(data)!=data.size()){m_status->setText(QStringLiteral("MP4 파일 쓰기 실패"));return false;}}
        if(!output.commit()){m_status->setText(output.errorString());return false;}
    } else if(format=="png") {
        QSaveFile manifest(directory+"/timeline.json");const auto json=QJsonDocument(QJsonObject{{"fps",fps},{"frames",count},{"duration",m_timeline.duration()},{"project",QJsonDocument::fromJson(MechanismTimeline::saveProject(m_steps,m_options)).object()}}).toJson();
        if(!manifest.open(QIODevice::WriteOnly)||manifest.write(json)!=json.size()||!manifest.commit()){m_status->setText(QStringLiteral("타임라인 기록 저장 실패"));return false;}
    }
    m_status->setText(QStringLiteral("저장 완료 · %1 프레임: %2").arg(count).arg(format=="png"?directory:path));return true;
}
void MechanismAnimationDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    fitWindowToScreen(this,parentWidget()?parentWidget()->screen():screen());
    if(windowHandle()&&!property("screenFitConnected").toBool()) {
        connect(windowHandle(),&QWindow::screenChanged,this,[this](QScreen *current){fitWindowToScreen(this,current);});
        setProperty("screenFitConnected",true);
    }
}
void MechanismAnimationDialog::closeEvent(QCloseEvent *event) {if(m_exporting){event->ignore();return;}play(false);m_player->hide();QDialog::closeEvent(event);}
void MechanismAnimationDialog::reject(){if(!m_exporting){play(false);m_player->hide();QDialog::reject();}}
}
