#include "ui/SidebarButton.h"
#include "ui/SidebarFlowLayout.h"
#include "app/MainWindow.h"

#include "export/ReactionCapture.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/AppPattern.h"
#include "ui/DialogThemeSync.h"

#include <QActionGroup>
#include <QDockWidget>
#include <QFileInfo>
#include <QGridLayout>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QSettings>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTabBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace youngcraft {
namespace {
using uiPattern::drawMotif;

class PatternToolBar final : public QToolBar {
public:
    explicit PatternToolBar(const QString &title,QWidget *parent):QToolBar(title,parent){}
    void setMotif(int motif) { m_motif=motif;setProperty("patternTemplate",motif);update(); }
protected:
    void paintEvent(QPaintEvent *event) override {
        QToolBar::paintEvent(event);
        if(m_motif<0)return;
        QPainter painter(this);uiPattern::paintBackdrop(painter,this,false);
    }
private:
    int m_motif=-1;
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
    case 24: { // A reference book and reaction-path nodes.
        p.drawRoundedRect(QRectF(3,4,26,24),2,2);p.drawLine(16,4,16,28);
        p.drawLine(7,10,12,10);p.drawLine(7,15,12,15);
        p.drawPolyline(QPolygonF{{19,22},{23,12},{27,18}});
        p.drawEllipse(QPointF(19,22),1,1);p.drawEllipse(QPointF(23,12),1,1);
        p.drawEllipse(QPointF(27,18),1,1);break;
    }
    case 25: p.drawRoundedRect(QRectF(3,4,26,24),2,2);p.drawLine(21,4,21,28);p.drawLine(24,10,26,10);p.drawLine(24,15,26,15);p.drawLine(24,20,26,20);break;
    case 26: p.drawRoundedRect(QRectF(3,4,26,24),2,2);p.drawLine(3,19,29,19);p.drawLine(8,24,24,24);break;
    case 27: p.drawLine(16,3,16,29);p.drawLine(3,16,29,16);
        p.drawPolyline(QPolygonF{{12,7},{16,3},{20,7}});p.drawPolyline(QPolygonF{{25,12},{29,16},{25,20}});
        p.drawPolyline(QPolygonF{{12,25},{16,29},{20,25}});p.drawPolyline(QPolygonF{{7,12},{3,16},{7,20}});break;
    case 28: p.drawRoundedRect(QRectF(6,13,20,15),2,2);p.drawArc(QRectF(10,3,12,20),0,180*16);p.drawLine(16,18,16,23);break;
    case 29: p.drawLine(5,7,27,7);p.drawLine(5,16,27,16);p.drawLine(5,25,27,25);
        p.setBrush(ink);p.drawEllipse(QPointF(12,7),3,3);p.drawEllipse(QPointF(22,16),3,3);p.drawEllipse(QPointF(15,25),3,3);break;
    case 30: p.drawRect(2,5,21,21);p.drawLine(9,5,9,26);p.drawLine(16,5,16,26);
        p.drawLine(2,12,23,12);p.drawLine(2,19,23,19);p.drawLine(24,15,30,15);p.drawLine(27,12,27,18);break;
    case 31: p.drawLine(4,5,13,5);p.drawLine(4,5,4,14);p.drawLine(28,5,19,5);p.drawLine(28,5,28,14);
        p.drawLine(4,27,13,27);p.drawLine(4,27,4,18);p.drawLine(28,27,19,27);p.drawLine(28,27,28,18);
        p.drawEllipse(QPointF(16,16),5,5);break;
    case 32: p.drawPolyline(QPolygonF{{12,10},{5,10},{5,28},{27,28},{27,10},{20,10}});
        p.drawLine(16,20,16,3);p.drawPolyline(QPolygonF{{11,8},{16,3},{21,8}});break;
    case 20: {p.drawEllipse(QPointF(11,16),8,8);QFont font=p.font();font.setBold(true);font.setPixelSize(15);p.setFont(font);p.drawText(QRectF(16,1,15,18),Qt::AlignCenter,QStringLiteral("#"));break;}
    default: p.drawRoundedRect(QRectF(3,8,17,10),5,5);p.drawRoundedRect(QRectF(12,15,17,10),5,5);p.drawLine(12,16,20,16);break;
    }
    p.end();
    if(id==2 || id==9) {
        // Center the actual glyph, including asymmetric pencil strokes, within its bitmap.
        const auto pixels=pix.toImage();QRect bounds;
        for(int y=0;y<pixels.height();++y)for(int x=0;x<pixels.width();++x)
            if(qAlpha(pixels.pixel(x,y)))bounds=bounds.united(QRect(x,y,1,1));
        if(!bounds.isEmpty()) {
            QPixmap centered(pix.size());centered.fill(Qt::transparent);QPainter align(&centered);
            align.drawPixmap(QPoint((pix.width()-bounds.width())/2-bounds.x(),(pix.height()-bounds.height())/2-bounds.y()),pix);
            align.end();pix=centered;
        }
    }
    pix.setDevicePixelRatio(2);
    QIcon icon(pix);
    QPixmap checked=pix;
    QPainter tint(&checked);
    tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tint.fillRect(checked.rect(),Qt::white);
    tint.end();
    icon.addPixmap(checked,QIcon::Normal,QIcon::On);
    icon.addPixmap(checked,QIcon::Active,QIcon::On);
    return icon;
}
}


