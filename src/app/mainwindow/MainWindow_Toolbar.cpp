#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/StyleEditorDialog.h"
#include "ui/AppPattern.h"
#include "ui/DialogThemeSync.h"

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
#include <QStandardPaths>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>
#include <cmath>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QIcon>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QToolButton>

namespace youngcraft {
namespace {
using uiPattern::drawMotif;
QString sidebarPatternStyle(int motif,const QColor &,const QColor &) {
    if(motif<0 || motif>5)return {};
    return QStringLiteral(
        "QDockWidget#ResultExplorerDock QAbstractItemView, QDockWidget#ResultExplorerDock QScrollArea, "
        "QDockWidget#ResultExplorerDock QStackedWidget, QDockWidget#ResultExplorerDock QSplitter, "
        "QTabWidget#SidebarPatternTabs::pane, QWidget[sidebarPatternPaper=\"true\"] "
        "{background:transparent; background-image:none;}"
        "QDockWidget#ResultExplorerDock QHeaderView::section, QDockWidget#ResultExplorerDock::title {background:transparent;}");
}

class PatternToolBar final : public QToolBar {
public:
    explicit PatternToolBar(const QString &title,QWidget *parent):QToolBar(title,parent){}
    void setMotif(int motif) { m_motif=motif;setProperty("patternTemplate",motif);update(); }
    void setMotifInk(const QColor &ink) { m_ink=ink;update(); }
protected:
    void paintEvent(QPaintEvent *event) override {
        QToolBar::paintEvent(event);
        if(m_motif<0)return;
        QPainter painter(this);uiPattern::paintBackdrop(painter,this,false);
    }
private:
    int m_motif=-1;
    QColor m_ink=QColor("#446781");
};

QIcon motifIcon(int motif) {
    QPixmap pix(56,56);pix.fill(Qt::transparent);
    QPainter p(&pix);p.scale(2,2);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcc9d0"),1));p.setBrush(QColor("#fbfcfd"));
    p.drawRoundedRect(QRectF(1,1,26,26),5,5);
    QPainterPath clip;clip.addRoundedRect(QRectF(2,2,24,24),4,4);p.setClipPath(clip);
    if(motif==0){p.translate(2,2);p.scale(.25,.25);drawMotif(p,QRect(0,0,96,96),motif,QColor("#617e8d"));}
    else drawMotif(p,QRect(2,2,24,24),motif,QColor("#617e8d"));p.end();
    pix.setDevicePixelRatio(2);return QIcon(pix);
}

QIcon captionedIcon(const QIcon &base,const QString &caption,const QString &shortcut={}) {
    QIcon result;
    for(auto state:{QIcon::Off,QIcon::On}){
        QPixmap pix(92,84);pix.fill(Qt::transparent);pix.setDevicePixelRatio(2);
        QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.setPen(state==QIcon::On?Qt::white:QColor("#344c60"));
        QFont font(QStringLiteral("Segoe UI"));font.setPixelSize(8);font.setBold(true);p.setFont(font);
        p.drawText(QRectF(1,0,45,10),Qt::AlignLeft|Qt::AlignVCenter,caption);
        base.paint(&p,QRect(10,10,26,26),Qt::AlignCenter,QIcon::Normal,state);
        font.setPixelSize(7);font.setBold(false);p.setFont(font);
        p.drawText(QRectF(0,33,45,9),Qt::AlignRight|Qt::AlignBottom,shortcut);
        p.end();result.addPixmap(pix,QIcon::Normal,state);result.addPixmap(pix,QIcon::Active,state);
    }
    return result;
}

QIcon actionIcon(int id, const QColor &ink=QColor("#2874b8")) {
    QPixmap pix(64,64); pix.fill(Qt::transparent);
    QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing); p.scale(2,2);
    p.setPen(QPen(ink,2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    switch(id) {
    case 0: p.drawRoundedRect(QRectF(6,3,20,26),2,2);p.drawLine(11,16,21,16);p.drawLine(16,11,16,21);break;
    case 1: {QPainterPath path;path.moveTo(3,26);path.lineTo(3,8);path.lineTo(12,8);path.lineTo(15,11);path.lineTo(29,11);path.lineTo(26,26);path.closeSubpath();p.drawPath(path);p.drawLine(4,16,27,16);break;}
    case 2: p.drawLine(7,24,23,8);p.drawLine(10,27,26,11);p.drawLine(23,8,26,11);p.drawLine(7,24,6,28);p.drawLine(6,28,10,27);p.drawLine(4,29,28,29);break;
    case 3: p.drawLine(7,9,16,21);p.drawLine(16,21,26,7);p.drawEllipse(QPointF(7,9),4,4);p.drawEllipse(QPointF(16,21),4,4);p.drawEllipse(QPointF(26,7),3,3);p.drawLine(21,28,29,28);p.drawLine(29,28,26,25);break;
    case 4: p.drawRect(3,3,20,20);p.drawRect(9,9,20,20);p.drawEllipse(QPointF(23,15),2,2);p.drawPolyline(QPolygonF{QPointF(10,26),QPointF(16,19),QPointF(21,24),QPointF(25,20),QPointF(28,24)});break;
    case 5: // Three aligned panels and adjustable guides: a tailored capture.
        p.drawRoundedRect(QRectF(3,4,26,18),2,2);
        p.drawLine(11,4,11,22);p.drawLine(21,4,21,22);
        p.drawPolyline(QPolygonF{{13,14},{15,10},{17,16},{19,12}});
        p.drawLine(3,28,29,28);p.setBrush(ink);
        p.drawEllipse(QPointF(10,28),2,2);p.drawEllipse(QPointF(23,28),2,2);break;
    case 6: case 7: p.drawLine(5,6,5,25);p.drawLine(15,6,15,25);p.drawLine(5,15,15,15);p.drawLine(21,16,29,16);if(id==7)p.drawLine(25,12,25,20);break;
    case 8: p.drawEllipse(QPointF(16,16),8,8);p.drawLine(16,2,16,11);p.drawLine(16,21,16,30);p.drawLine(2,16,11,16);p.drawLine(21,16,30,16);break;
    case 9: {QPainterPath path;path.moveTo(2,16);path.quadTo(16,-2,30,16);path.quadTo(16,34,2,16);p.drawPath(path);p.drawEllipse(QPointF(16,16),5,5);break;}
    case 11: case 23: p.drawRoundedRect(QRectF(3,4,26,24),2,2);p.drawLine(12,4,12,28);p.drawLine(6,10,9,10);p.drawLine(6,15,9,15);p.drawLine(6,20,9,20);break;
    case 12: p.drawLine(4,4,4,28);p.drawLine(4,28,29,28);p.drawPolyline(QPolygonF{{7,7},{12,16},{17,22},{23,24},{28,24}});break;
    case 13: {QPainterPath path;path.moveTo(2,16);for(int x=2;x<=30;++x)path.lineTo(x,16+8*std::sin((x-2)*3.141592653589793/7));p.drawPath(path);break;}
    case 14: p.drawLine(4,4,4,28);p.drawLine(4,28,29,28);p.drawPolyline(QPolygonF{{7,9},{12,21},{17,24},{22,21},{28,9}});for(const auto &point:QPolygonF{{7,9},{17,24},{28,9}})p.drawEllipse(point,2,2);break;
    case 15: {QPainterPath path;path.moveTo(3,25);path.cubicTo(10,25,10,7,16,7);path.cubicTo(22,7,22,25,29,25);p.drawPath(path);p.drawEllipse(QPointF(16,7),2,2);break;}
    case 16: p.drawLine(3,26,29,26);p.drawLine(5,7,27,7);p.drawLine(16,22,16,11);p.drawLine(12,15,16,11);p.drawLine(20,15,16,11);p.drawLine(7,26,7,20);p.drawLine(25,26,25,16);break;
    case 17: p.drawEllipse(QPointF(12,13),7,7);p.drawLine(10,13,14,13);p.drawLine(12,11,12,15);p.drawLine(23,10,29,10);p.drawLine(23,16,29,16);p.drawLine(5,26,29,26);p.drawLine(5,22,5,29);p.drawLine(17,22,17,29);break;
    case 18: p.save();p.translate(16,16);p.rotate(-35);p.drawEllipse(QRectF(-12,-6,12,12));p.drawEllipse(QRectF(0,-6,12,12));p.restore();p.drawEllipse(QPointF(16,16),2,2);break;
    case 19: {p.drawRoundedRect(QRectF(1.5,4,29,24),3,3);QFont font=p.font();font.setFamily(QStringLiteral("Segoe UI"));font.setBold(true);font.setPixelSize(12);p.setFont(font);p.drawText(QRectF(1,4,30,24),Qt::AlignCenter,QStringLiteral("XYZ"));break;}
    case 22: p.save();p.translate(16,16);for(int a:{0,60,120}){p.save();p.rotate(a);p.drawEllipse(QRectF(-13,-5,26,10));p.restore();}p.setBrush(ink);p.drawEllipse(QPointF(0,0),2,2);p.restore();break;
    case 21: p.drawRoundedRect(QRectF(5,2,22,28),2,2);p.drawLine(10,9,22,9);p.drawLine(10,15,22,15);p.drawLine(10,21,18,21);break;
    case 20: {p.drawEllipse(QPointF(11,16),8,8);QFont font=p.font();font.setBold(true);font.setPixelSize(15);p.setFont(font);p.drawText(QRectF(16,1,15,18),Qt::AlignCenter,QStringLiteral("#"));break;}
    default: p.drawRoundedRect(QRectF(3,8,17,10),5,5);p.drawRoundedRect(QRectF(12,15,17,10),5,5);p.drawLine(12,16,20,16);break;
    }
    p.end();
    pix.setDevicePixelRatio(2);
    QIcon icon(pix);
    QPixmap checked=pix;
    QPainter tint(&checked);
    tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tint.fillRect(checked.rect(),Qt::white);
    tint.end();
    icon.addPixmap(checked,QIcon::Normal,QIcon::On);
    icon.addPixmap(checked,QIcon::Active,QIcon::On);
    const QStringList captions={QStringLiteral("새 구조"),QStringLiteral("열기"),QStringLiteral("그리기"),"MM opt",QStringLiteral("복사"),QStringLiteral("캡처"),QStringLiteral("H 숨김"),QStringLiteral("H 표시"),QStringLiteral("중앙"),QStringLiteral("뷰"),QStringLiteral("동기화"),QStringLiteral("패널"),"OPT","FREQ","SCAN","IRC","TD-DFT","Atoms","Orbitals","XYZ",QStringLiteral("원자 번호"),"Output","QM opt",QStringLiteral("탐색")};
    const QMap<int,QString> shortcuts={{0,"Ctrl+N"},{1,"Ctrl+O"},{3,"Ctrl+Alt+O"},{4,"Ctrl+I"},{5,"Ctrl+Alt+I"}};
    return captionedIcon(icon,captions.value(id),shortcuts.value(id));
}
}


void MainWindow::createToolBar() {
    auto *toolbar = new PatternToolBar(QStringLiteral("주 도구 모음"),this);
    addToolBar(toolbar);
    toolbar->setObjectName(QStringLiteral("MainToolBar"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(46,42));
    int iconIndex=0;
    auto add = [this, toolbar, &iconIndex](const QString &text, const std::function<void()> &function) {
        auto *action = toolbar->addAction(actionIcon(iconIndex), text);
        action->setProperty("toolbarIconId",iconIndex);if(iconIndex==0)action->setShortcut(QKeySequence("Ctrl+N"));++iconIndex;
        action->setToolTip(text); action->setStatusTip(text);
        connect(action, &QAction::triggered, function);
    };
    add(QStringLiteral("새 구조"), [this] { newDrawingDocument(); });
    add(QStringLiteral("열기"), [this] { openFile(); });
    add(QStringLiteral("구조 그리기"), [this] {
        if (!m_currentFile.isEmpty()) newDrawingDocument(m_view->molecule());
        else setDrawingMode(!m_drawingMode);
    });
    add(QStringLiteral("MM opt"), [this] { startMMOptimization(); });
    auto *qm=toolbar->addAction(actionIcon(22),QStringLiteral("QM opt · xTB"));qm->setObjectName("QMOptimizationAction");qm->setProperty("toolbarIconId",22);
    qm->setToolTip(QStringLiteral("QM opt · GFN2 / GFN1 xTB 구조 최적화"));connect(qm,&QAction::triggered,this,[this]{startXtbCalculation();});
    add(QStringLiteral("이미지 복사"), [this] { copyImage(); });
    add(QStringLiteral("맞춤 캡처"), [this] { openReactionCapture(); });
    add(QStringLiteral("수소 숨기기"), [this] { setCarbonHydrogensVisible(false); });
    add(QStringLiteral("수소 숨기기 초기화"), [this] { setCarbonHydrogensVisible(true); });
    add(QStringLiteral("가운데 배치"), [this] { m_view->centerOnMolecule(); });
    add(QStringLiteral("베스트 뷰"), [this] { m_view->bestView(); });
    m_syncViewsAction->setIcon(actionIcon(10));
    m_syncViewsAction->setToolTip(QStringLiteral("뷰 동기화"));
    m_syncViewsAction->setProperty("toolbarIconId",10);
    toolbar->addAction(m_syncViewsAction);

    auto *spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    struct Theme { QString name, description, paper, surface, tint, ink, border; };
    const QVector<Theme> themes = {
        {QStringLiteral("구름 소다"), QStringLiteral("맑은 소다빛 종이 · 차분한 블루 잉크"), "#edf4fa", "#fbfdff", "#c5dfee", "#446781", "#c2d3df"},
        {QStringLiteral("세이지 온실"), QStringLiteral("세이지 · 아이보리 · 식물 표본실"), "#eef4ee", "#fcfdf8", "#cbdcc6", "#4d6c5b", "#c6d4c6"},
        {QStringLiteral("라일락 노트"), QStringLiteral("보랏빛 안개 · 따뜻한 노트 종이"), "#f2eef8", "#fefcff", "#dbceec", "#6d5b85", "#d4c9e1"},
        {QStringLiteral("오트밀 아틀리에"), QStringLiteral("오트밀 종이 · 부드러운 모카 잉크"), "#f6f1e7", "#fffdf7", "#e6d8bb", "#79634c", "#ded1bc"},
        {QStringLiteral("로즈 밀크"), QStringLiteral("빛바랜 장미 · 우윳빛 도자기"), "#f8eef0", "#fffcfc", "#edced7", "#865e6d", "#e4cbd3"},
        {QStringLiteral("살구 다실"), QStringLiteral("살구빛 햇살 · 크림색 찻잔"), "#faf0e8", "#fffdf9", "#f0d2b8", "#865f49", "#e5cfbd"}
    };
    auto *swatches=new QWidget(toolbar);
    swatches->setObjectName(QStringLiteral("AppThemeSelector"));
    auto *swatchLayout=new QHBoxLayout(swatches);
    swatchLayout->setContentsMargins(6,0,2,0);swatchLayout->setSpacing(4);
    auto *paletteLabel=new QLabel(QStringLiteral("palette"),swatches);
    paletteLabel->setFont(QFont(QStringLiteral("Segoe Print"),10,QFont::DemiBold));
    paletteLabel->setToolTip(QStringLiteral("오늘의 파스텔 팔레트를 골라보세요"));
    swatchLayout->addWidget(paletteLabel);
    auto *themeGroup=new QButtonGroup(swatches);
    themeGroup->setExclusive(true);
    for(int i=0;i<themes.size();++i) {
        const auto &t=themes[i];
        auto *button=new QToolButton(swatches);
        button->setObjectName(QStringLiteral("PastelTheme%1").arg(i));
        button->setAccessibleName(t.name);
        button->setToolTip(t.name+QStringLiteral(" · ")+t.description);
        button->setStatusTip(button->toolTip());
        button->setCheckable(true);button->setFixedSize(32,32);button->setIconSize(QSize(24,24));
        QPixmap chip(48,48);chip.fill(Qt::transparent);
        QPainter painter(&chip);painter.setRenderHint(QPainter::Antialiasing);painter.scale(2,2);
        painter.setPen(QPen(QColor(t.border),1));painter.setBrush(QColor(t.tint));
        painter.drawEllipse(QRectF(2,2,20,20));
        painter.setPen(Qt::NoPen);painter.setBrush(QColor(t.surface));painter.drawEllipse(QRectF(6,6,8,8));
        painter.setBrush(QColor(t.ink));painter.drawEllipse(QRectF(15,14,5,5));painter.end();
        chip.setDevicePixelRatio(2);button->setIcon(QIcon(chip));
        button->setStyleSheet(QStringLiteral(
            "QToolButton { padding:2px; border:2px solid transparent; border-radius:10px; background:transparent; }"
            "QToolButton:hover { border-color:%1; background:%2; }"
            "QToolButton:checked { border-color:%3; background:%2; }"
            "QToolButton:focus { border-style:dashed; border-color:%3; }").arg(t.border,t.surface,t.ink));
        themeGroup->addButton(button,i);swatchLayout->addWidget(button);
    }
    toolbar->addWidget(swatches);
    auto applyTheme = [this,toolbar,themes,themeGroup](int index) {
        index = qBound(0, index, int(themes.size())-1);
        const auto &t=themes[index];
        const QString text="#34404b",field=t.surface;
        setProperty("appPattern",QSettings().value(QStringLiteral("ui/patternTemplate"),-1).toInt());
        setProperty("appPatternPaper",QColor(t.paper));
        setProperty("appPatternInk",QColor(t.ink));

        QPalette colors=palette();
        colors.setColor(QPalette::Window,QColor(t.paper));
        colors.setColor(QPalette::WindowText,QColor(text));
        colors.setColor(QPalette::Text,QColor(text));
        colors.setColor(QPalette::ButtonText,QColor(text));
        colors.setColor(QPalette::Base,QColor(field));
        colors.setColor(QPalette::Button,QColor(field));
        colors.setColor(QPalette::AlternateBase,QColor(t.paper));
        colors.setColor(QPalette::Highlight,QColor(t.ink));
        colors.setColor(QPalette::HighlightedText,Qt::white);
        colors.setColor(QPalette::ToolTipBase,QColor(field));
        colors.setColor(QPalette::ToolTipText,QColor(text));
        setPalette(colors);
        for(auto *bar:findChildren<QToolBar *>())
            if(auto *patternBar=dynamic_cast<PatternToolBar *>(bar))patternBar->setMotifInk(QColor(t.ink));
        setStyleSheet(QStringLiteral(
            "QMainWindow,QDockWidget,QToolBar,QStatusBar { background:%1; color:%2; }"
            "QLabel,QCheckBox,QRadioButton,QGroupBox { color:%2; }"
            "QToolBar { border:0; spacing:4px; padding:4px; }"
            "QToolButton,QPushButton,QComboBox { padding:5px; border:1px solid %6; border-radius:6px; background:%4; color:%2; }"
            "QToolButton:checked,QPushButton:checked,QTabBar::tab:selected { background:%3; color:%5; }"
            "QToolButton:hover,QPushButton:hover { border:1px solid %3; background:%7; }"
            "QToolButton:checked:hover,QPushButton:checked:hover { background:%3; }"
            "QTabBar::tab { padding:6px 12px; color:%2; background:%1; }"
            "QAbstractItemView,QPlainTextEdit,QTextEdit,QLineEdit,QSpinBox,QDoubleSpinBox { background:%4; color:%2; selection-background-color:%3; selection-color:white; }"
            "QMenuBar,QMenu { background:%1; color:%2; } QMenu::item:selected { background:%3; color:%5; }"
            "QWidget#MoleculeViewport { background:%1; }"
            "QToolTip { background:%4; color:%2; border:1px solid %6; padding:5px; }"
        ).arg(t.paper,text,t.ink,field,QStringLiteral("#ffffff"),t.border,t.tint)
            +sidebarPatternStyle(QSettings().value(QStringLiteral("ui/patternTemplate"),-1).toInt(),QColor(t.ink),QColor(t.paper)));
        for(auto *a:toolbar->actions())if(a->property("toolbarIconId").isValid())a->setIcon(actionIcon(a->property("toolbarIconId").toInt(),QColor(t.ink)));
        m_explorerDock->toggleViewAction()->setIcon(actionIcon(11,QColor(t.ink)));
        for(auto *action:findChildren<QAction *>())
            if(action->property("analysisTab").isValid())action->setIcon(actionIcon(12+action->property("analysisTab").toInt(),QColor(t.ink)));
        m_atomLabelsAction->setIcon(actionIcon(20,QColor(t.ink)));
        themeGroup->button(index)->setChecked(true);
        for(auto *view:findChildren<MoleculeView *>())view->update();
        for(auto *surface:findChildren<QWidget *>(QStringLiteral("MoleculeViewport")))surface->update();
        QSettings().setValue(QStringLiteral("ui/theme"), index);
        uiPattern::synchronizeDialogs(this);
    };
    connect(themeGroup,&QButtonGroup::idClicked,this,applyTheme);

    addToolBarBreak(Qt::TopToolBarArea);
    auto *panels = new PatternToolBar(QStringLiteral("분석 패널 도구 모음"),this);
    addToolBar(panels);
    panels->setObjectName(QStringLiteral("AnalysisToolBar"));
    panels->setMovable(false);
    panels->setToolButtonStyle(Qt::ToolButtonIconOnly);
    panels->setIconSize(QSize(46,42));
    panels->setStyleSheet(QStringLiteral("QToolButton { padding:3px; }"));
    auto *togglePanel=m_explorerDock->toggleViewAction();
    togglePanel->setIcon(actionIcon(11));
    togglePanel->setToolTip(QStringLiteral("분석 패널 표시/숨기기"));
    togglePanel->setStatusTip(togglePanel->toolTip());
    panels->addAction(togglePanel);
    panels->addSeparator();
    const QStringList titles = {QStringLiteral("Opt"), QStringLiteral("Freq"),
        QStringLiteral("Scan"), QStringLiteral("IRC"), QStringLiteral("TD-DFT"),
        QStringLiteral("Atom Properties"), QStringLiteral("Orbitals"), QStringLiteral("XYZ")};
    const QStringList descriptions = {QStringLiteral("Opt · 구조 최적화 단계와 에너지 표시/숨기기"),
        QStringLiteral("Freq · 진동 모드와 스펙트럼 표시/숨기기"),
        QStringLiteral("Scan · 좌표 스캔과 에너지 곡선 표시/숨기기"),
        QStringLiteral("IRC · 반응 경로와 에너지 표시/숨기기"),
        QStringLiteral("TD-DFT · 전자 여기 상태와 흡수 스펙트럼 표시/숨기기"),
        QStringLiteral("Atom Properties · 원자별 전하와 속성 표시/숨기기"),
        QStringLiteral("Orbitals · 분자 궤도와 에너지 준위 표시/숨기기"),
        QStringLiteral("XYZ · 현재 구조의 XYZ 좌표 표시/숨기기")};
    for (int i = 0; i < titles.size(); ++i) {
        auto *action = panels->addAction(actionIcon(12+i),titles[i]);
        action->setObjectName(QStringLiteral("AnalysisPanelTab%1").arg(i));
        action->setProperty("analysisTab", i);
        action->setCheckable(true);
        action->setToolTip(descriptions[i]);
        action->setStatusTip(descriptions[i]);
        connect(action, &QAction::triggered, this, [this, i] {
            if (m_explorerDock->isVisible() && m_analysis->currentTab() == i) {
                m_explorerDock->hide();
            } else {
                m_analysis->setCurrentTab(i);
                m_explorerDock->show();
                m_explorerDock->raise();
            }
            updatePanelActions();
        });
    }
    auto *output=m_outputDock->toggleViewAction();output->setObjectName("ToggleOutputSource");
    output->setIcon(actionIcon(21));output->setToolTip(QStringLiteral("Output 원문 · 결과 출처 확인 / Ctrl+F 검색"));panels->addAction(output);
    connect(output,&QAction::triggered,this,[this](bool visible){if(visible)updateOutputSource();});
    auto *navigator=panels->addAction(actionIcon(23),QStringLiteral("계산 결과 탐색 트리"));
    navigator->setObjectName("ToggleResultNavigator");navigator->setCheckable(true);
    navigator->setChecked(QSettings().value(QStringLiteral("ui/showResultNavigator"),false).toBool());
    navigator->setToolTip(QStringLiteral("계산 결과 탐색 목록 표시/숨기기 · 분석 탭과 같은 결과를 탐색합니다"));
    connect(navigator,&QAction::toggled,this,[this](bool visible){QSettings().setValue(QStringLiteral("ui/showResultNavigator"),visible);for(auto *tree:findChildren<QTreeWidget*>())tree->setVisible(visible);});
    panels->addSeparator();
    m_atomLabelsAction->setIcon(actionIcon(20));
    m_atomLabelsAction->setStatusTip(m_atomLabelsAction->toolTip());
    panels->addAction(m_atomLabelsAction);
    panels->addSeparator();
    auto toolIcon=[](int kind){
        QPixmap pix(52,52);pix.fill(Qt::transparent);QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.scale(2,2);
        p.setPen(QPen(QColor("#54748b"),1.7));p.setBrush(QColor("#c5dfee"));
        if(kind==0){p.drawEllipse(QPointF(9,13),4,4);p.drawLine(14,13,23,13);p.drawLine(20,10,23,13);p.drawLine(20,16,23,13);}
        else if(kind==5){p.drawLine(5,19,12,8);p.drawLine(12,8,22,18);p.drawArc(QRectF(7,11,11,11),15*16,120*16);}
        else if(kind==6||kind==7){p.drawRoundedRect(QRectF(7,11,13,11),2,2);p.drawArc(QRectF(10,4,7,12),0,180*16);if(kind==7){p.setPen(QPen(QColor("#b77777"),2));p.drawLine(4,23,23,4);}}
        else {p.drawLine(6,14,13,7);p.drawLine(13,7,20,14);for(auto c:{QPointF(6,14),QPointF(13,7),QPointF(20,14)})p.drawEllipse(c,2.5,2.5);
            if(kind==2||kind==4){p.drawArc(QRectF(3,6,20,18),190*16,140*16);p.drawLine(21,20,23,15);p.drawLine(21,20,17,19);}
            else {p.drawLine(5,22,22,22);p.drawLine(19,19,22,22);p.drawLine(19,25,22,22);}
            if(kind>=3&&kind<=4){p.setPen(QPen(QColor("#ae8cbd"),1));p.setBrush(Qt::NoBrush);p.drawRoundedRect(QRectF(1,1,24,24),3,3);}
        }
        p.end();pix.setDevicePixelRatio(2);return QIcon(pix);
    };
    auto *editTools=new QActionGroup(this);editTools->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    const QStringList editNames={QStringLiteral("원자 / 선택 원자 이동 · 왼쪽 드래그"),QStringLiteral("Fragment 이동 · 연결된 조각 드래그"),QStringLiteral("Fragment 회전 · 연결된 조각 드래그"),QStringLiteral("분자 전체 이동 · 왼쪽 드래그"),QStringLiteral("분자 전체 회전 · 왼쪽 드래그")};
    for(int mode=0;mode<editNames.size();++mode){auto *a=panels->addAction(captionedIcon(toolIcon(mode),QStringList{QStringLiteral("원자 이동"),QStringLiteral("조각 이동"),QStringLiteral("조각 회전"),QStringLiteral("분자 이동"),QStringLiteral("분자 회전")}[mode],mode==0?QStringLiteral("Ctrl+D"):QString()),editNames[mode]);a->setObjectName(QStringLiteral("GeometryDragTool%1").arg(mode));a->setToolTip(editNames[mode]);a->setCheckable(true);editTools->addAction(a);
        connect(a,&QAction::triggered,this,[this,mode](bool on){if(!on){setAtomDragMode(false);return;}setAtomDragMode(true);m_view->setTransformMode(mode);updatePanelActions();setStatus(QStringLiteral("%1 · 우클릭: 시점 회전 · 가운데 버튼: 시점 이동").arg(QStringList{"원자 이동","Fragment 이동","Fragment 회전","분자 이동","분자 회전"}[mode]),0);});}
    auto *centricTool=panels->addAction(captionedIcon(toolIcon(5),QStringLiteral("결합 각도")),QStringLiteral("Fragment 각도 / 비틀림 · 결합 중심 편집"));centricTool->setObjectName("BondCentricTool");centricTool->setCheckable(true);editTools->addAction(centricTool);
    centricTool->setToolTip(QStringLiteral("결합 클릭 후 끝점 드래그: 각도 · Ctrl: 비틀림 · 오른쪽: 거리 · Alt: 반대쪽"));
    connect(centricTool,&QAction::triggered,this,[this](bool on){if(!on){m_view->setBondCentricMode(false);return;}ensureEditableDocument();setDrawingMode(false);setAtomDragMode(false);m_view->setBondCentricMode(true);updatePanelActions();setStatus(QStringLiteral("결합을 클릭하고 끝점을 드래그하세요. Ctrl: 비틀림 · 오른쪽: 거리 · Alt: 반대쪽"),0);});
    auto *freezeCore=panels->addAction(captionedIcon(toolIcon(6),QStringLiteral("구조 고정")),QStringLiteral("현재 구조 고정 · 선택 원자가 있으면 선택만 고정"));freezeCore->setObjectName("FreezeOptimizationCore");
    freezeCore->setToolTip(QStringLiteral("기존 구조를 고정한 뒤 새 조각을 추가하세요. MM / xTB에서 추가한 부분만 최적화합니다. 선택이 있으면 선택만 고정합니다."));
    connect(freezeCore,&QAction::triggered,this,[this]{auto edited=m_view->molecule();auto ids=m_view->selection().atoms;if(ids.isEmpty())for(int i=0;i<edited.atoms.size();++i)ids.insert(i);for(int i:ids)edited.atoms[i].optimizationFixed=true;commitEditedMolecule(edited,false);setStatus(QStringLiteral("원자 %1개 고정 · 추가 원자는 MM / xTB에서 자유 최적화됩니다.").arg(ids.size()),8000);});
    auto *unfreeze=panels->addAction(captionedIcon(toolIcon(7),QStringLiteral("고정 해제")),QStringLiteral("최적화 고정 해제 · 선택 또는 전체"));unfreeze->setObjectName("UnfreezeOptimizationCore");
    connect(unfreeze,&QAction::triggered,this,[this]{auto edited=m_view->molecule();const auto ids=m_view->selection().atoms;for(int i=0;i<edited.atoms.size();++i)if(ids.isEmpty()||ids.contains(i))edited.atoms[i].optimizationFixed=false;commitEditedMolecule(edited,false);setStatus(QStringLiteral("최적화 고정을 해제했습니다."));});
    auto *patternSpacer=new QWidget(panels);
    patternSpacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    panels->addWidget(patternSpacer);
    auto *templates=new QWidget(panels);
    templates->setObjectName(QStringLiteral("AppPatternSelector"));
    auto *templateRow=new QHBoxLayout(templates);
    templateRow->setContentsMargins(6,0,2,0);templateRow->setSpacing(4);
    auto *templateLabel=new QLabel(QStringLiteral("pattern"),templates);
    templateLabel->setFont(paletteLabel->font());
    // Give both rows the same width so the six options line up vertically.
    const int labelWidth=qMax(paletteLabel->sizeHint().width(),templateLabel->sizeHint().width());
    paletteLabel->setFixedWidth(labelWidth);templateLabel->setFixedWidth(labelWidth);
    templateLabel->setToolTip(QStringLiteral("문양 템플릿 · 다시 누르면 기본 종이로 돌아갑니다"));
    templateRow->addWidget(templateLabel);
    auto *patternGroup=new QButtonGroup(templates);patternGroup->setExclusive(false);
    const QStringList names={QStringLiteral("달빛 몽환"),QStringLiteral("랩 모눈"),QStringLiteral("트윌 사선"),QStringLiteral("소다 물결"),QStringLiteral("보태니컬 잎"),QStringLiteral("분자 격자")};
    const QStringList motifDescriptions={QStringLiteral("초승달 · 별빛 · 옅은 빛무리가 떠 있는 꿈속의 하늘"),QStringLiteral("정갈한 실험실 그래프 종이"),QStringLiteral("비스듬히 짜인 가벼운 패브릭"),QStringLiteral("부드럽게 이어지는 잔물결"),QStringLiteral("식물 표본을 닮은 잎사귀"),QStringLiteral("육각 고리와 원자로 이어진 화학 문양")};
    for(int i=0;i<6;++i){
        auto *button=new QToolButton(templates);
        button->setObjectName(QStringLiteral("PatternTemplate%1").arg(i));
        button->setAccessibleName(names[i]);button->setToolTip(names[i]+QStringLiteral(" · ")+motifDescriptions[i]+QStringLiteral("\n색상 팔레트와 자유롭게 조합 · 다시 누르면 문양 해제"));
        button->setStatusTip(button->toolTip());button->setCheckable(true);
        button->setFixedSize(32,32);button->setIconSize(QSize(28,28));button->setIcon(motifIcon(i));
        button->setStyleSheet(QStringLiteral("QToolButton {padding:0;border:2px solid transparent;border-radius:8px;background:transparent;} QToolButton:hover {border-color:#9aafb9;} QToolButton:checked {border-color:#4d7589;background:#e7f0f4;} QToolButton:focus {border-style:dashed;border-color:#4d7589;}"));
        patternGroup->addButton(button,i);templateRow->addWidget(button);
    }
    panels->addWidget(templates);
    auto applyPattern=[this,toolbar,panels,patternGroup,applyTheme](int index){
        index=qBound(-1,index,5);
        for(int i=0;i<6;++i)patternGroup->button(i)->setChecked(i==index);
        toolbar->setMotif(index);panels->setMotif(index);
        QSettings().setValue(QStringLiteral("ui/patternTemplate"),index);
        applyTheme(QSettings().value(QStringLiteral("ui/theme"),0).toInt());
    };
    connect(patternGroup,&QButtonGroup::idClicked,this,[patternGroup,applyPattern](int id){applyPattern(patternGroup->button(id)->isChecked()?id:-1);});
    applyPattern(QSettings().value(QStringLiteral("ui/patternTemplate"),-1).toInt());
    connect(m_explorerDock, &QDockWidget::visibilityChanged, this, &MainWindow::updatePanelActions);
    updatePanelActions();
    applyTheme(QSettings().value(QStringLiteral("ui/theme"),0).toInt());
}

void MainWindow::updatePanelActions() {
    for(int i=0;i<5;++i)if(auto *a=findChild<QAction *>(QStringLiteral("GeometryDragTool%1").arg(i)))a->setChecked(m_dragAtoms&&m_view&&m_view->transformMode()==i);
    if(auto *a=findChild<QAction *>("BondCentricTool"))a->setChecked(m_view&&m_view->bondCentricMode());
    for (auto *action : findChildren<QAction *>()) {
        const auto tab = action->property("analysisTab");
        if (tab.isValid()) action->setChecked(m_explorerDock->isVisible() && m_analysis->currentTab() == tab.toInt());
    }
}

} // namespace youngcraft
