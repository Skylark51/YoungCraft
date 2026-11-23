// Included by test_smoke.cpp. Keep this file compatible with baseline 0753d1a.
#include <QJsonObject>
#include <QSaveFile>
#include <functional>

namespace {
struct UxSettingsScope {
    QString org=QCoreApplication::organizationName(), app=QCoreApplication::applicationName();
    bool native=QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    UxSettingsScope() {
        QCoreApplication::setOrganizationName("YoungcraftTest");
        QCoreApplication::setApplicationName("UxReview20261123");
        QSettings().clear();
        QSettings().setValue("orbitals/remoteEnabled",false);
        QSettings().setValue("orbitals/autoGenerateHomo",false);
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);
    }
    ~UxSettingsScope() {
        QSettings().clear();
        QCoreApplication::setOrganizationName(org);QCoreApplication::setApplicationName(app);
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,native);
    }
};

struct UxEvidence {
    QString variant=qEnvironmentVariable("YOUNGCRAFT_UX_VARIANT","unspecified");
    QString directory;
    QJsonArray cases, steps;
    QString id, start, endpoint;
    int clicks=0;
    explicit UxEvidence(const QString &kind) {
        directory=qEnvironmentVariable("YOUNGCRAFT_UX_EVIDENCE");
        if(directory.isEmpty())directory=QDir::tempPath()+"/youngcraft-ux-review/"+variant;
        directory=QDir(directory).absolutePath()+"/scale-"+qEnvironmentVariable("QT_SCALE_FACTOR","1")+"/"+kind;
        QDir().mkpath(directory);
    }
    void begin(const QString &name,const QString &precondition,const QString &goal) {
        id=name;start=precondition;endpoint=goal;steps={};clicks=0;
    }
    bool click(QWidget *widget,const QString &label,QPoint point=QPoint()) {
        if(!widget || !widget->isVisible() || !widget->isEnabled()) {
            QTest::qFail(qPrintable("Unavailable click target: "+label),__FILE__,__LINE__);return false;
        }
        if(point.isNull())point=widget->rect().center();
        if(!widget->visibleRegion().contains(point)) {
            QTest::qFail(qPrintable("Clipped click target: "+label),__FILE__,__LINE__);return false;
        }
        ++clicks;
        steps.append(QJsonObject{{"click",clicks},{"target",label},{"object",widget->objectName()},
                                 {"x",point.x()},{"y",point.y()}});
        QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,point);
        QCoreApplication::processEvents();
        return true;
    }
    bool capture(QWidget *widget,const QString &name) {
        QTest::qWait(100);
        return widget && widget->grab().save(directory+"/"+name+".png");
    }
    void end(bool success) {
        cases.append(QJsonObject{{"id",id},{"start",start},{"endpoint",endpoint},
                                 {"clicks",clicks},{"observed",success},{"steps",steps}});
        write("clicks.json",QJsonObject{{"variant",variant},{"schema",1},{"cases",cases},
              {"definition","Actual QTest left mouse clicks; setup, typing and cleanup excluded. Entry endpoints are not completed scientific workflows."}});
    }
    bool write(const QString &name,const QJsonObject &value) {
        QSaveFile file(directory+"/"+name);
        if(!file.open(QIODevice::WriteOnly))return false;
        if(file.write(QJsonDocument(value).toJson())<0)return false;
        return file.commit();
    }
};