void MainWindow::createToolBar() {
    auto *toolbar = new PatternToolBar(QStringLiteral("주 도구 모음"), this);
    toolbar->setObjectName(QStringLiteral("MainToolBar"));
    toolbar->setMovable(false);
    toolbar->setAllowedAreas(Qt::TopToolBarArea);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(sidebarIconSize, sidebarIconSize));
    addToolBar(toolbar);

    const auto addCommonAction = [this, toolbar](const char *name, const QString &label, int icon) {
        auto *action = findChild<QAction *>(QString::fromLatin1(name));
        action->setIconText(label);
        action->setProperty("toolbarIconId", icon);
        toolbar->addAction(action);
    };
    addCommonAction("NewStructure", QStringLiteral("새 구조"), 0);
    addCommonAction("OpenStructureFile", QStringLiteral("열기"), 1);
    toolbar->addSeparator();
    addCommonAction("MMOptimizationAction", QStringLiteral("MM 최적화"), 3);
    addCommonAction("QMOptimizationAction", QStringLiteral("xTB"), 22);
    toolbar->addSeparator();
    addCommonAction("CopyStructureImage", QStringLiteral("이미지 복사"), 4);
    addCommonAction("CustomStructureCapture", QStringLiteral("맞춤 캡처"), 5);

    const auto addPopup = [this, toolbar](const QString &title, const QString &name, int iconId) {
        auto *action = new QAction(title, toolbar);
        action->setObjectName(name);
        action->setProperty("toolbarIconId", iconId);
        action->setMenu(new QMenu(title, this));
        toolbar->addAction(action);
        auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
        button->setPopupMode(QToolButton::InstantPopup);
        return action;
    };
    auto *exportAction = addPopup(QStringLiteral("내보내기"), QStringLiteral("ToolbarExport"), 32);
    auto *exportMenu = exportAction->menu();
    exportMenu->setToolTipsVisible(true);
    exportMenu->addAction(findChild<QAction *>(QStringLiteral("SaveStructureImage")));
    exportMenu->addAction(findChild<QAction *>(QStringLiteral("OpenMultiCapture")));
    auto *ircCapture = exportMenu->addAction(QStringLiteral("IRC·TS 캡처…"), this, [this] { openReactionCapture(); });
    connect(exportMenu, &QMenu::aboutToShow, this, [this, ircCapture] {
        const auto paths = ReactionCapture::companionPaths(m_currentFile);
        QString reason;
        if (paths.isEmpty() || m_view->molecule().isEmpty()) {
            reason = QStringLiteral("-freq-ts-freq.out 파일을 열어야 합니다.");
        } else {
            const auto mode = std::find_if(m_data.frequencies.cbegin(), m_data.frequencies.cend(),
                [](const FrequencyMode &frequency) { return frequency.imaginary || frequency.wavenumber < 0; });
            if (mode == m_data.frequencies.cend() || mode->displacement.size() != m_data.molecule.atoms.size())
                reason = QStringLiteral("유효한 허수 진동 모드가 필요합니다.");
            else if (!std::all_of(paths.cbegin(), paths.cend(), [](const QString &path) { return QFileInfo(path).isFile(); }))
                reason = QStringLiteral("같은 폴더에 정방향·역방향 IRC 결과 파일이 필요합니다.");
        }
        ircCapture->setEnabled(reason.isEmpty());
        ircCapture->setText(reason.isEmpty() ? QStringLiteral("IRC·TS 캡처…")
            : QStringLiteral("IRC·TS 캡처 · %1").arg(reason));
    });
    exportMenu->addAction(QStringLiteral("메커니즘 영상…"), this, &MainWindow::openMechanismAnimation);

    auto *viewMenu = findChild<QMenu *>(QStringLiteral("ViewMenu"));
    toolbar->addSeparator();
    const auto addDockToggle = [toolbar](QDockWidget *dock, const QString &name, const QString &label, int iconId) {
        auto *action = dock->toggleViewAction();
        action->setObjectName(name);
        action->setIconText(label);
        action->setToolTip(label);
        action->setProperty("toolbarIconId", iconId);
        toolbar->addAction(action);
    };
    addDockToggle(m_explorerDock, QStringLiteral("ToggleResultsSidebar"), QStringLiteral("결과 패널"), 11);
    addDockToggle(m_structureDock, QStringLiteral("ToggleStructureSidebar"), QStringLiteral("구조 패널"), 25);

    const auto addButton = [](QAction *action, QWidget *parent) {
        auto *button = new QToolButton(parent);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        sizeSidebarButton(button);
        button->setAccessibleName(action->text());
        button->setDefaultAction(action);
        if (action->menu()) button->setPopupMode(QToolButton::InstantPopup);
        return button;
    };

    m_analysisMenu = viewMenu->addMenu(QStringLiteral("분석 종류"));
    auto *analysisGroup = new QActionGroup(m_analysisMenu);
    auto *analysisGrid = new SidebarFlowLayout(m_analysisShortcuts);
    analysisGrid->setObjectName("AnalysisShortcutFlow");
    analysisGrid->setContentsMargins(0, 0, 0, 0);
    analysisGrid->setSpacing(4);
    const QStringList labels = {QStringLiteral("Opt"), QStringLiteral("Freq"),
        QStringLiteral("Scan"), QStringLiteral("IRC"), QStringLiteral("TD-DFT"),
        QStringLiteral("원자 속성"), QStringLiteral("Orbitals"), QStringLiteral("XYZ")};
    const auto titles = m_analysis->tabTitles();
    for (int i = 0; i < titles.size(); ++i) {
        auto *action = m_analysisMenu->addAction(labels[i], this, [this, i] {
            m_explorerDock->show();
            m_analysis->setCurrentTab(i);
        });
        action->setObjectName(QStringLiteral("AnalysisPanelTab%1").arg(i));
        action->setToolTip(titles[i]);
        action->setProperty("analysisTab", i);
        action->setProperty("toolbarIconId", 12 + i);
        action->setCheckable(true);
        analysisGroup->addAction(action);
        auto *button = addButton(action, m_analysisShortcuts);
        button->setObjectName(QStringLiteral("AnalysisShortcut%1").arg(i));
        analysisGrid->addWidget(button);
    }
    auto *output = m_outputDock->toggleViewAction();
    output->setObjectName(QStringLiteral("ToggleOutputSource"));
    output->setToolTip(QStringLiteral("Output 원문 · 결과 출처 확인 / Ctrl+F 검색"));
    output->setProperty("toolbarIconId", 26);
    viewMenu->addAction(output);
    connect(output, &QAction::triggered, this, [this](bool visible) { if (visible) updateOutputSource(); });
    auto *navigator = viewMenu->addAction(QStringLiteral("계산 결과 탐색 트리"));
    navigator->setObjectName(QStringLiteral("ToggleResultNavigator"));
    navigator->setCheckable(true);
    navigator->setChecked(QSettings().value(QStringLiteral("ui/showResultNavigator"), false).toBool());
    connect(navigator, &QAction::toggled, this, [this](bool visible) {
        QSettings().setValue(QStringLiteral("ui/showResultNavigator"), visible);
        for (auto *sidebar : m_sidebars->findChildren<QTreeWidget *>()) {
            auto *panel=sidebar->parentWidget()->findChild<AnalysisPanel*>();
            sidebar->setVisible(visible && (!panel || (panel->currentTab()!=1 && panel->currentTab()!=6 && panel->currentTab()!=7)));
        }
    });
    output->setIconText(QStringLiteral("원문 보기"));
    toolbar->addAction(output);
    auto *sourceButton = addButton(output, m_analysisShortcuts);
    sourceButton->setObjectName(QStringLiteral("ResultSourceButton"));
    analysisGrid->addWidget(sourceButton);
    for (auto *action : toolbar->actions()) {
        if (auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action))) {
            sizeSidebarButton(button);
            button->setAccessibleName(action->iconText());
        }
    }

    auto *displayPage = m_structureTabs->widget(0);
    auto *displayLayout = static_cast<QVBoxLayout *>(displayPage->layout());
    auto *displayTools = new SidebarFlowLayout;
    displayTools->setObjectName("StructureDisplayFlow");
    displayTools->setSpacing(4);
    displayLayout->addLayout(displayTools);
    displayLayout->addStretch();
    const auto addDisplayButton = [&](QAction *action, int iconId) {
        action->setProperty("toolbarIconId", iconId);
        displayTools->addWidget(addButton(action, displayPage));
    };
    addDisplayButton(findChild<QAction *>("BestStructureView"), 31);
    addDisplayButton(findChild<QAction *>("CenterStructureView"), 8);
    addDisplayButton(findChild<QAction *>("HideCarbonHydrogens"), 6);
    addDisplayButton(findChild<QAction *>("RestoreCarbonHydrogens"), 7);
    addDisplayButton(findChild<QAction *>("ShowAtomNumbers"), 20);
    addDisplayButton(m_atomFocusMenu->menuAction(), 17);
    addDisplayButton(m_syncViewsAction, 10);

    auto *tabs = m_structureTabs->tabBar();
    tabs->setExpanding(false);
    for (int tab = 0; tab < tabs->count(); ++tab) {
        const auto label = tabs->tabText(tab);
        tabs->setTabToolTip(tab, label);
        tabs->setAccessibleTabName(tab, label);
        tabs->setTabText(tab, {});
    }

    auto *editPage = qobject_cast<QScrollArea *>(m_structureTabs->widget(1))->widget();
    auto *editLayout = static_cast<QVBoxLayout *>(editPage->layout());
    auto *editButtons = new SidebarFlowLayout;
    editButtons->setObjectName("StructureEditFlow");
    editButtons->setSpacing(4);
    editLayout->insertLayout(0, editButtons);
    const auto addEditButton = [&](QAction *action, int iconId) {
        action->setProperty("toolbarIconId", iconId);
        editButtons->addWidget(addButton(action, editPage));
    };
    auto *draw = new QAction(QStringLiteral("현재 구조 편집"), editPage);
    draw->setObjectName(QStringLiteral("DrawCurrentStructure"));
    connect(draw, &QAction::triggered, this, [this] {
        if (!m_currentFile.isEmpty()) newDrawingDocument(m_view->molecule());
        else setDrawingMode(!m_drawingMode);
    });
    addEditButton(draw, 2);

    m_viewportEditMenu = new QMenu(QStringLiteral("정밀 편집"), editPage);
    auto *editTools = new QActionGroup(m_viewportEditMenu);
    editTools->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    const QStringList editNames = {QStringLiteral("원자 이동"), QStringLiteral("조각 각도"),
        QStringLiteral("조각 비틀림"), QStringLiteral("분자 이동"), QStringLiteral("분자 회전")};
    const QStringList editHints = {QStringLiteral("선택 원자를 왼쪽 버튼으로 드래그 · Ctrl+D"),
        QStringLiteral("가지 원자를 드래그하여 각도 조정"), QStringLiteral("가지 원자를 드래그하여 결합축 회전"),
        QStringLiteral("분자를 왼쪽 버튼으로 드래그하여 이동"), QStringLiteral("분자를 왼쪽 버튼으로 드래그하여 회전")};
    m_viewportEditMenu->setToolTipsVisible(true);
    for (int mode = 0; mode < editNames.size(); ++mode) {
        auto *action = m_viewportEditMenu->addAction(editNames[mode]);
        action->setObjectName(QStringLiteral("GeometryDragTool%1").arg(mode));
        action->setToolTip(editHints[mode]);
        action->setCheckable(true);
        if (mode == 0) addEditButton(action, 27);
        editTools->addAction(action);
        connect(action, &QAction::triggered, this, [this, mode, hint = editHints[mode]](bool on) {
            setAtomDragMode(on);
            if (!on) return;
            m_view->setTransformMode(mode);
            m_workspace->setCurrentWidget(m_moleculeWorkspace);
            updatePanelActions();
            setStatus(hint, 0);
        });
    }
    auto *centricTool = m_viewportEditMenu->addAction(QStringLiteral("결합 중심 편집"));
    centricTool->setObjectName(QStringLiteral("BondCentricTool"));
    centricTool->setCheckable(true);
    centricTool->setToolTip(QStringLiteral("결합 끝점 드래그: 각도 · Ctrl: 비틀림 · 오른쪽: 거리 · Alt: 반대쪽"));
    editTools->addAction(centricTool);
    connect(centricTool, &QAction::triggered, this, [this, centricTool](bool on) {
        if (on) {
            ensureEditableDocument();
            setDrawingMode(false);
            setAtomDragMode(false);
            m_workspace->setCurrentWidget(m_moleculeWorkspace);
            setStatus(centricTool->toolTip(), 0);
        }
        m_view->setBondCentricMode(on);
        updatePanelActions();
    });
    m_viewportEditMenu->addSeparator();
    auto *freezeCore = m_viewportEditMenu->addAction(QStringLiteral("구조 고정"), this, [this] {
        auto edited = m_view->molecule();
        const auto ids = m_view->selection().atoms;
        for (int i = 0; i < edited.atoms.size(); ++i)
            if (ids.isEmpty() || ids.contains(i)) edited.atoms[i].optimizationFixed = true;
        commitEditedMolecule(edited, false);
        setStatus(QStringLiteral("원자 %1개 고정 · 추가 원자는 MM / xTB에서 자유 최적화됩니다.")
            .arg(ids.isEmpty() ? edited.atoms.size() : ids.size()), 8000);
    });
    freezeCore->setObjectName(QStringLiteral("FreezeOptimizationCore"));
    addEditButton(freezeCore, 28);
    freezeCore->setToolTip(QStringLiteral("선택 원자를 고정합니다. 선택이 없으면 현재 구조 전체를 고정합니다."));
    m_viewportEditMenu->addAction(QStringLiteral("고정 해제"), this, [this] {
        auto edited = m_view->molecule();
        const auto ids = m_view->selection().atoms;
        for (int i = 0; i < edited.atoms.size(); ++i)
            if (ids.isEmpty() || ids.contains(i)) edited.atoms[i].optimizationFixed = false;
        commitEditedMolecule(edited, false);
        setStatus(QStringLiteral("최적화 고정을 해제했습니다."));
    })->setObjectName(QStringLiteral("UnfreezeOptimizationCore"));
    m_viewportEditMenu->addSeparator();
    m_viewportEditMenu->addAction(QStringLiteral("편집 도구 종료"), this, [this] {
        m_view->setBondCentricMode(false);
        if (m_dragAtoms) setAtomDragMode(false);
        if (m_drawingMode) setDrawingMode(false);
        updatePanelActions();
    });

    addEditButton(m_viewportEditMenu->menuAction(), 29);
    m_viewportEditMenu->addSeparator();
    m_viewportEditMenu->addAction(m_geometryAction);
    auto *insertMenu = new QMenu(QStringLiteral("조각 / 리간드"), editPage);
    insertMenu->addAction(QStringLiteral("조각 / 리간드 삽입…"), this, [this] { openFragmentBrowser(); });
    insertMenu->addAction(QStringLiteral("금속–리간드–산소 조합…"), this, &MainWindow::buildLabComplex);
    addEditButton(insertMenu->menuAction(), 30);

    struct Theme { QString name, description, paper, surface, tint, ink, border; };
    const QVector<Theme> themes = {
        {QStringLiteral("구름 소다"), QStringLiteral("맑은 소다빛 종이 · 차분한 블루 잉크"), "#edf4fa", "#fbfdff", "#c5dfee", "#446781", "#c2d3df"},
        {QStringLiteral("세이지 온실"), QStringLiteral("세이지 · 아이보리 · 식물 표본실"), "#eef4ee", "#fcfdf8", "#cbdcc6", "#4d6c5b", "#c6d4c6"},
        {QStringLiteral("라일락 노트"), QStringLiteral("보랏빛 안개 · 따뜻한 노트 종이"), "#f2eef8", "#fefcff", "#dbceec", "#6d5b85", "#d4c9e1"},
        {QStringLiteral("오트밀 아틀리에"), QStringLiteral("오트밀 종이 · 부드러운 모카 잉크"), "#f6f1e7", "#fffdf7", "#e6d8bb", "#79634c", "#ded1bc"},
        {QStringLiteral("로즈 밀크"), QStringLiteral("빛바랜 장미 · 우윳빛 도자기"), "#f8eef0", "#fffcfc", "#edced7", "#865e6d", "#e4cbd3"},
        {QStringLiteral("살구 다실"), QStringLiteral("살구빛 햇살 · 크림색 찻잔"), "#faf0e8", "#fffdf9", "#f0d2b8", "#865f49", "#e5cfbd"},
        {QStringLiteral("교수님이 보고있어"), QStringLiteral("교수님 사진 배경 · 불투명도 60% · 차분한 블루 그레이"), "#edf1f5", "#fbfcfe", "#ced9e3", "#4c6278", "#c7d1dc"}
    };
    auto *themeMenu = viewMenu->addMenu(QStringLiteral("테마"));
    auto *paletteMenu = themeMenu->addMenu(QStringLiteral("팔레트"));
    paletteMenu->setObjectName(QStringLiteral("AppThemeSelector"));
    paletteMenu->setToolTipsVisible(true);
    auto *themeGroup = new QActionGroup(paletteMenu);
    for (int i = 0; i < themes.size(); ++i) {
        const auto &theme = themes[i];
        auto *action = paletteMenu->addAction(theme.name);
        action->setObjectName(QStringLiteral("PastelTheme%1").arg(i));
        action->setToolTip(theme.description);
        action->setCheckable(true);
        QPixmap chip(24, 24);
        chip.fill(Qt::transparent);
        QPainter painter(&chip);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QColor(theme.border));
        painter.setBrush(QColor(theme.tint));
        painter.drawEllipse(QRectF(2, 2, 20, 20));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(theme.surface));
        painter.drawEllipse(QRectF(6, 6, 8, 8));
        painter.setBrush(QColor(theme.ink));
        painter.drawEllipse(QRectF(15, 14, 5, 5));
        painter.end();
        action->setIcon(QIcon(chip));
        action->setData(i);
        themeGroup->addAction(action);
    }
    auto applyTheme = [this,themes,themeGroup](int index) {
        index = qBound(0, index, int(themes.size())-1);
        const auto &t=themes[index];
        setProperty("professorBackdrop", index == 6);
        auto *glowTimer=findChild<QTimer*>(QStringLiteral("ProfessorGlowTimer"));
        if(!glowTimer) {
            glowTimer=new QTimer(this);glowTimer->setObjectName(QStringLiteral("ProfessorGlowTimer"));
            glowTimer->setInterval(40);
            connect(glowTimer,&QTimer::timeout,this,[this]{
                if(isMinimized())return;
                for(auto *view:findChildren<MoleculeView*>())if(view->isVisible())view->update();
            });
        }
        if(index==6)glowTimer->start();else glowTimer->stop();
        const QString text="#34404b",field=t.surface;
        const QString border=QColor(t.ink).lighter(150).name();
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
        setStyleSheet(QStringLiteral(
            "QMainWindow,QDockWidget,QToolBar,QStatusBar { background:%1; color:%2; }"
            "QLabel,QCheckBox,QRadioButton,QGroupBox { color:%2; }"
            "QToolBar { border:0; spacing:4px; padding:4px; }"
            "QDockWidget { border:0; }"
            "QDockWidget::title { background:%7; color:%2; padding:1px 8px; text-align:left; }"
            "QDockWidget::close-button { subcontrol-position:top right; width:20px; height:20px; }"
            "QDockWidget::close-button:hover { background:%4; border:1px solid %3; }"
            "QMainWindow::separator { background:%6; width:4px; height:4px; }"
            "QMainWindow::separator:hover { background:%3; }"
            "QDockWidget QStackedWidget,QDockWidget QScrollArea,QDockWidget QTabWidget::pane { background:%1; border:0; }"
            "QWidget#StructureToolsContainer,QWidget#StructureDisplayPage,QWidget#StructureEditPage,QDockWidget QScrollArea > QWidget { background:%1; }"
            "QDockWidget QAbstractItemView { border:1px solid %6; border-radius:4px; }"
            "QHeaderView::section { background:%7; color:%2; border:0; padding:4px; }"
            "QToolButton,QPushButton,QComboBox { padding:5px; border:1px solid %6; border-radius:6px; background:%4; color:%2; }"
            "QToolButton:checked,QPushButton:checked,QTabBar::tab:selected { background:%3; color:%5; }"
            "QToolButton:hover,QPushButton:hover { border:1px solid %3; background:%7; }"
            "QToolButton:checked:hover,QPushButton:checked:hover { background:%3; }"
            "QToolButton:disabled,QPushButton:disabled,QToolButton:disabled:checked,QPushButton:disabled:checked { color:#68717b; background:%1; border-color:%6; }"
            "QToolButton[sidebarButton=true],QPushButton[sidebarButton=true] { padding:0; border-radius:7px; }"
            "QToolButton:checked,QPushButton:checked { border-color:%3; }"
            "QToolButton[sidebarButton=true]::menu-indicator { image:none; width:0; }"
            "QToolButton[sidebarButton=true]:focus,QPushButton[sidebarButton=true]:focus { border:1px solid %3; }"
            "QTabWidget::pane { border:1px solid %6; background:%1; }"
            "QTabWidget#DocumentTabs::pane { border:0; }"
            "QTabBar::tab { padding:6px 12px; color:%2; background:%4; border:1px solid %6; }"
            "QTabBar::tab:hover { background:%7; }"
            "QTabBar::tab:selected { background:%3; color:%5; border-color:%3; }"
            "QTabWidget#StructureToolsTabs QTabBar::tab { width:30px; height:30px; padding:0;"
            " margin-right:4px; border:1px solid %6; border-radius:7px; background:%4; }"
            "QTabWidget#StructureToolsTabs QTabBar::tab:hover { border-color:%3; background:%7; }"
            "QTabWidget#StructureToolsTabs QTabBar::tab:selected { border-color:%3; background:%3; }"
            "QAbstractItemView,QPlainTextEdit,QTextEdit,QLineEdit,QSpinBox,QDoubleSpinBox { background:%4; color:%2; selection-background-color:%3; selection-color:white; }"
            "QMenuBar,QMenu { background:%1; color:%2; } QMenu::item:selected { background:%3; color:%5; }"
            "QWidget#MoleculeViewport { background:%1; }"
            "QToolTip { background:%4; color:%2; border:1px solid %6; padding:5px; }"
        ).arg(t.paper,text,t.ink,field,QStringLiteral("#ffffff"),border,t.tint));
        for (auto *action : findChildren<QAction *>()) {
            const auto iconId = action->property("toolbarIconId");
            if (iconId.isValid()) action->setIcon(actionIcon(iconId.toInt(), QColor(t.ink)));
        }
        for (int tab = 0; tab < m_structureTabs->count(); ++tab)
            m_structureTabs->setTabIcon(tab, actionIcon(tab == 0 ? 9 : 2, QColor(t.ink)));
        themeGroup->actions().at(index)->setChecked(true);
        for(auto *view:findChildren<MoleculeView *>())view->update();
        for(auto *surface:findChildren<QWidget *>(QStringLiteral("MoleculeViewport")))surface->update();
        QSettings().setValue(QStringLiteral("ui/theme"), index);
        uiPattern::synchronizeDialogs(this);
    };
    connect(themeGroup, &QActionGroup::triggered, this, [applyTheme](QAction *action) {
        applyTheme(action->data().toInt());
    });

    auto *patternMenu = themeMenu->addMenu(QStringLiteral("대화상자·시작 화면 문양"));
    patternMenu->setObjectName(QStringLiteral("AppPatternSelector"));
    patternMenu->setToolTipsVisible(true);
    auto *patternGroup = new QActionGroup(patternMenu);
    patternGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    const QStringList names={QStringLiteral("달빛 몽환"),QStringLiteral("랩 모눈"),QStringLiteral("트윌 사선"),QStringLiteral("소다 물결"),QStringLiteral("보태니컬 잎"),QStringLiteral("분자 격자")};
    const QStringList motifDescriptions={QStringLiteral("초승달 · 별빛 · 옅은 빛무리가 떠 있는 꿈속의 하늘"),QStringLiteral("정갈한 실험실 그래프 종이"),QStringLiteral("비스듬히 짜인 가벼운 패브릭"),QStringLiteral("부드럽게 이어지는 잔물결"),QStringLiteral("식물 표본을 닮은 잎사귀"),QStringLiteral("육각 고리와 원자로 이어진 화학 문양")};
    auto applyPattern = [toolbar, patternGroup, applyTheme](int index) {
        index = qBound(-1, index, 5);
        for (auto *action : patternGroup->actions()) action->setChecked(action->data().toInt() == index);
        toolbar->setMotif(index);
        QSettings().setValue(QStringLiteral("ui/patternTemplate"), index);
        applyTheme(QSettings().value(QStringLiteral("ui/theme"), 0).toInt());
    };
    patternMenu->addAction(QStringLiteral("문양 없음"), this, [applyPattern] { applyPattern(-1); });
    for (int i = 0; i < names.size(); ++i) {
        auto *action = patternMenu->addAction(motifIcon(i), names[i]);
        action->setObjectName(QStringLiteral("PatternTemplate%1").arg(i));
        action->setToolTip(motifDescriptions[i]);
        action->setCheckable(true);
        action->setData(i);
        patternGroup->addAction(action);
    }
    connect(patternGroup, &QActionGroup::triggered, this, [applyPattern](QAction *action) {
        applyPattern(action->isChecked() ? action->data().toInt() : -1);
    });
    applyPattern(QSettings().value(QStringLiteral("ui/patternTemplate"), -1).toInt());
    updatePanelActions();
}