// Find the shortest existing mouse route: direct button, toolbar menu, or menu bar.
// New names are runtime strings only; no new application APIs are referenced.
QList<QAction *> uxMenuPath(QMenu *menu,QAction *target,QSet<QMenu *> seen={}) {
    if(!menu || seen.contains(menu))return {};seen.insert(menu);
    if(menu->actions().contains(target))return {target};
    QList<QAction *> best;
    for(auto *action:menu->actions())if(action->menu()) {
        auto path=uxMenuPath(action->menu(),target,seen);
        if(!path.isEmpty()) {path.prepend(action);if(best.isEmpty() || path.size()<best.size())best=path;}
    }
    return best;
}
void uxScheduleMenu(UxEvidence *e,QMenu *menu,QList<QAction *> path) {
    QTimer::singleShot(80,menu,[e,menu,path] {
        if(path.isEmpty())return;
        auto *action=path.first();auto remaining=path;remaining.removeFirst();
        if(!remaining.isEmpty())uxScheduleMenu(e,action->menu(),remaining);
        e->click(menu,action->objectName().isEmpty()?action->text():action->objectName(),menu->actionGeometry(action).center());
    });
}
bool uxAction(MainWindow &window,UxEvidence &e,QAction *target) {
    if(!target){QTest::qFail("Missing action",__FILE__,__LINE__);return false;}
    QWidget *opener=nullptr;QPoint point;QMenu *menu=nullptr;QList<QAction *> best;
    for(auto *button:window.findChildren<QToolButton *>()) {
        if(!button->isVisible() || !button->isEnabled() || !button->defaultAction())continue;
        if(button->defaultAction()==target && button->popupMode()!=QToolButton::InstantPopup)
            return e.click(button,target->objectName(),QPoint(button->width()/3,button->height()/2));
        auto path=uxMenuPath(button->defaultAction()->menu(),target);
        if(!path.isEmpty() && (best.isEmpty() || path.size()<best.size())) {
            opener=button;point=button->rect().center();menu=button->defaultAction()->menu();best=path;
        }
    }
    for(auto *root:window.menuBar()->actions()) {
        auto path=uxMenuPath(root->menu(),target);
        if(!path.isEmpty() && (best.isEmpty() || path.size()<best.size())) {
            opener=window.menuBar();point=window.menuBar()->actionGeometry(root).center();menu=root->menu();best=path;
        }
    }
    if(!opener){QTest::qFail(qPrintable("No mouse route: "+target->objectName()+target->text()),__FILE__,__LINE__);return false;}
    uxScheduleMenu(&e,menu,best);
    if(!e.click(opener,"open menu: "+menu->title(),point))return false;
    QTest::qWait(350);
    return !QTest::currentTestFailed();
}
QAction *uxNamed(MainWindow &w,const char *name) {return w.findChild<QAction *>(name);}
QAction *uxFragmentAction(MainWindow &w) {
    if(auto *action=uxNamed(w,"InsertFragmentAction"))return action;
    if(auto *menu=w.findChild<QMenu *>("StructureBuildMenu"))
        for(auto *action:menu->actions())if(action->text().startsWith("Fragment"))return action;
    return nullptr;
}
bool uxList(UxEvidence &e,QListWidget *list,int row) {
    if(!list || row<0 || row>=list->count())return false;
    return e.click(list->viewport(),list->objectName()+" row "+QString::number(row),list->visualItemRect(list->item(row)).center());
}
}