void MainWindow::updatePanelActions() {
    if (!m_viewportEditMenu) return;
    m_explorer->setVisible(QSettings().value("ui/showResultNavigator",false).toBool() && m_analysis->currentTab()!=1 && m_analysis->currentTab()!=6 && m_analysis->currentTab()!=7);
    const bool hasStructure = !m_view->molecule().isEmpty();
    for (const auto *name : {"MMOptimizationAction", "QMOptimizationAction",
                             "CopyStructureImage"}) {
        findChild<QAction *>(QString::fromLatin1(name))->setEnabled(hasStructure);
    }
    m_structureTabs->widget(0)->setEnabled(hasStructure);
    m_viewportEditMenu->menuAction()->setEnabled(hasStructure);
    m_structureTabs->findChild<QAction *>(QStringLiteral("DrawCurrentStructure"))->setEnabled(
        hasStructure || !m_drawingMode);
    m_drawPanels->setVisible(m_drawingMode);

    const bool bondCentric = m_view->bondCentricMode();
    m_viewportEditMenu->findChild<QAction *>(QStringLiteral("BondCentricTool"))->setChecked(bondCentric);
    for (int mode = 0; mode < 5; ++mode) {
        auto *action = m_viewportEditMenu->findChild<QAction *>(QStringLiteral("GeometryDragTool%1").arg(mode));
        action->setEnabled(hasStructure);
        action->setChecked(m_dragAtoms && m_view->transformMode() == mode);
    }
    m_viewportEditMenu->findChild<QAction *>(QStringLiteral("FreezeOptimizationCore"))->setEnabled(hasStructure);

    const auto placeShortcut = [](QToolButton *button, bool visible) {
        button->setVisible(visible);
    };
    for (auto *action : m_analysisMenu->actions()) {
        const int index = action->property("analysisTab").toInt();
        action->setChecked(index == m_analysis->currentTab());
        auto *button = m_analysisShortcuts->findChild<QToolButton *>(QStringLiteral("AnalysisShortcut%1").arg(index));
        placeShortcut(button, m_analysis->hasTabData(index));
    }
    auto *sourceButton = m_analysisShortcuts->findChild<QToolButton *>(QStringLiteral("ResultSourceButton"));
    placeShortcut(sourceButton, !m_data.filePath.isEmpty());
    m_analysisShortcuts->layout()->invalidate();
    m_analysisShortcuts->updateGeometry();
}

} // namespace youngcraft