void SmokeTest::uxClickMeasurements() {
    if(QGuiApplication::platformName()=="offscreen")QSKIP("Remote Windows/OpenGL desktop required");
    UxSettingsScope isolated;UxEvidence evidence("clicks");
    const auto fixture=FileDispatcher::load("tests/data/mvp.out");QVERIFY(fixture.parseResult.success);
    // Deterministic extra IRC point for linked selection, not a parser accuracy fixture.
    auto loaded=fixture;auto &data=loaded.parseResult.data;
    for(int i=0;i<2;++i){IRCPoint p;p.index=i+1;p.coordinate=i;p.relativeKcal=i;p.frameIndex=i;p.direction="Forward";data.ircPoints.append(p);}
    auto run=[&](const QString &id,const QString &start,const QString &goal,int tab,const std::function<void(MainWindow &)> &body) {
        MainWindow w;w.resize(1400,900);w.show();w.onFileLoaded(loaded);w.m_analysis->setCurrentTab(tab);w.m_animation->stop();
        QTest::qWait(150);evidence.begin(id,start,goal);body(w);
        const bool ok=!QTest::currentTestFailed();
        if(ok && !evidence.capture(&w,id))QTest::qFail("Screenshot save failed",__FILE__,__LINE__);
        evidence.end(ok && !QTest::currentTestFailed());
        return !QTest::currentTestFailed();
    };
    // Modal endpoints are captured then dismissed. Dismissal is outside the measured task.
    auto modal=[&](MainWindow &w,QAction *action,const QString &expected,const QString &choice=QString()) {
        bool reached=false;QTimer watcher;
        QObject::connect(&watcher,&QTimer::timeout,&w,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!dialog)return;
            if(dialog->objectName()=="SaveStructureChoice" && !choice.isEmpty()) {
                evidence.click(dialog->findChild<QPushButton *>(choice),choice);return;
            }
            if((expected=="file-picker" && qobject_cast<QFileDialog *>(dialog)) || dialog->objectName()==expected ||
               (expected=="image-options" && qobject_cast<QFileDialog *>(dialog))) {
                watcher.stop();reached=true;
                if(!evidence.capture(dialog,evidence.id+"-dialog"))QTest::qFail("Modal screenshot save failed",__FILE__,__LINE__);
                dialog->reject();
            }
        });
        watcher.start(70);
        QTimer::singleShot(10000,&watcher,[&]{watcher.stop();if(auto *d=qobject_cast<QDialog *>(QApplication::activeModalWidget()))d->reject();});
        QVERIFY(uxAction(w,evidence,action));QVERIFY2(reached,qPrintable("Endpoint not reached: "+expected));
    };
    if(!run("open-file-picker","Loaded result, main window","File picker displayed (file choice not measured)",0,[&](MainWindow &w){modal(w,uxNamed(w,"OpenStructureFile"),"file-picker");}))return;
    if(!run("save-image-options","Loaded result, main window","Image save picker displayed (file choice and save not measured)",0,[&](MainWindow &w){
        auto *action=uxNamed(w,"SaveImageDirect");if(!action)action=uxNamed(w,"SaveStructureImage");
        modal(w,action,"image-options","SavePictureChoice");}))return;
    if(!run("project-library","Loaded result, main window","Project library displayed",0,[&](MainWindow &w){modal(w,uxNamed(w,"ProjectLibraryAction"),"ProjectLibraryDialog");}))return;
    if(!run("fragment-library","Loaded result, display tools active","Fragment library displayed (insertion not measured)",0,[&](MainWindow &w){modal(w,uxFragmentAction(w),"FragmentBrowser");}))return;
    if(!run("copy-image","Loaded result, main window","PNG on clipboard",0,[&](MainWindow &w){
        QApplication::clipboard()->clear();QVERIFY(uxAction(w,evidence,uxNamed(w,"CopyStructureImage")));
        QVERIFY(QApplication::clipboard()->mimeData()->hasFormat("image/png"));}))return;
    if(!run("frame-select","Geometry panel active, final frame selected","First frame applied to 3D view",0,[&](MainWindow &w){
        QVERIFY(uxList(evidence,w.m_analysis->findChild<QListWidget *>("FrameList"),0));QCOMPARE(w.m_currentFrame,0);
        QCOMPARE(w.m_view->molecule().atoms[0].position,w.m_data.frames[0].coordinates[0]);}))return;
    if(!run("imaginary-select-play","Frequency panel active, no vibration playing","Imaginary mode selected and playing",1,[&](MainWindow &w){
        QVERIFY(uxList(evidence,w.m_analysis->findChild<QListWidget *>("FrequencyList"),0));QCOMPARE(w.m_currentFrequency,0);QVERIFY(w.m_animation->isPlaying());w.m_animation->stop();}))return;
    if(!run("scan-point","Scan panel active","Second scan point applies matching structure",2,[&](MainWindow &w){
        QVERIFY(uxList(evidence,w.m_analysis->findChild<QListWidget *>("ScanList"),1));
        QCOMPARE(w.m_currentFrame,w.m_data.scanPoints[1].frameIndex);}))return;
    if(!run("irc-point","IRC panel active","Second IRC point applies matching structure",3,[&](MainWindow &w){
        QVERIFY(uxList(evidence,w.m_analysis->findChild<QListWidget *>("IrcPointList"),1));QCOMPARE(w.m_currentFrame,1);}))return;
    if(!run("frequency-access","Coordinates panel active","Frequency panel displayed",7,[&](MainWindow &w){
        QVERIFY(uxAction(w,evidence,uxNamed(w,"AnalysisPanelTab1")));QCOMPARE(w.m_analysis->currentTab(),1);}))return;
    if(!run("nto-access","TD-DFT panel active, transition coefficients loaded","NTO analysis dialog displayed",4,[&](MainWindow &w){
        QVERIFY(evidence.click(w.m_analysis->findChild<QPushButton *>("TddftTransitionAnalysis"),"TddftTransitionAnalysis"));
        auto *dialog=w.findChild<QDialog *>("TransitionAnalysisDialog");QVERIFY(dialog && dialog->isVisible());
        QVERIFY(evidence.capture(dialog,"nto-analysis"));dialog->close();}))return;
    if(!run("orbital-access","Orbital result panel active","Orbital manager displayed (HOMO generation separately tested)",6,[&](MainWindow &w){
        QVERIFY(evidence.click(w.m_analysis->findChild<QPushButton *>("OpenOrbitalWindow"),"OpenOrbitalWindow"));
        auto *dialog=w.findChild<QDialog *>("OrbitalManagerWindow");QVERIFY(dialog && dialog->isVisible());
        QVERIFY(evidence.capture(dialog,"orbital-manager"));dialog->close();}))return;
    if(!run("mm-options","Loaded result, main window","MM options displayed (calculation separately tested)",0,[&](MainWindow &w){
        QVERIFY(uxAction(w,evidence,uxNamed(w,"MMOptimizationAction")));auto *panel=w.findChild<QWidget *>("MMOptimizationPanel");QVERIFY(panel && panel->isVisible());}))return;
}

void SmokeTest::uxResponsiveLayouts() {
    if(QGuiApplication::platformName()=="offscreen")QSKIP("Remote Windows/OpenGL desktop required");
    UxSettingsScope isolated;UxEvidence evidence("layout");
    const auto loaded=FileDispatcher::load("tests/data/mvp.out");QVERIFY(loaded.parseResult.success);
    QJsonArray layouts;bool serious=false;
    const qreal scale=qEnvironmentVariable("QT_SCALE_FACTOR","1").toDouble();
    const QList<QSize> sizes{{820,640},{1400,900},{qRound(3840/scale),qRound(2160/scale)}};
    for(int index=0;index<sizes.size();++index) {
        QSettings().remove("window/geometry");QSettings().remove("window/sidebarPanelState");
        MainWindow w;w.show();w.onFileLoaded(loaded);w.m_analysis->setCurrentTab(0);w.resize(sizes[index]);QTest::qWait(200);
        const QString label=QStringList{"small","normal","4k"}[index];
        QJsonArray issues,controls;
        auto inspect=[&](QWidget *root,const QString &page) {
            auto buttons=root->findChildren<QAbstractButton *>();
            for(auto *button:buttons) {
                if(!button->isVisible() || button->window()!=root->window())continue;
                const QRect rect(button->mapTo(root,QPoint(0,0)),button->size());
                QString name=button->objectName();if(name.isEmpty())name=button->accessibleName();if(name.isEmpty())name=button->text();
                controls.append(QJsonObject{{"page",page},{"name",name},{"x",rect.x()},{"y",rect.y()},
                  {"width",rect.width()},{"height",rect.height()},{"enabled",button->isEnabled()},
                  {"tooltip",button->toolTip()},{"icon",!button->icon().isNull()}});
                bool scroll=false;for(auto *p=button->parentWidget();p && p!=root;p=p->parentWidget())if(qobject_cast<QAbstractScrollArea *>(p))scroll=true;
                if(!root->rect().contains(rect) && !scroll)issues.append(QJsonObject{{"type","outside-window"},{"page",page},{"control",name}});
                if(button->visibleRegion().isEmpty() && !scroll)issues.append(QJsonObject{{"type","not-reachable"},{"page",page},{"control",name}});
                for(auto *other:buttons) {
                    if(other==button || !other->isVisible() || other->parentWidget()!=button->parentWidget())continue;
                    if(reinterpret_cast<quintptr>(other)<reinterpret_cast<quintptr>(button))continue;
                    const auto overlap=button->geometry().intersected(other->geometry());
                    if(overlap.width()>2 && overlap.height()>2)issues.append(QJsonObject{{"type","sibling-overlap"},{"page",page},{"control",name},{"other",other->objectName()}});
                }
            }
        };
        inspect(&w,"main");QVERIFY(evidence.capture(&w,label+"-main"));
        auto *toolbar=w.findChild<QToolBar *>("MainToolBar");QVERIFY(toolbar);QVERIFY(evidence.capture(toolbar,label+"-icons"));
        auto *tabs=w.findChild<QTabWidget *>("StructureToolsTabs");QVERIFY(tabs);tabs->setCurrentIndex(1);w.setDrawingMode(true);QTest::qWait(100);
        inspect(&w,"editor");QVERIFY(evidence.capture(&w,label+"-editor"));
        StyleEditorDialog style(w.m_style,&w.m_data.molecule,-1,&w);style.show();style.resize(qMin(720,sizes[index].width()),qMin(520,sizes[index].height()));QTest::qWait(100);
        inspect(&style,"style");QVERIFY(evidence.capture(&style,label+"-style"));style.reject();
        const auto *screen=w.screen();
        layouts.append(QJsonObject{{"size",label},{"requested_width",sizes[index].width()},{"requested_height",sizes[index].height()},
          {"actual_width",w.width()},{"actual_height",w.height()},{"device_pixel_ratio",w.devicePixelRatioF()},
          {"screen_width",screen?screen->geometry().width():0},{"screen_height",screen?screen->geometry().height():0},
          {"viewport_width",w.m_view->width()},{"viewport_height",w.m_view->height()},{"issues",issues},{"controls",controls}});
        // Baseline defects are evidence, not a reason to lose before screenshots.
        serious|=!issues.isEmpty() || w.width()>sizes[index].width()+2 || w.height()>sizes[index].height()+2;
    }
    QVERIFY(evidence.write("layout.json",QJsonObject{{"variant",evidence.variant},{"layouts",layouts},
      {"note","Scroll content may extend beyond its viewport; visible controls and sibling button geometry inspected. Full-window grabs may extend beyond remote screen; screen dimensions recorded."}}));
    if(evidence.variant=="after")QVERIFY2(!serious,"Layout issues detected; inspect layout.json and PNGs");
}
