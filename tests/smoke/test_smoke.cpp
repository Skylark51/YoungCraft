#include <QTextBrowser>
#include <QScreen>
#include "compute/GaussianConnection.h"
#include "analysis/AnalysisEngine.h"
#include <QVector2D>
#include <QGroupBox>
#include "ui/OutputSourceView.h"
#include "settings/AppPattern.h"
#include "ui/SidebarFlowLayout.h"
#include "ui/CenteredIconTabs.h"
#include "ui/StartupSplash.h"
#include "ui/OrbitalStudyWidget.h"
#include "compute/GaussianTools.h"
#include "render/IsosurfaceBuilder.h"
#include "app/MainWindow.h"
#ifdef Q_OS_WIN
#include <qt_windows.h>
#undef near
#undef far
#endif
#include <QTabBar>
#include <QScreen>
#include <thread>
#include <chrono>
#include "animation/AnimationController.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include "ui/ScanResultsDialog.h"
#include "ui/ReactionCaptureDialog.h"
#include "ui/MultiCaptureDialog.h"
#include "ui/MechanismAnimationDialog.h"
#include "ui/ReactionCoordinateWidget.h"
#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProgressDialog>
#include "export/ReactionCapture.h"
#include "io/XYZParser.h"
#include <QDialogButtonBox>
#include <QTableWidget>
#include "ui/XtbDialog.h"
#include <QPointer>
#include "ui/StyleEditorDialog.h"
#include "ui/GeometryMeasureBar.h"
#include "ui/ConstraintEditor.h"
#include "model/GeometryEditor.h"
#include "model/StructureTools.h"
#include "model/LabComplexBuilder.h"
#include "model/LigandReplacement.h"
#include <QDir>
#include <QListWidget>
#include <QSignalBlocker>

#include <QCheckBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSettings>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTreeWidget>
#include <QSplitter>
#include <QToolButton>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QWheelEvent>
#include <QStackedWidget>
#include <QSurfaceFormat>
#include <QImageReader>
#include <QPainter>

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QScopeGuard>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QStandardPaths>
#include <QDialog>
#include <QTimer>
#include <QtTest>
#include "export/ImageExporter.h"

using namespace youngcraft;

static QToolButton *toolButtonForAction(QWidget *parent, QAction *action) {
    for (auto *button : parent->findChildren<QToolButton *>())
        if (button->defaultAction() == action) return button;
    return nullptr;
}

class SmokeTest final : public QObject {
    Q_OBJECT
private slots:
    void unlimitedSelectionAndHoverElements();
    void sidebarOptimizationFixedSelection();
    void bilingualCategorizedHelp();
    void periodicTableGrouping();
    void vibrationVectorLengthControl();
    void vibrationVectorContrast();
    void ligandReplacementWorkflow();
    void orbitalCacheAndDetachedWindow();
    void reactionCoordinatePresentationWorkflow();
    void hydrogenRemovalActions();
    void wholeMechanismAnimationWorkflow();
    void mechanismPowerPointWorkflow();
    void suppliedMechanismPreview();
    void atomFocusAcrossWindowsAndTabs();
    void atomFocusTransparentExport();
    void projectLibraryOpensOnDemand();
    void additionalInteractionRequests();
    void toolbarCleanupAndMolecularInformation();
    void vdwAndIsometricView();
    void drawingBlankRightClickSelects();
    void directGeometryToolsAndFixedCore();
    void selectAllAndFragmentRotation();
    void optimizationGraphWindow();
    void patternTemplatesWorkflow();
    void patternViewportContinuity();
    void professorPhotoTheme();
    void measurementBarPatternContinuity();
    void sidebarPatternContinuity();
    void sidebarIconFlowLayout();
    void frequencyPagingAndDetails();
    void saveChoicesAndFormats();
    void dialogThemeSynchronization();
    void orbitalStudyControlsAndRendering();
    void orbitalResponsiveSidebar();
    void orbitalThemesAndPatterns();
    void realRemoteOrbitalResults();
    void automaticRemoteConnectionAndHomo();
    void pastedXYZWorkflow();
    void reactionCaptureWorkflow();
    void multiCaptureWorkflow();
    void reactionCaptureHydrogenControls();
    void scanAutomaticMeasurement();
    void measurementCalloutsAvoidAtoms();
    void scanDisplayDefaults();
    void scanArrowNavigation();
    void scanOptimizationPoints();
    void calculationConstraintEditor();
    void chargedMMFallbackPreservesConstraints();
    void chargedMMFormalChargesReachWorker();
    void xtbDistortedResultDoesNotAutoApply();
    void calculationDefaultPanels();
    void tddftPublicationSpectrum();
    void outputSourceNavigationAndSearch();
    void contextualTsCopyButton();
    void scanAxesAndApproximateTs();
    void scanAxisTitlesPersist();
    void publicationPlotExports();
    void mechanismStageRangesAndCopy();
    void copiesTsAnimation_data();
    void copiesTsAnimation();
    void movesDocumentsBetweenWindows();
    void nativeTabDragBetweenWindows_data();
    void nativeTabDragBetweenWindows();
    void selectedBondShortcutAndColigandDialog();
    void labLigandBrowserBuildsSelectedMetal();
    void continuousSketchClicksStrokeRingAndUndo();
    void themesEmptySidebarAndScanApproach();
    void atomVisibilityPreservesViewAndScroll_data();
    void atomVisibilityPreservesViewAndScroll();
    void topPanelToolbarFollowsDocuments();
    void scanVisibilityAndFlexibleViewport();
    void rectangularViewportPickingAndCapture();
    void chemcraftSelectionAndClipboard();
    void chemcraftGeneratedDialogsAndExtraction();
    void mainWindowStartsWithKoreanUi();
    void startupSplashShowsAndCloses();
    void loadsGaussianFixtureWithoutBlocking();
    void opensDroppedFilesFromViewportAndAnalysis();
    void moleculeViewKeepsManualBond();
    void rendersShadowAndTransparentCapture();
    void supersamplingSmoothsDiagonalEdges();
    void copiesMoleculeWithTransparentBackground();
    void rendersOutwardFacesAndMolecularDepth();
    void rendersContinuousBondsAndCleanSphereShadows();
    void presetsApplyFromMenuAndEditor();
    void exposesAnalysisPanelRestoreAction();
    void exposesKoreanFrequencyControls();
    void styleEditorAdjustsAtomAndBondSizes();
    void frequencyAnimationEmitsDisplacedCoordinates();
    void frequencySelectionStartsAnimationAndShowsCoordinates();
    void copiesCurrentElementCoordinates();
    void geometryMeasureBarWorks();
    void geometryEditsFromOrdinarySelection();
    void geometryEditPreservesCalculation();
    void geometryEditInputAndInvalidSelection();
    void documentsAreIndependent();
    void matchingAtomViewsSynchronize_data();
    void matchingAtomViewsSynchronize();
    void optimizationGraphSynchronizes();
    void scanWithoutGeometryIsSelectable();
    void realScanGuiInputs();
    void drawingModeCreatesAtomsAndBonds();
    void toolbarDrawKeepsWindowOpen();
    void autoHydrogenFollowsDrawingEditsAndUndo();
    void mmStopTerminatesWorker();
    void mmChunkRegression();
    void mmPauseResumeStopConverge();
    void xtbGeometryActions();
    void xtbCompactColumns();
    void crystalModesSwitchStructures();
    void styleEditorPreviewsImmediately();
    void editorInputTransactionsAndSelection();
    void editorPreservesCalculationFrames();
    void editorFragmentUndoAndKeyboard();
    void editorVisualPreview();
    void atomNumbersToolbarLabelsEveryAtom();
    void atomClicksMeasureDistanceAngleAndDihedral();
};

void SmokeTest::toolbarCleanupAndMolecularInformation() {
    MainWindow w;w.show();auto *toolbar=w.findChild<QToolBar *>("MainToolBar");QVERIFY(toolbar);
    auto *exports=w.findChild<QAction *>("ToolbarExport")->menu();
    QVERIFY(exports->actions().contains(w.findChild<QAction *>("CustomStructureCapture")));
    QVERIFY(exports->actions().contains(w.findChild<QAction *>("CopyTSAnimation")));
    QVERIFY(!toolbar->widgetForAction(w.findChild<QAction *>("CustomStructureCapture")));
    for(auto *top:w.menuBar()->actions())QVERIFY(top->text()!=QStringLiteral("Orbitals"));
    for(auto *action:w.findChildren<QAction *>()) {
        QVERIFY(action->text()!=QStringLiteral("안티앨리어싱 없이 현재 이미지 복사"));
        QVERIFY(action->text()!=QStringLiteral("기본 편집기 옵션"));
        QVERIFY(action->text()!=QStringLiteral("구조 정리"));
    }
    Molecule m;m.atoms={{1,8,"O",{}},{2,1,"H",{1,0,0}},{3,1,"H",{-1,0,0}}};
    m.atoms[2].visible=false;w.newDrawingDocument(m);w.setDrawingMode(false);
    QCOMPARE(AnalysisEngine::molecularFormula(m),QStringLiteral("H2O"));
    Molecule organic;organic.atoms={{1,8,"O",{}},{2,6,"C",{}},{3,1,"H",{}},{4,6,"C",{}},{5,0,"X",{}}};
    QCOMPARE(AnalysisEngine::molecularFormula(organic),QStringLiteral("C2HO"));
    QVERIFY(qAbs(atomicMass(44)-101.07)<1e-6); // Ru was missing from the original mass table.
    QVERIFY(qAbs(atomicMass(46)-106.42)<1e-6);
    QVERIFY(qAbs(atomicMass(92)-238.03)<.01);
    bool checked=false;
    QTimer::singleShot(0,&w,[&] {
        auto *dialog=w.findChild<QDialog *>("MolecularInformationDialog");
        if(!dialog)return;
        auto *formula=dialog->findChild<QLabel *>("MolecularFormula");auto *mass=dialog->findChild<QLabel *>("MolecularMass");
        auto *buttons=dialog->findChild<QDialogButtonBox *>();
        checked=formula&&formula->text()=="H2O"&&mass&&mass->text().contains(QString::number(AnalysisEngine::molecularMass(m),'f',4));
        const auto preview=qEnvironmentVariable("YOUNGCRAFT_CLEANUP_PREVIEW");if(!preview.isEmpty())checked=checked&&dialog->grab().save(preview+"-mass.png");
        if(buttons&&buttons->button(QDialogButtonBox::Ok))buttons->button(QDialogButtonBox::Ok)->click();else dialog->reject();
    });
    w.findChild<QAction *>("MolecularInformationAction")->trigger();QVERIFY(checked);
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_CLEANUP_PREVIEW");if(!preview.isEmpty())QVERIFY(w.grab().save(preview+"-toolbar.png"));
}
void SmokeTest::vdwAndIsometricView() {
    MainWindow w;w.resize(1100,800);w.show();Molecule m;m.atoms={{1,6,"C",{-2,0,0}},{2,8,"O",{2,0,0}}};m.setBond(0,1,1);
    w.newDrawingDocument(m);w.setDrawingMode(false);auto *view=w.m_view;
    auto style=StyleManager::preset(QStringLiteral("Ball-and-Stick"));style.showVdwSpheres=false;w.applyStyle(style);view->bestView();
    QTest::qWait(100);const auto before=view->image();const float original=view->atomVisualRadius(m.atoms[0]);
    auto *vdw=w.findChild<QAction *>("ShowVdwSpheres");QVERIFY(vdw);vdw->trigger();
    QVERIFY(view->style().showVdwSpheres);QVERIFY(view->atomVisualRadius(m.atoms[0])>original);
    QVERIFY(qAbs(view->atomVisualRadius(m.atoms[0])-vanDerWaalsRadius(6))<1e-5);
    QTest::qWait(100);QVERIFY(view->image()!=before);
    const auto fitted=view->viewProjection();for(const auto &atom:m.atoms) {
        const auto center=view->project(atom.position,fitted);
        const auto edge=view->project(atom.position+QVector3D(view->atomVisualRadius(atom),0,0),fitted);
        const float radius=qAbs(edge.x()-center.x());QVERIFY(center.x()-radius>=0);QVERIFY(center.x()+radius<=view->width());
    }
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_CLEANUP_PREVIEW");if(!preview.isEmpty())QVERIFY(w.grab().save(preview+"-vdw.png"));
    const int source=w.m_documentTabs->currentIndex();w.newDrawingDocument(m);style.showVdwSpheres=false;w.applyStyle(style);QVERIFY(!vdw->isChecked());
    w.m_documentTabs->setCurrentIndex(source);QVERIFY(vdw->isChecked());vdw->trigger();
    QVERIFY(!view->style().showVdwSpheres);QVERIFY(qAbs(view->atomVisualRadius(m.atoms[0])-original)<1e-5);QCOMPARE(view->molecule().bonds.size(),1);
    w.findChild<QAction *>("IsometricView")->trigger();QVERIFY(!view->style().perspective);
    const auto vp=view->viewProjection();const auto zero=view->project({},vp);
    QVector<float> lengths;for(const auto &axis:{QVector3D(1,0,0),QVector3D(0,1,0),QVector3D(0,0,1)}) {
        const auto end=view->project(axis,vp);lengths<<QVector2D(end.x()-zero.x(),end.y()-zero.y()).length();
    }
    QVERIFY(qAbs(lengths[0]-lengths[1])<.01f);QVERIFY(qAbs(lengths[1]-lengths[2])<.01f);
}
void SmokeTest::drawingBlankRightClickSelects() {
    MainWindow w;w.resize(1100,800);w.show();Molecule m;m.atoms={{1,6,"C",{}},{2,8,"O",{1.3f,0,0}}};m.setBond(0,1,1);
    w.newDrawingDocument(m);auto *view=w.m_view;view->bestView();QTest::qWait(100);
    auto *arm=w.m_drawBar->findChild<QPushButton *>("ArmDrawingTool");QVERIFY(arm);
    view->setDrawingElement(6);view->setChainDrawing(true);view->setDrawingAnchorAt(m.atoms[0].position);
    QTest::mouseClick(view,Qt::RightButton,Qt::NoModifier,QPoint(10,10));
    QVERIFY(!view->drawingArmed());QVERIFY(!arm->isChecked());QCOMPARE(view->cursor().shape(),Qt::ArrowCursor);QVERIFY(!view->hasGhostBond());
    const auto carbon=view->project(m.atoms[0].position,view->viewProjection());
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(carbon.x()),qRound(carbon.y())));
    QVERIFY(view->selection().atoms.contains(0));QCOMPARE(view->molecule().atoms.size(),2);
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(12,12));QCOMPARE(view->molecule().atoms.size(),2);
    view->setProperty("pendingCenter",26);view->setProperty("pendingFragment","not-a-real-template");view->setTemplatePreview(0);
    QTest::mouseClick(view,Qt::RightButton,Qt::NoModifier,QPoint(10,10));
    QVERIFY(!view->drawingArmed());QVERIFY(!view->m_insertPreview);QCOMPARE(view->property("pendingCenter").toInt(),0);QVERIFY(view->property("pendingFragment").toString().isEmpty());
    auto *quick=w.m_drawBar->findChild<QPushButton *>("QuickElement_C");QVERIFY(quick);quick->click();QVERIFY(view->drawingArmed());
    const auto rotation=view->cameraState().rotation;
    QTest::mousePress(view,Qt::RightButton,Qt::NoModifier,QPoint(10,10));
    QMouseEvent move(QEvent::MouseMove,QPointF(60,50),QPointF(view->mapToGlobal(QPoint(60,50))),Qt::NoButton,Qt::RightButton,Qt::NoModifier);QApplication::sendEvent(view,&move);
    QTest::mouseRelease(view,Qt::RightButton,Qt::NoModifier,QPoint(60,50));
    QVERIFY(view->drawingArmed());QVERIFY(view->cameraState().rotation!=rotation);
}

void SmokeTest::additionalInteractionRequests(){
    MainWindow w;w.resize(1580,960);w.show();w.newDrawingDocument();
    auto *view=w.m_view;QTest::qWait(60);QVERIFY(!view->drawingArmed());
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(100,120));QCOMPARE(view->molecule().atoms.size(),0);
    QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,QPoint(120,140));QTest::mouseMove(view,QPoint(170,180),20);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,QPoint(170,180));QCOMPARE(view->molecule().atoms.size(),0);
    w.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();QVERIFY(view->drawingArmed());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(170,180));QVERIFY(!view->molecule().atoms.isEmpty());
    w.setDrawingMode(false);w.setDrawingMode(true);QVERIFY(!view->drawingArmed());
    auto *quick=w.m_drawBar->findChild<QPushButton *>("QuickElement_C");QVERIFY(quick);quick->click();QVERIFY(view->drawingArmed());
    w.setDrawingMode(false);Molecule m;m.atoms={{1,8,"O",{0,0,0}},{2,1,"H",{1,0,0}},{3,1,"H",{0,1,0}}};m.setBond(0,1,1);m.setBond(0,2,1);w.commitEditedMolecule(m,true,false);view->bestView();
    w.applyAtomSelection({0,1});w.m_geometrySelection={0,1};view->setGeometrySelection({0,1});
    QTest::mouseClick(view,Qt::RightButton,Qt::NoModifier,QPoint(30,30));QVERIFY(view->selection().atoms.isEmpty());QVERIFY(w.m_geometrySelection.isEmpty());QVERIFY(view->geometrySelection().isEmpty());
    w.applyAtomSelection({0,1});const auto camera=view->cameraState();QTest::mousePress(view,Qt::RightButton,Qt::NoModifier,QPoint(30,30));QTest::mouseMove(view,QPoint(80,65),20);QTest::mouseRelease(view,Qt::RightButton,Qt::NoModifier,QPoint(80,65));QCOMPARE(view->selection().atoms.size(),2);QVERIFY(view->cameraState().rotation!=camera.rotation);
    FrequencyMode mode;mode.number=1;mode.wavenumber=1000;mode.displacement={{0.1f,0,0},{-0.1f,0,0},{0,0.1f,0}};w.m_data.frequencies={mode};w.m_analysis->setData(w.m_data);w.showFrequency(0);
    auto *toggle=w.m_workspace->findChild<QPushButton *>("ViewportFrequencyToggle");QVERIFY(toggle);QVERIFY(toggle->isVisible());QVERIFY(w.m_animation->isPlaying());toggle->click();QVERIFY(!w.m_animation->isPlaying());
    const auto paused=view->molecule().atoms[0].position;QTest::qWait(70);QCOMPARE(view->molecule().atoms[0].position,paused);toggle->click();QVERIFY(w.m_animation->isPlaying());QTest::qWait(70);QVERIFY(view->molecule().atoms[0].position!=paused);
    w.m_workspace->findChild<QPushButton *>("ViewportFrequencyRestore")->click();QVERIFY(!w.m_animation->isPlaying());QCOMPARE(view->molecule().atoms[0].position,w.m_data.molecule.atoms[0].position);
    auto *toolbar = w.findChild<QToolBar *>("MainToolBar");
    QVERIFY(toolbar);
    for (const auto *name : {"MMOptimizationAction", "QMOptimizationAction"}) {
        auto *action = w.findChild<QAction *>(name);
        QVERIFY(action && action->isEnabled() && toolbar->widgetForAction(action));
    }
    if(!qEnvironmentVariableIsEmpty("YOUNGCRAFT_EDIT_PREVIEW")){QTest::qWait(100);w.grab().save(qEnvironmentVariable("YOUNGCRAFT_EDIT_PREVIEW"));}
}

void SmokeTest::directGeometryToolsAndFixedCore(){
    MainWindow w;w.resize(1580,960);w.show();
    Molecule m;m.atoms={{1,8,"O",{-2,0,0}},{2,1,"H",{-1,0,0}},{3,8,"O",{2,0,0}},{4,1,"H",{3,0,0}}};m.setBond(0,1,1);m.setBond(2,3,1);
    w.newDrawingDocument(m);w.setDrawingMode(false);w.m_view->bestView();QTest::qWait(100);
    auto *view=w.m_view;
    auto *freeze=w.findChild<QAction *>("FreezeOptimizationCore");QVERIFY(freeze);freeze->trigger();
    for(const auto &a:view->molecule().atoms)QVERIFY(a.optimizationFixed);
    auto edited=view->molecule();edited.atoms<<Atom{5,1,"H",{2,1,0}};w.commitEditedMolecule(edited,true,false);
    for(int i=0;i<4;++i)QVERIFY(view->molecule().atoms[i].optimizationFixed);QVERIFY(!view->molecule().atoms[4].optimizationFixed);
    ConstraintEditor editor(view->molecule(),{2,4});QVector<GeometryConstraint> entries;QString error;QVERIFY2(editor.read(entries,&error),qPrintable(error));QCOMPARE(entries.size(),4);
    w.undoDrawingEdit();QCOMPARE(view->molecule().atoms.size(),4);w.undoDrawingEdit();for(const auto &a:view->molecule().atoms)QVERIFY(!a.optimizationFixed);
    auto point=[&](int i){auto p=view->project(view->molecule().atoms[i].position,view->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    for(int mode:{0,1,2,3,4}){
        auto *tool=w.findChild<QAction *>(QString("GeometryDragTool%1").arg(mode));QVERIFY(tool);tool->trigger();QCOMPARE(view->transformMode(),mode);
        const Molecule before=view->molecule();QPoint start=point(1),end=start+QPoint(45,27);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);QTest::mouseMove(view,end,20);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,end);
        if(mode==0||mode>=3)
            QVERIFY((view->molecule().atoms[1].position-before.atoms[1].position).length()>0.01);
        else QCOMPARE(view->molecule().atoms[1].position,before.atoms[1].position);
        QCOMPARE(view->molecule().atoms[2].position,before.atoms[2].position);
        if(mode>0)QVERIFY(std::abs(GeometryEditor::distance(view->molecule(),0,1)-GeometryEditor::distance(before,0,1))<0.00001);
        w.undoDrawingEdit();
    }
    if(!qEnvironmentVariableIsEmpty("YOUNGCRAFT_EDIT_PREVIEW"))w.grab().save(qEnvironmentVariable("YOUNGCRAFT_EDIT_PREVIEW")+".tools.png");
}
void SmokeTest::selectAllAndFragmentRotation(){
    MainWindow w;w.resize(1500,900);w.show();
    Molecule m;
    m.atoms={{1,6,"C",{0,0,0}},{2,6,"C",{1.54f,0,0}},
             {3,1,"H",{2.0f,1,0}},{4,1,"H",{2.0f,-0.5f,0.9f}},
             {5,1,"H",{2.0f,-0.5f,-0.9f}},{6,8,"O",{6,0,0}}};
    m.setBond(0,1,1);for(int i=2;i<=4;++i)m.setBond(1,i,1);
    w.newDrawingDocument(m);w.setDrawingMode(false);
    MoleculeView *view=w.m_view;view->bestView();QTest::qWait(80);
    auto *selectAll=w.findChild<QAction *>("SelectAllVisibleAtoms");QVERIFY(selectAll);
    QCOMPARE(selectAll->shortcut(),QKeySequence::SelectAll);
    view->setFocus();QTest::keyClick(view,Qt::Key_A,Qt::ControlModifier);
    QTRY_COMPARE(view->selection().atoms.size(),6);
    auto point=[&](int i){auto p=view->project(view->molecule().atoms[i].position,view->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    auto *torsion=w.findChild<QAction *>("GeometryDragTool2");QVERIFY(torsion);torsion->trigger();
    const auto before=view->molecule();
    const QPoint start=point(1),end=start+QPoint(75,0);
    QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);
    QTest::mouseMove(view,end,20);
    QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,end);
    QCOMPARE(view->molecule().atoms[0].position,before.atoms[0].position);
    QCOMPARE(view->molecule().atoms[5].position,before.atoms[5].position);
    QVERIFY((view->molecule().atoms[2].position-before.atoms[2].position).length()>0.05f);
    QCOMPARE(view->selection().atoms,QSet<int>({1,2,3,4}));
}
void SmokeTest::optimizationGraphWindow(){
    AnalysisPanel panel;panel.resize(400,700);panel.show();CalculationData data;
    data.molecule.atoms={{1,1,"H",{0,0,0}}};
    for(int i=0;i<3;++i){Frame f;f.coordinates={{float(i),0,0}};f.metrics.energyHartree=-10-i*0.01;data.frames<<f;}
    panel.setData(data);panel.setCurrentTab(0);
    auto *button=panel.findChild<QPushButton *>("OpenOptimizationGraph");auto *dialog=panel.findChild<QDialog *>("OptimizationResultsDialog");auto *plot=panel.findChild<PlotWidget *>("OptimizationEnergyPlot");
    QVERIFY(button);QVERIFY(dialog);QVERIFY(plot);QVERIFY(button->isEnabled());QVERIFY(!dialog->isVisible());QVERIFY(plot->parentWidget()==dialog);
    button->click();QVERIFY(dialog->isVisible());QSignalSpy selected(&panel,&AnalysisPanel::frameRequested);plot->pointClicked(1);QCOMPARE(panel.currentFrame(),1);QCOMPARE(selected.size(),1);
    dialog->close();QVERIFY(!dialog->isVisible());button->click();QVERIFY(dialog->isVisible());
    panel.setData({});QVERIFY(!button->isEnabled());QVERIFY(!dialog->isVisible());
}

void SmokeTest::scanDisplayDefaults() {
    MainWindow window;
    LoadedFile loaded;
    loaded.parseResult.success = true;
    auto &data = loaded.parseResult.data;
    data.molecule.atoms = {{1,6,"C",{0,0,0}}, {2,1,"H",{1,0,0}},
                           {3,8,"O",{0,2,0}}, {4,1,"H",{1,2,0}}};
    data.molecule.setBond(0,1,1);
    data.molecule.setBond(2,3,1);
    data.scanDetected = true;
    data.scanDimensions = 2;
    ScanPoint point;
    point.index = 1;
    point.coordinate = 1.23456;
    point.coordinate2 = 2.34567;
    point.relativeKcal = 3.45678;
    point.energyHartree = -123.45678;
    data.scanPoints = {point};
    window.onFileLoaded(loaded);
    QVERIFY(window.m_view->style().showHydrogen);
    QVERIFY(!window.m_view->molecule().atoms[1].visible);
    QVERIFY(window.m_view->molecule().atoms[3].visible);
    QAction *reset = window.findChild<QAction *>(QStringLiteral("RestoreCarbonHydrogens"));
    QVERIFY(reset);
    reset->trigger();
    QVERIFY(window.m_view->molecule().atoms[1].visible);
    QCOMPARE(window.m_view->molecule().atoms.size(), 4);
    // Opening a file again starts in the same removal state.
    window.onFileLoaded(loaded);
    QVERIFY(!window.m_view->molecule().atoms[1].visible);
    const auto *list = window.m_analysis->findChild<QListWidget *>("ScanList");
    QVERIFY(list);
    QCOMPARE(list->item(0)->text(), QStringLiteral("점 1  |  1.23 / 2.35  |  ΔE 3.46 kcal/mol"));
    QCOMPARE(list->item(0)->toolTip(), QStringLiteral("E = -123.46 Hartree"));
    QCOMPARE(window.m_data.scanPoints[0].coordinate, point.coordinate);
    QCOMPARE(window.m_data.scanPoints[0].relativeKcal, point.relativeKcal);
    for (const auto &name : {"Distance", "Angle", "Dihedral"}) {
        const auto *box = window.m_geometryBar->findChild<QDoubleSpinBox *>(QString("Geometry%1Value").arg(name));
        QVERIFY(box);
        QCOMPARE(box->decimals(), 2);
    }
    data.scanDimensions = 1;
    window.onFileLoaded(loaded);
    QCOMPARE(list->item(0)->text(), QStringLiteral("점 1  |  좌표 1.23  |  ΔE 3.46 kcal/mol"));
}

void SmokeTest::measurementCalloutsAvoidAtoms() {
    Molecule molecule;
    molecule.atoms = {{1,8,"O",{-0.6f,0,0}}, {2,1,"H",{0.6f,0,0}},
                      {3,6,"C",{-5,-3,0}}, {4,6,"C",{5,3,0}},
                      {5,7,"N",{0,1.1f,0}}};
    molecule.setBond(0,1,1);
    molecule.setBond(0,4,1);
    MoleculeView view;
    view.resize(420,320);
    auto style = StyleManager::preset("Youngcraft Studio");
    style.background = Qt::white;
    view.setStyle(style);
    view.setMolecule(molecule);
    view.setGeometrySelection({0,1});
    view.m_measurementVisible[0] = true;
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    for (const QSize size : {QSize(420,320), QSize(300,260)}) {
        view.resize(size);
        QTest::qWait(30);
        QImage overlay(size, QImage::Format_ARGB32_Premultiplied);
        overlay.fill(Qt::transparent);
        QPainter painter(&overlay);
        const auto vp = view.viewProjection();
        view.drawGeometryMeasurements(vp, &painter);
        painter.end();
        int backgroundPixels = 0, pinkPixels = 0, redPixels = 0;
        for (int y = 0; y < overlay.height(); ++y)
            for (int x = 0; x < overlay.width(); ++x) {
                const auto color = overlay.pixelColor(x,y);
                if (color.alpha() > 100 && color.red() > 200 && color.green() < 120) {
                    if (color.blue() > 130) ++pinkPixels;
                    if (color.blue() < 100) ++redPixels;
                }
                if (color.red() < 250 || color.green() < 250 || color.blue() < 250 || color.alpha() < 190)
                    continue;
                ++backgroundPixels;
                for (const auto &atom : molecule.atoms) {
                    const auto center = view.project(atom.position, vp);
                    const auto edge = view.project(atom.position + view.m_rotation.conjugated().rotatedVector(
                        QVector3D(view.atomVisualRadius(atom),0,0)), vp);
                    const double radius = QLineF(QPointF(center.x(),center.y()), QPointF(edge.x(),edge.y())).length()+4;
                    QVERIFY2(!QRectF(center.x()-radius,center.y()-radius,2*radius,2*radius).contains(QPointF(x,y)),
                             "Measurement callout covers an atom");
                }
            }
        QVERIFY(backgroundPixels > 100);
        QVERIFY(pinkPixels > 3);
        QVERIFY(redPixels > 3);
        const auto capture = qEnvironmentVariable("YOUNGCRAFT_CALLOUT_CAPTURE");
        if (!capture.isEmpty() && size.width() == 420)
            QVERIFY(view.imageAtSize(size, false).save(capture));
    }
}

void SmokeTest::scanAutomaticMeasurement() {
    QFile input("tests/data/mvp.out"); QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray contents = input.readAll();
    contents.replace(" Summary of Optimized Potential Surface Scan",
                     " ! R1 R(1,2) 1.0 Scan !\n Summary of Optimized Potential Surface Scan");
    QTemporaryFile file(QDir::tempPath()+"/youngcraft-measure-scan-XXXXXX.out");
    QVERIFY(file.open()); file.write(contents); file.flush();
    auto loaded = FileDispatcher::load(file.fileName(), {});
    QVERIFY(loaded.parseResult.success);
    QCOMPARE(loaded.parseResult.data.scanAtoms, QVector<int>({0,1}));
    MainWindow w; w.resize(1400,900); w.show();
    w.onFileLoaded(loaded);
    QCOMPARE(w.m_view->geometrySelection(), QVector<int>({0,1}));
    QVERIFY(w.m_view->m_measurementVisible[0]);
    QVERIFY(w.m_measurementReadout->text().contains(QStringLiteral("Å")));
    QVERIFY(!w.m_analysis->findChild<PlotWidget *>("ScanPlot"));
    QVERIFY(!w.findChild<QWidget *>("ScanWorkspace"));
    w.showScanPoint(1);
    QCOMPARE(w.m_view->geometrySelection(), QVector<int>({0,1}));
    const double distance = GeometryEditor::distance(w.m_view->molecule(),0,1);
    QVERIFY(w.m_measurementReadout->text().contains(QString::number(distance,'f',2)));
    const auto capture = qEnvironmentVariable("YOUNGCRAFT_SCAN_MEASURE_CAPTURE");
    QTest::qWait(150);
    const QImage measuredScreen = w.m_view->grab().toImage();
    const QImage measured = w.m_view->imageAtSize(QSize(600,600), false);
    QVERIFY(!measured.isNull());
    if (!capture.isEmpty()) { QVERIFY(w.grab().save(capture)); QVERIFY(measured.save(capture+".molecule.png")); }
    auto *clear = w.m_workspace->findChild<QPushButton *>("ClearAtomMeasurement");
    QVERIFY(clear); clear->click();
    QVERIFY(w.m_view->geometrySelection().isEmpty());
    const QImage cleared = w.m_view->imageAtSize(QSize(600,600), false);
    QVERIFY(measured != cleared);
    QTest::qWait(30);
    const QImage clearedScreen = w.m_view->grab().toImage();
    auto selectionPixels = [](const QImage &image) {
        int count = 0;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x) {
                const QColor c = image.pixelColor(x,y);
                if (c.red()>180 && c.green()>100 && c.green()<210 && c.blue()<100) ++count;
            }
        return count;
    };
    QVERIFY(selectionPixels(measuredScreen) > selectionPixels(clearedScreen) + 50);
    w.showScanPoint(0);
    QVERIFY(w.m_view->geometrySelection().isEmpty());
    // Reopening initializes the scan again; malformed atom references are ignored.
    w.onFileLoaded(loaded);
    QCOMPARE(w.m_view->geometrySelection(), QVector<int>({0,1}));
    loaded.parseResult.data.scanAtoms = {0,999999};
    w.onFileLoaded(loaded);
    QVERIFY(w.m_view->geometrySelection().isEmpty());
    QVERIFY(!w.m_view->m_measurementVisible[0]);
}

void SmokeTest::xtbDistortedResultDoesNotAutoApply() {
    const QByteArray previousPath=qgetenv("PATH");
    qputenv("PATH",QDir::currentPath().toUtf8()+QDir::listSeparator().toLatin1()+previousPath);
    const auto restorePath=qScopeGuard([&]{qputenv("PATH",previousPath);qunsetenv("YOUNGCRAFT_FAKE_XTB_MODE");});
    QTemporaryDir runtime;QVERIFY(runtime.isValid());
    QDir app(QCoreApplication::applicationDirPath());
    QVERIFY(QFile::copy(app.filePath("mock_xtb.exe"),runtime.filePath("xtb.exe")));
    QVERIFY(QFile::copy(app.filePath("Qt6Core.dll"),runtime.filePath("Qt6Core.dll")));
    Molecule m;m.atoms={{1,8,"O",{0,0,0}},{2,1,"H",{1,0,0}},{3,1,"H",{0,1,0}}};
    m.setBond(0,1,1);m.setBond(0,2,1);
    qputenv("YOUNGCRAFT_FAKE_XTB_MODE","distorted");
    auto *dialog=new XtbDialog(m,runtime.path(),nullptr,2,true);
    const auto closeDialog=qScopeGuard([&]{dialog->close();});
    QSignalSpy applied(dialog,&XtbDialog::applyGeometry),restored(dialog,&XtbDialog::restorePreview);
    dialog->show();
    QTRY_VERIFY_WITH_TIMEOUT(dialog->findChild<QPushButton *>("XtbApply")->isEnabled(),5000);
    QCOMPARE(applied.size(),0);QVERIFY(restored.size()>=2);
    QVERIFY(dialog->findChild<QLabel *>("XtbStatus")->text().contains(QStringLiteral("원본 구조를 유지")));
    qunsetenv("YOUNGCRAFT_FAKE_XTB_MODE");
}

void SmokeTest::chargedMMFormalChargesReachWorker() {
    const auto worker=qEnvironmentVariable("YOUNGCRAFT_CHARGED_MM_WORKER");
    QVERIFY(!worker.isEmpty());qputenv("YOUNGCRAFT_MM_TEST_WORKER",worker.toUtf8());
    MainWindow w;w.show();Molecule m;
    m.atoms={{1,7,"N",{0,0,0}},{2,1,"H",{1,0,0}},{3,1,"H",{-0.3f,1,0}},{4,1,"H",{-0.3f,-0.4f,0.9f}},{5,1,"H",{-0.3f,-0.4f,-0.9f}}};
    m.atoms[0].formalCharge=1;m.charge=1;
    for(int i=1;i<5;++i)m.setBond(0,i,1);
    w.newDrawingDocument(m);w.setDrawingMode(false);
    QTimer::singleShot(10,&w,[&]{
        auto *options=w.findChild<QWidget *>("MMOptimizationPanel");QVERIFY(options);
        options->findChild<QPushButton *>("MMStart")->click();
    });
    w.startMMOptimization();
    QTRY_VERIFY_WITH_TIMEOUT(w.m_mmState==MainWindow::MMState::Finished || w.m_mmState==MainWindow::MMState::Failed,10000);
    QCOMPARE(w.m_mmState,MainWindow::MMState::Finished);
    QCOMPARE(w.m_mmField,QString("MMFF94"));QVERIFY(w.m_mmStep>0);
    QVERIFY(!w.findChild<XtbDialog *>());
    QCOMPARE(w.m_data.molecule.charge,1);QCOMPARE(w.m_data.molecule.atoms[0].formalCharge,1);
    qunsetenv("YOUNGCRAFT_MM_TEST_WORKER");
}

void SmokeTest::chargedMMFallbackPreservesConstraints() {
    const auto worker=qEnvironmentVariable("YOUNGCRAFT_CHARGED_MM_WORKER");
    QVERIFY(!worker.isEmpty());qputenv("YOUNGCRAFT_MM_TEST_WORKER",worker.toUtf8());
    qputenv("YOUNGCRAFT_XTB_TEST_RUNTIME",QDir::current().filePath("xtb_runtime").toUtf8());
    MainWindow w;w.show();Molecule m;
    m.atoms={{1,8,"O",{0,0,0}},{2,1,"H",{1.1f,0,0}},{3,1,"H",{-0.3f,1,0}},{4,1,"H",{-0.3f,-0.4f,0.9f}}};
    m.charge=1;m.setBond(0,1,1);m.setBond(0,2,1);m.setBond(0,3,1);
    w.newDrawingDocument(m);w.setDrawingMode(false);
    const QVector<GeometryConstraint> entries={{"fixed",{0},0},{"distance",{0,1},1.1},{"angle",{1,0,2},105},{"dihedral",{1,0,2,3},-80}};
    QTimer::singleShot(10,&w,[&]{
        auto *options=w.findChild<QWidget *>("MMOptimizationPanel");QVERIFY(options);
        auto *editor=options->findChild<QWidget *>("ConstraintEditor");QVERIFY(editor);
        static_cast<ConstraintEditor *>(editor)->setEntries(entries);
        options->findChild<QPushButton *>("MMStart")->click();
    });
    w.startMMOptimization();
    QTRY_VERIFY_WITH_TIMEOUT(w.findChild<XtbDialog *>(),10000);
    auto *dialog=w.findChild<XtbDialog *>();
    QCOMPARE(dialog->findChild<QComboBox *>("XtbMethod")->currentData().toInt(),0);
    QCOMPARE(dialog->findChild<QSpinBox *>("XtbCharge")->value(),1);
    QVector<GeometryConstraint> actual;QString error;
    QVERIFY(static_cast<ConstraintEditor *>(dialog->findChild<QWidget *>("ConstraintEditor"))->read(actual,&error));
    QCOMPARE(actual.size(),entries.size());
    for(int i=0;i<entries.size();++i){QCOMPARE(actual[i].type,entries[i].type);QCOMPARE(actual[i].atoms,entries[i].atoms);QCOMPARE(actual[i].target,entries[i].target);}
    QVERIFY(!dialog->findChild<QCheckBox *>("XtbPreserveFramework")->isChecked());
    QTRY_VERIFY_WITH_TIMEOUT(dialog->findChild<QPushButton *>("XtbApply")->isEnabled(),30000);
    QVERIFY2(dialog->findChild<QLabel *>("XtbStatus")->text().contains(QStringLiteral("최적화 수렴")),qPrintable(dialog->findChild<QLabel *>("XtbStatus")->text()));
    QCOMPARE(w.m_view->molecule().charge,1);
    QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),0,1)-1.1)<0.05);
    dialog->close();qunsetenv("YOUNGCRAFT_MM_TEST_WORKER");qunsetenv("YOUNGCRAFT_XTB_TEST_RUNTIME");
}

void SmokeTest::calculationConstraintEditor() {
    Molecule m; m.atoms={{1,6,"C",{0,1,0}},{2,6,"C",{0,0,0}},{3,6,"C",{1,0,0}},{4,6,"C",{1,0,1}}};
    for(int n:{2,3,4}) {
        QVector<int> ids;for(int i=0;i<n;++i)ids<<i;
        ConstraintEditor editor(m,ids); editor.show();
        editor.findChild<QPushButton *>("AddConstraint")->click();
        QVector<GeometryConstraint> entries;QString error;
        QVERIFY2(editor.read(entries,&error),qPrintable(error)); QCOMPARE(entries.size(),1);
        QCOMPARE(entries[0].atoms,ids); QCOMPARE(entries[0].target,n==2?1.0:90.0);
        editor.findChild<QPushButton *>("AddConstraint")->click();
        QVERIFY(!editor.read(entries,&error));
        editor.findChild<QPushButton *>("RemoveConstraint")->click();
        QVERIFY(editor.read(entries,&error));
    }
}

void SmokeTest::scanOptimizationPoints() {
    MainWindow window; window.resize(1400, 1000); window.show();
    auto loaded = FileDispatcher::load("tests/data/mvp.out", {});
    QVERIFY(loaded.parseResult.success);
    auto &data = loaded.parseResult.data;
    data.scanPoints[0].optimizationFrames = {0, 1};
    data.scanPoints[0].frameIndex = 1;
    data.scanPoints[1].optimizationFrames = {1};
    data.scanPoints[1].inProgress = true;
    window.onFileLoaded(loaded);
    QVERIFY(!window.findChild<QWidget *>("ScanWorkspace"));
    window.m_analysis->findChild<QPushButton *>("OpenScanGraph")->click();
    auto *dialog=window.m_analysis->findChild<ScanResultsDialog *>(); QVERIFY(dialog);
    auto *plot = dialog->findChild<PlotWidget *>("ScanOptimizationEnergyPlot");
    auto *status = dialog->findChild<QLabel *>("ScanOptimizationStatus");
    auto *scanList = window.m_analysis->findChild<QListWidget *>("ScanList");
    QVERIFY(plot); QVERIFY(plot->isVisible());
    QCOMPARE(plot->series().size(), 2);
    QCOMPARE(plot->selectedPoint(), 1);
    QVERIFY(QMetaObject::invokeMethod(plot, "pointClicked", Qt::DirectConnection, Q_ARG(int, 0)));
    QCOMPARE(window.m_currentFrame, 0);
    QCOMPARE(window.m_view->molecule().atoms[0].position, data.frames[0].coordinates[0]);
    scanList->setCurrentRow(1);
    QCOMPARE(plot->series().size(), 1);
    QCOMPARE(window.m_currentFrame, 1);
    QVERIFY(status->text().contains(QStringLiteral("진행 중")));
    QVERIFY(scanList->item(1)->text().contains(QStringLiteral("진행 중")));
    QVERIFY(plot->mapToGlobal(QPoint()).y() > dialog->findChild<PlotWidget *>("ScanResultsPlot")->mapToGlobal(QPoint()).y());
    window.showScanPoint(0);
    QCOMPARE(plot->series().size(), 2);
    QCOMPARE(plot->selectedPoint(), 1);
    if (const auto preview = qEnvironmentVariable("YOUNGCRAFT_SCAN_HISTORY_PREVIEW"); !preview.isEmpty()) {
        QTest::qWait(200);
        QVERIFY(window.grab().save(preview));
        QVERIFY(dialog->grab().save(preview+".graph.png"));
    }
    data.scanDimensions=2; data.scanAxisLabel2="Coordinate 2";
    data.frames[0].metrics.energyHartree=kNaN;
    window.onFileLoaded(loaded); window.showScanPoint(0);
    QCOMPARE(plot->series().size(),1);
    auto *history=dialog->findChild<QListWidget *>("ScanOptimizationFrames");
    QCOMPARE(history->count(),2); QVERIFY(history->item(0)->text().contains(QStringLiteral("SCF 이전")));
    history->setCurrentRow(0); QCOMPARE(window.m_currentFrame,0);
    data.scanDetected = false; data.scanPoints.clear();
    window.onFileLoaded(loaded);
    QVERIFY(!window.findChild<QWidget *>("ScanWorkspace"));
}

void SmokeTest::scanArrowNavigation() {
    MainWindow window; window.resize(1400,900); window.show();
    auto loaded = FileDispatcher::load("tests/data/mvp.out", {});
    QVERIFY(loaded.parseResult.success);
    QVERIFY(!loaded.parseResult.data.scanPoints.isEmpty());
    // Extend the two-point fixture with a distinct third geometry for bidirectional navigation.
    auto &data=loaded.parseResult.data;
    Frame frame=data.frames.last();
    frame.coordinates[0].setX(frame.coordinates[0].x()+0.1f);
    ScanPoint point=data.scanPoints.last();
    point.index=3; point.coordinate+=0.1; point.frameIndex=data.frames.size();
    data.frames.push_back(frame); data.scanPoints.push_back(point);
    window.onFileLoaded(loaded);
    QCOMPARE(window.m_analysis->currentTab(), 2);
    auto *list = window.m_analysis->findChild<QListWidget *>("ScanList");
    QVERIFY(list); QVERIFY(list->count() >= 3);
    QCOMPARE(list->currentRow(),0);
    // Main viewport and sidebar list both follow scan points, rather than opt frames.
    QTest::keyClick(window.m_view, Qt::Key_Right);
    QCOMPARE(list->currentRow(),1);
    QCOMPARE(window.m_currentFrame,window.m_data.scanPoints[1].frameIndex);
    QTest::keyClick(list, Qt::Key_Right);
    QCOMPARE(list->currentRow(),2);
    QTest::keyClick(list, Qt::Key_Left);
    QCOMPARE(list->currentRow(),1);
    auto *open = window.m_analysis->findChild<QPushButton *>("OpenScanGraph");
    QVERIFY(open); open->click();
    auto *dialog = window.m_analysis->findChild<ScanResultsDialog *>(); QVERIFY(dialog);
    auto *table = dialog->findChild<QTableWidget *>("ScanResultsTable");
    auto *plot = dialog->findChild<PlotWidget *>("ScanResultsPlot");
    QSignalSpy selected(dialog, &ScanResultsDialog::pointSelected);
    QTest::keyClick(table,Qt::Key_Right);
    QCOMPARE(table->currentRow(),2); QCOMPARE(list->currentRow(),2);
    QCOMPARE(window.m_currentFrame,window.m_data.scanPoints[2].frameIndex);
    QTest::keyClick(plot,Qt::Key_Left);
    QCOMPARE(table->currentRow(),1); QCOMPARE(list->currentRow(),1);
    dialog->setSelectedPoint(0); selected.clear();
    QTest::keyClick(table,Qt::Key_Left);
    QCOMPARE(table->currentRow(),0); QCOMPARE(selected.count(),0);
    dialog->setSelectedPoint(table->rowCount()-1);
    QTest::keyClick(dialog,Qt::Key_Right);
    QCOMPARE(table->currentRow(),table->rowCount()-1); QCOMPARE(selected.count(),0);
    QTest::keyClick(dialog,Qt::Key_Left); QCOMPARE(selected.count(),1);
    // Numeric controls retain their own arrow-key behavior.
    auto *threshold = dialog->findChild<QDoubleSpinBox *>(); QVERIFY(threshold);
    const int before = table->currentRow();
    QTest::keyClick(threshold,Qt::Key_Left); QCOMPARE(table->currentRow(),before);
    dialog->close();
}

void SmokeTest::outputSourceNavigationAndSearch(){
    MainWindow window;window.resize(1500,1000);window.show();
    auto loaded=FileDispatcher::load("tests/data/mvp.out",{});QVERIFY(loaded.parseResult.success);
    window.openFile(QFileInfo("tests/data/mvp.out").absoluteFilePath());
    QTRY_VERIFY_WITH_TIMEOUT(!window.m_data.excitedStates.isEmpty(),10000);window.m_explorer->hide();
    auto *toggle=window.findChild<QAction*>("ToggleOutputSource");QVERIFY(toggle);
    toggle->trigger();QVERIFY(window.m_outputDock->isVisible());
    auto *source=window.findChild<QPlainTextEdit*>("OutputSourceText");QVERIFY(source);
    QTRY_VERIFY_WITH_TIMEOUT(source->toPlainText().contains("Excited State"),10000);
    window.updateOutputSource("excited_states",0,true);
    QVERIFY(source->textCursor().selectedText().contains("Excited State"));
    QFile raw("tests/data/mvp.out");QVERIFY(raw.open(QIODevice::ReadOnly));const auto bytes=raw.readAll();
    const auto offset=loaded.parseResult.data.index.sectionOffsets.value("excited_states").first();
    QCOMPARE(source->textCursor().blockNumber(),int(bytes.left(offset).count('\n')));
    window.updateOutputSource("frequency_modes",1,true);QVERIFY(source->textCursor().selectedText().contains("Frequencies --"));
    window.updateOutputSource("frame_geometry",0,true);QVERIFY(source->textCursor().selectedText().contains("orientation:"));
    auto *query=window.findChild<QLineEdit*>("OutputFind");QVERIFY(query);query->setText("SCF Done");
    QTest::keyClick(query,Qt::Key_Return);QVERIFY(source->textCursor().selectedText().contains("SCF Done"));
    const int first=source->textCursor().position();QTest::keyClick(query,Qt::Key_Return);QVERIFY(source->textCursor().position()!=first);
    QTest::keyClick(&window,Qt::Key_F,Qt::ControlModifier);QTRY_VERIFY(query->hasFocus());
    const auto path=qEnvironmentVariable("YOUNGCRAFT_REVIEW_DIR");if(!path.isEmpty()){QDir().mkpath(path);QVERIFY(window.grab().save(path+"/output-source.png"));}
    window.newDrawingDocument(loaded.parseResult.data.molecule);QTRY_VERIFY(source->toPlainText().isEmpty());
    window.m_documentTabs->setCurrentIndex(0);QTRY_VERIFY_WITH_TIMEOUT(source->toPlainText().contains("Excited State"),10000);
}

void SmokeTest::tddftPublicationSpectrum() {
    MainWindow window;window.resize(1450,960);window.show();
    auto loaded=FileDispatcher::load("tests/data/mvp.out",{});QVERIFY(loaded.parseResult.success);
    auto &data=loaded.parseResult.data;
    data.route="# td b3lyp/6-31g(d)";data.scanDetected=false;data.scanPoints.clear();data.ircPoints.clear();data.frequencies.clear();
    QVERIFY(!data.excitedStates.isEmpty());
    const auto base=data.excitedStates.first();data.excitedStates.clear();
    for(int i=0;i<4;++i){auto state=base;state.state=i+1;state.wavelengthNm=285+65*i;state.energyEv=1239.84/state.wavelengthNm;state.oscillatorStrength=i==1?0.65:0.12+0.08*i;data.excitedStates.push_back(state);}
    window.onFileLoaded(loaded);
    auto *plot=window.m_tddftWorkspacePlot;QVERIFY(plot);
    QCOMPARE(window.m_workspace->currentWidget(),window.m_tddftWorkspace);
    QCOMPARE(plot->axisRange().yMin,0.0);QVERIFY(plot->series().size()>1000);
    QVERIFY(plot->axisRange().xMin>=0);QCOMPARE(plot->styleOptions().background.alpha(),0);
    QCOMPARE(plot->styleOptions().lineColor,QColor("#d62027"));QVERIFY(plot->styleOptions().thickness>=2.5);
    QVERIFY(plot->styleOptions().axisThickness>=1.8);QCOMPARE(plot->styleOptions().axisFont.family(),QString("Times New Roman"));QVERIFY(plot->styleOptions().axisFont.bold());
    QTimer::singleShot(0,[&]{auto *dialog=window.findChild<QDialog*>("PlotAxisSettings");QVERIFY(dialog);
        dialog->findChild<QCheckBox*>("PlotAutoRange")->setChecked(false);
        dialog->findChild<QDoubleSpinBox*>("PlotXMin")->setValue(200);dialog->findChild<QDoubleSpinBox*>("PlotXMax")->setValue(700);
        dialog->findChild<QDoubleSpinBox*>("PlotYMin")->setValue(0);dialog->findChild<QDoubleSpinBox*>("PlotYMax")->setValue(1.4);
        dialog->findChild<QDoubleSpinBox*>("PlotXStep")->setValue(100);dialog->findChild<QDoubleSpinBox*>("PlotYStep")->setValue(.2);
        dialog->accept();
    });
    window.findChild<QPushButton*>("TddftAxisSettings")->click();
    QCOMPARE(plot->axisRange().xMin,200.0);QCOMPARE(plot->axisRange().xMax,700.0);QCOMPARE(plot->axisRange().yMax,1.4);
    const auto ticks=plot->axisTicks(true);QVERIFY(ticks.size()>2);QCOMPARE(ticks[1]-ticks[0],100.0);
    const auto yticks=plot->axisTicks(false);QVERIFY(yticks.size()>2);QVERIFY(qAbs(yticks[1]-yticks[0]-.2)<1e-10);
    window.findChild<QPushButton*>("TddftAutoRange")->click();QVERIFY(plot->axisRange().xMax!=700);

    double maximum=0;for(const auto &point:plot->series()){QVERIFY(std::isfinite(point.y()));maximum=std::max(maximum,point.y());}
    QVERIFY(qAbs(maximum-1)<1e-8);
    auto *normalize=window.m_analysis->findChild<QCheckBox *>("TddftNormalize");QVERIFY(normalize);
    normalize->setChecked(false);QCOMPARE(plot->styleOptions().yTitle,QString("Calculated absorption (a.u.)"));
    normalize->setChecked(true);
    auto *shape=window.m_analysis->findChild<QComboBox *>("TddftBroadeningShape");QVERIFY(shape);
    shape->setCurrentIndex(2);QVERIFY(plot->series().isEmpty());QCOMPARE(plot->axisRange().yMin,0.0);
    shape->setCurrentIndex(0);
    QTemporaryDir directory;QVERIFY(directory.isValid());
    PlotWidget::ExportOptions options;
    const auto image=plot->publicationImage(options);QVERIFY(!image.isNull());
    QCOMPARE(image.width(),qRound(options.widthMm/25.4*600));QCOMPARE(image.pixelColor(0,0).alpha(),0);
    QString error;QVERIFY2(plot->savePublication(directory.filePath("spectrum.pdf"),options,&error),qPrintable(error));
    QFile pdf(directory.filePath("spectrum.pdf"));QVERIFY(pdf.open(QIODevice::ReadOnly));QVERIFY(pdf.read(5)=="%PDF-");
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_REVIEW_DIR");
    if(!capture.isEmpty()){QDir().mkpath(capture);QTest::qWait(150);QVERIFY(window.grab().save(capture+"/tddft-workspace.png"));QVERIFY(image.save(capture+"/spectrum-600dpi.png"));QVERIFY(plot->savePublication(capture+"/spectrum.pdf",options,&error));}
}

void SmokeTest::calculationDefaultPanels() {
    MainWindow window; window.show();
    auto loaded = FileDispatcher::load("tests/data/mvp.out", {});
    QVERIFY(loaded.parseResult.success);
    auto &data = loaded.parseResult.data;
    data.scanDetected=false; data.scanPoints.clear(); data.ircPoints.clear();
    data.frequencies.clear(); data.excitedStates.clear();
    data.route="# opt b3lyp/6-31g(d)";
    window.m_explorerDock->hide(); window.onFileLoaded(loaded);
    QCOMPARE(window.m_analysis->currentTab(),0); QVERIFY(!window.m_explorerDock->isVisible());
    data.route="# opt freq b3lyp/6-31g(d)";
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),1);
    data.route="# opt IRC=(calcfc,forward) freq";
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),3);
    data.route="# opt freq scan";
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),2);
    data.route="# tda=(nstates=10)";
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),4);
    QCOMPARE(window.m_workspace->currentWidget(),window.m_tddftWorkspace);
    // Actual results select the panel even if the route is missing.
    data.route.clear(); data.frequencies.push_back(FrequencyMode{});
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),1);
    data.ircPoints.push_back(IRCPoint{});
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),3);
    data.scanDetected=true;
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),2);
    data.scanDetected=false; data.ircPoints.clear(); data.frequencies.clear();
    data.route="# hf/sto-3g title=frequency";
    window.onFileLoaded(loaded); QCOMPARE(window.m_analysis->currentTab(),0);
    auto *toolbar = window.findChild<QToolBar *>("MainToolBar"); QVERIFY(toolbar);
    QStringList buttons;
    for (auto *action : toolbar->actions()) if (!action->isSeparator()) buttons << action->iconText();
    QCOMPARE(buttons, QStringList({QStringLiteral("새 구조"), QStringLiteral("열기"),
        QStringLiteral("MM 최적화"), QStringLiteral("xTB"), QStringLiteral("이미지 복사"),
        QStringLiteral("맞춤 캡처"), QStringLiteral("내보내기"), QStringLiteral("결과 패널"),
        QStringLiteral("구조 패널"), QStringLiteral("원문 보기")}));
    auto *xyz = window.findChild<QAction *>("AnalysisPanelTab7");
    QVERIFY(xyz && toolButtonForAction(window.m_explorerDock, xyz));
}

void SmokeTest::contextualTsCopyButton() {
    AnalysisPanel panel; panel.show(); panel.setCurrentTab(1);
    auto *copy=panel.findChild<QPushButton *>("CopyFrequencyAnimation"); QVERIFY(copy);
    QVERIFY(!copy->isVisible());
    CalculationData data; FrequencyMode mode; mode.wavenumber=100;
    data.frequencies={mode}; panel.setData(data); QVERIFY(!copy->isVisible());
    mode.imaginary=true; mode.wavenumber=-200;
    data.frequencies={mode}; panel.setData(data);
    auto *details=panel.findChild<QPushButton*>("OpenFrequencyDetails");QVERIFY(details);details->click();QVERIFY(copy->isVisible());
    QSignalSpy requested(&panel,&AnalysisPanel::animationCopyRequested);
    copy->click(); QCOMPARE(requested.count(),1);
    panel.findChild<QDialog*>("FrequencyDetailsDialog")->hide();
    panel.setCurrentTab(0); QVERIFY(!copy->isVisible());
    panel.setCurrentTab(1); details->click();QVERIFY(copy->isVisible());
    panel.setData(CalculationData{}); QVERIFY(!copy->isVisible());
}

void SmokeTest::scanAxesAndApproximateTs() {
    PlotWidget plot;
    plot.resize(680, 360);
    plot.setScanSeries({{1,0},{2,4},{3,1},{4,8},{5,2}}, "Distance", "Energy");
    QCOMPARE(plot.approximateTsPoint(), 3);
    QCOMPARE(plot.styleOptions().xTitle, QString("Distance"));
    QVERIFY(plot.setManualRange({0,6,-2,10}));
    plot.setSelectedPoint(3);
    const QImage image = plot.image();
    int redPixels = 0;
    for (int y = 0; y < image.height(); ++y) for (int x = 0; x < image.width(); ++x) {
        const QColor c = image.pixelColor(x,y);
        if (c.red() > 160 && c.red() < 220 && c.green() < 70 && c.blue() < 80) ++redPixels;
    }
    QVERIFY(redPixels > 50);
    const QString capture = qEnvironmentVariable("YOUNGCRAFT_SCAN_CAPTURE");
    if (!capture.isEmpty()) QVERIFY(image.save(capture));
    plot.setScanSeries({{3,0},{2,4},{1,0}}, "Distance", "Energy");
    QCOMPARE(plot.approximateTsPoint(), 1);
    plot.setScanSeries({{1,0},{2,1},{3,2}}, "Distance", "Energy");
    QCOMPARE(plot.approximateTsPoint(), -1);
    plot.setScanSeries({{1,2},{2,2},{3,2}}, "Distance", "Energy");
    QCOMPARE(plot.approximateTsPoint(), -1);
    plot.setSeries({{1,0},{2,4},{3,0}});
    QCOMPARE(plot.approximateTsPoint(), -1);
    CalculationData data;
    for (int i=0; i<5; ++i) {
        ScanPoint point; point.coordinate=i+1; point.relativeKcal=i==2?8:0;
        data.scanPoints.push_back(point);
    }
    ScanResultsDialog dialog; dialog.setData(data);
    auto *results=dialog.findChild<PlotWidget *>("ScanResultsPlot"); QVERIFY(results);
    QCOMPARE(results->approximateTsPoint(), 2);
    data.scanDimensions=2; dialog.setData(data);
    QCOMPARE(results->approximateTsPoint(), -1);
}

void SmokeTest::scanAxisTitlesPersist() {
    const auto org=QCoreApplication::organizationName(), app=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");
    QCoreApplication::setApplicationName("ScanAxisTitlesPersistence");
    QSettings().clear();
    auto cleanup=qScopeGuard([&] { QSettings().clear(); QCoreApplication::setOrganizationName(org); QCoreApplication::setApplicationName(app); });
    CalculationData data; data.scanAxisLabel="Distance (Å)";
    for (int i=0;i<5;++i) { ScanPoint p; p.coordinate=i+1; p.relativeKcal=i==2?12:0; data.scanPoints.push_back(p); }
    ScanResultsDialog dialog; dialog.setData(data);
    auto *plot=dialog.findChild<PlotWidget *>("ScanResultsPlot"); QVERIFY(plot);
    QVERIFY(!plot->styleOptions().showMechanism);
    QCOMPARE(plot->styleOptions().lineColor,QColor(Qt::black));
    QCOMPARE(plot->styleOptions().borderColor,QColor(Qt::black));
    QCOMPARE(plot->styleOptions().lineStyle,Qt::DashLine);
    QVERIFY(plot->styleOptions().thickness>1.6);
    QVERIFY(plot->styleOptions().axisFont.pointSize()>10);
    QVERIFY(plot->styleOptions().tickFont.pointSize()>9);
    auto edit=[&](ScanResultsDialog &target, const QString &x, const QString &y, bool save) {
        QTimer::singleShot(0,&target,[&target,x,y,save] {
            auto *editor=target.findChild<QDialog *>("ScanAxisTitlesDialog"); QVERIFY(editor);
            editor->findChild<QLineEdit *>("ScanXAxisTitle")->setText(x);
            editor->findChild<QLineEdit *>("ScanYAxisTitle")->setText(y);
            editor->findChild<QDialogButtonBox *>()->button(save?QDialogButtonBox::Save:QDialogButtonBox::Cancel)->click();
        });
        target.findChild<QPushButton *>("EditScanAxisTitles")->click();
    };
    edit(dialog,"Reaction coordinate (Å)","Relative energy (kcal/mol)",true);
    dialog.setData(data);
    QCOMPARE(plot->styleOptions().xTitle,QString("Reaction coordinate (Å)"));
    QCOMPARE(plot->styleOptions().yTitle,QString("Relative energy (kcal/mol)"));
    ScanResultsDialog reopened; reopened.setData(data);
    auto *other=reopened.findChild<PlotWidget *>("ScanResultsPlot");
    QCOMPARE(other->styleOptions().xTitle,plot->styleOptions().xTitle);
    edit(reopened,"Cancelled X","Cancelled Y",false);
    reopened.setData(data);
    QCOMPARE(other->styleOptions().xTitle,plot->styleOptions().xTitle);
    data.scanDimensions=2; data.scanAxisLabel2="Angle (°)"; reopened.setData(data);
    QCOMPARE(other->styleOptions().xTitle,data.scanAxisLabel);
    QCOMPARE(other->styleOptions().yTitle,data.scanAxisLabel2);
    edit(reopened,"Surface X","Surface Y",true); reopened.setData(data);
    QCOMPARE(other->styleOptions().xTitle,QString("Surface X"));
    QCOMPARE(other->styleOptions().yTitle,QString("Surface Y"));
    data.scanDimensions=1; reopened.setData(data);
    QCOMPARE(other->styleOptions().xTitle,QString("Reaction coordinate (Å)"));
    QTimer::singleShot(0,&reopened,[&] {
        auto *editor=reopened.findChild<QDialog *>("ScanStyleDialog"); QVERIFY(editor);
        editor->findChild<QLineEdit *>("ScanXAxisTitle")->setText("Style X");
        editor->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    });
    reopened.findChild<QPushButton *>("EditScanStyle")->click(); reopened.setData(data);
    QCOMPARE(other->styleOptions().xTitle,QString("Style X"));
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_SCAN_STYLE_CAPTURE");
    if (!capture.isEmpty()) { dialog.resize(1200,720); dialog.show(); QTest::qWait(80); QVERIFY(dialog.grab().save(capture)); }
}

void SmokeTest::publicationPlotExports() {
    PlotWidget plot;
    plot.resize(680,420);
    plot.setScanSeries({{1.5,0},{1.7,4},{1.9,12},{2.1,22},{2.3,18},{2.5,9},{2.7,-3}},
                       QStringLiteral("Co-O distance (Å)"), QStringLiteral("ΔE (kcal mol⁻¹)"));
    QVERIFY(plot.setManualRange({1.4,2.8,-5,25}));
    auto style=plot.styleOptions();
    style.xIncrement=0.2; style.yIncrement=5;
    plot.setStyleOptions(style);
    const auto xs=plot.axisTicks(true), ys=plot.axisTicks(false);
    QVERIFY(xs.size()>3 && ys.size()>3);
    for (int i=1;i<xs.size();++i) QVERIFY(qAbs(xs[i]-xs[i-1]-0.2)<1e-8);
    for (int i=1;i<ys.size();++i) QVERIFY(qAbs(ys[i]-ys[i-1]-5)<1e-8);
    style.xIncrement=style.yIncrement=0; plot.setStyleOptions(style);
    for (double value:plot.axisTicks(false)) QVERIFY(qAbs(value/5-std::round(value/5))<1e-8);
    const auto range=plot.axisRange();
    PlotWidget::ExportOptions o;
    o.transparent=true;
    const auto image=plot.publicationImage(o);
    QCOMPARE(image.width(),qRound(84.6/25.4*600));
    QCOMPARE(image.height(),qRound(64.0/25.4*600));
    QVERIFY(qAbs(image.dotsPerMeterX()*0.0254-600)<1);
    QCOMPARE(image.pixelColor(0,0).alpha(),0);
    plot.setSelectedPoint(3);
    QCOMPARE(plot.publicationImage(o),image); // Selection is an interaction, not figure data.
    QCOMPARE(plot.axisRange().xMin,range.xMin);
    QCOMPARE(plot.styleOptions().axisFont,style.axisFont);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QString error;
    QVERIFY2(plot.savePublication(temporary.filePath("figure.pdf"),o,&error),qPrintable(error));
    QFile pdf(temporary.filePath("figure.pdf")); QVERIFY(pdf.open(QIODevice::ReadOnly));
    QVERIFY(pdf.read(8).startsWith("%PDF-"));
    QVERIFY(plot.savePublication(temporary.filePath("figure.png"),o,&error));
    QImage saved(temporary.filePath("figure.png")); QCOMPARE(saved.convertToFormat(image.format()),image);
    QVERIFY(!plot.savePublication(temporary.filePath("figure.bad"),o,&error));
    QVERIFY(!error.isEmpty());
    auto invalid=o; invalid.widthMm=0;
    QVERIFY(plot.publicationImage(invalid).isNull());
    const QString directory=qEnvironmentVariable("YOUNGCRAFT_PUBLICATION_REVIEW");
    if (!directory.isEmpty()) {
        QDir().mkpath(directory);
        o.transparent=false;
        QVERIFY(plot.savePublication(directory+"/scan-single.pdf",o));
        QVERIFY(plot.savePublication(directory+"/scan-single.png",o));
        o.widthMm=177.8; o.heightMm=110;
        style.showMechanism=true;
        style.mechanismStages={{"Reactant",1.5,1.9},{"Transition region",1.9,2.3},{"Product",2.3,2.7}};
        plot.setStyleOptions(style);
        QVERIFY(plot.savePublication(directory+"/scan-double.pdf",o));
        QVERIFY(plot.savePublication(directory+"/scan-double.png",o));
        QVERIFY(plot.image().save(directory+"/scan-screen.png"));
        auto single=o; single.widthMm=84.6; single.heightMm=64;
        QVERIFY(plot.savePublication(directory+"/scan-single-stages.png",single));
    }
    // Spectral sticks and bar plots must include the measured zero baseline.
    plot.clearManualRange();
    style.showMechanism=false; plot.setStyleOptions(style);
    plot.setSeries({},"Wavelength (nm)","Intensity");
    plot.setStickSeries({{300,4},{450,8}});
    QVERIFY(plot.autoRange().yMin<=0);
    QVERIFY(!plot.publicationImage(o).isNull());
    plot.setBarSeries({{1,10},{2,30}});
    QVERIFY(plot.autoRange().yMin<=0);
    if (!directory.isEmpty()) QVERIFY(plot.savePublication(directory+"/bars.png",o));
    plot.setHeatmapSeries({{1,1,0,0},{2,1,5,1},{1,2,10,2},{2,2,20,3}},"Distance 1 (Å)","Distance 2 (Å)","ΔE (kcal mol⁻¹)");
    QVERIFY(plot.axisRange().yMin>0); // 1D manual ranges cannot crop a 2D surface.
    if (!directory.isEmpty()) QVERIFY(plot.savePublication(directory+"/heatmap.png",o));
    QVERIFY(!plot.publicationImage(o).isNull());
    // Preview UI uses the same physical-size renderer.
    QTimer::singleShot(0,&plot,[&] {
        auto *dialog=plot.findChild<QDialog *>("PublicationExportDialog"); QVERIFY(dialog);
        auto *width=dialog->findChild<QDoubleSpinBox *>("PublicationWidth"); QVERIFY(width);
        QCOMPARE(width->value(),84.6);
        auto *preview=dialog->findChild<QLabel *>("PublicationPreview"); QVERIFY(preview);
        QVERIFY(!preview->pixmap().isNull());
        if (!directory.isEmpty()) QVERIFY(dialog->grab().save(directory+"/export-ui.png"));
        dialog->reject();
    });
    plot.showExportDialog();
}

void SmokeTest::mechanismStageRangesAndCopy() {
    CalculationData data;
    data.scanDimensions=1; data.scanAxisLabel=QStringLiteral("Reaction coordinate");
    for (int i=0;i<=9;++i) {
        ScanPoint point; point.index=i; point.coordinate=i;
        point.relativeKcal=20-std::abs(4-i)*4; data.scanPoints.push_back(point);
    }
    ScanResultsDialog dialog; dialog.setData(data); dialog.resize(1250,680); dialog.show();
    auto *plot=dialog.findChild<PlotWidget *>("ScanResultsPlot"); QVERIFY(plot);
    const auto defaults=plot->styleOptions(); QVERIFY(!defaults.showMechanism);
    QCOMPARE(defaults.mechanismStages.size(),3);
    QCOMPARE(defaults.mechanismStages[0].text,QString("Reactant"));
    QCOMPARE(defaults.mechanismStages[1].text,QString("Transition state"));
    QCOMPARE(defaults.mechanismStages[2].text,QString("Product"));
    QCOMPARE(defaults.mechanismStages[0].start,0.0);
    QCOMPARE(defaults.mechanismStages[2].end,9.0);
    auto *edit=dialog.findChild<QPushButton *>("EditMechanismStages"); QVERIFY(edit);
    bool edited=false;
    QTimer::singleShot(0,&dialog,[&] {
        auto *editor=dialog.findChild<QDialog *>("MechanismStagesDialog"); QVERIFY(editor);
        auto *table=editor->findChild<QTableWidget *>("MechanismStagesTable"); QVERIFY(table);
        auto *buttons=editor->findChild<QDialogButtonBox *>(); QVERIFY(buttons);
        table->item(0,0)->setText("Intermediate");
        auto *start=qobject_cast<QDoubleSpinBox *>(table->cellWidget(0,1));
        auto *end=qobject_cast<QDoubleSpinBox *>(table->cellWidget(0,2));
        start->setValue(4); end->setValue(2);
        QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
        start->setValue(0.5); end->setValue(2.5);
        QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        buttons->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(plot->styleOptions().mechanismStages[0].start,0.5);
        buttons->button(QDialogButtonBox::Cancel)->click(); edited=true;
    });
    edit->click(); QVERIFY(edited);
    QCOMPARE(plot->styleOptions().mechanismStages[0].text,QString("Reactant"));
    QTimer::singleShot(0,&dialog,[&] {
        auto *editor=dialog.findChild<QDialog *>("MechanismStagesDialog"); QVERIFY(editor);
        auto *table=editor->findChild<QTableWidget *>("MechanismStagesTable");
        editor->findChild<QCheckBox *>()->setChecked(true);
        table->item(0,0)->setText("Intermediate");
        qobject_cast<QDoubleSpinBox *>(table->cellWidget(0,1))->setValue(0.25);
        qobject_cast<QDoubleSpinBox *>(table->cellWidget(0,2))->setValue(2.75);
        editor->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    });
    edit->click();
    QCOMPARE(plot->styleOptions().mechanismStages[0].text,QString("Intermediate"));
    QCOMPARE(plot->styleOptions().mechanismStages[0].start,0.25);
    // Check the actual rendered endpoint coordinates and the empty text gap.
    PlotWidget isolated;
    isolated.resize(800,400);
    PlotWidget::StyleOptions isolatedStyle;
    isolatedStyle.showMechanism=true; isolatedStyle.showY=false;
    isolatedStyle.tickLabels=false; isolatedStyle.majorGrid=false;
    isolatedStyle.showLine=false; isolatedStyle.showPoints=false;
    isolatedStyle.mechanismStages={{"I",2,8}};
    isolated.setStyleOptions(isolatedStyle);
    QVERIFY(isolated.setManualRange({0,10,0,1}));
    const auto rendered=isolated.image(true);
    // Verify rendered range endpoints using the public data rectangle.
    const auto area=isolated.plotArea();
    const qreal lane=area.bottom()+12+isolatedStyle.mechanismOffset;
    auto hasInk=[&](qreal x,qreal y) {
        const qreal ratio=rendered.devicePixelRatio();
        for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx)
            if(rendered.pixelColor(qRound(x*ratio)+dx,qRound(y*ratio)+dy).alpha()>30) return true;
        return false;
    };
    QVERIFY(hasInk(area.left()+area.width()*0.2+2,lane));
    QVERIFY(hasInk(area.left()+area.width()*0.8-2,lane));
    QVERIFY(!hasInk(area.left()+area.width()*0.1,lane));
    const QImage annotated=plot->image(true); QVERIFY(!annotated.isNull());
    auto hidden=plot->styleOptions(); hidden.showMechanism=false; plot->setStyleOptions(hidden);
    QVERIFY(annotated!=plot->image(true));
    hidden.showMechanism=true; plot->setStyleOptions(hidden);
    auto *copy=static_cast<QPushButton *>(nullptr);
    for (auto *button:dialog.findChildren<QPushButton *>()) if (button->text()==QStringLiteral("그림 복사")) copy=button;
    QVERIFY(copy); copy->click(); QCOMPARE(QApplication::clipboard()->image(),plot->publicationImage(PlotWidget::ExportOptions{}));
    const auto directory=qEnvironmentVariable("YOUNGCRAFT_MECHANISM_REVIEW_DIR");
    if (!directory.isEmpty()) {
        QDir().mkpath(directory);
        QVERIFY(dialog.grab().save(directory+"/mechanism-ui.png"));
        QVERIFY(annotated.save(directory+"/mechanism-transparent.png"));
    }
    QVERIFY(plot->setManualRange({1,8,-5,25})); QVERIFY(!plot->image().isNull());
    data.scanDimensions=2; dialog.setData(data); QVERIFY(!edit->isEnabled());
    QVERIFY(PlotWidget::defaultMechanismStages(2,1).isEmpty());
}

void SmokeTest::copiesTsAnimation_data() {
    QTest::addColumn<int>("scenario");
    QTest::newRow("paused-selected-mode") << 0;
    QTest::newRow("playing-after-tab-transfer") << 1;
    QTest::newRow("auto-imaginary-mode") << 2;
    QTest::newRow("no-frequency-keeps-clipboard") << 3;
}
void SmokeTest::copiesTsAnimation() {
    if (QGuiApplication::platformName() == "offscreen") QSKIP("Requires OpenGL");
    QFETCH(int, scenario);
    auto originalWindow = std::make_unique<MainWindow>();
    MainWindow movedWindow;
    MainWindow *window = originalWindow.get();
    window->show();
    Molecule molecule;
    molecule.atoms = {{1,6,"C",{-0.7f,0,0}}, {2,8,"O",{0.7f,0,0}}};
    molecule.setBond(0,1,1.0,false,BondStyle::Solid,true);
    window->newDrawingDocument(molecule);
    window->setDrawingMode(false);
    QApplication::clipboard()->setText("unchanged clipboard");
    auto *button = window->m_analysis->findChild<QPushButton *>("CopyFrequencyAnimation");
    QVERIFY(button);
    if (scenario == 3) {
        button->click();
        QCOMPARE(QApplication::clipboard()->text(), QString("unchanged clipboard"));
        QVERIFY(window->m_statusLabel->text().contains(QStringLiteral("모드")));
        return;
    }
    FrequencyMode mode;
    mode.number = 1; mode.wavenumber = -350; mode.imaginary = true;
    mode.displacement = {{-0.6f,0.4f,0}, {0.6f,-0.4f,0}};
    window->m_data.frequencies = {mode};
    window->m_analysis->setData(window->m_data);
    window->m_analysis->setCurrentTab(1);
    window->showFrequency(0);
    window->m_animation->stop();
    window->m_analysis->findChild<QDoubleSpinBox *>("FrequencyCycleTime")->setValue(0.4);
    window->m_analysis->findChild<QDoubleSpinBox *>("FrequencyAmplitude")->setValue(1.0);
    window->handleGeometryPick(0); window->handleGeometryPick(1);
    auto camera = window->m_view->cameraState();
    camera.orthoScale *= 1.3f; window->m_view->setCameraState(camera);
    if (scenario == 1) {
        window->m_animation->startFrequency();
        QVERIFY(window->moveDocumentTo(0, &movedWindow));
        originalWindow.reset();
        window = &movedWindow;
        button = window->m_analysis->findChild<QPushButton *>("CopyFrequencyAnimation");
    }
    if (scenario == 2) window->m_currentFrequency = -1;
    const int frequency = window->m_currentFrequency;
    const auto before = window->m_view->molecule();
    const auto selection = window->m_geometrySelection;
    const auto undo = window->m_undoMolecules.size();
    const bool playing = window->m_animation->isPlaying();
    button->click();
    QCOMPARE(window->m_animation->isPlaying(), playing);
    QCOMPARE(window->m_currentFrequency, frequency);
    QCOMPARE(window->m_geometrySelection, selection);
    QCOMPARE(window->m_undoMolecules.size(), undo);
    QCOMPARE(window->m_view->cameraState().orthoScale, camera.orthoScale);
    for (int i=0;i<before.atoms.size();++i) QCOMPARE(window->m_view->molecule().atoms[i].position, before.atoms[i].position);
    QCOMPARE(window->m_view->molecule().bonds.size(), before.bonds.size());
    QVERIFY(window->m_statusLabel->text().contains(QStringLiteral("복사했습니다")));
    window->m_animation->stop();
    const auto *mime = QApplication::clipboard()->mimeData();
    QVERIFY(mime->hasFormat("image/gif")); QVERIFY(mime->hasUrls());
#ifdef Q_OS_WIN
    QVERIFY(IsClipboardFormatAvailable(CF_HDROP));
    QVERIFY(IsClipboardFormatAvailable(RegisterClipboardFormatW(L"GIF")));
#endif
    const QByteArray bytes = mime->data("image/gif");
    QVERIFY(bytes.startsWith("GIF89a"));
    QCOMPARE(mime->urls().size(), 1);
    const QString path = mime->urls().first().toLocalFile();
    const auto cleanClipboard = qScopeGuard([&] { QApplication::clipboard()->clear(); QFile::remove(path); });
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), bytes); file.close();
    QImageReader reader(path, "gif");
    QVERIFY2(reader.canRead(), qPrintable(reader.errorString()));
    QCOMPARE(reader.imageCount(), 8);
    QImage first = reader.read(); QVERIFY(!first.isNull());
    QCOMPARE(first.pixelColor(0,0).alpha(), 0);
    QCOMPARE(reader.nextImageDelay(), 50);
    bool moved = false;
    for (int i=1;i<8;++i) { const QImage next = reader.read(); QVERIFY(!next.isNull()); if(next != first) moved = true; }
    QVERIFY2(moved, "Clipboard GIF must contain real moving frames");
    reader.setFileName(QString());
    if (scenario == 0) {
        const auto directory = qEnvironmentVariable("YOUNGCRAFT_TS_COPY_REVIEW_DIR");
        if (!directory.isEmpty()) {
            QDir().mkpath(directory);
            QFile::remove(directory + "/ts-animation.gif");
            QVERIFY(QFile::copy(path, directory + "/ts-animation.gif"));
            QVERIFY(window->grab().save(directory + "/ts-copy-ui.png"));
        }
    }
    QApplication::clipboard()->clear();
    QVERIFY(QFile::remove(path));
}

void SmokeTest::nativeTabDragBetweenWindows_data() {
    QTest::addColumn<bool>("ontoTabBar");
    QTest::addColumn<bool>("reorderFirst");
    QTest::newRow("empty-window-body") << false << false;
    QTest::newRow("existing-tab-bar") << true << false;
    QTest::newRow("reorder-then-move") << false << true;
}
void SmokeTest::nativeTabDragBetweenWindows() {
#ifdef Q_OS_WIN
    if (QGuiApplication::platformName() != "windows") QSKIP("Requires the Windows drag backend");
    if (!qEnvironmentVariableIsEmpty("QT_SCALE_FACTOR")) QSKIP("Native drag needs unmodified Windows desktop scaling");
    QFETCH(bool, ontoTabBar);
    QFETCH(bool, reorderFirst);
    MainWindow source, target;
    if (ontoTabBar) { target.newDrawingDocument(); target.setDrawingMode(false); }
    source.setGeometry(20, 40, 850, 700);
    target.setGeometry(920, 40, 850, 700);
    source.newDrawingDocument();
    auto *originalWorkspace = source.m_workspace;
    if (reorderFirst) source.newDrawingDocument();
    source.setDrawingMode(false);
    source.showNormal(); target.showNormal();
    source.setGeometry(20, 40, 850, 700); target.setGeometry(920, 40, 850, 700);
    source.raise(); source.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&source));
    QVERIFY(QTest::qWaitForWindowExposed(&target));
    QVERIFY(QTest::qWaitForWindowActive(&source));
    QTest::qWait(250);
    auto *bar = source.m_documentTabs->tabBar();
    const QPoint start = bar->mapToGlobal(bar->tabRect(0).center());
    const QPoint horizontal = bar->mapToGlobal(bar->tabRect(reorderFirst ? 1 : 0).center() + QPoint(10, 0));
    const QPoint outside = bar->mapToGlobal(QPoint(bar->tabRect(0).center().x(), bar->height()+60));
    const QPoint destination = ontoTabBar
        ? target.m_documentTabs->tabBar()->mapToGlobal(target.m_documentTabs->tabBar()->tabRect(0).center())
        : target.m_workspace->mapToGlobal(target.m_workspace->rect().center());
    QCursor::setPos(start);
    QTest::qWait(100);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    QTest::qWait(200);
    const double ratio = source.devicePixelRatioF();
    std::thread mouse([outside, destination, horizontal, reorderFirst, ratio] {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        if (reorderFirst) {
            SetCursorPos(qRound(horizontal.x()*ratio), qRound(horizontal.y()*ratio));
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        SetCursorPos(qRound(outside.x()*ratio), qRound(outside.y()*ratio));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        SetCursorPos(qRound(destination.x()*ratio), qRound(destination.y()*ratio));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    });
    auto joinMouse = qScopeGuard([&] { mouse.join(); });
    QTRY_COMPARE_WITH_TIMEOUT(target.m_documentTabs->count(), ontoTabBar ? 2 : 1, 5000);
    QCOMPARE(source.m_documentTabs->count(), reorderFirst ? 1 : 0);
    QCOMPARE(target.m_workspace, originalWorkspace);
    // Render after reparenting to catch an invalidated OpenGL context.
    QTest::qWait(150);
    QVERIFY(!target.m_view->grabFramebuffer().isNull());
#else
    QSKIP("Requires Windows");
#endif
}

void SmokeTest::movesDocumentsBetweenWindows() {
    auto source = std::make_unique<MainWindow>();
    MainWindow target;
    source->show(); target.show();
    Molecule molecule;
    molecule.atoms = {{1,6,"C",{0,0,0}}, {2,8,"O",{1.4f,0,0}}, {3,1,"H",{-1,0,0}}};
    molecule.setBond(0,1,1.0,false,BondStyle::Solid,true);
    source->newDrawingDocument(molecule);
    source->setDrawingMode(false);
    auto edited = molecule; edited.atoms[1].position.setY(0.5f);
    source->commitEditedMolecule(edited, false);
    source->m_view->setAtomVisibility({true,true,false});
    source->handleGeometryPick(0); source->handleGeometryPick(1);
    auto camera = source->m_view->cameraState();
    camera.orthoScale *= 1.7f; source->m_view->setCameraState(camera);
    auto *view = source->m_view;
    auto *sidebar = source->m_currentSidebar;
    auto *workspace = source->m_workspace;
    const int undoCount = source->m_undoMolecules.size();
    target.newDrawingDocument(molecule);
    const auto unrelated = target.m_activeDocument;
    // Move an inactive document as well as an active one.
    source->newDrawingDocument(molecule);
    QVERIFY(source->moveDocumentTo(0, &target));
    QCOMPARE(source->m_documentTabs->count(), 1);
    QCOMPARE(target.m_documentTabs->count(), 2);
    QCOMPARE(target.m_view, view);
    QCOMPARE(target.m_currentSidebar, sidebar);
    QCOMPARE(target.m_workspace, workspace);
    QVERIFY(target.m_structureDock->isAncestorOf(target.m_drawBar));
    QCOMPARE(target.m_view->molecule().atoms[1].position.y(), 0.5f);
    QVERIFY(!target.m_view->molecule().atoms[2].visible);
    QCOMPARE(target.m_geometrySelection, QVector<int>({0,1}));
    QCOMPARE(target.m_view->cameraState().orthoScale, camera.orthoScale);
    QCOMPARE(target.m_undoMolecules.size(), undoCount);
    source.reset();
    QCoreApplication::processEvents();
    // The moved UI must now invoke the receiving window, after its source dies.
    auto *clear = target.m_workspace->findChild<QPushButton *>("ClearAtomMeasurement");
    QVERIFY(clear); clear->click(); QVERIFY(target.m_geometrySelection.isEmpty());
    auto *element = target.m_drawElement;
    element->setCurrentText("N");
    QCOMPARE(target.m_view->m_drawAtomicNumber, 7);
    QCOMPARE(unrelated->view->molecule().atoms[1].position.y(), 0.0f);
    target.undoDrawingEdit();
    QCOMPARE(target.m_view->molecule().atoms[1].position.y(), 0.0f);
    target.redoDrawingEdit();
    QCOMPARE(target.m_view->molecule().atoms[1].position.y(), 0.5f);
    MainWindow third;
    QVERIFY(target.moveDocumentTo(target.m_documentTabs->currentIndex(), &third));
    QCOMPARE(third.m_view, view);
    third.handleGeometryPick(0); third.handleGeometryPick(1);
    third.m_workspace->findChild<QPushButton *>("ClearAtomMeasurement")->click();
    QVERIFY(third.m_geometrySelection.isEmpty());
    third.closeDocument(0);
    QCOMPARE(third.m_activeDocument, third.m_emptyDocument);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Loading documents stay put until their worker has delivered the result.
    QSignalSpy loaded(&target, &MainWindow::documentLoaded);
    target.openFile(QDir::current().absoluteFilePath("tests/data/mvp.out"));
    const int pending = target.m_documentTabs->currentIndex();
    QVERIFY(!target.moveDocumentTo(pending, &third));
    QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 10000);
    QVERIFY(target.moveDocumentTo(pending, &third));
    QCOMPARE(third.m_data.frames.size(), third.m_activeDocument->data.frames.size());
    QVERIFY(!third.m_data.molecule.isEmpty());
    third.showFrame(0);
    QCOMPARE(third.m_currentFrame, 0);
}

void SmokeTest::selectedBondShortcutAndColigandDialog() {
    MainWindow w;w.resize(1200,850);w.show();
    Molecule m;m.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.4f,0,0}}};
    w.newDrawingDocument(m);w.setDrawingMode(false);{QSignalBlocker block(w.m_autoHButton);w.m_autoHButton->setChecked(true);}
    w.m_view->selection().atoms={0,1};
    QAction *shortcut=nullptr;for(auto *a:w.findChildren<QAction *>())if(a->shortcut()==QKeySequence("Ctrl+B"))shortcut=a;
    QVERIFY(shortcut);shortcut->trigger();
    QCOMPARE(w.m_view->molecule().bonds.size(),1);QVERIFY(w.m_view->molecule().bonds[0].manual);
    QCOMPARE(w.m_view->molecule().atoms.size(),2);QCOMPARE(w.m_view->selection().atoms.size(),2);
    QVERIFY(!w.m_bondEditMode);shortcut->trigger();QVERIFY(w.m_view->molecule().bonds.isEmpty());
    w.undoDrawingEdit();QCOMPARE(w.m_view->molecule().bonds.size(),1);
    w.redoDrawingEdit();QVERIFY(w.m_view->molecule().bonds.isEmpty());
    // Imported calculation creates an editable copy with the selected connectivity.
    w.m_data.frames.append(Frame{});w.m_view->selection().atoms={0,1};
    const int tabs=w.m_documentTabs->count();shortcut->trigger();
    QCOMPARE(w.m_documentTabs->count(),tabs+1);QCOMPARE(w.m_view->molecule().bonds.size(),1);
    w.m_view->selection().clear();w.clearGeometrySelection();
    w.handleGeometryPick(0);w.handleGeometryPick(1);shortcut->trigger();QVERIFY(w.m_view->molecule().bonds.isEmpty());
    QCOMPARE(w.m_geometrySelection.size(),2);shortcut->trigger();QCOMPARE(w.m_view->molecule().bonds.size(),1);
    w.m_view->selection().clear();w.clearGeometrySelection();shortcut->trigger();QVERIFY(w.m_bondEditMode);shortcut->trigger();QVERIFY(!w.m_bondEditMode);
    QTimer::singleShot(100,&w,[&]{
        auto *d=w.findChild<QDialog *>("LabComplexDialog");QVERIFY(d);
        auto *adduct=d->findChild<QComboBox *>("LabAdduct");auto *solvent=d->findChild<QComboBox *>("LabColigand");
        QVERIFY(adduct);QVERIFY(solvent);QCOMPARE(solvent->count(),3);
        for(int i=0;i<adduct->count();++i)if(adduct->itemText(i).startsWith("hydroperoxo")&&adduct->itemText(i).contains("end-on"))adduct->setCurrentIndex(i);
        for(int i=0;i<solvent->count();++i)if(solvent->itemText(i).startsWith("MeCN"))solvent->setCurrentIndex(i);
        d->accept();
    });
    w.buildLabComplex();int solventDonors=0;
    const auto &result=w.m_view->molecule();
    for(const auto &b:result.bonds)if(b.coordination&&result.atoms[b.first==0?b.second:b.first].atomicNumber==7)++solventDonors;
    QVERIFY(solventDonors>=5);QVERIFY(MoleculeEditController::validate(result));
}

void SmokeTest::atomVisibilityPreservesViewAndScroll_data() {
    QTest::addColumn<bool>("perspective");
    QTest::newRow("orthographic") << false;
    QTest::newRow("perspective") << true;
}

void SmokeTest::atomVisibilityPreservesViewAndScroll() {
    QFETCH(bool, perspective);
    MainWindow w; w.resize(1400, 900); w.show();
    Molecule m;
    m.atoms = {{1,6,"C",{}},{2,1,"H",{1,0,0}},{3,8,"O",{0,1,0}},
               {4,1,"H",{0,2,0}},{5,0,"X",{0,0,2}}};
    m.setBond(0,1,1); m.setBond(2,3,1);
    w.newDrawingDocument(m); w.setDrawingMode(false);
    auto *v = w.m_view;
    // Simulate a later trajectory frame: hiding must not restore the source geometry.
    QVector<QVector3D> live;
    for (const auto &a : m.atoms) live.push_back(a.position + QVector3D(2,3,4));
    v->setCoordinates(live);
    v->m_rotation = QQuaternion::fromEulerAngles(25,45,10);
    v->m_target = {4,5,6}; v->m_distance = 13; v->m_orthoScale = 2.3f;
    const auto rotation = v->m_rotation; const auto target = v->m_target;
    const auto center = v->m_center; const auto radius = v->m_radius;
    w.setPerspective(perspective);
    w.m_analysis->setCurrentTab(1);
    // This assertion requires the explorer even when the saved layout hides it.
    w.m_explorerDock->show();
    w.m_explorer->show();
    for (int i=0; i<120; ++i) w.m_explorer->addTopLevelItem(new QTreeWidgetItem({QString::number(i)}));
    QTest::qWait(100);
    auto *scroll = w.m_explorer->verticalScrollBar();
    QVERIFY(scroll->maximum() > 0); scroll->setValue(scroll->maximum()/2);
    const int scrollValue = scroll->value();
    const auto projection = v->viewProjection();
    auto trigger = [&](const QString &name) {
        for (auto *a : w.findChildren<QAction *>()) if (a->text() == name) { a->trigger(); return true; }
        return false;
    };
    auto *toolbar = w.findChild<QToolBar *>(QStringLiteral("MainToolBar"));
    QVERIFY(toolbar);
    const auto toolbarActions = toolbar->actions();
    int copyIndex = -1;
    for (int i=0; i<toolbarActions.size(); ++i)
        if (toolbarActions[i]->text() == QStringLiteral("이미지 복사")) copyIndex = i;
    QVERIFY(copyIndex >= 0);
    auto *removeHydrogen = w.findChild<QAction *>("HideCarbonHydrogens");
    auto *resetHydrogen = w.findChild<QAction *>("RestoreCarbonHydrogens");
    QVERIFY(removeHydrogen);
    QVERIFY(resetHydrogen);
    QVERIFY(toolButtonForAction(w.m_structureDock, removeHydrogen));
    QVERIFY(toolButtonForAction(w.m_structureDock, resetHydrogen));
    w.applyAtomSelection({0,2});
    // A reset must affect only C-bound H, leaving unrelated hidden atoms alone.
    w.m_data.molecule.atoms[4].visible = false;
    w.updateAtomVisibility();
    for (auto *action : {resetHydrogen, removeHydrogen, removeHydrogen, resetHydrogen, resetHydrogen}) {
        action->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(v->molecule().atoms.size(), m.atoms.size());
        QCOMPARE(v->molecule().bonds.size(), m.bonds.size());
        QCOMPARE(v->molecule().atoms[1].visible, action == resetHydrogen);
        QVERIFY(v->molecule().atoms[3].visible); // O-H stays visible.
        QVERIFY(!v->molecule().atoms[4].visible);
        QCOMPARE(v->selection().atoms, QSet<int>({0,2}));
        QCOMPARE(v->m_rotation, rotation); QCOMPARE(v->m_target, target);
        QCOMPARE(v->m_distance, 13.0f); QCOMPARE(v->m_orthoScale, 2.3f);
        QCOMPARE(v->m_center, center); QCOMPARE(v->m_radius, radius);
        QCOMPARE(v->viewProjection(), projection);
        QCOMPARE(scroll->value(), scrollValue); QCOMPARE(w.m_analysis->currentTab(), 1);
        for (int i=0; i<live.size(); ++i) QCOMPARE(v->molecule().atoms[i].position, live[i]);
    }
    const QStringList actions = {QStringLiteral("탄소에 결합된 수소 숨기기"),
        QStringLiteral("더미 원자 숨기기"), QStringLiteral("선택 원자 숨기기"),
        QStringLiteral("모든 수소 숨기기"), QStringLiteral("숨긴 원자 모두 복원")};
    w.applyAtomSelection({0,2});
    for (const auto &name : actions) {
        QVERIFY(trigger(name));
        QCoreApplication::processEvents();
        QCOMPARE(v->m_rotation, rotation); QCOMPARE(v->m_target, target);
        QCOMPARE(v->m_distance, 13.0f); QCOMPARE(v->m_orthoScale, 2.3f);
        QCOMPARE(v->m_center, center); QCOMPARE(v->m_radius, radius);
        QCOMPARE(v->viewProjection(), projection);
        QCOMPARE(scroll->value(), scrollValue); QCOMPARE(w.m_analysis->currentTab(), 1);
        for (int i=0; i<live.size(); ++i) QCOMPARE(v->molecule().atoms[i].position, live[i]);
        if (name == actions[0]) { QVERIFY(!v->molecule().atoms[1].visible); QVERIFY(v->molecule().atoms[3].visible); }
        if (name == actions[1]) QVERIFY(!v->molecule().atoms[4].visible);
        if (name == actions[2]) { QVERIFY(!v->molecule().atoms[0].visible); QVERIFY(!v->molecule().atoms[2].visible); }
    }
    for (const auto &a : v->molecule().atoms) QVERIFY(a.visible);
    QVERIFY(v->style().showHydrogen);
    // The result tree and atom table select a single atom without a selection set.
    w.applyAtomSelection({}); w.showAtom(3);
    QVERIFY(trigger(QStringLiteral("선택 원자 숨기기")));
    QVERIFY(!v->molecule().atoms[3].visible);
    QCOMPARE(v->viewProjection(), projection); QCOMPARE(scroll->value(), scrollValue);
}

void SmokeTest::topPanelToolbarFollowsDocuments() {
    MainWindow w; w.resize(1400,900); w.show();
    auto *toolbar = w.findChild<QToolBar *>("MainToolBar"); QVERIFY(toolbar);
    QCOMPARE(w.toolBarArea(toolbar), Qt::TopToolBarArea);
    QCOMPARE(w.findChildren<QToolBar *>().size(), 1);
    QCOMPARE(toolbar->toolButtonStyle(), Qt::ToolButtonIconOnly);
    const auto commonActions = {"NewStructure", "OpenStructureFile", "MMOptimizationAction",
                                "QMOptimizationAction", "CopyStructureImage", "ToolbarOrbitals", "ApplicationSettingsAction",
                                "ToolbarExport", "ToggleResultsSidebar", "ToggleStructureSidebar", "ToggleOutputSource"};
    for (const auto *name : commonActions) {
        auto *action = w.findChild<QAction *>(name);
        QVERIFY(action);
        auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
        QVERIFY(button && !button->icon().isNull());
        QCOMPARE(button->size(), QSize(32, 32));
        QVERIFY(!button->accessibleName().isEmpty());
    }
    QVERIFY(!w.findChild<QAction *>("MMOptimizationAction")->isEnabled());
    QVERIFY(!w.findChild<QAction *>("CopyStructureImage")->isEnabled());
    QCOMPARE(w.dockWidgetArea(w.m_explorerDock), Qt::LeftDockWidgetArea);
    QCOMPARE(w.dockWidgetArea(w.m_structureDock), Qt::RightDockWidgetArea);
    auto *leftToggle = toolbar->widgetForAction(w.m_explorerDock->toggleViewAction());
    auto *rightToggle = toolbar->widgetForAction(w.m_structureDock->toggleViewAction());
    QVERIFY(leftToggle && rightToggle && leftToggle->isVisible() && rightToggle->isVisible());
    w.m_explorerDock->show();
    w.m_structureDock->show();
    QTest::mouseClick(leftToggle, Qt::LeftButton);
    QVERIFY(!w.m_explorerDock->isVisible());
    QVERIFY(w.m_structureDock->isVisible());
    QTest::mouseClick(rightToggle, Qt::LeftButton);
    QVERIFY(!w.m_explorerDock->isVisible());
    QVERIFY(!w.m_structureDock->isVisible());
    QTest::mouseClick(leftToggle, Qt::LeftButton);
    QVERIFY(w.m_explorerDock->isVisible());
    QVERIFY(!w.m_structureDock->isVisible());
    QTest::mouseClick(rightToggle, Qt::LeftButton);
    QVERIFY(w.m_structureDock->isVisible());

    Molecule m; m.atoms = {{1,6,"C",{}}}; w.newDrawingDocument(m); w.setDrawingMode(false);
    QVERIFY(w.findChild<QAction *>("MMOptimizationAction")->isEnabled());
    QVERIFY(w.findChild<QAction *>("CopyStructureImage")->isEnabled());
    const int first = w.m_documentTabs->currentIndex();
    auto *frequency = w.findChild<QToolButton *>("AnalysisShortcut1");
    auto *opt = w.findChild<QToolButton *>("AnalysisShortcut0");
    QVERIFY(frequency && opt && opt->isVisible() && !frequency->isVisible());
    auto *source = qobject_cast<QToolButton *>(toolbar->widgetForAction(w.findChild<QAction *>("ToggleOutputSource")));
    QVERIFY(source && source->isVisible());
    QVERIFY(!w.findChild<QToolButton *>("ResultSourceButton"));
    w.m_data.route = QStringLiteral("# opt freq");
    w.m_analysis->setData(w.m_data);
    QVERIFY(frequency->isVisible());
    QTest::mouseClick(frequency, Qt::LeftButton);
    QCOMPARE(w.m_analysis->currentTab(), 1);
    QVERIFY(frequency->isChecked());
    auto *splitter = w.m_currentSidebar->findChild<QSplitter *>("SidebarPanelSplitter");
    // The sidebar itself is the splitter.
    if (!splitter) splitter = qobject_cast<QSplitter *>(w.m_currentSidebar);
    QVERIFY(splitter); QCOMPARE(splitter->orientation(), Qt::Vertical);
    auto *firstPanel = w.m_analysis;
    w.newDrawingDocument(m); w.setDrawingMode(false);
    auto *atoms = w.findChild<QToolButton *>("AnalysisShortcut5");
    QVERIFY(atoms && atoms->isVisible() && !frequency->isVisible());
    QTest::mouseClick(atoms, Qt::LeftButton);
    QCOMPARE(w.m_analysis->currentTab(), 5);
    QVERIFY(atoms->isChecked());
    w.m_documentTabs->setCurrentIndex(first);
    QCOMPARE(w.m_analysis, firstPanel);
    QCOMPARE(w.m_analysis->currentTab(), 1);
    QVERIFY(frequency->isVisible() && frequency->isChecked() && !atoms->isChecked());
    QTest::mouseClick(leftToggle, Qt::LeftButton);
    QVERIFY(!w.m_explorerDock->isVisible());
    const QString path = QFINDTESTDATA("tests/data/mvp.out");
    QVERIFY(!path.isEmpty());
    w.openFile(path);
    QTRY_VERIFY_WITH_TIMEOUT(w.m_workspace->property("loadFinished").toBool(), 10000);
    QVERIFY(!w.m_explorerDock->isVisible());
    QTest::mouseClick(leftToggle, Qt::LeftButton);
    QVERIFY(source->isVisible());
    QVERIFY(w.m_view->height() > 200);

    w.resize(820, 640);
    QTRY_COMPARE(w.size(), QSize(820, 640));
    auto *bottomToggle = toolbar->widgetForAction(w.m_outputDock->toggleViewAction());
    QVERIFY(bottomToggle && bottomToggle->isVisible());
    QTest::mouseClick(bottomToggle, Qt::LeftButton);
    QTRY_VERIFY(w.m_outputDock->isVisible());
    QTRY_COMPARE(w.size(), QSize(820, 640));
    QCOMPARE(frequency->size(), QSize(32, 32));
    QVERIFY(!frequency->icon().isNull());
    const auto outputButtons = w.m_outputDock->findChildren<QPushButton *>();
    QCOMPARE(outputButtons.size(), 4);
    for (auto *button : outputButtons) {
        QCOMPARE(button->size(), QSize(32, 32));
        QVERIFY(!button->icon().isNull() && !button->accessibleName().isEmpty());
        QVERIFY(w.m_outputDock->rect().contains(button->mapTo(w.m_outputDock, button->rect().bottomRight())));
    }
    QCOMPARE(w.m_view->geometry(), w.m_view->parentWidget()->rect());
    const auto compactCapture = qEnvironmentVariable("YOUNGCRAFT_COMPACT_CAPTURE");
    if (!compactCapture.isEmpty()) {
        QTest::qWait(120);
        QVERIFY(w.grab().save(compactCapture));
    }
    QTest::mouseClick(bottomToggle, Qt::LeftButton);
    QVERIFY(!w.m_outputDock->isVisible());
    const auto capture = qEnvironmentVariable("YOUNGCRAFT_TOP_PANEL_CAPTURE");
    if (!capture.isEmpty()) {
        w.m_analysis->setCurrentTab(1);
        w.m_structureTabs->setCurrentIndex(0);
        QTest::qWait(150); QVERIFY(w.grab().save(capture));
    }
}

void SmokeTest::rectangularViewportPickingAndCapture(){
    MainWindow window;window.resize(1700,850);window.show();
    Molecule molecule;molecule.atoms={{1,6,"C",{-4,0,0}},{2,8,"O",{4,0,0}}};
    window.newDrawingDocument(molecule);window.setDrawingMode(false);
    QTest::qWait(100);auto *view=window.m_view;
    QCOMPARE(view->geometry(),view->parentWidget()->rect());QVERIFY(view->width()>view->height()*1.3);
    for(bool perspective:{false,true}){
        auto style=view->style();style.perspective=perspective;style.showLabels=false;view->setStyle(style);view->bestView();QTest::qWait(80);
        const auto vp=view->viewProjection();
        for(int i=0;i<2;++i){
            const auto ndc=(vp*QVector4D(molecule.atoms[i].position,1)).toVector3DAffine();
            const QPoint point(qRound((ndc.x()*.5+.5)*view->width()),qRound((1-(ndc.y()*.5+.5))*view->height()));
            QSignalSpy picked(view,&MoleculeView::atomPicked);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
            QVERIFY(!picked.isEmpty());QCOMPARE(picked.last()[0].toInt(),i);
        }
        const auto state=view->cameraState();const auto geometry=view->geometry();
        const auto square=view->imageAtSize(QSize(900,900),true,true);QCOMPARE(square.size(),QSize(900,900));
        const auto bounds=ImageExporter::contentBounds(square);QVERIFY(!bounds.isEmpty());
        QVERIFY(bounds.left()>2);QVERIFY(bounds.right()<square.width()-3);
        QVERIFY(bounds.top()>2);QVERIFY(bounds.bottom()<square.height()-3);
        QCOMPARE(view->geometry(),geometry);QCOMPARE(view->cameraState().rotation,state.rotation);
        QCOMPARE(view->cameraState().orthoScale,state.orthoScale);QCOMPARE(view->cameraState().distance,state.distance);
        const auto crop=ImageExporter::cropToContent(square);QVERIFY(crop.width()<square.width());QVERIFY(crop.height()<square.height());
    }
    QTest::qWait(100);
    const auto path=qEnvironmentVariable("YOUNGCRAFT_FLEXIBLE_VIEWPORT_PREVIEW");if(!path.isEmpty())QVERIFY(window.grab().save(path));
    window.resize(750,1050);QTest::qWait(100);QCOMPARE(view->geometry(),view->parentWidget()->rect());
    QVERIFY(view->height()>view->width());
}

void SmokeTest::scanVisibilityAndFlexibleViewport() {
    MainWindow w; w.resize(1400,900); w.show(); QTest::qWait(60);
    QVERIFY(!w.findChild<QWidget *>("ScanWorkspace"));
    QCOMPARE(w.m_view->geometry(),w.m_view->parentWidget()->rect());
    const QStringList labels = w.m_analysis->tabTitles();
    for (int i=0; i<labels.size(); ++i) {
        auto *a=w.findChild<QAction *>(QString("AnalysisPanelTab%1").arg(i));
        QVERIFY(a); QCOMPARE(a->toolTip(), labels[i]);
        auto *button = w.m_analysisShortcuts->findChild<QToolButton *>(QString("AnalysisShortcut%1").arg(i));
        QVERIFY(button && button->defaultAction() == a);
    }
    // A real Gaussian optimization/frequency output with its Scan sections removed.
    QFile input("tests/data/mvp.out"); QVERIFY(input.open(QIODevice::ReadOnly));
    auto data=input.readAll(); data.replace("opt freq scan", "opt freq");
    const int start=data.indexOf(" Summary of Optimized Potential Surface Scan");
    const int end=data.indexOf(" Frequencies --", start); QVERIFY(start>=0); QVERIFY(end>start);
    data.remove(start,end-start);
    QTemporaryFile ordinary(QDir::tempPath()+"/youngcraft-optfreq-XXXXXX.out");
    QVERIFY(ordinary.open()); ordinary.write(data); ordinary.flush();
    w.openFile(ordinary.fileName());
    QTRY_VERIFY_WITH_TIMEOUT(w.m_workspace->property("loadFinished").toBool(),10000);
    QVERIFY(!w.m_data.scanDetected); QVERIFY(!w.findChild<QWidget *>("ScanWorkspace"));
    const int nonScan=w.m_documentTabs->currentIndex();
    QTest::qWait(60); QCOMPARE(w.m_view->geometry(),w.m_view->parentWidget()->rect());
    QVERIFY(w.m_explorerDock->width()<=360);
    QVERIFY(w.m_view->width()>w.m_explorerDock->width());
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_SIDEBAR_CAPTURE");
    if (!capture.isEmpty()) { w.m_analysis->setCurrentTab(1); QTest::qWait(100); QVERIFY(w.grab().save(capture)); }
    w.openFile("tests/data/mvp.out");
    QTRY_VERIFY_WITH_TIMEOUT(w.m_workspace->property("loadFinished").toBool(),10000);
    QVERIFY(w.m_data.scanDetected); QVERIFY(!w.findChild<QWidget *>("ScanWorkspace"));
    QTest::qWait(60); QCOMPARE(w.m_view->geometry(),w.m_view->parentWidget()->rect());
    if (!capture.isEmpty()) QVERIFY(w.grab().save(capture+".scan.png"));
    w.m_documentTabs->setCurrentIndex(nonScan);
    QVERIFY(!w.findChild<QWidget *>("ScanWorkspace"));
    for (const QSize size : {QSize(900,700),QSize(1600,950)}) {
        w.resize(size); QTest::qWait(60);
        QCOMPARE(w.m_view->geometry(),w.m_view->parentWidget()->rect());
        QVERIFY(w.m_explorerDock->width()<=360);
        QVERIFY(w.m_view->width()>w.m_explorerDock->width());
    }
    w.newDrawingDocument(); QTest::qWait(60);
    QVERIFY(!w.findChild<QWidget *>("ScanWorkspace")); QCOMPARE(w.m_view->geometry(),w.m_view->parentWidget()->rect());
    // A detected but unfinished Scan has no graph data to display yet.
    auto pendingScan=FileDispatcher::load(ordinary.fileName(), {});
    QVERIFY(pendingScan.parseResult.success);
    pendingScan.parseResult.data.scanDetected=true;
    pendingScan.parseResult.data.scanPoints.clear();
    w.onFileLoaded(pendingScan); QTest::qWait(60);
    QVERIFY(!w.findChild<QWidget *>("ScanWorkspace"));
}

void SmokeTest::chemcraftSelectionAndClipboard() {
    const auto *clipboard=QApplication::clipboard()->mimeData();
    auto *saved=new QMimeData;if(clipboard)for(const auto &format:clipboard->formats())saved->setData(format,clipboard->data(format));
    auto restore=qScopeGuard([saved]{QApplication::clipboard()->setMimeData(saved);});
    MainWindow w;w.show();Molecule m;m.atoms={{1,6,"C",{}},{2,8,"O",{1.3f,0,0}},{3,1,"H",{-.5f,.8f,0}},{4,1,"H",{6,0,0}}};
    m.setBond(0,1,2);m.setBond(0,2,1);w.newDrawingDocument(m);{const QSignalBlocker block(w.m_autoHButton);w.m_autoHButton->setChecked(true);}w.setDrawingMode(false);w.m_view->setFocus();
    auto action=[&](const QString &label){for(auto *a:w.findChildren<QAction *>())if(a->text()==label)return a;return static_cast<QAction *>(nullptr);};
    w.applyAtomSelection({1});auto *expand=action(QStringLiteral("선택 확장"));QVERIFY(expand);expand->trigger();QCOMPARE(w.selectedStructureAtoms(),QSet<int>({0,1}));
    auto *connected=action(QStringLiteral("선택 원자에 결합된 원자 전체 선택"));QVERIFY(connected);connected->trigger();QCOMPARE(w.selectedStructureAtoms(),QSet<int>({0,1,2}));
    auto *invert=action(QStringLiteral("선택 반전"));QVERIFY(invert);invert->trigger();QCOMPARE(w.selectedStructureAtoms(),QSet<int>({3}));
    w.applyAtomSelection({0,1});w.findChild<QAction *>("CopyStructureFragment")->trigger();QVERIFY(QApplication::clipboard()->mimeData()->hasFormat("application/x-youngcraft-fragment-v1"));
    w.findChild<QAction *>("PasteStructureFragment")->trigger();QCOMPARE(w.m_view->molecule().atoms.size(),6);QCOMPARE(w.m_view->molecule().bonds.size(),3);
    QCOMPARE(w.m_view->molecule().bonds.last().order,2.0);QCOMPARE(w.selectedStructureAtoms(),QSet<int>({4,5}));
    w.undoDrawingEdit();QCOMPARE(w.m_view->molecule().atoms.size(),4);w.redoDrawingEdit();QCOMPARE(w.m_view->molecule().atoms.size(),6);
    w.openFile("tests/data/mvp.out");QTRY_VERIFY_WITH_TIMEOUT(!w.m_currentFile.isEmpty(),5000);
    const auto sourcePath=w.m_currentFile;const auto frames=w.m_data.frames;const auto sourceMolecule=w.m_view->molecule();
    const int source=w.m_documentTabs->currentIndex();w.m_view->setFocus();w.pasteStructureFragment();QVERIFY(w.m_documentTabs->currentIndex()!=source);QCOMPARE(w.m_data.molecule.atoms.size(),sourceMolecule.atoms.size()+2);
    w.m_documentTabs->setCurrentIndex(source);QCOMPARE(w.m_currentFile,sourcePath);QCOMPARE(w.m_data.frames.size(),frames.size());QCOMPARE(w.m_view->molecule().atoms.size(),sourceMolecule.atoms.size());
}
void SmokeTest::chemcraftGeneratedDialogsAndExtraction() {
    MainWindow w;w.show();Molecule m;m.atoms={{1,8,"O",{}},{2,1,"H",{.95f,0,0}},{3,1,"H",{-.3f,.9f,0}}};m.setBond(0,1,1);m.setBond(0,2,1);w.newDrawingDocument(m);w.setDrawingMode(false);
    bool checked=false;
    QTimer::singleShot(0,&w,[&]{auto*d=w.findChild<QDialog *>("StructureTextDialog");if(d){auto*p=d->findChild<QPlainTextEdit *>("StructureTextPreview");checked=p&&p->toPlainText().split('\n')[1].split(QRegularExpression("\\s+")).size()==3;d->reject();}});
    w.showZMatrix();QVERIFY(checked);checked=false;
    QTimer::singleShot(0,&w,[&]{auto*d=w.findChild<QDialog *>("GaussianInputDialog");if(d){auto*p=d->findChild<QPlainTextEdit *>("GaussianInputPreview");auto*spin=d->findChild<QSpinBox *>("GaussianMultiplicity");checked=p&&p->toPlainText().contains("0 1\nO");if(spin){spin->setValue(2);checked=checked&&p->toPlainText().isEmpty();}d->reject();}});
    w.createGaussianInput();QVERIFY(checked);
    const int source=w.m_documentTabs->currentIndex();const int tabs=w.m_documentTabs->count();checked=false;
    QTimer::singleShot(0,&w,[&]{auto*d=w.findChild<QDialog *>("FragmentExtractor");if(d){auto*l=d->findChild<QListWidget *>("ExtractFragmentList");checked=l&&l->count()==1;d->accept();}});
    w.extractStructureFragments();QVERIFY(checked);QCOMPARE(w.m_documentTabs->count(),tabs+1);QCOMPARE(w.m_data.molecule.atoms.size(),3);w.m_documentTabs->setCurrentIndex(source);QCOMPARE(w.m_data.molecule.atoms.size(),3);
}

void SmokeTest::atomNumbersToolbarLabelsEveryAtom() {
    MainWindow window;
    window.resize(1200, 850);
    window.show();
    const RenderStyle originalStyle = window.m_style;
    Molecule molecule;
    molecule.atoms = {{1, 8, "O", {-2, 0, 0}}, {2, 6, "C", {0, 0, 0}}, {3, 1, "H", {2, 0, 0}}};
    molecule.atoms[1].label = QStringLiteral("CA");
    window.newDrawingDocument(molecule);
    window.setDrawingMode(false);
    RenderStyle style = StyleManager::preset(QStringLiteral("Youngcraft Studio"));
    style.showLabels = false;
    style.showHydrogen = true;
    window.applyStyle(style);
    auto *action = window.findChild<QAction *>(QStringLiteral("ShowAtomNumbers"));
    window.m_structureTabs->setCurrentIndex(0);
    auto *display = toolButtonForAction(window.m_structureDock, action);
    QVERIFY(action && display && display->isVisible());
    QVERIFY(!action->isChecked());
    auto *view = window.m_view;
    view->bestView();
    QTest::qWait(150);
    const bool visual = QGuiApplication::platformName() != "offscreen";
    QImage before;
    if (visual) { QVERIFY(view->isValid()); before = view->image(); }
    QTest::mouseClick(display, Qt::LeftButton);
    QVERIFY(action->isChecked());
    QVERIFY(view->style().showLabels);
    if (visual) {
        QTest::qWait(100);
        const QImage after = view->image();
        for (const Atom &atom : view->molecule().atoms) {
            const auto screen = view->project(atom.position, view->viewProjection());
            const qreal scale = after.devicePixelRatio();
            const QRect region(qRound((screen.x() - 16) * scale), qRound((screen.y() - 16) * scale),
                               qRound(32 * scale), qRound(32 * scale));
            QVERIFY2(before.copy(region) != after.copy(region), "Each atom, including named atoms and H, must render a label");
        }
        const QString capture = qEnvironmentVariable("YOUNGCRAFT_ATOM_NUMBERS_CAPTURE");
        if (!capture.isEmpty()) { QVERIFY(window.grab().save(capture)); QVERIFY(after.save(capture + ".scene.png")); }
    }
    const int numberedTab = window.m_documentTabs->currentIndex();
    window.newDrawingDocument(molecule);
    style.showLabels = false;
    window.applyStyle(style);
    QVERIFY(!action->isChecked());
    window.m_documentTabs->setCurrentIndex(numberedTab);
    QVERIFY(action->isChecked());
    QVERIFY(window.m_view->style().showLabels);
    action->trigger();
    QVERIFY(!action->isChecked());
    QVERIFY(!window.m_view->style().showLabels);
    window.applyStyle(originalStyle);
}

void SmokeTest::atomClicksMeasureDistanceAngleAndDihedral() {
    if (QGuiApplication::platformName() == "offscreen") QSKIP("Picking requires a Windows OpenGL viewport");
    MainWindow window;
    window.resize(1300, 850);
    window.show();
    Molecule molecule;
    molecule.atoms = {{1, 8, "O", {-1, 0, 0}}, {2, 6, "C", {0, 0, 0}},
                      {3, 7, "N", {0, 1, 0}}, {4, 1, "H", {1, 1, 1}}, {5, 6, "C", {2, -1, 0}}};
    window.newDrawingDocument(molecule);
    window.setDrawingMode(false);
    const RenderStyle originalStyle = window.m_style;
    auto style = originalStyle;
    style.showLabels = true; style.showHydrogen = true;
    window.applyStyle(style);
    auto *view = window.m_view;
    view->bestView();
    QTest::qWait(150);
    QVERIFY(view->isValid());
    // Viewing measurements must preserve calculation frames and avoid an editing copy.
    Frame frame;
    for (const Atom &atom : molecule.atoms) frame.coordinates << atom.position;
    window.m_data.frames = {frame, frame};
    window.m_currentFile = QStringLiteral("measurement-fixture.out");
    const int tabCount = window.m_documentTabs->count();
    auto *readout = window.m_measurementReadout;
    auto click = [&](int id) {
        const auto p = view->project(view->molecule().atoms[id].position, view->viewProjection());
        QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(qRound(p.x()), qRound(p.y())));
    };
    click(0); click(1);
    QCOMPARE(window.m_geometrySelection, QVector<int>({0, 1}));
    QVERIFY(readout->text().contains(QStringLiteral("거리 1.00 Å")));
    click(1); // A second click removes an atom from the ordered measurement.
    QCOMPARE(window.m_geometrySelection, QVector<int>({0}));
    QCOMPARE(view->selectedAtom(), -1);
    QVERIFY(!view->selection().atoms.contains(1));
    QVERIFY(!readout->text().contains(QStringLiteral("거리 ")));
    click(1);
    QCOMPARE(window.m_geometrySelection, QVector<int>({0, 1}));
    click(2);
    QVERIFY(readout->text().contains(QStringLiteral("각도 90.00°")));
    click(3);
    QVERIFY(readout->text().contains(QStringLiteral("이면각 135.00°")));
    QVERIFY(readout->text().contains(QStringLiteral("1 → 2 → 3 → 4")));
    click(3);
    QCOMPARE(window.m_geometrySelection, QVector<int>({0, 1, 2}));
    QVERIFY(!readout->text().contains(QStringLiteral("이면각 ")));
    click(1); // Removing a middle atom preserves the remaining click order.
    QCOMPARE(window.m_geometrySelection, QVector<int>({0, 2}));
    QVERIFY(readout->text().contains(QStringLiteral("거리 1.41 Å")));
    QVERIFY(!readout->text().contains(QStringLiteral("각도 ")));
    click(0); click(2);
    QVERIFY(window.m_geometrySelection.isEmpty());
    QCOMPARE(view->geometrySelection(), QVector<int>());
    QVERIFY(readout->text().contains(QStringLiteral("원자를 순서대로 클릭")));
    click(0); click(1); click(2); click(3);
    const QString capture = qEnvironmentVariable("YOUNGCRAFT_MEASUREMENT_CAPTURE");
    if (!capture.isEmpty()) { QVERIFY(view->image().save(capture + ".scene.png")); QTest::qWait(50); QVERIFY(window.grab().save(capture)); }
    auto coordinates = frame.coordinates;
    coordinates[0] = {-2, 0, 0};
    view->setCoordinates(coordinates);
    QVERIFY(readout->text().contains(QStringLiteral("거리 2.00 Å")));
    QCOMPARE(window.m_documentTabs->count(), tabCount);
    QCOMPARE(window.m_data.frames.size(), 2);
    QCOMPARE(window.m_data.frames[0].coordinates, frame.coordinates);
    QVERIFY(window.m_undoMolecules.isEmpty());
    const int measuredTab = window.m_documentTabs->currentIndex();
    window.newDrawingDocument(molecule);
    QVERIFY(window.m_geometrySelection.isEmpty());
    window.m_documentTabs->setCurrentIndex(measuredTab);
    QCOMPARE(window.m_geometrySelection, QVector<int>({0, 1, 2, 3}));
    QCOMPARE(window.m_measurementReadout, readout);
    QVERIFY(readout->text().contains(QStringLiteral("거리 2.00 Å")));
    click(4);
    QCOMPARE(window.m_geometrySelection, QVector<int>({0,1,2,3,4}));
    QCOMPARE(view->selection().atoms.size(),5);
    QVERIFY(readout->text().contains(QStringLiteral("거리 ")));
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
    QVERIFY(window.m_geometrySelection.isEmpty());
    click(0); click(1);
    window.m_moleculeWorkspace->findChild<QPushButton *>(QStringLiteral("ClearAtomMeasurement"))->click();
    QVERIFY(window.m_geometrySelection.isEmpty());
    click(0); click(1); click(2); click(3);
    coordinates[2] = {1, 0, 0}; // Collinear centers have no defined dihedral.
    view->setCoordinates(coordinates);
    QVERIFY(readout->text().contains(QStringLiteral("이면각 정의 불가")));
    window.applyStyle(originalStyle);
}

void SmokeTest::mmChunkRegression() {
    qputenv("YOUNGCRAFT_MM_TEST_WORKER", QDir(QCoreApplication::applicationDirPath()).filePath("mock_mm_worker.exe").toUtf8());
    MainWindow window; window.show();
    Molecule molecule; molecule.atoms={{1,6,"C",{0,0,0}},{2,6,"C",{2,0,0}}}; molecule.setBond(0,1,1);
    window.newDrawingDocument(molecule);
    QTimer::singleShot(10, &window, [&] {
        auto *dialog=window.findChild<QWidget *>("MMOptimizationPanel"); QVERIFY(dialog);
        dialog->findChild<QSpinBox *>("MMMaximumSteps")->setValue(120); dialog->findChild<QPushButton *>("MMStart")->click();
    });
    window.startMMOptimization();
    QTRY_VERIFY_WITH_TIMEOUT(window.m_mmStep >= 48, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(window.m_mmStep > 50, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(window.m_mmStep, 120, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(!window.m_mmProcess, 5000);
    QCOMPARE(window.m_mmState,MainWindow::MMState::Finished);
    QVector<int> expected={0,8,48,56,64,72,80,88,96,104,112,120};
    QCOMPARE(window.m_mmSeries.size(),expected.size());
    for(int i=0;i<expected.size();++i)QCOMPARE(window.m_mmSeries[i].x(),double(expected[i]));
    QCOMPARE(window.m_view->molecule().atoms[0].position,window.m_data.molecule.atoms[0].position);
    QVERIFY(qAbs(window.m_data.molecule.atoms[0].position.x()-0.120f)<1e-6f);
    qunsetenv("YOUNGCRAFT_MM_TEST_WORKER");
}


void SmokeTest::mmPauseResumeStopConverge() {
    qputenv("YOUNGCRAFT_MM_TEST_WORKER", QDir(QCoreApplication::applicationDirPath()).filePath("mock_mm_worker.exe").toUtf8());
    MainWindow window; window.show(); Molecule m;m.atoms={{1,6,"C",{0,0,0}},{2,6,"C",{2,0,0}}};m.setBond(0,1,1);window.newDrawingDocument(m);
    auto begin=[&]{window.startMMOptimization();auto *d=window.findChild<QWidget *>("MMOptimizationPanel");QVERIFY(d);d->findChild<QSpinBox *>("MMMaximumSteps")->setValue(120);d->findChild<QPushButton *>("MMStart")->click();};
    bool paused=false;
    connect(window.m_mmProgress,&QProgressBar::valueChanged,&window,[&](int step){if(step==48&&!paused){paused=true;window.m_mmPauseButton->click();}});
    begin();QTRY_VERIFY_WITH_TIMEOUT(paused,3000);QCOMPARE(window.m_mmState,MainWindow::MMState::Paused);QTest::qWait(200);QCOMPARE(window.m_mmStep,48);
    window.m_mmPauseButton->click();QTRY_VERIFY_WITH_TIMEOUT(window.m_mmStep>=64,3000);window.m_mmStopButton->click();QTRY_COMPARE_WITH_TIMEOUT(window.m_mmState,MainWindow::MMState::Finished,3000);int stopped=window.m_mmStep;QTest::qWait(100);QCOMPARE(window.m_mmStep,stopped);QVERIFY(!window.m_mmProcess);
    qputenv("YOUNGCRAFT_FAKE_CONVERGE","1");begin();QTRY_VERIFY_WITH_TIMEOUT(!window.m_mmProcess,3000);QCOMPARE(window.m_mmStep,64);QCOMPARE(window.m_mmState,MainWindow::MMState::Finished);QVERIFY(window.m_mmLabel->text().contains(QStringLiteral("수렴")));
    qunsetenv("YOUNGCRAFT_FAKE_CONVERGE");qunsetenv("YOUNGCRAFT_MM_TEST_WORKER");
}
void SmokeTest::xtbCompactColumns(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("XtbCompactColumns");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow owner;owner.resize(1400,900);owner.show();
    Molecule molecule;molecule.atoms={{1,28,"Ni",{0,0,0}},{2,7,"N",{1.8f,0,0}},{3,8,"O",{0,1.8f,0}}};molecule.rebuildBonds();
    auto *dialog=new XtbDialog(molecule,QString(),&owner,2,false,{0,1});dialog->show();QTest::qWait(50);
    auto *left=dialog->findChild<QScrollArea*>("XtbSettingsScroll");auto *right=dialog->findChild<QScrollArea*>("XtbResultsScroll");
    auto *editor=static_cast<ConstraintEditor*>(dialog->findChild<QWidget*>("ConstraintEditor"));auto *plot=dialog->findChild<PlotWidget*>("XtbEnergyPlot");QVERIFY(left&&right&&editor&&plot);
    auto *add=editor->findChild<QPushButton*>("AddConstraint");auto *remove=editor->findChild<QPushButton*>("RemoveConstraint");
    auto *freeze=editor->findChild<QPushButton*>("FreezeSelectedAtoms");auto *all=editor->findChild<QPushButton*>("FreezeAllAtoms");
    add->click();QVector<GeometryConstraint> constraints;QString error;QVERIFY2(editor->read(constraints,&error),qPrintable(error));QCOMPARE(constraints.size(),1);QCOMPARE(constraints[0].atoms,QVector<int>({0,1}));
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_XTB_LAYOUT_REVIEW");if(!capture.isEmpty())QDir().mkpath(capture);
    for(int theme=0;theme<6;++theme){
        owner.findChild<QAction*>(QString("PastelTheme%1").arg(theme))->trigger();
        for(const QSize size:{QSize(960,720),QSize(860,620)}){
            dialog->resize(size);QTest::qWait(35);QCOMPARE(dialog->size(),size);
            QVERIFY(left->mapTo(dialog,QPoint(left->width(),0)).x()<=right->mapTo(dialog,QPoint()).x());
            QCOMPARE(left->horizontalScrollBar()->maximum(),0);QCOMPARE(right->horizontalScrollBar()->maximum(),0);
            QCOMPARE(add->y(),remove->y());QCOMPARE(freeze->y(),all->y());QCOMPARE(add->x(),freeze->x());QVERIFY(freeze->y()>add->y());
            int y=-1;
            for(const char *name:{"XtbStart","XtbCancel","XtbOutput","XtbApply","XtbOpen","XtbSave"}){
                auto *button=dialog->findChild<QPushButton*>(name);QVERIFY(button);const auto position=button->mapTo(dialog,QPoint());
                QVERIFY(dialog->rect().contains(QRect(position,button->size())));if(y<0)y=position.y();QCOMPARE(position.y(),y);
            }
            auto *table=editor->findChild<QTableWidget*>("ConstraintTable");QVERIFY(plot->mapTo(dialog,QPoint()).x()>=right->mapTo(dialog,QPoint()).x());
            QVERIFY(plot->mapTo(dialog,QPoint()).y()>table->mapTo(dialog,table->rect().bottomLeft()).y());
            if(size.height()>=700)QVERIFY(right->viewport()->rect().contains(plot->mapTo(right->viewport(),plot->rect().bottomRight())));
            if(!capture.isEmpty())QVERIFY(dialog->grab().save(capture+QString("/theme-%1-%2x%3.png").arg(theme).arg(size.width()).arg(size.height())));
        }
    }
    dialog->close();
}

void SmokeTest::xtbGeometryActions() {
    qputenv("YOUNGCRAFT_XTB_TEST_RUNTIME",QDir::current().filePath("xtb_runtime").toUtf8());
    MainWindow window;window.show();Molecule m;m.atoms={{1,8,"O",{0,0,0}},{2,1,"H",{1.2f,0,0}},{3,1,"H",{-0.3f,1.1f,0}}};m.rebuildBonds();window.newDrawingDocument(m);
    window.startXtbCalculation();auto *dialog=window.findChild<XtbDialog *>();QVERIFY(dialog);dialog->findChild<QPushButton *>("XtbStart")->click();
    QVERIFY(!dialog->findChild<QWidget*>("ConstraintEditor")->isEnabled());QVERIFY(dialog->findChild<QPushButton*>("XtbOutput")->isEnabled());
    auto *apply=dialog->findChild<QPushButton *>("XtbApply");QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(),15000);QVERIFY(dialog->findChild<QLabel *>("XtbStatus")->text().contains(QStringLiteral("최적화 수렴")));QVERIFY(dialog->findChild<QWidget*>("ConstraintEditor")->isEnabled());
    auto *plot=dialog->findChild<PlotWidget *>("XtbEnergyPlot");QVERIFY(plot->series().size()>1);
    QVERIFY(dialog->grab().save(QDir::current().filePath("docs/xtb-dialog.png")));
    auto optimized=window.m_view->molecule();QVERIFY((optimized.atoms[1].position-m.atoms[1].position).length()>0.01f);
    apply->click();QCOMPARE(window.m_data.molecule.atoms[1].position,optimized.atoms[1].position);
    int tabs=window.m_documentTabs->count();dialog->findChild<QPushButton *>("XtbOpen")->click();QCOMPARE(window.m_documentTabs->count(),tabs+1);QCOMPARE(window.m_data.molecule.atoms[1].position,optimized.atoms[1].position);
    dialog->close();qunsetenv("YOUNGCRAFT_XTB_TEST_RUNTIME");
}

void SmokeTest::crystalModesSwitchStructures() {
    MainWindow window;
    window.show();
    window.openFile(QStringLiteral("tests/data/structure/inversion.cif"));
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QVERIFY(tabs);
    QTRY_VERIFY_WITH_TIMEOUT(tabs->currentWidget()->property("loadFinished").toBool(), 10000);
    QVERIFY(tabs->currentWidget()->property("loadSuccess").toBool());
    auto *view = tabs->currentWidget()->findChild<MoleculeView *>();
    auto *mode = tabs->currentWidget()->findChild<QComboBox *>(QStringLiteral("CrystalDisplayMode"));
    auto *cell = tabs->currentWidget()->findChild<QCheckBox *>(QStringLiteral("CrystalCellVisible"));
    QVERIFY(view); QVERIFY(mode); QVERIFY(cell);
    QCOMPARE(view->molecule().atoms.size(), 1);
    mode->setCurrentIndex(1);
    QCOMPARE(view->molecule().atoms.size(), 2);
    mode->setCurrentIndex(2);
    QCOMPARE(view->molecule().atoms.size(), 16);
    cell->click();
    QVERIFY(!cell->isChecked());
    mode->setCurrentIndex(0);
    QCOMPARE(view->molecule().atoms.size(), 1);
}

void SmokeTest::styleEditorPreviewsImmediately() {
    MoleculeView view;
    const RenderStyle starting = StyleManager::preset(QStringLiteral("Youngcraft Studio"));
    view.setStyle(starting);
    StyleEditorDialog dialog(starting);
    connect(&dialog, &StyleEditorDialog::styleApplied, &view, &MoleculeView::setStyle);
    auto *atom = dialog.findChild<QSlider *>(QStringLiteral("AtomScaleSlider"));
    auto *bond = dialog.findChild<QSlider *>(QStringLiteral("BondRadiusSlider"));
    auto *preset = dialog.findChild<QComboBox *>(QStringLiteral("StylePreset"));
    QVERIFY(atom); QVERIFY(bond); QVERIFY(preset);
    atom->setValue(62);
    QVERIFY(qAbs(view.style().atomScale - 0.62f) < 0.001f);
    bond->setValue(19);
    QVERIFY(qAbs(view.style().bondRadius - 0.19f) < 0.001f);
    preset->setCurrentText(QStringLiteral("Dark Studio"));
    QCOMPARE(view.style().name, QStringLiteral("Dark Studio"));
    QCOMPARE(view.style().background, QColor(Qt::black));
}

void SmokeTest::mmStopTerminatesWorker() {
    const QDir applicationDir(QCoreApplication::applicationDirPath());
    const bool bundledWorker = QFileInfo::exists(applicationDir.filePath(QStringLiteral("mm_runtime/YoungcraftMM.exe"))) ||
        QFileInfo::exists(applicationDir.filePath(QStringLiteral("dist/mm_runtime/YoungcraftMM.exe")));
    if (!bundledWorker) {
        const QString python = QStandardPaths::findExecutable(QStringLiteral("python"));
        if (python.isEmpty()) QSKIP("No MM worker available");
        QProcess probe;
        probe.start(python, {QStringLiteral("-c"), QStringLiteral("import rdkit")});
        if (!probe.waitForFinished(3000) || probe.exitCode() != 0) QSKIP("No RDKit development fallback");
    }
    MainWindow window;
    window.show();
    QAction *draw = nullptr, *optimize = nullptr;
    for (QAction *action : window.findChildren<QAction *>()) {
        if (action->text() == QStringLiteral("새 구조 그리기")) draw = action;
        if (action->objectName() == QStringLiteral("MMOptimizationAction")) optimize = action;
    }
    QVERIFY(draw); QVERIFY(optimize);
    draw->trigger();
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QVERIFY(tabs);
    auto *view = tabs->currentWidget()->findChild<MoleculeView *>();
    QVERIFY(view);
    QVERIFY(QMetaObject::invokeMethod(view, "canvasClicked", Q_ARG(QVector3D, QVector3D(0, 0, 0))));
    QVERIFY(QMetaObject::invokeMethod(view, "canvasClicked", Q_ARG(QVector3D, QVector3D(1.8f, 0, 0))));
    QVERIFY(QMetaObject::invokeMethod(view, "bondDrawn", Q_ARG(int, 0), Q_ARG(int, 1), Q_ARG(QVector3D, QVector3D())));
    QTimer acceptDialog;
    connect(&acceptDialog, &QTimer::timeout, &window, [&] {
        for (QWidget *widget : QApplication::topLevelWidgets())
            if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
                acceptDialog.stop(); dialog->accept(); return;
            }
    });
    acceptDialog.start(10);
    optimize->trigger();
    window.findChild<QPushButton *>("MMStart")->click();
    acceptDialog.stop();
    auto *stop = tabs->currentWidget()->findChild<QPushButton *>(QStringLiteral("MMStopButton"));
    QVERIFY(stop);
    stop->click();
    QCOMPARE(stop->text(), QStringLiteral("닫기"));
    for (QProcess *process : window.findChildren<QProcess *>()) {
        QPointer<QProcess> guarded=process;
        QTRY_VERIFY_WITH_TIMEOUT(!guarded || guarded->state()==QProcess::NotRunning, 3000);
    }
}

void SmokeTest::autoHydrogenFollowsDrawingEditsAndUndo() {
    MainWindow window;
    window.resize(1100, 750);
    window.show();
    QAction *newDrawing = nullptr, *undo = nullptr, *redo = nullptr;
    for (QAction *action : window.findChildren<QAction *>()) {
        if (action->text() == QStringLiteral("새 구조 그리기")) newDrawing = action;
        if (action->text() == QStringLiteral("실행 취소")) undo = action;
        if (action->text().section('\t',0,0) == QStringLiteral("다시 실행")) redo = action;
    }
    QVERIFY(newDrawing); QVERIFY(undo); QVERIFY(redo);
    QVERIFY(window.findChild<QMenu *>("EditBuildMenu")->actions().contains(newDrawing));
    QCOMPARE(newDrawing->shortcut(),QKeySequence(QStringLiteral("Ctrl+N")));
    QCOMPARE(redo->shortcuts().size(),2);
    for(const auto &key:redo->shortcuts())QVERIFY(redo->text().section('\t',1).contains(key.toString(QKeySequence::NativeText)));
    const auto menuReview=qEnvironmentVariable("YOUNGCRAFT_EDIT_MENU_REVIEW");
    if(!menuReview.isEmpty()) {
        QDir().mkpath(menuReview);
        auto *edit=window.findChild<QMenu *>("EditMenu");QVERIFY(edit);
        edit->popup(window.mapToGlobal(QPoint(30,40)));QTest::qWait(60);
        QVERIFY(edit->height()<600);
        QVERIFY(edit->grab().save(menuReview+"/edit-menu.png"));
        edit->hide();
        auto *build=window.findChild<QMenu *>("StructureBuildMenu");QVERIFY(build);
        build->popup(window.mapToGlobal(QPoint(30,40)));QTest::qWait(60);
        QVERIFY(build->grab().save(menuReview+"/build-menu.png"));build->hide();
    }
    newDrawing->trigger();
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QVERIFY(tabs);
    auto *view = tabs->currentWidget()->findChild<MoleculeView *>();
    auto *autoH = window.m_drawBar->findChild<QPushButton *>(QStringLiteral("AutoHydrogenButton"));
    auto *order = window.m_drawBar->findChild<QComboBox *>(QStringLiteral("DrawBondOrder"));
    QVERIFY(view); QVERIFY(autoH); QVERIFY(order);
    QVERIFY(!autoH->isChecked());
    autoH->click();
    QVERIFY(QMetaObject::invokeMethod(view, "canvasClicked", Q_ARG(QVector3D, QVector3D(0, 0, 0))));
    QCOMPARE(view->molecule().atoms.size(), 5); // CH4
    QVERIFY(QMetaObject::invokeMethod(view, "canvasClicked", Q_ARG(QVector3D, QVector3D(1.5f, 0, 0))));
    QVERIFY(QMetaObject::invokeMethod(view, "bondDrawn", Q_ARG(int, 0), Q_ARG(int, 1), Q_ARG(QVector3D, QVector3D())));
    QCOMPARE(view->molecule().atoms.size(), 8); // CH3-CH3
    order->setCurrentIndex(1);
    QVERIFY(QMetaObject::invokeMethod(view, "bondDrawn", Q_ARG(int, 0), Q_ARG(int, 1), Q_ARG(QVector3D, QVector3D())));
    QCOMPARE(view->molecule().atoms.size(), 6); // CH2=CH2
    order->setCurrentIndex(2);
    QVERIFY(QMetaObject::invokeMethod(view, "bondDrawn", Q_ARG(int, 0), Q_ARG(int, 1), Q_ARG(QVector3D, QVector3D())));
    QCOMPARE(view->molecule().atoms.size(), 4); // HC#CH
    undo->trigger();
    QCOMPARE(view->molecule().atoms.size(), 6);
    redo->trigger();
    QCOMPARE(view->molecule().atoms.size(), 4);
    auto *deleteAtom = window.m_drawBar->findChild<QPushButton *>("Document_deleteAtom");
    QVERIFY(deleteAtom);
    view->setSelectedAtom(1);
    deleteAtom->click();
    QCOMPARE(view->molecule().atoms.size(), 5); // Remaining C becomes CH4.
    undo->trigger();
    QCOMPARE(view->molecule().atoms.size(), 4);
    QAction *clearBonds = nullptr;
    for (QAction *action : window.findChildren<QAction *>())
        if (action->text() == QStringLiteral("모든 결합 지우기")) clearBonds = action;
    QVERIFY(clearBonds);
    clearBonds->trigger();
    QCOMPARE(view->molecule().atoms.size(), 10); // Two isolated CH4 fragments.
}

void SmokeTest::toolbarDrawKeepsWindowOpen() {
    QCoreApplication::setOrganizationName(QStringLiteral("Youngcraft"));
    QCoreApplication::setApplicationName(QStringLiteral("Youngcraft"));
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    format.setAlphaBufferSize(8);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
    MainWindow window;
    window.resize(1000, 700);
    window.show();
    auto *toolbar = window.findChild<QToolBar *>(QStringLiteral("MainToolBar"));
    QVERIFY(toolbar);
    auto *draw = window.findChild<QAction *>("DrawCurrentStructure");
    QVERIFY(draw);
    window.m_structureDock->show();
    window.m_structureTabs->setCurrentIndex(1);
    auto *button = toolButtonForAction(window.m_structureDock, draw);
    QVERIFY(button && button->isVisible());
    const auto chooseDraw = [&] {
        QTest::mouseClick(button, Qt::LeftButton);
        QVERIFY(window.m_structureDock->isAncestorOf(window.m_drawBar));
        QVERIFY(window.m_drawBar->isVisible());
    };

    chooseDraw();
    QVERIFY(window.isVisible());
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QVERIFY(tabs);
    MoleculeView *view = nullptr;
    for (MoleculeView *candidate : window.findChildren<MoleculeView *>())
        if (candidate->isVisible()) view = candidate;
    QVERIFY(view);
    window.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier,
                      QPoint(view->width() / 2, view->height() / 2));
    QCOMPARE(view->molecule().atoms.size(), 1);

    const QString path = QFINDTESTDATA("tests/data/mvp.out");
    QVERIFY(!path.isEmpty());
    window.openFile(path);
    QTRY_VERIFY_WITH_TIMEOUT(tabs->currentWidget()->property("loadFinished").toBool(), 15000);
    chooseDraw();
    QVERIFY(window.isVisible());
    QCOMPARE(tabs->count(), 2);
    view = tabs->currentWidget()->findChild<MoleculeView *>();
    QVERIFY(view);
    QVERIFY(view->isVisible());
    QVERIFY(!view->molecule().atoms.isEmpty());
    auto *finish = window.m_drawBar->findChild<QPushButton *>("Document_finishDrawing");
    QVERIFY(finish);
    finish->click();
    QVERIFY(!window.m_drawingMode && !window.m_drawBar->isVisible());
}

void SmokeTest::drawingModeCreatesAtomsAndBonds() {
    MainWindow window;
    window.resize(1000, 700);
    window.show();
    QAction *drawAction = nullptr;
    QAction *optimizeAction = nullptr;
    for (QAction *action : window.findChildren<QAction *>()) {
        if (action->text() == QStringLiteral("새 구조 그리기")) drawAction = action;
        if (action->objectName() == QStringLiteral("MMOptimizationAction")) optimizeAction = action;
    }
    QVERIFY(drawAction);
    QVERIFY(optimizeAction);
    QCOMPARE(optimizeAction->shortcut(), QKeySequence(QStringLiteral("Ctrl+Alt+O")));
    drawAction->trigger();
    MoleculeView *view = nullptr;
    for (MoleculeView *candidate : window.findChildren<MoleculeView *>())
        if (candidate->isVisible()) view = candidate;
    QVERIFY(view);
    QVERIFY(view->isVisible());
    const QPoint center(view->width() / 2, view->height() / 2);
    window.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(view->molecule().atoms.size(), 1);
    QComboBox *element = nullptr;
    for (QComboBox *candidate : window.findChildren<QComboBox *>(QStringLiteral("DrawElementSelector")))
        if (candidate->isVisible()) element = candidate;
    QVERIFY(element);
    element->setCurrentText(QStringLiteral("O"));
    const QPoint second = center + QPoint(90, 0);
    QTest::mousePress(view, Qt::LeftButton, Qt::NoModifier, center);
    for (int x = 5; x < 90; x += 5) {
        const QPoint current = center + QPoint(x, 0);
        QMouseEvent rapidMove(QEvent::MouseMove, QPointF(current), QPointF(view->mapToGlobal(current)),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(view, &rapidMove);
        QCOMPARE(view->molecule().atoms.size(), 1); // Preview must not edit topology.
    }
    QMouseEvent drawMove(QEvent::MouseMove, QPointF(second), QPointF(view->mapToGlobal(second)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view, &drawMove);
    QVERIFY(view->hasGhostBond());
    auto *previewOverlay = view->findChild<QWidget *>(QStringLiteral("DrawingPreviewOverlay"));
    QVERIFY(previewOverlay);
    if (view->isValid()) {
        const QImage preview = previewOverlay->grab().toImage();
        QVERIFY(!preview.isNull());
        int paintedPixels = 0;
        for (int y = 0; y < preview.height(); ++y)
            for (int x = 0; x < preview.width(); ++x)
                if (preview.pixelColor(x, y).alpha() > 0) ++paintedPixels;
        QVERIFY(paintedPixels > 20);
        QVERIFY(paintedPixels < preview.width() * preview.height() / 3);
    }
    const QVector3D previewPosition = view->ghostPosition();
    QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, second);
    QCOMPARE(view->molecule().atoms.size(), 2);
    QCOMPARE(view->molecule().bonds.size(), 1);
    QCOMPARE(view->molecule().atoms[1].atomicNumber, 8);
    QVERIFY(view->molecule().bonds[0].manual);
    QVERIFY((view->molecule().atoms[1].position - previewPosition).length() < 0.02f);
    element->setCurrentText(QStringLiteral("N"));
    QPushButton *replace = nullptr;
    for (QPushButton *candidate : window.findChildren<QPushButton *>())
        if (candidate->isVisible() && (candidate->text() == QStringLiteral("선택 원자 → 현재 원소") || candidate->accessibleName() == QStringLiteral("선택 원자 → 현재 원소"))) replace = candidate;
    QVERIFY(replace);
    replace->click();
    QCOMPARE(view->molecule().atoms[1].atomicNumber, 7);
    QCOMPARE(view->molecule().atoms[1].element, QStringLiteral("N"));
    QVERIFY(!view->molecule().atoms[1].colorOverride.isValid());
    QPushButton *finish = nullptr;
    for (QPushButton *candidate : window.findChildren<QPushButton *>())
        if (candidate->isVisible() && (candidate->text() == QStringLiteral("그리기 종료") || candidate->accessibleName() == QStringLiteral("그리기 종료"))) finish = candidate;
    QVERIFY(finish);
    finish->click();
    QTest::keyClick(&window, Qt::Key_D, Qt::ControlModifier);
    const QPoint moved = center + QPoint(0, 55);
    const QVector3D beforeDrag = view->molecule().atoms[0].position;
    QTest::mousePress(view, Qt::LeftButton, Qt::NoModifier, center);
    QMouseEvent atomMove(QEvent::MouseMove, QPointF(moved), QPointF(view->mapToGlobal(moved)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view, &atomMove);
    QCOMPARE(view->molecule().atoms[0].position, beforeDrag);
    QVERIFY(view->m_groupTranslation.length() > 0.1f);
    QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, moved);
    QVERIFY((view->molecule().atoms[0].position-beforeDrag).length()>0.1f);
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python"));
    const QDir applicationDir(QCoreApplication::applicationDirPath());
    const bool bundledWorker = QFileInfo::exists(applicationDir.filePath(QStringLiteral("mm_runtime/YoungcraftMM.exe"))) ||
        QFileInfo::exists(applicationDir.filePath(QStringLiteral("dist/mm_runtime/YoungcraftMM.exe")));
    if (!bundledWorker) {
        if (python.isEmpty()) return;
        QProcess probe;
        probe.start(python, {QStringLiteral("-c"), QStringLiteral("import rdkit")});
        if (!probe.waitForFinished(3000) || probe.exitCode() != 0) return;
    }
    const float initialDistance = (view->molecule().atoms[1].position - view->molecule().atoms[0].position).length();
    auto *progress = window.m_moleculeWorkspace->findChild<QProgressBar *>(QStringLiteral("MMProgress"));
    QVERIFY(progress);
    int firstWorkerStep = -1;
    float distanceAtFirstWorkerStep = -1.0f;
    connect(progress, &QProgressBar::valueChanged, &window, [&](int value) {
        if (value > 0 && firstWorkerStep < 0) {
            firstWorkerStep = value;
            distanceAtFirstWorkerStep = (view->molecule().atoms[1].position -
                                         view->molecule().atoms[0].position).length();
        }
    });
    QTimer acceptDialog;
    connect(&acceptDialog, &QTimer::timeout, &window, [&acceptDialog] {
        for (QWidget *widget : QApplication::topLevelWidgets())
            if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
                acceptDialog.stop(); dialog->accept(); return;
            }
    });
    acceptDialog.start(10);
    optimizeAction->trigger();
    window.findChild<QPushButton *>("MMStart")->click();
    acceptDialog.stop();
    QPushButton *pause = nullptr;
    for (QPushButton *candidate : window.findChildren<QPushButton *>(QStringLiteral("MMPauseButton")))
        if (candidate->isVisible()) pause = candidate;
    QVERIFY(pause);
    pause->click();
    QTest::qWait(200);
    QCOMPARE((view->molecule().atoms[1].position - view->molecule().atoms[0].position).length(), initialDistance);
    pause->click();
    QTRY_VERIFY_WITH_TIMEOUT(qAbs((view->molecule().atoms[1].position - view->molecule().atoms[0].position).length()
                               - initialDistance) > 0.001f, 5000);
    QTest::mousePress(view, Qt::RightButton, Qt::NoModifier, center);
    const QPoint rotated = center + QPoint(24, 18);
    QMouseEvent rotateMove(QEvent::MouseMove, QPointF(rotated), QPointF(view->mapToGlobal(rotated)),
                           Qt::NoButton, Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(view, &rotateMove);
    QTest::mouseRelease(view, Qt::RightButton, Qt::NoModifier, rotated);
    QWheelEvent zoomEvent(QPointF(center), QPointF(view->mapToGlobal(center)), QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(view, &zoomEvent);
    QVERIFY(view->isVisible());
    QCOMPARE(firstWorkerStep, 8);
    QCOMPARE(distanceAtFirstWorkerStep, initialDistance);
    PlotWidget *mmPlot = nullptr;
    for (PlotWidget *candidate : window.findChildren<PlotWidget *>(QStringLiteral("MMEnergyPlot")))
        if (candidate->isVisible()) mmPlot = candidate;
    QVERIFY(mmPlot);
    QVERIFY(mmPlot->series().size() >= 2);
    QVERIFY(progress);
    QVERIFY(progress->value() > 0);
    QCOMPARE(mmPlot->series().first().x(), 0.0);
    QCOMPARE(mmPlot->series()[1].x(), 8.0); // Graph contains worker snapshots, not display frames.
}

void SmokeTest::geometryEditsFromOrdinarySelection() {
    MainWindow w;
    w.show();
    Molecule molecule;
    molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1,0,0}},{3,6,"C",{1,1,0}},
                    {4,7,"N",{2,1,1}},{5,1,"H",{3,1,1}}};
    molecule.setBond(0,1,1);molecule.setBond(1,2,1);molecule.setBond(2,3,1);molecule.setBond(3,4,1);
    w.newDrawingDocument(molecule);w.setDrawingMode(false);
    QVERIFY(!w.m_geometryMode);
    QVERIFY(!w.m_geometryBar->isVisible());
    w.handleGeometryPick(0);w.handleGeometryPick(1);
    w.m_moleculeWorkspace->findChild<QPushButton *>("EditAtomMeasurement")->click();
    QVERIFY(w.m_geometryBar->isVisible());
    auto apply=[&](const QString &kind,double value){
        w.m_geometryBar->findChild<QDoubleSpinBox *>("Geometry"+kind+"Value")->setValue(value);
        w.m_geometryBar->findChild<QPushButton *>("Geometry"+kind+"Apply")->click();
    };
    apply("Distance",1.75);
    QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),0,1)-1.75)<1e-4);
    QCOMPARE(w.m_view->molecule().atoms[0].position,molecule.atoms[0].position);
    QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),3,4)-1)<1e-5);
    QCOMPARE(w.m_undoMolecules.size(),1);
    w.undoDrawingEdit();QCOMPARE(w.m_view->molecule().atoms[1].position,molecule.atoms[1].position);
    w.redoDrawingEdit();QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),0,1)-1.75)<1e-4);
    w.undoDrawingEdit();
    w.m_geometryBar->setMovement(GeometryMoveMode::OneSide,true);
    apply("Distance",1.5);
    QCOMPARE(w.m_view->molecule().atoms[1].position,molecule.atoms[1].position);
    QVERIFY(qAbs(w.m_view->molecule().atoms[0].position.x()+0.5f)<1e-5);
    w.undoDrawingEdit();
    w.m_geometryBar->setMovement(GeometryMoveMode::BothSides,false);
    apply("Distance",2);
    QVERIFY(qAbs(w.m_view->molecule().atoms[0].position.x()+0.5f)<1e-5);
    QVERIFY(qAbs(w.m_view->molecule().atoms[1].position.x()-1.5f)<1e-5);
    w.undoDrawingEdit();
    w.m_geometryBar->setMovement(GeometryMoveMode::OneSide,false);
    w.handleGeometryPick(2);w.handleGeometryPick(3);
    apply("Angle",120);
    QVERIFY(qAbs(GeometryEditor::angleDegrees(w.m_view->molecule(),0,1,2)-120)<1e-3);
    apply("Dihedral",-75);
    QVERIFY(qAbs(GeometryEditor::dihedralDegrees(w.m_view->molecule(),0,1,2,3)+75)<1e-3);
    w.m_geometryBar->setMovement(GeometryMoveMode::OneSide,true);
    const auto fixed=w.m_view->molecule().atoms[3].position;
    apply("Dihedral",60);
    QVERIFY(qAbs(GeometryEditor::dihedralDegrees(w.m_view->molecule(),0,1,2,3)-60)<1e-3);
    QCOMPARE(w.m_view->molecule().atoms[3].position,fixed);
    QVERIFY(w.m_measurementReadout->text().contains(QStringLiteral("이면각 60.00°")));
    const auto path=qEnvironmentVariable("YOUNGCRAFT_GEOMETRY_CAPTURE");
    if(!path.isEmpty()){QTest::qWait(100);QVERIFY(w.grab().save(path));}
    w.clearGeometrySelection();QVERIFY(!w.m_geometryBar->isVisible());
}

void SmokeTest::geometryEditPreservesCalculation() {
    MainWindow w;
    Molecule molecule;
    molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1,0,0}},{3,7,"N",{1,1,0}}};
    molecule.setBond(0,1,1);molecule.setBond(1,2,1);
    w.newDrawingDocument(molecule);w.setDrawingMode(false);
    Frame frame;for(const auto &a:molecule.atoms)frame.coordinates<<a.position;
    w.m_data.frames={frame,frame};w.m_currentFile="geometry-fixture.out";
    auto *original=w.m_view;
    const int originalTab=w.m_documentTabs->currentIndex();
    const int count=w.m_documentTabs->count();
    w.handleGeometryPick(0);w.handleGeometryPick(1);
    QCOMPARE(w.m_documentTabs->count(),count);
    w.m_geometryBar->setMovement(GeometryMoveMode::OneSide,true);
    w.m_geometryBar->findChild<QDoubleSpinBox *>("GeometryDistanceValue")->setValue(2);
    w.m_geometryBar->findChild<QPushButton *>("GeometryDistanceApply")->click();
    QCOMPARE(w.m_documentTabs->count(),count+1);
    QVERIFY(w.m_view!=original);QVERIFY(!w.m_drawingMode);
    QCOMPARE(w.m_geometrySelection,QVector<int>({0,1}));
    QVERIFY(w.m_geometryBar->moveFirstSide());
    QCOMPARE(w.m_view->molecule().atoms[1].position,molecule.atoms[1].position);
    QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),0,1)-2)<1e-4);
    QCOMPARE(w.m_undoMolecules.size(),1);
    w.undoDrawingEdit();QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),0,1)-1)<1e-4);
    w.m_documentTabs->setCurrentIndex(originalTab);
    QCOMPARE(w.m_view,original);QCOMPARE(w.m_data.frames.size(),2);
    QCOMPARE(w.m_data.frames[0].coordinates,frame.coordinates);
    QCOMPARE(w.m_view->molecule().atoms[0].position,molecule.atoms[0].position);
    QVERIFY(w.m_undoMolecules.isEmpty());
}

void SmokeTest::geometryEditInputAndInvalidSelection() {
    MainWindow w;w.show();
    Molecule molecule;
    molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1,0,0}},{3,6,"C",{2,0,0}},{4,7,"N",{3,0,0}}};
    w.newDrawingDocument(molecule);w.setDrawingMode(false);
    for(int i=0;i<4;++i)w.handleGeometryPick(i);
    w.m_moleculeWorkspace->findChild<QPushButton *>("EditAtomMeasurement")->click();
    QVERIFY(!w.m_geometryBar->findChild<QDoubleSpinBox *>("GeometryDihedralValue")->isEnabled());
    auto *value=w.m_geometryBar->findChild<QDoubleSpinBox *>("GeometryDistanceValue");
    value->setFocus();value->setValue(1.5);
    w.m_geometryBar->findChild<QPushButton *>("GeometryDistanceShow")->setFocus();
    QCoreApplication::processEvents();
    QVERIFY(w.m_undoMolecules.isEmpty()); // Leaving the field never changes coordinates.
    value->setFocus();value->selectAll();QTest::keyClicks(value,"1.75");QTest::keyClick(value,Qt::Key_Return);
    QCOMPARE(w.m_undoMolecules.size(),1);
    QVERIFY(qAbs(GeometryEditor::distance(w.m_view->molecule(),0,1)-1.75)<1e-4);
    w.m_geometryBar->setSelection(molecule,{0,99}); // Stale indices must not be measured.
    QVERIFY(!w.m_geometryBar->findChild<QWidget *>("GeometryDistanceRow")->isEnabled());
    molecule.setBond(0,1,1);molecule.setBond(1,2,1);molecule.setBond(2,0,1);
    w.newDrawingDocument(molecule);w.setDrawingMode(false);w.handleGeometryPick(0);w.handleGeometryPick(1);
    w.m_geometryBar->findChild<QDoubleSpinBox *>("GeometryDistanceValue")->setValue(2);
    w.m_geometryBar->findChild<QPushButton *>("GeometryDistanceApply")->click();
    QVERIFY(w.m_undoMolecules.isEmpty());
    QCOMPARE(w.m_view->molecule().atoms[1].position,molecule.atoms[1].position);
}

void SmokeTest::geometryMeasureBarWorks(){
    Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1,0,0}},{3,6,"C",{1,1,0}},{4,7,"N",{2,1,1}}};molecule.setBond(0,1,1);molecule.setBond(1,2,1);molecule.setBond(2,3,1);GeometryMeasureBar bar;bar.show();bar.setSelection(molecule,{0,1});QVERIFY(bar.findChild<QWidget*>(QStringLiteral("GeometryDistanceRow"))->isEnabled());QVERIFY(!bar.findChild<QWidget*>(QStringLiteral("GeometryAngleRow"))->isEnabled());bar.setSelection(molecule,{0,1,2,3});QVERIFY(bar.findChild<QWidget*>(QStringLiteral("GeometryAngleRow"))->isEnabled());QVERIFY(bar.findChild<QWidget*>(QStringLiteral("GeometryDihedralRow"))->isEnabled());bool shown=false;connect(&bar,&GeometryMeasureBar::distanceVisibilityChanged,this,[&](bool value){shown=value;});bar.findChild<QPushButton*>(QStringLiteral("GeometryDistanceShow"))->click();QVERIFY(shown);MeasurementTextStyle style;style.fontFamily="Arial";style.pointSize=14;style.bold=true;style.color=Qt::red;bool styleChanged=false;connect(&bar,&GeometryMeasureBar::measurementStyleChanged,this,[&](MeasurementKind,const MeasurementTextStyle&){styleChanged=true;});bar.setMeasurementStyle(MeasurementKind::Distance,style);QVERIFY(styleChanged);QCOMPARE(bar.measurementStyle(MeasurementKind::Distance).color,QColor(Qt::red));
    MoleculeView view;view.setMolecule(molecule);Molecule edited=view.molecule();QVERIFY(GeometryEditor::setDistance(edited,0,1,2.0,GeometryMoveMode::OneSide));QVector<QVector3D> coordinates;for(const Atom&atom:edited.atoms)coordinates<<atom.position;view.setCoordinates(coordinates);QVERIFY(qAbs(GeometryEditor::distance(view.molecule(),0,1)-2.0)<1e-4);
}

void SmokeTest::startupSplashShowsAndCloses() {
    const auto oldVersion=QCoreApplication::applicationVersion();QCoreApplication::setApplicationVersion("0.1.0");
    auto restoreVersion=qScopeGuard([&]{QCoreApplication::setApplicationVersion(oldVersion);});
    QVERIFY(!QPixmap(":/youngcraft/icon.png").isNull());
    StartupSplash splash;splash.show();QVERIFY(splash.isVisible());
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_SPLASH_PREVIEW");if(!preview.isEmpty())QVERIFY(splash.grab().save(preview));
    QWidget ready;QElapsedTimer elapsed;elapsed.start();bool completed=false;
    splash.finishWhenReady(&ready,[&]{completed=true;});
    QTest::mouseClick(&splash,Qt::LeftButton);
    QTest::qWait(250);QVERIFY(splash.isVisible());QVERIFY(!ready.isVisible());QVERIFY(!completed);
    QTRY_VERIFY_WITH_TIMEOUT(!splash.isVisible(),1200);QVERIFY(ready.isVisible());QVERIFY(completed);QVERIFY(elapsed.elapsed()>=480);QVERIFY(elapsed.elapsed()<1000);
    qInfo()<<"Splash-to-workspace elapsed milliseconds:"<<elapsed.elapsed();
    // Readiness before show must not consume the logo's visible interval.
    StartupSplash delayed;QWidget delayedReady;bool delayedCompleted=false;
    delayed.finishWhenReady(&delayedReady,[&]{delayedCompleted=true;});
    QTest::qWait(550);QVERIFY(!delayedReady.isVisible());QVERIFY(!delayedCompleted);
    QElapsedTimer delayedElapsed;delayedElapsed.start();delayed.show();
    QTest::qWait(250);QVERIFY(delayed.isVisible());QVERIFY(!delayedReady.isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(delayedCompleted,1000);QVERIFY(delayedElapsed.elapsed()>=480);
    // Time spent preparing the workspace after painting is not added again.
    StartupSplash prepared;prepared.show();QTest::qWait(300);
    QWidget preparedReady;QElapsedTimer remaining;remaining.start();
    prepared.finishWhenReady(&preparedReady);
    QTRY_VERIFY_WITH_TIMEOUT(preparedReady.isVisible(),700);
    QVERIFY(remaining.elapsed()<450);
    StartupSplash closedEarly;closedEarly.show();auto *temporary=new QWidget;temporary->show();closedEarly.finishWhenReady(temporary);delete temporary;
    QTRY_VERIFY_WITH_TIMEOUT(!closedEarly.isVisible(),4500);
}

void SmokeTest::mainWindowStartsWithKoreanUi() {
    MainWindow window;
    window.show();
    QVERIFY(window.windowTitle().contains(QStringLiteral("Youngcraft")));
    QVERIFY(!window.menuBar()->actions().isEmpty());
    auto *geometryBar=window.findChild<GeometryMeasureBar *>(QStringLiteral("GeometryMeasureBar"));QVERIFY(geometryBar);QVERIFY(!geometryBar->isVisible());QAction *geometryAction=nullptr;for(QAction *menuAction:window.menuBar()->actions()){if(!menuAction->menu())continue;for(QAction *action:menuAction->menu()->actions())if(action->text()==QStringLiteral("구조 파라미터(거리/각/이면각) 표시"))geometryAction=action;}QVERIFY(geometryAction);geometryAction->trigger();QVERIFY(geometryBar->isVisible());QVERIFY(geometryAction->isChecked());geometryAction->trigger();QVERIFY(!geometryBar->isVisible());
}

void SmokeTest::moleculeViewKeepsManualBond() {
    Molecule molecule;
    molecule.atoms = {{1, 6, QStringLiteral("C"), {0, 0, 0}},
                      {2, 8, QStringLiteral("O"), {1.2f, 0, 0}}};
    molecule.setBond(0, 1, 2.0);
    MoleculeView view;
    view.setMolecule(molecule);
    QCOMPARE(view.molecule().bonds.size(), 1);
    QCOMPARE(view.molecule().bonds.first().order, 2.0);
}

void SmokeTest::supersamplingSmoothsDiagonalEdges() {
    if (QGuiApplication::platformName()=="offscreen") QSKIP("Requires OpenGL");
    MoleculeView view; view.resize(400,300);
    Molecule m; m.atoms={{1,6,"C",{-2,-.8f,0}},{2,6,"C",{2,.8f,0}}}; m.setBond(0,1,1);
    RenderStyle style; style.perspective=false; style.lighting=false; style.shadows=false;
    style.depthCueing=false; style.showAxes=false; style.showLabels=false;
    style.background=Qt::white; style.bondColorMode=BondColorMode::Uniform;
    style.bondColor=Qt::blue; style.elementColors[6]=Qt::blue;
    style.atomScale=.035f; style.bondRadius=.035f; style.antiAliasing=false;
    view.setStyle(style); view.setMolecule(m); view.show(); QTest::qWait(100);
    QVERIFY(view.isValid());
    const auto projection=view.viewProjection();
    const QImage before=view.image(false); QVERIFY(!view.m_supersamplingActive);
    style.antiAliasing=true; view.setStyle(style);
    const QImage after=view.image(false); QVERIFY(view.m_supersamplingActive);
    const QSize native(qRound(view.width()*view.devicePixelRatioF()),qRound(view.height()*view.devicePixelRatioF()));
    QCOMPARE(view.m_supersampleFbo->size(),native*2); QCOMPARE(after.size(),before.size());
    QCOMPARE(view.viewProjection(),projection);
    auto partialPixels=[](const QImage &im) {
        int count=0;
        for (int y=0;y<im.height();++y) for(int x=0;x<im.width();++x) {
            auto c=im.pixelColor(x,y);
            if(c.red()>5 && c.red()<250 && c.blue()>250) ++count;
        }
        return count;
    };
    QVERIFY2(partialPixels(after)>partialPixels(before)+100,"Diagonal silhouette needs intermediate coverage pixels");
    QCOMPARE(view.pickAtom(QPoint(qRound(view.project(m.atoms[0].position,projection).x()),
                                 qRound(view.project(m.atoms[0].position,projection).y()))),0);
    const QImage transparent=view.image(true); QCOMPARE(transparent.size(),after.size());
    QCOMPARE(transparent.pixelColor(0,0).alpha(),0);
    int partialAlpha=0;
    for(int y=0;y<transparent.height();++y) for(int x=transparent.width()/5;x<transparent.width()*4/5;++x) {
        const auto c=transparent.pixelColor(x,y);
        if(c.alpha()>20 && c.alpha()<235) {
            ++partialAlpha;
            QVERIFY2(c.blue()>245,"Transparent edge colors must retain straight alpha without dark halos");
        }
    }
    qInfo() << "edge pixels before/after/transparent:" << partialPixels(before) << partialPixels(after) << partialAlpha << "format:" << transparent.format();
    QVERIFY(partialAlpha>100);
    style.background=Qt::black; view.setStyle(style);
    const auto darkTransparent=view.image(true);
    QCOMPARE(darkTransparent,transparent);
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_SSAA_CAPTURE_DIR");
    if(!capture.isEmpty()) {
        QVERIFY(before.save(capture+"/before.png")); QVERIFY(after.save(capture+"/after.png"));
        QVERIFY(transparent.save(capture+"/transparent.png"));
    }
    view.resize(512,384); QTest::qWait(60); view.image();
    QCOMPARE(view.m_supersampleFbo->size(),QSize(qRound(512*view.devicePixelRatioF()),qRound(384*view.devicePixelRatioF()))*2);
    const QImage previewBefore=view.image(false,true);
    const auto publication=view.imageAtSize(QSize(1600,1200),true,true);
    QCOMPARE(publication.size(),QSize(1600,1200));
    QCOMPARE(publication.dotsPerMeterX(),qRound(600.0/0.0254));
    QCOMPARE(publication.pixelColor(0,0).alpha(),0);
    int publicationEdges=0;
    for(int y=0;y<publication.height();++y) for(int x=0;x<publication.width();++x) {
        const auto c=publication.pixelColor(x,y);
        if(c.alpha()>20 && c.alpha()<235) {
            ++publicationEdges;
            QVERIFY2(c.blue()>245,"Offscreen export must keep blue on partially covered transparent edges");
        }
    }
    QVERIFY(publicationEdges>200);
    QCOMPARE(view.image(false,true),previewBefore);
    QTemporaryDir publicationDir; QVERIFY(publicationDir.isValid());
    QString publicationError;
    const auto publicationPath=publicationDir.filePath("publication.png");
    QVERIFY2(ImageExporter::saveImage(publication,publicationPath,1,true,&publicationError),qPrintable(publicationError));
    const QImage savedPublication(publicationPath);
    QCOMPARE(savedPublication.size(),publication.size());
    QCOMPARE(savedPublication.dotsPerMeterX(),publication.dotsPerMeterX());
    QCOMPARE(savedPublication.pixelColor(0,0).alpha(),0);
    view.makeCurrent(); QCOMPARE(view.glGetError(),GLenum(GL_NO_ERROR)); view.doneCurrent();
}

void SmokeTest::rendersShadowAndTransparentCapture() {
    Molecule molecule;
    molecule.atoms = {{1, 6, QStringLiteral("C"), {-0.7f, 0, 0}},
                      {2, 8, QStringLiteral("O"), {0.7f, 0, 0}}};
    molecule.setBond(0, 1, 2.0);
    MoleculeView view; view.resize(320, 240);
    RenderStyle style; style.shadows = true; style.shadowStrength = 0.5f; style.twoLights = false;
    view.setStyle(style); view.setMolecule(molecule); view.show();
    QTest::qWait(100);
    if (!view.isValid()) QSKIP("offscreen platform did not provide an OpenGL context");
    const QImage image = view.image(true);
    QVERIFY(!image.isNull());
    QVERIFY(image.hasAlphaChannel());
}

void SmokeTest::copiesMoleculeWithTransparentBackground() {
    QClipboard *clipboard = QApplication::clipboard();
    QVERIFY(clipboard);
    auto *previous = new QMimeData;
    if (const QMimeData *mime = clipboard->mimeData()) {
        for (const QString &format : mime->formats()) previous->setData(format, mime->data(format));
    }
    const auto restoreClipboard = qScopeGuard([&] { clipboard->setMimeData(previous); });

    MainWindow window;
    window.resize(900, 700);
    window.setProperty("appPattern", -1);
    Molecule molecule;
    molecule.atoms = {{1, 6, "C", {0, 0, 0}}};
    window.newDrawingDocument(molecule);
    window.setDrawingMode(false);
    window.show();
    QTest::qWait(100);
    if (!window.m_view->isValid()) QSKIP("No OpenGL context");
    QAction *copy = nullptr;
    for (QAction *action : window.findChildren<QAction *>())
        if (action->text() == QStringLiteral("이미지 복사") && !action->shortcut().isEmpty()) copy = action;
    QVERIFY(copy);
    QToolBar *toolbar = window.findChild<QToolBar *>(QStringLiteral("MainToolBar"));
    QVERIFY(toolbar);
    QAction *toolbarCopy = nullptr;
    for (QAction *action : toolbar->actions())
        if (action->text() == QStringLiteral("이미지 복사")) toolbarCopy = action;
    QVERIFY(toolbarCopy);

    int method = 0;
    for (const QColor &background : {QColor(Qt::white), QColor(31, 41, 59), QColor(Qt::black)}) {
        RenderStyle style = window.m_view->style();
        style.background = background;
        style.showLabels = false;
        window.m_view->setStyle(style);
        clipboard->clear();
        if (method == 0) copy->trigger();
        else if (method == 1) QTest::keyClick(&window, Qt::Key_I, Qt::ControlModifier);
        else toolbarCopy->trigger();
        ++method;
        const QString pasteProbe = qEnvironmentVariable("YOUNGCRAFT_WORD_PASTE_PROBE");
        if (!pasteProbe.isEmpty()) {
            QProcess probe;
            probe.start(QStringLiteral("powershell.exe"), {QStringLiteral("-NoProfile"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-File"), pasteProbe});
            QTRY_VERIFY_WITH_TIMEOUT(probe.state() == QProcess::NotRunning, 45000);
            QVERIFY2(probe.exitCode() == 0, probe.readAllStandardError().constData());
            const QImage pasted(qEnvironmentVariable("YOUNGCRAFT_WORD_PASTE_IMAGE"));
            QVERIFY2(!pasted.isNull(), probe.readAllStandardOutput().constData());
            QCOMPARE(pasted.pixelColor(0, 0).alpha(), 0);
        }
        const QMimeData *mime = clipboard->mimeData();
        QVERIFY(mime);
        QVERIFY(mime->hasFormat(QStringLiteral("image/png")));
#ifdef Q_OS_WIN
        const QString windowsPng = QStringLiteral("application/x-qt-windows-mime;value=\"PNG\"");
        QVERIFY(mime->hasFormat(windowsPng));
        QVERIFY(!mime->hasFormat(QStringLiteral("application/x-qt-image")));
        const QImage native = QImage::fromData(mime->data(windowsPng), "PNG");
#else
        QVERIFY(mime->hasImage());
        const QImage native = clipboard->image();
#endif
        const QImage png = QImage::fromData(mime->data(QStringLiteral("image/png")), "PNG");
        for (const QImage &image : {native, png}) {
            QVERIFY(!image.isNull());
            QVERIFY(image.hasAlphaChannel());
            QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
            QCOMPARE(ImageExporter::contentBounds(image), image.rect());
            QVERIFY(image.width() < 3000 && image.height() < 3000);
            int opaquePixels = 0;
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    if (image.pixelColor(x, y).alpha() == 255) ++opaquePixels;
            QVERIFY2(opaquePixels > 100, "Copied molecule must retain visible opaque atoms");
        }
        const QImage viewport = window.m_view->image();
        QCOMPARE(viewport.pixelColor(0, 0), background);
    }
}

void SmokeTest::rendersOutwardFacesAndMolecularDepth() {
    Molecule molecule;
    molecule.atoms = {{1, 6, "C", {0,0,0}}};
    MoleculeView view; view.resize(480, 400);
    RenderStyle style;
    style.shadows = false; style.twoLights = false; style.perspective = false;
    style.lightDirection = {0,0,1}; style.ambientIntensity = 0.08f;
    style.elementColors.insert(6, QColor(160,180,210));
    view.setStyle(style); view.setMolecule(molecule); view.show();
    QTest::qWait(100);
    if (!view.isValid()) QSKIP("No OpenGL context");
    QVERIFY(view.m_glReady); QVERIFY(view.m_program->isLinked()); QVERIFY(view.m_shadowReady);
    view.makeCurrent();
    const auto checkWinding = [](QOpenGLBuffer &buffer, int count) {
        QVector<MoleculeView::Vertex> vertices(count);
        if (!buffer.bind() || !buffer.read(0, vertices.data(), count * int(sizeof(MoleculeView::Vertex)))) return false;
        buffer.release();
        for (int i = 0; i < count; i += 3) {
            const auto &a = vertices[i], &b = vertices[i+1], &c = vertices[i+2];
            const auto normal = QVector3D::crossProduct(b.position-a.position, c.position-a.position);
            if (normal.lengthSquared() > 1e-10f && QVector3D::dotProduct(normal, a.normal+b.normal+c.normal) <= 0) return false;
        }
        return true;
    };
    QVERIFY(checkWinding(view.m_sphereBuffer, view.m_sphereVertexCount));
    QVERIFY(checkWinding(view.m_sphereMediumBuffer, view.m_sphereMediumVertexCount));
    QVERIFY(checkWinding(view.m_sphereLowBuffer, view.m_sphereLowVertexCount));
    view.doneCurrent();
    const auto sample = [&](const QImage &image, const QVector3D &position) {
        const auto point = view.project(position, view.viewProjection());
        return image.pixelColor(qRound(point.x()*view.devicePixelRatioF()), qRound(point.y()*view.devicePixelRatioF()));
    };
    const QImage sphere = view.image();
    const QColor center = sample(sphere, {}), edge = sample(sphere, {0.35f,0,0});
    QVERIFY2(center.red() > edge.red() + 12, "Front hemisphere should have a visible key highlight and shading gradient");
    molecule.atoms = {{1,6,"C",{-1,0,1}}, {2,6,"C",{1,0,-1}}};
    style.lighting = false; style.depthCueing = true; style.depthCueStrength = 0.6f;
    style.background = Qt::white; style.elementColors.insert(6, QColor(100,150,200));
    view.setStyle(style); view.setMolecule(molecule);
    const QImage depth = view.image();
    const QColor near = sample(depth, molecule.atoms[0].position), far = sample(depth, molecule.atoms[1].position);
    QVERIFY2(far.red() > near.red() + 30, "Depth cue must separate front and rear atoms rather than washing out the whole molecule");
    view.m_distance *= 2; view.m_orthoScale *= 1.4f;
    const QImage zoomed = view.image();
    QVERIFY(qAbs(sample(zoomed, molecule.atoms[0].position).red() - near.red()) <= 3);
    QVERIFY(qAbs(sample(zoomed, molecule.atoms[1].position).red() - far.red()) <= 3);
}

void SmokeTest::rendersContinuousBondsAndCleanSphereShadows() {
    Molecule molecule; molecule.atoms = {{1,6,"C",{0,0,0}}};
    MoleculeView view; view.resize(400,400);
    RenderStyle style; style.perspective = false; style.twoLights = false;
    style.shadowStrength = 0.65f; style.elementColors.insert(6, QColor(160,180,210));
    view.setStyle(style); view.setMolecule(molecule); view.show(); QTest::qWait(100);
    if (!view.isValid()) QSKIP("No OpenGL context");
    QVERIFY(view.m_glReady); QVERIFY(view.m_shadowReady);
    for (const float pixelRadius : {15.0f, 35.0f, 100.0f}) {
        const float radius = view.atomVisualRadius(molecule.atoms[0]);
        view.m_orthoScale = radius * view.height() / (2*pixelRadius);
        style.shadows = false; view.setStyle(style); const QImage clean = view.image();
        style.shadows = true; view.setStyle(style); const QImage shadow = view.image();
        int tested = 0, discontinuities = 0;
        const float r = pixelRadius * view.devicePixelRatioF();
        for (int y = 0; y < clean.height(); ++y) for (int x = 0; x < clean.width(); ++x) {
            const float dx = x-clean.width()/2.0f, dy = y-clean.height()/2.0f;
            if (dx*dx+dy*dy > r*r*0.85f*0.85f) continue;
            const QColor a = clean.pixelColor(x,y), b = shadow.pixelColor(x,y);
            ++tested;
            if (qAbs(a.red()-b.red()) > 15 || qAbs(a.green()-b.green()) > 15 || qAbs(a.blue()-b.blue()) > 15) ++discontinuities;
        }
        QVERIFY2(discontinuities <= qMax(4, tested/100), qPrintable(QString("Unexpected self-shadow patches: %1/%2 at radius %3").arg(discontinuities).arg(tested).arg(pixelRadius)));
    }
    const auto sample = [&](const QImage &image, const QVector3D &position) {
        const auto point = view.project(position, view.viewProjection());
        return image.pixelColor(qRound(point.x()*view.devicePixelRatioF()), qRound(point.y()*view.devicePixelRatioF()));
    };
    molecule.atoms = {{1,6,"C",{-1,0,0}}, {2,7,"N",{1,0,0}}}; molecule.setBond(0,1,1);
    style.lighting = false; style.shadows = false; style.showBondOrders = false;
    style.elementColors.insert(6, QColor(100,160,220)); style.elementColors.insert(7, QColor(240,140,160));
    view.setStyle(style); view.setMolecule(molecule); view.m_orthoScale = 1.6f;
    const QImage bond = view.image();
    const auto center = sample(bond, {}), left = sample(bond, {-0.008f,0,0}), right = sample(bond, {0.008f,0,0});
    QVERIFY(center.red() > 130 && center.red() < 205);
    QVERIFY2(qAbs(right.red()-left.red()) < 20, "Endpoint colors should transition smoothly across the bond center");
    // Exposed tube ends must remain solid even when the atom is smaller than the bond.
    molecule.atoms = {{1,6,"C",{0,0,-0.8f}}, {2,6,"C",{0,0,0.8f}}}; molecule.bonds.clear(); molecule.setBond(0,1,1);
    style.displayMode = DisplayMode::Stick; style.atomScale = 0.03f; style.bondRadius = 0.20f;
    view.setStyle(style); view.setMolecule(molecule);
    QVERIFY2(sample(view.image(), {0.1f,0,0.8f}).red() < 220, "Rounded tube cap should fill the exposed end");
    // Rounding short dashes must not fill the configured gaps.
    molecule.atoms = {{1,6,"C",{-1.5f,0,0}}, {2,6,"C",{1.5f,0,0}}}; molecule.bonds.clear(); molecule.setBond(0,1,1);
    molecule.bonds[0].style = BondStyle::Dashed;
    style.showBondOrders = true; style.bondRadius = 0.10f; style.dashLength = 0.30f; style.dashGap = 0.25f;
    view.setStyle(style); view.setMolecule(molecule); view.m_orthoScale = 1.8f;
    const QImage dashed = view.image();
    QVERIFY(sample(dashed, {-1.35f,0,0}).red() < 220);
    QVERIFY(sample(dashed, {-1.075f,0,0}).red() > 235);
}

void SmokeTest::presetsApplyFromMenuAndEditor() {
    const QString previousOrganization = QCoreApplication::organizationName();
    const QString previousApplication = QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");
    QCoreApplication::setApplicationName("PresetUiIsolation");
    QSettings().clear();
    {
        MainWindow window;
        auto *menu = window.findChild<QMenu *>("DisplayPresets");
        QVERIFY(menu);
        for (const auto &name : {QStringLiteral("GaussView 유사 스타일"), QStringLiteral("CPK / Space-Filling"), QStringLiteral("Dark Studio")}) {
            QAction *chosen = nullptr;
            for (auto *action : menu->actions()) if (action->text() == name) chosen = action;
            QVERIFY(chosen); chosen->trigger();
            QCOMPARE(window.m_view->style().name, name);
            QCOMPARE(StyleManager::load().name, name);
        }
        StyleEditorDialog editor(window.m_style);
        connect(&editor, &StyleEditorDialog::styleApplied, window.m_view, &MoleculeView::setStyle);
        auto *combo = editor.findChild<QComboBox *>("StylePreset");
        auto *scale = editor.findChild<QDoubleSpinBox *>("SpaceFillingScale");
        auto *relative = editor.findChild<QCheckBox *>("CameraRelativeLights");
        QVERIFY(combo); QVERIFY(scale); QVERIFY(relative);
        combo->setCurrentText(QStringLiteral("CPK / Space-Filling"));
        QVERIFY(scale->isEnabled()); scale->setValue(0.85);
        QVERIFY(qAbs(window.m_view->style().spaceFillingScale - 0.85f) < 0.001f);
        relative->setChecked(false); QVERIFY(!window.m_view->style().cameraRelativeLights);
        QPushButton *save = nullptr;
        for (auto *button : editor.findChildren<QPushButton *>()) if (button->text() == QStringLiteral("저장")) save = button;
        QVERIFY(save); save->click();
        QVERIFY(qAbs(StyleManager::load().spaceFillingScale - 0.85f) < 0.001f);
        combo->setCurrentText(QStringLiteral("사용자 지정"));
        QVERIFY(qAbs(window.m_view->style().spaceFillingScale - 0.85f) < 0.001f);
    }
    QSettings().clear();
    QCoreApplication::setOrganizationName(previousOrganization);
    QCoreApplication::setApplicationName(previousApplication);
}

void SmokeTest::exposesAnalysisPanelRestoreAction() {
    MainWindow window;
    QAction *restore = nullptr;
    QAction *resetLayout = nullptr;
    for (QAction *menuAction : window.menuBar()->actions()) {
        if (!menuAction->menu()) continue;
        for (QAction *action : menuAction->menu()->actions()) {
            if (action->objectName() == QStringLiteral("ToggleResultsSidebar")) restore = action;
            if (action->text() == QStringLiteral("기본 패널 배치 복원")) resetLayout = action;
        }
    }
    QVERIFY(restore);
    QVERIFY(restore->isCheckable());
    QVERIFY(resetLayout);
}

void SmokeTest::exposesKoreanFrequencyControls() {
    AnalysisPanel panel;
    QStringList checkTexts;
    for (QCheckBox *check : panel.findChildren<QCheckBox *>()) checkTexts.push_back(check->text());
    QVERIFY(checkTexts.contains(QStringLiteral("결합 다시 계산")));
    QVERIFY(checkTexts.contains(QStringLiteral("변위 벡터 표시")));
    QVERIFY(checkTexts.contains(QStringLiteral("스케일 벡터")));
    QStringList buttonTexts;
    for (QPushButton *button : panel.findChildren<QPushButton *>()) buttonTexts.push_back(button->accessibleName());
    QVERIFY(buttonTexts.contains(QStringLiteral("원래 구조")));
    QVERIFY(buttonTexts.contains(QStringLiteral("애니메이션 저장")));
    QCOMPARE(panel.frequencyAmplitude(), 0.5);
    QCOMPARE(panel.frequencyCycleTime(), 2.0);
    QVERIFY(panel.recountBondsEnabled());
}

void SmokeTest::styleEditorAdjustsAtomAndBondSizes() {
    Molecule molecule;
    molecule.atoms = {{1, 1, QStringLiteral("H"), {0, 0, 0}},
                      {2, 6, QStringLiteral("C"), {1.1f, 0, 0}}};
    StyleEditorDialog dialog(RenderStyle{}, &molecule);
    auto *atomScale = dialog.findChild<QSlider *>(QStringLiteral("AtomScaleSlider"));
    auto *element = dialog.findChild<QComboBox *>(QStringLiteral("ElementSelector"));
    auto *elementScale = dialog.findChild<QSlider *>(QStringLiteral("ElementRadiusSlider"));
    auto *bondRadius = dialog.findChild<QSlider *>(QStringLiteral("BondRadiusSlider"));
    QVERIFY(atomScale);
    QVERIFY(element);
    QVERIFY(elementScale);
    QVERIFY(bondRadius);

    atomScale->setValue(55);
    element->setCurrentIndex(element->findData(6));
    elementScale->setValue(140);
    bondRadius->setValue(18);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "apply", Qt::DirectConnection));

    const RenderStyle adjusted = dialog.style();
    QCOMPARE(adjusted.atomScale, 0.55f);
    QCOMPARE(adjusted.elementRadiusScales.value(6), 1.40f);
    QCOMPARE(adjusted.bondRadius, 0.18f);
}

void SmokeTest::frequencyAnimationEmitsDisplacedCoordinates() {
    AnimationController controller;
    FrequencyMode mode;
    mode.number = 1;
    mode.displacement = {{0.25f, 0.0f, 0.0f}};
    bool moved = false;
    connect(&controller, &AnimationController::coordinatesReady, this,
            [&moved](const QVector<QVector3D> &coordinates) {
                moved = !coordinates.isEmpty() && qAbs(coordinates.first().x()) > 0.001f;
            });
    controller.setFrequency({QVector3D(0.0f, 0.0f, 0.0f)}, mode);
    controller.setAmplitude(0.5);
    controller.setCycleTime(0.2);
    controller.startFrequency();
    QTRY_VERIFY_WITH_TIMEOUT(moved, 1000);
    controller.stop();
}

void SmokeTest::frequencySelectionStartsAnimationAndShowsCoordinates() {
    QString path = qEnvironmentVariable("YOUNGCRAFT_FREQUENCY_INPUT");
    if (path.isEmpty()) path = QFINDTESTDATA("tests/data/mvp.out");
    QVERIFY2(!path.isEmpty(), "tests/data/mvp.out를 찾지 못했습니다.");
    MainWindow window;
    window.show();
    window.openFile(path);
    QTRY_VERIFY_WITH_TIMEOUT(window.windowTitle().contains(QFileInfo(path).fileName()), 15000);
    auto *documents = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QTRY_VERIFY_WITH_TIMEOUT(documents->currentWidget()->property("loadFinished").toBool(), 15000);
    auto *panel = [&]() -> AnalysisPanel * { for (auto *p : window.findChildren<AnalysisPanel *>()) if (p->isVisible()) return p; return nullptr; }();
    auto *view = documents->currentWidget()->findChild<MoleculeView *>();
    auto *frequencyList = panel ? panel->findChild<QListWidget *>(QStringLiteral("FrequencyList")) : nullptr;
    QVERIFY(panel);
    QVERIFY(view);
    QVERIFY(frequencyList);
    QVERIFY(frequencyList->count() >= 2);
    const QVector3D base = view->molecule().atoms.first().position;
    frequencyList->setCurrentRow(1);
    QTRY_VERIFY_WITH_TIMEOUT((view->molecule().atoms.first().position - base).length() > 1.0e-5f, 1000);
    const QVector3D firstDisplaced = view->molecule().atoms.first().position;
    QTRY_VERIFY_WITH_TIMEOUT((view->molecule().atoms.first().position - firstDisplaced).length() > 1.0e-5f, 1000);

    auto *tabs = panel->findChild<QTabWidget *>();
    QVERIFY(tabs);
    int coordinateTab = -1;
    for (int i = 0; i < tabs->count(); ++i)
        if (tabs->tabText(i) == QStringLiteral("좌표")) coordinateTab = i;
    QVERIFY(coordinateTab >= 0);
    tabs->setCurrentIndex(coordinateTab);
    QApplication::processEvents();
    auto *coordinates = panel->findChild<QPlainTextEdit *>(QStringLiteral("CoordinateText"));
    QVERIFY(coordinates);
    QVERIFY(coordinates->toPlainText().contains(QStringLiteral("X")));
    QVERIFY(coordinates->toPlainText().contains(QStringLiteral("O")));
}

void SmokeTest::copiesCurrentElementCoordinates() {
    AnalysisPanel panel;
    auto *button = panel.findChild<QPushButton *>(QStringLiteral("CopyCoordinateButton"));
    QVERIFY(button);
    QVERIFY(!button->isEnabled());
    QApplication::clipboard()->setText(QStringLiteral("existing clipboard"));
    Molecule molecule;
    Atom carbon; carbon.index = 17; carbon.element = QStringLiteral("C");
    carbon.position = {1.25f, -2.5f, 0.0f};
    Atom chlorine; chlorine.index = 42; chlorine.element = QStringLiteral("Cl");
    chlorine.position = {-0.125f, 3.75f, 4.5f};
    molecule.atoms = {carbon, chlorine};
    molecule.title = QStringLiteral("Do not copy title");
    panel.setCoordinateMolecule(molecule);
    QVERIFY(button->isEnabled());
    button->click();
    auto rows = QApplication::clipboard()->text().trimmed().split('\n');
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows[0].simplified(), QStringLiteral("C 1.25000000 -2.50000000 0.00000000"));
    QCOMPARE(rows[1].simplified(), QStringLiteral("Cl -0.12500000 3.75000000 4.50000000"));
    molecule.atoms[0].position.setX(9.5f);
    panel.setCoordinateMolecule(molecule);
    button->click();
    QVERIFY(QApplication::clipboard()->text().contains(QStringLiteral("9.50000000")));
    auto refresh = connect(&panel, &AnalysisPanel::coordinateViewRequested, &panel, [&] {
        molecule.atoms[0].position.setX(-7.25f);
        panel.setCoordinateMolecule(molecule);
    });
    button->click();
    QVERIFY(QApplication::clipboard()->text().contains(QStringLiteral("-7.25000000")));
    disconnect(refresh);
    const auto copied = QApplication::clipboard()->text();
    panel.setCoordinateMolecule(Molecule{});
    QVERIFY(!button->isEnabled());
    button->click();
    QCOMPARE(QApplication::clipboard()->text(), copied);
}

void SmokeTest::loadsGaussianFixtureWithoutBlocking() {
    QString path = qEnvironmentVariable("YOUNGCRAFT_CAPTURE_INPUT");
    if (path.isEmpty()) path = QFINDTESTDATA("tests/data/mvp.out");
    QVERIFY2(!path.isEmpty(), "tests/data/mvp.out를 찾지 못했습니다.");
    MainWindow window;
    window.show();
    window.openFile(path);
    QTRY_VERIFY_WITH_TIMEOUT(window.windowTitle().contains(QFileInfo(path).fileName()), 15000);
    auto *documents = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QTRY_VERIFY_WITH_TIMEOUT(documents->currentWidget()->property("loadFinished").toBool(), 15000);
    if (MoleculeView *view = documents->currentWidget()->findChild<MoleculeView *>()) {
        view->setStyle(StyleManager::preset(QStringLiteral("Chemcraft 참조")));
        view->bestView();
        QApplication::processEvents();
        const QString capturePath = qEnvironmentVariable("YOUNGCRAFT_CAPTURE_PATH");
        if (!capturePath.isEmpty()) QVERIFY(view->image().save(capturePath));
    }
}

void SmokeTest::opensDroppedFilesFromViewportAndAnalysis() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QStringList names = {QStringLiteral("시작 구조.xyz"), QStringLiteral("다음 구조.xyz"),
                               QStringLiteral("마지막 구조.xyz")};
    QList<QUrl> files;
    for (const auto &name : names) {
        QFile file(directory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("1\nDrop fixture\nC 0 0 0\n") > 0);
        files.push_back(QUrl::fromLocalFile(file.fileName()));
    }

    MainWindow window;
    window.show();
    QSignalSpy loaded(&window, &MainWindow::documentLoaded);
    auto dropOn = [](QWidget *target, const QList<QUrl> &urls) {
        QMimeData mime;
        mime.setUrls(urls);
        const auto actions = Qt::CopyAction | Qt::MoveAction;
        const QPoint position(5, 5);
        QDragEnterEvent enter(position, actions, &mime, Qt::LeftButton, Qt::ShiftModifier);
        QApplication::sendEvent(target, &enter);
        QVERIFY(enter.isAccepted());
        QCOMPARE(enter.dropAction(), Qt::CopyAction);
        QDropEvent drop(position, actions, &mime, Qt::LeftButton, Qt::ShiftModifier);
        QApplication::sendEvent(target, &drop);
        QVERIFY(drop.isAccepted());
        QCOMPARE(drop.dropAction(), Qt::CopyAction);
    };
    dropOn(window.m_view, {files[0]});
    QTRY_COMPARE(loaded.size(), 1);
    window.m_analysis->setCurrentTab(7);
    auto *input = window.m_analysis->findChild<QPlainTextEdit *>("CoordinatePasteInput");
    QVERIFY(input && input->viewport()->acceptDrops());
    dropOn(input->viewport(), {files[0], files[1], QUrl::fromLocalFile(directory.path()), files[2]});
    QTRY_COMPARE(loaded.size(), 3);
    QCOMPARE(window.m_documentTabs->count(), 3);
    for (int i = 0; i < names.size(); ++i) {
        QCOMPARE(window.m_documentTabs->tabText(i), names[i]);
        QVERIFY(window.m_documentTabs->widget(i)->property("loadSuccess").toBool());
        QVERIFY(QFileInfo::exists(files[i].toLocalFile()));
    }
}

void SmokeTest::matchingAtomViewsSynchronize_data() {
    QTest::addColumn<bool>("perspective");
    QTest::newRow("orthographic") << false;
    QTest::newRow("perspective") << true;
}

void SmokeTest::matchingAtomViewsSynchronize() {
    QFETCH(bool, perspective);
    MainWindow w; w.resize(1200,850); w.show();
    auto *sync = w.findChild<QAction *>(QStringLiteral("SyncMatchingAtomViews"));
    QVERIFY(sync); QVERIFY(sync->isCheckable()); QVERIFY(!sync->isChecked());
    QVERIFY(toolButtonForAction(w.m_structureDock, sync));
    Molecule a; a.atoms = {{1,6,"C",{0,0,0}}, {2,8,"O",{2,0,0}}}; a.setBond(0,1,1);
    w.newDrawingDocument(a); w.setDrawingMode(false);
    auto *first = w.m_view; const int firstTab = w.m_documentTabs->currentIndex();
    Molecule b = a;
    for (auto &atom : b.atoms) atom.position += QVector3D(10,20,-30);
    b.atoms[1].visible = false;
    w.newDrawingDocument(b); w.setDrawingMode(false);
    auto *second = w.m_view; const int secondTab = w.m_documentTabs->currentIndex();
    Molecule c = a; c.atoms.push_back({3,1,"H",{0,1,0}});
    w.newDrawingDocument(c); w.setDrawingMode(false);
    auto *different = w.m_view;
    const CameraState differentBefore = different->cameraState();
    w.m_documentTabs->setCurrentIndex(firstTab);
    w.setPerspective(perspective);
    first->m_rotation = QQuaternion::fromEulerAngles(22,47,13);
    first->m_target = first->m_center + QVector3D(1,2,3);
    first->m_distance = 19; first->m_orthoScale = 4.5f;
    const auto firstCoordinates = first->molecule().atoms;
    const auto secondCoordinates = second->molecule().atoms;
    sync->trigger(); QVERIFY(sync->isChecked());
    auto compareCameras = [](MoleculeView *left, MoleculeView *right) {
        const auto l=left->cameraState(), r=right->cameraState();
        QVERIFY((l.centerOffset-r.centerOffset).length() < 1e-5f);
        QCOMPARE(l.rotation,r.rotation); QCOMPARE(l.distance,r.distance);
        QCOMPARE(l.orthoScale,r.orthoScale); QCOMPARE(l.perspective,r.perspective);
        QCOMPARE(l.perspectiveFov,r.perspectiveFov);
    };
    compareCameras(first,second);
    QCOMPARE(different->cameraState().rotation,differentBefore.rotation);
    QCOMPARE(different->cameraState().orthoScale,differentBefore.orthoScale);
    QVERIFY(!second->molecule().atoms[1].visible);
    first->rotateBy({25,-12}); first->panBy({8,5});
    compareCameras(first,second);
    QWheelEvent wheel(QPointF(100,100),QPointF(100,100),QPoint(),QPoint(0,120),
                      Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(first,&wheel);
    compareCameras(first,second);
    // Either tab can lead, and its projection must survive tab switches.
    w.m_documentTabs->setCurrentIndex(secondTab);
    second->rotateBy({-17,30}); second->panBy({-4,2});
    w.setPerspective(!perspective);
    auto style = w.m_style; style.perspectiveFov = 52; w.applyStyle(style);
    compareCameras(first,second);
    w.m_documentTabs->setCurrentIndex(firstTab);
    QCOMPARE(w.m_style.perspective,!perspective);
    QCOMPARE(w.m_style.perspectiveFov,52.0f);
    compareCameras(first,second);
    // New tabs and asynchronous file loads inherit the established camera.
    const auto beforeNewTab = first->cameraState();
    w.newDrawingDocument(b); w.setDrawingMode(false);
    compareCameras(first,w.m_view);
    QCOMPARE(first->cameraState().rotation,beforeNewTab.rotation);
    QTemporaryFile late(QDir::tempPath()+"/youngcraft-sync-XXXXXX.xyz");
    QVERIFY(late.open()); late.write("2\ntranslated\nC 50 60 70\nO 52 60 70\n"); late.flush();
    w.openFile(late.fileName());
    auto *page = w.m_documentTabs->currentWidget();
    QTRY_VERIFY_WITH_TIMEOUT(page->property("loadFinished").toBool(),10000);
    QVERIFY(page->property("loadSuccess").toBool());
    compareCameras(first,w.m_view);
    QCOMPARE(first->cameraState().rotation,beforeNewTab.rotation);
    // Hiding atoms, captures and syncing never edit molecular coordinates.
    w.setCarbonHydrogensVisible(false);
    for (int i=0;i<a.atoms.size();++i) {
        QCOMPARE(first->molecule().atoms[i].position,firstCoordinates[i].position);
        QCOMPARE(second->molecule().atoms[i].position,secondCoordinates[i].position);
    }
    QCOMPARE(first->molecule().bonds.size(),1);
    const auto secondBeforeDisable = second->cameraState();
    sync->trigger(); QVERIFY(!sync->isChecked());
    w.m_documentTabs->setCurrentIndex(firstTab);
    first->rotateBy({40,20}); first->panBy({7,9});
    QCOMPARE(second->cameraState().rotation,secondBeforeDisable.rotation);
    QVERIFY((second->cameraState().centerOffset-secondBeforeDisable.centerOffset).length()<1e-5f);
    // Re-enabling uses the current tab as the new reference.
    sync->trigger(); compareCameras(first,second);
    w.closeDocument(firstTab);
    QVERIFY(w.m_view); // Closing a reference tab leaves no dangling camera pointer.
}

void SmokeTest::documentsAreIndependent() {
    QTemporaryFile large(QDir::tempPath() + "/youngcraft-large-XXXXXX.out");
    QTemporaryFile smallFile(QDir::tempPath() + "/youngcraft-small-XXXXXX.out");
    QVERIFY(large.open()); QVERIFY(smallFile.open());
    const QByteArray orientation(" Input orientation:\n -----\n Center Atomic Atomic Coordinates\n Number Number Type X Y Z\n -----\n 1 1 0 0 0 0\n 2 1 0 0 0 2\n -----\n");
    for (int i = 0; i < 1500; ++i) large.write(orientation + " SCF Done: E(RHF) = -10 A.U.\n");
    smallFile.write(orientation + " SCF Done: E(RHF) = -20 A.U.\n");
    large.flush(); smallFile.flush();
    MainWindow window;
    window.show();
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QSignalSpy completed(&window, &MainWindow::documentLoaded);
    window.openFile(large.fileName());
    QWidget *first = tabs->currentWidget();
    window.openFile(smallFile.fileName());
    QWidget *second = tabs->currentWidget();
    QCOMPARE(tabs->count(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(second->property("loadFinished").toBool(), 15000);
    QTRY_VERIFY_WITH_TIMEOUT(first->property("loadFinished").toBool(), 30000);
    QCOMPARE(completed.count(), 2);
    qInfo().noquote() << "Async completion order:" << completed[0][0].toString() << "then" << completed[1][0].toString();
    QCOMPARE(tabs->currentWidget(), second);
    QVERIFY(first->property("loadSuccess").toBool());
    QVERIFY(second->property("loadSuccess").toBool());
    auto activePanel = [&]() -> AnalysisPanel * {
        for (auto *panel : window.findChildren<AnalysisPanel *>()) if (panel->isVisible()) return panel;
        return nullptr;
    };
    auto *secondPanel = activePanel(); QVERIFY(secondPanel);
    QCOMPARE(secondPanel->findChild<QListWidget *>("FrameList")->count(), 1);
    QVERIFY(secondPanel->findChild<PlotWidget *>("OptimizationEnergyPlot")->toolTip().contains("-20.000000000"));
    secondPanel->setCurrentTab(2);
    second->findChild<MoleculeView *>()->setSelectedAtom(1);
    tabs->setCurrentWidget(first);
    auto *firstPanel = activePanel(); QVERIFY(firstPanel); QVERIFY(firstPanel != secondPanel);
    QCOMPARE(firstPanel->findChild<QListWidget *>("FrameList")->count(), 1500);
    QVERIFY(firstPanel->findChild<PlotWidget *>("OptimizationEnergyPlot")->toolTip().contains("-10.000000000"));
    firstPanel->findChild<QListWidget *>("FrameList")->setCurrentRow(8);
    tabs->setCurrentWidget(second);
    QCOMPARE(secondPanel->currentTab(), 2);
    QCOMPARE(second->findChild<MoleculeView *>()->selectedAtom(), 1);
    window.openFile(large.fileName());
    QCOMPARE(tabs->count(), 2); QCOMPARE(tabs->currentWidget(), first);
    QCOMPARE(firstPanel->currentFrame(), 8);
    QTest::keyClick(&window, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tabs->currentWidget(), second);
    QTest::keyClick(&window, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tabs->currentWidget(), first);
    QVERIFY(QMetaObject::invokeMethod(tabs, "tabCloseRequested", Q_ARG(int, tabs->currentIndex())));
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->currentWidget(), second);
    window.openFile(large.fileName());
    QVERIFY(QMetaObject::invokeMethod(tabs, "tabCloseRequested", Q_ARG(int, tabs->currentIndex())));
    QCOMPARE(tabs->count(), 1);
    QTest::qWait(300);
    QCOMPARE(secondPanel->findChild<QListWidget *>("FrameList")->count(), 1);
    QTest::keyClick(&window, Qt::Key_W, Qt::ControlModifier);
    QCOMPARE(tabs->count(), 0);
    QCOMPARE(window.windowTitle(), QStringLiteral("Youngcraft"));
}

void SmokeTest::optimizationGraphSynchronizes() {
    CalculationData data;
    for (int i = 0; i < 4; ++i) { Frame f; f.metrics.energyHartree = i == 1 || i == 3 ? kNaN : -10.0 - i; data.frames.push_back(f); }
    AnalysisPanel panel;
    panel.setData(data);
    panel.resize(700, 800);
    panel.show();
    auto *plot = panel.findChild<PlotWidget *>("OptimizationEnergyPlot");
    QVERIFY(plot);
    QCOMPARE(plot->series().size(), 2);
    QCOMPARE(plot->series()[0].y(), 2.0 * 627.509474);
    QCOMPARE(plot->series()[1].x(), 3.0);
    QCOMPARE(plot->selectedPoint(), 1);
    QSignalSpy requested(&panel, &AnalysisPanel::frameRequested);
    panel.setCurrentFrame(0);
    QCOMPARE(requested.count(), 0); // Playback/display updates must not request another frame.
    QCOMPARE(plot->selectedPoint(), 0);
    QApplication::processEvents();
    // The second point is the minimum at the right end of the plotted range.
    const auto range=plot->axisRange();
    const auto area=plot->plotArea();
    const auto point=plot->series()[1];
    const QPoint target(qRound(area.left()+(point.x()-range.xMin)/(range.xMax-range.xMin)*area.width()),
                        qRound(area.bottom()-(point.y()-range.yMin)/(range.yMax-range.yMin)*area.height()));
    QTest::mouseClick(plot, Qt::LeftButton, Qt::NoModifier,target);
    QCOMPARE(panel.currentFrame(), 2);
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.last().first().toInt(), 2);
    panel.findChild<QSlider *>("FrameSlider")->setValue(1);
    QCOMPARE(plot->selectedPoint(), -1);
    QCOMPARE(requested.count(), 2);
    MainWindow window; window.show();
    window.openFile(QFINDTESTDATA("tests/data/mvp.out"));
    auto *tabs = window.findChild<QTabWidget *>("DocumentTabs");
    QTRY_VERIFY_WITH_TIMEOUT(tabs->currentWidget()->property("loadFinished").toBool(), 15000);
    auto *animation = tabs->currentWidget()->findChild<AnimationController *>();
    QSignalSpy frames(animation, &AnimationController::frameChanged);
    QVERIFY(QMetaObject::invokeMethod(&window, "toggleFrameAnimation"));
    QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 3, 1000);
    QVERIFY(animation->isPlaying());
    animation->stop();
}

void SmokeTest::scanWithoutGeometryIsSelectable() {
    QTemporaryFile file(QDir::tempPath() + "/youngcraft-summary-XXXXXX.out");
    QVERIFY(file.open());
    file.write(" Summary of Optimized Potential Surface Scan\n 1 2\n Eigenvalues -- -10.0-11.0\n Normal termination of Gaussian\n"); file.flush();
    MainWindow window; window.show(); window.openFile(file.fileName());
    auto *tabs = window.findChild<QTabWidget *>("DocumentTabs");
    QTRY_VERIFY_WITH_TIMEOUT(tabs->currentWidget()->property("loadFinished").toBool(), 15000);
    QVERIFY(tabs->currentWidget()->property("loadSuccess").toBool());
    window.m_analysis->findChild<QPushButton *>("OpenScanGraph")->click();
    auto *plot = window.m_analysis->findChild<PlotWidget *>("ScanResultsPlot");
    QCOMPARE(plot->series().size(), 2);
    QVERIFY(plot->isVisible());
    QVERIFY(QMetaObject::invokeMethod(plot, "pointClicked", Q_ARG(int, 1)));
    QCOMPARE(plot->selectedPoint(), 1);
    QVERIFY(tabs->currentWidget()->findChild<MoleculeView *>()->molecule().isEmpty());
    for (auto *panel : window.findChildren<AnalysisPanel *>()) if (panel->isVisible()) {
        QCOMPARE(panel->findChild<QListWidget *>("ScanList")->currentRow(), 1);
        panel->findChild<QListWidget *>("ScanList")->setCurrentRow(0);
        QCOMPARE(plot->selectedPoint(), 0);
    }
}

void SmokeTest::realScanGuiInputs() {
    const QString directory = qEnvironmentVariable("YOUNGCRAFT_SCAN_INPUT_DIR");
    if (directory.isEmpty()) QSKIP("Set YOUNGCRAFT_SCAN_INPUT_DIR for real-file GUI validation");
    const auto files = QDir(directory).entryInfoList({"*scan*.out"}, QDir::Files, QDir::Name);
    QVERIFY(!files.isEmpty());
    MainWindow window; window.resize(1400, 900); window.show();
    auto *tabs = window.findChild<QTabWidget *>("DocumentTabs");
    int number = 0;
    for (const auto &file : files) {
        window.openFile(file.absoluteFilePath());
        auto *page = tabs->currentWidget();
        QTRY_VERIFY_WITH_TIMEOUT(page->property("loadFinished").toBool(), 90000);
        QVERIFY(page->property("loadSuccess").toBool());
        window.m_analysis->findChild<QPushButton *>("OpenScanGraph")->click();
        auto *plot = window.m_analysis->findChild<PlotWidget *>("ScanResultsPlot");
        QVERIFY(plot->isVisible()); QVERIFY(!plot->series().isEmpty());
        AnalysisPanel *panel = nullptr;
        for (auto *candidate : window.findChildren<AnalysisPanel *>()) if (candidate->isVisible()) panel = candidate;
        QVERIFY(panel);
        panel->setCurrentTab(2);
        for (int i = 0; i < plot->series().size(); ++i) {
            QVERIFY(QMetaObject::invokeMethod(plot, "pointClicked", Q_ARG(int, i)));
            QCOMPARE(plot->selectedPoint(), i);
            QCOMPARE(panel->findChild<QListWidget *>("ScanList")->currentRow(), i);
        }
        qInfo().noquote() << "GUI Scan visible and all points selectable:" << file.fileName() << plot->series().size();
        const QString captures = qEnvironmentVariable("YOUNGCRAFT_GUI_CAPTURE_DIR");
        if (!captures.isEmpty()) {
            QApplication::processEvents();
            QVERIFY(window.grab().save(QDir(captures).filePath(QStringLiteral("scan-gui-%1.png").arg(++number))));
        }
        panel->setCurrentTab(0);
        auto *optimization = panel->findChild<PlotWidget *>("OptimizationEnergyPlot");
        QVERIFY(optimization->isVisible()); QVERIFY(!optimization->series().isEmpty());
        if (!captures.isEmpty()) {
            QApplication::processEvents();
            QVERIFY(window.grab().save(QDir(captures).filePath(QStringLiteral("optimization-gui-%1.png").arg(number))));
        }
        QVERIFY(QMetaObject::invokeMethod(optimization, "pointClicked", Q_ARG(int, 0)));
        QCOMPARE(optimization->selectedPoint(), 0);
        QVERIFY(QMetaObject::invokeMethod(tabs, "tabCloseRequested", Q_ARG(int, 0)));
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

void SmokeTest::editorInputTransactionsAndSelection() {
    MainWindow w;w.show();w.newDrawingDocument();QTest::qWait(30);auto *v=w.m_view;w.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();
    QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,v->rect().center());QCOMPARE(v->molecule().atoms.size(),1);
    auto screen=[&](int i){auto p=v->project(v->molecule().atoms[i].position,v->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    auto move=[&](const QPoint &p,Qt::MouseButton button,Qt::KeyboardModifiers mods){QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(v->mapToGlobal(p)),Qt::NoButton,button,mods);QApplication::sendEvent(v,&e);};
    w.m_drawElement->setCurrentText("N");w.m_drawBar->findChild<QSpinBox *>("DrawFormalCharge")->setValue(1);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,screen(0));QCOMPARE(v->molecule().atoms.size(),1);QCOMPARE(v->molecule().atoms[0].atomicNumber,7);QCOMPARE(v->molecule().atoms[0].formalCharge,1);
    w.undoDrawingEdit();QCOMPARE(v->molecule().atoms[0].atomicNumber,6);w.redoDrawingEdit();QCOMPARE(v->molecule().atoms[0].atomicNumber,7);
    auto before=v->molecule();QPoint start=screen(0),end=start+QPoint(130,0);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,start);move(end,Qt::LeftButton,Qt::AltModifier);QCOMPARE(v->molecule().atoms.size(),1);QVERIFY(v->hasGhostBond());QTest::mouseRelease(v,Qt::LeftButton,Qt::AltModifier,end);QCOMPARE(v->molecule().atoms.size(),2);QCOMPARE(v->molecule().bonds.size(),1);
    w.undoDrawingEdit();QCOMPARE(v->molecule().atoms.size(),1);w.redoDrawingEdit();QCOMPARE(v->molecule().atoms.size(),2);
    // Existing target is used, and Automatic preserves explicit order without a new undo step.
    int steps=w.m_undoMolecules.size();start=screen(0);end=screen(1);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,start);move(end,Qt::LeftButton,Qt::NoModifier);QTest::mouseRelease(v,Qt::LeftButton,Qt::NoModifier,end);QCOMPARE(v->molecule().atoms.size(),2);QCOMPARE(v->molecule().bonds.size(),1);QCOMPARE(w.m_undoMolecules.size(),steps);
    QPoint midpoint=(screen(0)+screen(1))/2;w.m_drawBondOrder->setCurrentIndex(w.m_drawBondOrder->findData(0.0));QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().bonds[0].order,2.0);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().bonds[0].order,3.0);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().bonds[0].order,1.0);
    QTest::mouseClick(v,Qt::RightButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().atoms.size(),2);QVERIFY(v->molecule().bonds.isEmpty());w.undoDrawingEdit();
    w.setDrawingMode(false);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,screen(0));QTest::mouseClick(v,Qt::LeftButton,Qt::ShiftModifier,screen(1));QCOMPARE(v->selection().atoms.size(),2);QTest::mouseClick(v,Qt::LeftButton,Qt::ControlModifier,screen(0));QCOMPARE(v->selection().atoms.size(),1);QTest::mouseDClick(v,Qt::LeftButton,Qt::NoModifier,screen(0));QCOMPARE(v->selection().atoms.size(),2);
    QPoint min=screen(0)-QPoint(40,40),max=screen(1)+QPoint(40,40);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,min);move(max,Qt::LeftButton,Qt::NoModifier);QTest::mouseRelease(v,Qt::LeftButton,Qt::NoModifier,max);QCOMPARE(v->selection().atoms.size(),2);
    auto positions=v->molecule();double length=GeometryEditor::distance(positions,0,1);w.setAtomDragMode(true);steps=w.m_undoMolecules.size();start=screen(0);end=start+QPoint(0,50);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,start);for(int i=1;i<=10;++i)move(start+QPoint(0,i*5),Qt::LeftButton,Qt::NoModifier);QCOMPARE(v->molecule().atoms[0].position,positions.atoms[0].position);QTest::mouseRelease(v,Qt::LeftButton,Qt::NoModifier,end);QCOMPARE(w.m_undoMolecules.size(),steps+1);QVERIFY(qAbs(GeometryEditor::distance(v->molecule(),0,1)-length)<1e-5);w.undoDrawingEdit();QCOMPARE(v->molecule().atoms[0].position,positions.atoms[0].position);
    w.setDrawingMode(true);w.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();QTest::mouseClick(v,Qt::RightButton,Qt::NoModifier,screen(0));QCOMPARE(v->molecule().atoms.size(),1);QVERIFY(v->molecule().bonds.isEmpty());
}
void SmokeTest::editorPreservesCalculationFrames() {
    MainWindow w;w.show();w.openFile("tests/data/mvp.out");
    QTRY_VERIFY_WITH_TIMEOUT(!w.m_data.molecule.isEmpty(),5000);
    auto original=w.m_activeDocument;auto frames=w.m_data.frames;auto originalMolecule=w.m_data.molecule;auto active=w.m_view->molecule();int count=w.m_documentTabs->count();
    w.setDrawingMode(true);QCOMPARE(w.m_documentTabs->count(),count+1);QVERIFY(w.m_activeDocument!=original);QVERIFY(w.m_data.frames.isEmpty());QCOMPARE(w.m_data.molecule.atoms[0].position,active.atoms[0].position);
    w.addDrawnAtom({10,0,0});w.undoDrawingEdit();QCOMPARE(w.m_data.molecule.atoms.size(),active.atoms.size());QCOMPARE(original->data.frames.size(),frames.size());for(int i=0;i<frames.size();++i)QCOMPARE(original->data.frames[i].coordinates,frames[i].coordinates);QCOMPARE(original->data.molecule.atoms.size(),originalMolecule.atoms.size());
}
void SmokeTest::editorFragmentUndoAndKeyboard(){
    MainWindow w;w.show();w.newDrawingDocument();QTest::keyClick(&w,Qt::Key_F);QTest::keyClick(&w,Qt::Key_E);QCOMPARE(w.m_drawElement->currentText(),QString("Fe"));QTest::keyClick(&w,Qt::Key_2);QCOMPARE(w.m_drawBondOrder->currentData().toDouble(),2.0);QTest::keyClick(&w,Qt::Key_A);QCOMPARE(w.m_drawBondOrder->currentData().toDouble(),0.0);
    auto *v=w.m_view;v->setProperty("pendingCenter",27);v->setProperty("pendingCharge",2);v->setProperty("pendingGeometry",7);QVERIFY(w.consumeInsertion(-1,{}));QCOMPARE(v->molecule().atoms.size(),1);QCOMPARE(v->molecule().coordinationSites.size(),6);w.undoDrawingEdit();QVERIFY(v->molecule().isEmpty());w.redoDrawingEdit();QCOMPARE(v->molecule().coordinationSites.size(),6);
    v->setProperty("pendingFragment",":/fragments/ligands/bidentate/bpy-bipyridine.cjson");w.m_autoHButton->setChecked(true);int count=v->molecule().atoms.size();QVERIFY(w.consumeInsertion(0,{}, {1,0,0}));int coordination=0;for(auto b:v->molecule().bonds)if(b.coordination)++coordination;QCOMPARE(coordination,2);QVERIFY(MoleculeEditController::validate(v->molecule()));w.undoDrawingEdit();QCOMPARE(v->molecule().atoms.size(),count);w.redoDrawingEdit();QVERIFY(v->molecule().atoms.size()>count);
}

void SmokeTest::editorVisualPreview(){
    if(QGuiApplication::platformName()=="offscreen")QSKIP("OpenGL visual verification runs on Windows platform");
    MainWindow w;w.resize(1200,850);w.show();Molecule m;FragmentLibrary::coordinationCenter(m,27,2,7,{});Fragment f;QString error;QVERIFY(FragmentLibrary::read(":/fragments/ligands/monodentate/carbonyl.cjson",f,&error));QVERIFY(FragmentLibrary::insert(m,f,0,{}, {1,0,0},&error));w.newDrawingDocument(m);w.applyStyle(StyleManager::preset(QStringLiteral("Chemcraft 참조")));auto *v=w.m_view;w.m_drawElement->setCurrentText("Co");w.m_drawBar->findChild<QSpinBox *>("DrawFormalCharge")->setValue(2);v->selection().atoms={0};QTest::qWait(100);QVERIFY(v->isValid());v->bestView();QTest::qWait(100);
    auto projected=v->project(v->molecule().atoms[0].position,v->viewProjection());
    QPoint probe=v->mapTo(&w,QPoint(qRound(projected.x())+20,qRound(projected.y())+15));
    QImage composed=w.grab().toImage();QColor pixel=composed.pixelColor(probe);
    QVERIFY(pixel.value()>0);QVERIFY(pixel!=v->style().background); // Transparent editor overlay must retain the molecule below it.
    QString capture=qEnvironmentVariable("YOUNGCRAFT_EDITOR_CAPTURE");if(!capture.isEmpty()){QVERIFY(w.grab().save(capture));QVERIFY(v->image().save(capture+".scene.png"));}
    auto before=v->molecule();w.setDrawingMode(false);v->setBondCentricMode(true);int index=before.bondIndex(1,2);QVERIFY(index>=0);v->m_activeBond=index;
    auto p=v->project(before.atoms[2].position,v->viewProjection());QPoint start(qRound(p.x()),qRound(p.y())),end=start+QPoint(40,0);
    QTest::mousePress(v,Qt::RightButton,Qt::NoModifier,start);QMouseEvent e(QEvent::MouseMove,QPointF(end),QPointF(v->mapToGlobal(end)),Qt::NoButton,Qt::RightButton,Qt::NoModifier);QApplication::sendEvent(v,&e);QCOMPARE(v->molecule().atoms[2].position,before.atoms[2].position);QVERIFY(v->m_centricDrag);QTest::mouseRelease(v,Qt::RightButton,Qt::NoModifier,end);QVERIFY(qAbs(GeometryEditor::distance(v->molecule(),1,2)-GeometryEditor::distance(before,1,2)-0.4)<1e-4);w.undoDrawingEdit();QCOMPARE(v->molecule().atoms[2].position,before.atoms[2].position);
}

void SmokeTest::dialogThemeSynchronization(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("DialogThemeSync");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1450,960);window.show();
    window.findChild<QAction *>("PatternTemplate0")->trigger();
    Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.3f,0,0}}};molecule.setBond(0,1,1);
    auto *xtb=new XtbDialog(molecule,QString(),&window,2,false,{});xtb->show();
    StyleEditorDialog style(window.m_style,&molecule,-1,&window);style.show();
    QDialog nested(xtb);nested.resize(300,200);nested.show();QTest::qWait(80);
    window.m_view->setMolecule(molecule);
    auto captureCamera=window.m_view->cameraState();captureCamera.orthoScale=3;window.m_view->setCameraState(captureCamera);
    CalculationData captureData;captureData.molecule=molecule;
    auto *capture=new ReactionCaptureDialog(captureData,window.m_view,.5,1,false,&window);capture->resize(1160,800);
    const QString longName="1-CoIII-Me3-TPADP-AfterCC-forOO_Cleavage-scan-freq-ts-freq-ircfwd-opt-freq.out";
    capture->m_names[0]->setText(longName);capture->m_names[1]->setText("TS");capture->m_names[2]->setText(longName);
    capture->show();
    auto *multi=new MultiCaptureDialog({{"short",molecule,window.m_style,window.m_view->cameraState()},{longName,molecule,window.m_style,window.m_view->cameraState()}},&window);multi->show();QTest::qWait(80);
    for(QDialog *dialog:{static_cast<QDialog*>(xtb),static_cast<QDialog*>(&style),&nested,static_cast<QDialog*>(capture),static_cast<QDialog*>(multi)}) {
        QVERIFY(uiPattern::active(dialog));QCOMPARE(dialog->property("appPatternPaper"),window.property("appPatternPaper"));
    }
    auto aligned=[&]{
        QCOMPARE(capture->m_views[0]->y(),capture->m_views[1]->y());QCOMPARE(capture->m_views[1]->y(),capture->m_views[2]->y());
        QCOMPARE(capture->m_views[0]->height(),capture->m_views[1]->height());QCOMPARE(capture->m_views[1]->height(),capture->m_views[2]->height());
        QVERIFY(qAbs(capture->m_views[0]->width()-capture->m_views[1]->width())<=1);QVERIFY(qAbs(capture->m_views[1]->width()-capture->m_views[2]->width())<=1);
        QCOMPARE(multi->m_views[0]->y(),multi->m_views[1]->y());QCOMPARE(multi->m_views[0]->height(),multi->m_views[1]->height());
        QVERIFY(qAbs(multi->m_views[0]->width()-multi->m_views[1]->width())<=1);
    };aligned();
    for(int i=0;i<6;++i){
        window.findChild<QAction *>(QString("PastelTheme%1").arg(i))->trigger();
        for(QDialog *dialog:{static_cast<QDialog*>(xtb),static_cast<QDialog*>(&style),&nested,static_cast<QDialog*>(capture),static_cast<QDialog*>(multi)}){
            QCOMPARE(dialog->property("appPattern").toInt(),0);
            QCOMPARE(dialog->property("appPatternPaper"),window.property("appPatternPaper"));
            QCOMPARE(dialog->palette().color(QPalette::Window),window.palette().color(QPalette::Window));
            QVERIFY(dialog->property("sidebarPaperWatched").toBool());
        }
        aligned();
    }
    window.findChild<QAction *>("PastelTheme2")->trigger();
    capture->resize(1000,820);multi->resize(1000,820);QTest::qWait(50);aligned();
    capture->m_includeTs->setChecked(false);QTest::qWait(30);
    QCOMPARE(capture->m_views[0]->y(),capture->m_views[2]->y());QVERIFY(capture->m_views[0]->width()>capture->width()*.4);
    capture->m_includeTs->setChecked(true);capture->resize(1160,800);multi->resize(1200,800);QTest::qWait(50);aligned();
    const auto path=qEnvironmentVariable("YOUNGCRAFT_DIALOG_THEME_REVIEW");
    if(!path.isEmpty()){QTest::qWait(70);QVERIFY(xtb->grab().save(path));QVERIFY(style.grab().save(path+".style.png"));QVERIFY(capture->grab().save(path+".capture.png"));QVERIFY(multi->grab().save(path+".multi.png"));}
    window.findChild<QAction *>("PatternTemplate0")->trigger();QVERIFY(!uiPattern::active(xtb));QVERIFY(!uiPattern::active(&nested));
    nested.hide();style.hide();xtb->hide();
}

void SmokeTest::saveChoicesAndFormats(){
    QTemporaryDir temporary;QVERIFY(temporary.isValid());
    const auto review=qEnvironmentVariable("YOUNGCRAFT_SAVE_REVIEW");if(!review.isEmpty())QDir().mkpath(review);
    QImage source(32,24,QImage::Format_ARGB32);source.fill(QColor(10,20,30));source.setPixelColor(0,0,Qt::transparent);source.setPixelColor(5,6,QColor(40,100,220,128));
    source.setDotsPerMeterX(23622);source.setDotsPerMeterY(23622);
    for(const QString &format:{QString("png"),QString("gif"),QString("jpg"),QString("jpeg"),QString("tif"),QString("tiff"),QString("bmp")}){
        const auto path=temporary.filePath("sample."+format);QString error;
        QVERIFY2(ImageExporter::saveImage(source,path,1,true,&error),qPrintable(error));QVERIFY(QFileInfo(path).size()>0);
        if(format=="tif"||format=="tiff"){
            QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));QVERIFY(file.read(4)==QByteArray("II*\0",4));
        }else{
            const QImage restored(path);QCOMPARE(restored.size(),source.size());
            if(format=="png"||format=="gif")QCOMPARE(restored.pixelColor(0,0).alpha(),0);
            if(format=="bmp")QCOMPARE(restored.pixelColor(0,0),QColor(Qt::white));
        }
        if(!review.isEmpty()){
            QFile from(path);QVERIFY(from.open(QIODevice::ReadOnly));QFile to(review+"/sample."+format);QVERIFY(to.open(QIODevice::WriteOnly));to.write(from.readAll());
        }
    }
    QString error;QVERIFY(!ImageExporter::saveImage(source,temporary.filePath("missing/sample.png"),1,true,&error));QVERIFY(!error.isEmpty());

    const bool nativeDisabled=QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);
    auto restore=qScopeGuard([&]{QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,nativeDisabled);});
    MainWindow window;window.resize(1000,700);window.show();
    Molecule molecule;molecule.title=QStringLiteral("저장 좌표 확인");molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.3f,0,0}}};molecule.setBond(0,1,1);
    window.newDrawingDocument(molecule);window.setDrawingMode(false);
    auto current=molecule;current.atoms[0].position={2.25f,-1.5f,.75f};window.m_view->setMolecule(current);
    auto *save=window.findChild<QAction*>("SaveStructureImage");QVERIFY(save);QCOMPARE(save->shortcut(),QKeySequence("Ctrl+S"));
    const auto exercise=[&](bool xyz,const QString &path){
        bool failed=false,visitedChoice=false,visitedFile=false;int ticks=0;QTimer driver;driver.setInterval(20);
        connect(&driver,&QTimer::timeout,&window,[&]{
            auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            if(++ticks>300){failed=true;dialog->reject();return;}
            if(dialog->objectName()=="SaveStructureChoice"){
                visitedChoice=true;
                if(!review.isEmpty())dialog->grab().save(review+"/save-choice.png");
                auto *choice=dialog->findChild<QPushButton*>(xyz?"SaveXYZChoice":"SavePictureChoice");if(!choice){failed=true;dialog->reject();return;}choice->click();
            }else if(auto *file=qobject_cast<QFileDialog*>(dialog)){
                visitedFile=true;
                if(!xyz){const auto filters=file->nameFilters().join(';');for(const char *extension:{"*.png","*.gif","*.jpg","*.tif","*.bmp"})if(!filters.contains(extension))failed=true;file->selectNameFilter("PNG 그림 (*.png)");}
                file->selectFile(path);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);
            }else if(auto *resolution=qobject_cast<QInputDialog*>(dialog)){resolution->setIntValue(512);resolution->accept();}
            else if(auto *question=qobject_cast<QMessageBox*>(dialog)){
                if(question->windowTitle()==QStringLiteral("투명 배경"))question->button(QMessageBox::Yes)->click();
                else{failed=true;question->reject();}
            }else{failed=true;dialog->reject();}
        });
        driver.start();save->trigger();driver.stop();QVERIFY(!failed);QVERIFY(visitedChoice&&visitedFile);
    };
    const auto xyzPath=temporary.filePath("현재 구조.xyz");exercise(true,xyzPath);QVERIFY(QFileInfo::exists(xyzPath));
    const auto parsed=XYZParser::parse(xyzPath);QVERIFY(parsed.success);QCOMPARE(parsed.data.molecule.atoms.size(),2);
    QCOMPARE(parsed.data.molecule.atoms[0].position,current.atoms[0].position);QCOMPARE(parsed.data.molecule.atoms[1].position,current.atoms[1].position);
    const auto imageBase=temporary.filePath("rendered");exercise(false,imageBase);const QImage rendered(imageBase+".png");QVERIFY(!rendered.isNull());QCOMPARE(qMax(rendered.width(),rendered.height()),512);
    bool cancelled=false;QTimer::singleShot(30,[&]{auto *dialog=window.findChild<QDialog*>("SaveStructureChoice");if(dialog){cancelled=true;dialog->reject();}});save->trigger();QVERIFY(cancelled);
}

void SmokeTest::frequencyPagingAndDetails(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("FrequencyPages");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1000,650);window.show();
    Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.3f,0,0}}};molecule.setBond(0,1,1);
    window.newDrawingDocument(molecule);window.setDrawingMode(false);
    for(int i=0;i<12;++i){FrequencyMode mode;mode.number=i+1;mode.wavenumber=10.0*(i+1);mode.irIntensity=i+.25;mode.ramanActivity=i+.5;mode.displacement={{.1f,0,0},{-.1f,0,0}};window.m_data.frequencies.append(mode);}
    window.m_analysis->setData(window.m_data);
    auto *navigator=window.findChild<QAction*>("ToggleResultNavigator");navigator->setChecked(true);
    window.m_analysis->setCurrentTab(0);QVERIFY(window.m_explorer->isVisible());
    window.m_analysis->setCurrentTab(1);QTest::qWait(80);QVERIFY(window.m_explorer->isHidden());QVERIFY(navigator->isChecked());
    auto *panel=window.m_analysis;auto *list=panel->findChild<QListWidget*>("FrequencyList");auto *next=panel->findChild<QPushButton*>("NextFrequencyPage");
    auto *details=panel->findChild<QPushButton*>("OpenFrequencyDetails");QVERIFY(list&&next&&details);
    QCOMPARE(panel->findChild<QWidget*>("FrequencyToolbar")->findChildren<QPushButton*>().size(),5);
    const auto verifyPage=[&](int first,int size){int visible=0;for(int i=0;i<list->count();++i){QCOMPARE(!list->item(i)->isHidden(),i>=first&&i<first+size);if(!list->item(i)->isHidden())++visible;}QCOMPARE(visible,size);};
    verifyPage(0,5);QCOMPARE(list->verticalScrollBarPolicy(),Qt::ScrollBarAlwaysOff);
    QSignalSpy requests(panel,&AnalysisPanel::frequencyRequested);
    next->click();verifyPage(5,5);QCOMPARE(panel->currentFrequency(),5);QCOMPARE(window.m_currentFrequency,5);QCOMPARE(requests.size(),1);
    next->click();verifyPage(10,2);next->click();verifyPage(0,5);
    panel->setCurrentFrequency(4);panel->selectNext();verifyPage(5,5);QCOMPARE(panel->currentFrequency(),5);
    auto *scroll=window.m_currentSidebar->findChild<QScrollArea*>("SidebarAnalysisScroll");QVERIFY(scroll);
    for(const char *name:{"FrequencyAmplitude","FrequencyCycleTime","FrequencyScaledVectors"}){
        auto *control=panel->findChild<QWidget*>(name);QVERIFY(control&&control->isVisible());
        QVERIFY(scroll->viewport()->rect().contains(control->mapTo(scroll->viewport(),control->rect().bottomRight())));
    }
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_FREQUENCY_REVIEW");if(!capture.isEmpty())QDir().mkpath(capture);
    if(!capture.isEmpty())QVERIFY(window.grab().save(capture+"/frequency-page.png"));
    details->click();auto *dialog=panel->findChild<QDialog*>("FrequencyDetailsDialog");QVERIFY(dialog->isVisible());
    auto *table=dialog->findChild<QTableWidget*>("FrequencyDetailsTable");QCOMPARE(table->rowCount(),12);QCOMPARE(table->columnCount(),5);
    QCOMPARE(table->item(10,2)->text(),QString("10.2500"));table->setCurrentCell(10,0);verifyPage(10,2);QCOMPARE(window.m_currentFrequency,10);
    if(!capture.isEmpty())QVERIFY(dialog->grab().save(capture+"/frequency-details.png"));
    dialog->hide();window.m_analysis->setCurrentTab(0);QVERIFY(window.m_explorer->isVisible());
    for(int tab:{6,7}){
        panel->setCurrentTab(tab);QVERIFY(window.m_explorer->isHidden());
        navigator->setChecked(false);navigator->setChecked(true);QVERIFY(window.m_explorer->isHidden());
        if(!capture.isEmpty()){QTest::qWait(30);QVERIFY(window.grab().save(capture+QString("/navigator-hidden-tab-%1.png").arg(tab)));}
        panel->setCurrentTab(0);QVERIFY(window.m_explorer->isVisible());
    }
    navigator->setChecked(false);panel->setCurrentTab(7);panel->setCurrentTab(0);QVERIFY(window.m_explorer->isHidden());
    navigator->setChecked(true);QVERIFY(window.m_explorer->isVisible());
    window.m_data.frequencies.resize(3);panel->setData(window.m_data);panel->setCurrentTab(1);verifyPage(0,3);QVERIFY(!next->isEnabled());
    window.m_data.frequencies.clear();panel->setData(window.m_data);verifyPage(0,0);QVERIFY(!next->isEnabled());
}

void SmokeTest::sidebarIconFlowLayout(){
    QWidget host;host.resize(400,400);
    QWidget strip(&host);
    auto *flow=new SidebarFlowLayout(&strip);
    QList<QToolButton*> controls;
    for(int i=0;i<9;++i){auto *button=new QToolButton(&strip);button->setFixedSize(32,32);flow->addWidget(button);controls.append(button);}
    host.show();strip.show();
    const auto verify=[&](int width){
        strip.resize(width,flow->heightForWidth(width));QTest::qWait(20);
        int position=0;const int columns=qMax(1,(width+4)/36);
        for(auto *button:controls)if(!button->isHidden()){
            QCOMPARE(button->geometry(),QRect((position%columns)*36,(position/columns)*36,32,32));
            QVERIFY(strip.rect().contains(button->geometry()));++position;
        }
    };
    for(int width:{32,68,104,176,320})verify(width);
    controls[2]->hide();verify(104);controls[2]->show();verify(320);host.hide();

    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("SidebarIconFlow");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1400,900);window.show();window.openFile(QFINDTESTDATA("tests/data/mvp.out"));
    QTRY_VERIFY_WITH_TIMEOUT(!window.m_data.molecule.isEmpty(),10000);
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_FLOW_REVIEW");if(!capture.isEmpty())QDir().mkpath(capture);
    for(int width:{188,208,360,188}){
        window.resizeDocks({window.m_structureDock},{width},Qt::Horizontal);QTest::qWait(60);
        auto *displayFlow=window.findChild<QLayout*>("StructureDisplayFlow");QVERIFY(displayFlow);QCOMPARE(displayFlow->count(),7);
        const auto checkFlow=[&](QLayout *layout){
            int position=0;const int columns=qMax(1,(layout->geometry().width()+4)/36);
            const auto origin=layout->geometry().topLeft();
            for(int i=0;i<layout->count();++i){auto *item=layout->itemAt(i);if(item->isEmpty())continue;
                QCOMPARE(item->geometry().topLeft(),origin+QPoint((position%columns)*36,(position/columns)*36));
                QCOMPARE(item->geometry().size(),QSize(32,32));++position;
            }
        };
        checkFlow(displayFlow);
        auto *first=displayFlow->itemAt(0)->widget();auto *tabs=window.m_structureTabs->tabBar();
        QCOMPARE(first->mapTo(window.m_structureDock,QPoint()).x(),tabs->mapTo(window.m_structureDock,tabs->tabRect(0).topLeft()).x());
        auto *centered=dynamic_cast<CenteredIconTabBar*>(tabs);QVERIFY(centered);
        for(int tab=0;tab<2;++tab){
            QVERIFY(qAbs(centered->iconRect(tab).center().x()-centered->buttonRect(tab).center().x())<=1);
            QVERIFY(qAbs(centered->iconRect(tab).center().y()-centered->buttonRect(tab).center().y())<=1);
            const auto pixels=window.m_structureTabs->tabIcon(tab).pixmap(QSize(18,18)).toImage();QRect bounds;
            for(int y=0;y<pixels.height();++y)for(int x=0;x<pixels.width();++x)if(qAlpha(pixels.pixel(x,y)))bounds=bounds.united(QRect(x,y,1,1));
            QVERIFY(!bounds.isEmpty());QVERIFY(qAbs(bounds.center().x()-pixels.rect().center().x())<=1);QVERIFY(qAbs(bounds.center().y()-pixels.rect().center().y())<=1);
        }
        checkFlow(window.m_analysisShortcuts->layout());
        window.m_structureTabs->setCurrentIndex(1);QTest::qWait(30);checkFlow(window.findChild<QLayout*>("StructureEditFlow"));
        window.m_structureTabs->setCurrentIndex(0);QTest::qWait(30);
        if(!capture.isEmpty())QVERIFY(window.grab().save(capture+QString("/sidebar-width-%1.png").arg(width)));
    }
}

void SmokeTest::sidebarPatternContinuity(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("SidebarSolidWorkspace");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();window.m_explorer->show();QTest::qWait(100);
    QVERIFY(window.m_explorer->isHeaderHidden());
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_SIDEBAR_REVIEW");
    if(!capture.isEmpty())QDir().mkpath(capture);
    for(int theme=0;theme<6;++theme){
        window.findChild<QAction *>(QString("PastelTheme%1").arg(theme))->trigger();
        for(int pattern=0;pattern<6;++pattern){
            window.findChild<QAction *>(QString("PatternTemplate%1").arg(pattern))->trigger();
            QVERIFY(!uiPattern::active(window.m_view));
            QTest::qWait(10);
            const QImage image=window.m_explorer->viewport()->grab().toImage();
            const auto base=window.m_explorer->palette().color(QPalette::Base);
            QCOMPARE(image.pixelColor(image.width()-10,image.height()-10),base);
            if(window.findChild<QAction *>(QString("PatternTemplate%1").arg(pattern))->isChecked())
                window.findChild<QAction *>(QString("PatternTemplate%1").arg(pattern))->trigger();
        }
        for(const QSize size:{QSize(1000,650),QSize(1280,800),QSize(1500,960)}){
            const QSize frame = window.frameGeometry().size() - window.size();
            const QSize available = window.screen()->availableGeometry().size() - frame;
            const QSize bounded(qMin(size.width(),available.width()),qMin(size.height(),available.height()));
            window.resize(bounded);QTest::qWait(35);
            QCOMPARE(window.size(),bounded);
            const auto left=window.m_explorerDock->geometry(),right=window.m_structureDock->geometry();
            QVERIFY(!left.intersects(right));
            QVERIFY(window.rect().contains(left));QVERIFY(window.rect().contains(right));
            QVERIFY(window.centralWidget()->width()>=120);
            for(auto *dock:{window.m_explorerDock,window.m_structureDock}){
                for(auto *button:dock->findChildren<QToolButton*>()){
                    if(!button->isVisible()||!button->property("sidebarButton").toBool())continue;
                    QCOMPARE(button->size(),QSize(32,32));
                    QVERIFY(dock->rect().contains(QRect(button->mapTo(dock,QPoint()),button->size())));
                }
                dock->toggleViewAction()->trigger();QTest::qWait(10);QVERIFY(dock->isHidden());
                dock->toggleViewAction()->trigger();QTest::qWait(10);QVERIFY(dock->isVisible());
            }
            if(!capture.isEmpty())QVERIFY(window.grab().save(capture+QString("/theme-%1-%2x%3.png").arg(theme).arg(size.width()).arg(size.height())));
        }
    }
}

void SmokeTest::measurementBarPatternContinuity(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("MeasurementPaper");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,950);window.show();
    window.findChild<QAction *>("PatternTemplate0")->trigger();QTest::qWait(70);
    auto *row=window.findChild<QWidget*>("WorkspaceMeasurementRow");QVERIFY(row);QVERIFY(row->height()<60);
    const auto actual=row->grab().toImage();QImage expected(row->size(),QImage::Format_ARGB32);
    expected.fill(Qt::transparent);{QPainter painter(&expected);uiPattern::paintBackdrop(painter,row);}
    const auto dpr=row->devicePixelRatioF();
    for(int x=5;x<row->width()-5;x+=13){const auto a=actual.pixelColor(qRound(x*dpr),qRound(2*dpr)),b=expected.pixelColor(x,2);QVERIFY(qAbs(a.red()-b.red())<3&&qAbs(a.green()-b.green())<3);}
    const auto path=qEnvironmentVariable("YOUNGCRAFT_MEASUREMENT_PAPER_PREVIEW");if(!path.isEmpty())QVERIFY(window.grab().save(path));
}

void SmokeTest::professorPhotoTheme() {
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("ProfessorTheme");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1280,800);window.show();QTest::qWait(150);
    auto *action=window.findChild<QAction*>("PastelTheme6");QVERIFY(action);
    QCOMPARE(action->text(),QStringLiteral("교수님이 보고있어"));
    const auto plain=window.m_view->grabFramebuffer();QVERIFY(!plain.isNull());
    const auto exported=window.m_view->image(false,true);
    action->trigger();QTest::qWait(100);QVERIFY(action->isChecked());
    QCOMPARE(QSettings().value("ui/theme").toInt(),6);
    const auto photo=window.m_view->grabFramebuffer();QVERIFY(!photo.isNull());QVERIFY(photo!=plain);
    const auto center=photo.pixelColor(photo.width()/2,photo.height()/4);
    const auto paper=window.property("appPatternPaper").value<QColor>();
    QVERIFY(qAbs(center.red()-paper.red())+qAbs(center.green()-paper.green())+qAbs(center.blue()-paper.blue())>30);
    QCOMPARE(window.m_view->image(false,true),exported);
    QTest::qWait(400);const auto animated=window.m_view->grabFramebuffer();QVERIFY(animated!=photo);
    // Capture the bright phase so the eye effect is easy to review.
    QImage brightest;int highest=-1;
    for(int i=0;i<65;++i){QTest::qWait(40);const auto frame=window.m_view->grabFramebuffer();
        const int side=qMax(window.m_view->width(),window.m_view->height());
        const int x=qRound((0.410*side-(side-window.m_view->width())/2.0)*frame.devicePixelRatio());
        const int y=qRound((0.299*side-(side-window.m_view->height())/2.0)*frame.devicePixelRatio());
        const auto c=frame.pixelColor(x,y);const int light=c.red()+c.green()+c.blue();
        if(light>highest){highest=light;brightest=window.grab().toImage();}
    }
    QVERIFY(highest>650);
    const auto path=qEnvironmentVariable("YOUNGCRAFT_PROFESSOR_PREVIEW");if(!path.isEmpty())QVERIFY(brightest.save(path));
    {MainWindow restored;QVERIFY(restored.property("professorBackdrop").toBool());QVERIFY(restored.findChild<QAction*>("PastelTheme6")->isChecked());}
    window.resize(1000,650);QTest::qWait(100);QVERIFY(!window.m_view->grabFramebuffer().isNull());
    window.findChild<QAction*>("PastelTheme0")->trigger();QTest::qWait(100);
    QVERIFY(!window.property("professorBackdrop").toBool());
    const auto clean=window.m_view->grabFramebuffer();QCOMPARE(clean.pixelColor(2,2),window.m_view->style().background);
}

void SmokeTest::patternViewportContinuity(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("PatternViewport");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();QTest::qWait(150);
    auto *pattern=window.findChild<QAction *>("PatternTemplate0");QVERIFY(pattern);
    pattern->trigger();QTest::qWait(100);
    auto *view=window.m_view;QVERIFY(view->isValid());
    const auto paper=window.property("appPatternPaper").value<QColor>();
    const auto frame=view->grabFramebuffer();QVERIFY(!frame.isNull());
    QCOMPARE(frame.pixelColor(2,2),view->style().background);
    QVERIFY(!uiPattern::active(view));
    auto *surface=window.findChild<QWidget*>("MoleculeViewport");QVERIFY(surface);
    // Compare the GL background to the same window-space motif rendered as a widget.
    QImage expected(surface->size(),QImage::Format_ARGB32);expected.fill(paper);
    {QPainter painter(&expected);uiPattern::paintBackdrop(painter,surface);}
    const auto shot=surface->grab().toImage();const qreal dpr=surface->devicePixelRatioF();
    const auto debugPath=qEnvironmentVariable("YOUNGCRAFT_VIEWPORT_PATTERN_REVIEW");
    if(!debugPath.isEmpty()){shot.save(debugPath+".surface.png");expected.save(debugPath+".expected.png");}
    for(int y=10;y<surface->height();y+=37)for(int x=10;x<surface->width();x+=41){
        const auto inView=view->geometry().adjusted(0,0,-1,-1);
        if(inView.contains(x,y))continue;
        const auto a=shot.pixelColor(qRound(x*dpr),qRound(y*dpr)),b=expected.pixelColor(x,y);
        QVERIFY2(qAbs(a.red()-b.red())<9 && qAbs(a.green()-b.green())<9,qPrintable(QString("Pattern continuity at %1,%2: actual %3 expected %4").arg(x).arg(y).arg(a.name(),b.name())));
    }
    Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.3f,0,0}}};molecule.setBond(0,1,1);
    view->setMolecule(molecule);view->bestView();QTest::qWait(120);
    QVERIFY(!view->grabFramebuffer().isNull());
    const auto transparent=view->image(true,true);QVERIFY(!transparent.isNull());QVERIFY(transparent.pixelColor(0,0).alpha()<8);
    const QString path=qEnvironmentVariable("YOUNGCRAFT_VIEWPORT_PATTERN_REVIEW");
    if(!path.isEmpty())QVERIFY(window.grab().save(path));
    pattern->trigger();QVERIFY(!uiPattern::active(view));
    QVERIFY(view->image(true,true)==transparent);
}

void SmokeTest::patternTemplatesWorkflow(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("PatternTemplates");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();QTest::qWait(120);
    auto *top=window.findChild<QToolBar *>("MainToolBar");QVERIFY(top);
    auto *selector=window.findChild<QMenu *>("AppPatternSelector");QVERIFY(selector);
    QCOMPARE(selector->actions().size(),7);
    const auto style=window.m_view->style();
    QString capture=qEnvironmentVariable("YOUNGCRAFT_PATTERN_REVIEW");if(!capture.isEmpty())QDir().mkpath(capture);

    for(int i=0;i<6;++i){
        auto *pattern=window.findChild<QAction *>(QString("PatternTemplate%1").arg(i));QVERIFY(pattern);
        QVERIFY(selector->actions().contains(pattern));
        auto *color=window.findChild<QAction *>(QString("PastelTheme%1").arg(i));QVERIFY(color);
        pattern->trigger();QCOMPARE(top->property("patternTemplate").toInt(),i);
        QCOMPARE(QSettings().value("ui/patternTemplate").toInt(),i);QVERIFY(!uiPattern::active(top));
        QVERIFY(!uiPattern::active(window.m_analysis));
        for(int j=0;j<6;++j)QCOMPARE(window.findChild<QAction *>(QString("PatternTemplate%1").arg(j))->isChecked(),j==i);
        color->trigger();QCOMPARE(top->property("patternTemplate").toInt(),i);
        QCOMPARE(window.m_view->style().background,style.background);
        QTest::qWait(40);if(!capture.isEmpty())QVERIFY(window.grab().save(capture+QString("/pattern-%1.png").arg(i+1)));
    }
    {MainWindow restored;QCOMPARE(restored.findChild<QToolBar*>("MainToolBar")->property("patternTemplate").toInt(),5);}
    window.findChild<QAction *>("PatternTemplate5")->trigger();
    QCOMPARE(top->property("patternTemplate").toInt(),-1);
}

void SmokeTest::themesEmptySidebarAndScanApproach(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("OctoberUpdate");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();QTest::qWait(80);
    auto *plot=window.findChild<PlotWidget *>("OptimizationEnergyPlot");QVERIFY(plot);QVERIFY(plot->isHidden());
    auto *themes=window.findChild<QMenu *>("AppThemeSelector");QVERIFY(themes);
    const auto swatches=themes->actions();QCOMPARE(swatches.size(),7);
    QString errors;const auto library=FragmentLibrary::load(&errors);Fragment ligand,adduct;
    for(auto f:library){if(f.name=="12-MTP (Me3-TPADP)")ligand=f;if(f.name.startsWith("peroxo")&&f.name.contains("eta2"))adduct=f;}
    Molecule complex;QVERIFY2(LabComplexBuilder::assemble(ligand,adduct,26,3,1,6,complex,&errors),qPrintable(errors));
    window.newDrawingDocument(complex);window.setDrawingMode(false);window.m_view->bestView();
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_REVIEW_DIR");if(!capture.isEmpty())QDir().mkpath(capture);
    for(int i=0;i<swatches.size();++i){swatches[i]->trigger();QVERIFY(swatches[i]->isChecked());
        QCOMPARE(QSettings().value("ui/theme").toInt(),i);QTest::qWait(70);
        QCOMPARE(window.m_view->style().background,window.m_style.background);
        const auto shot=window.grab();
        if(window.m_view->isValid()){
            const auto pixels=shot.toImage();const auto ratio=shot.devicePixelRatio();
            const auto topLeft=window.m_view->mapTo(&window,QPoint(0,0));
            int colored=0;
            for(int y=qRound(topLeft.y()*ratio);y<qRound((topLeft.y()+window.m_view->height())*ratio);y+=3)
                for(int x=qRound(topLeft.x()*ratio);x<qRound((topLeft.x()+window.m_view->width())*ratio);x+=3){
                    const auto c=pixels.pixelColor(x,y);
                    if((c.blue()>c.red()*1.4&&c.blue()>c.green()*1.2)||(c.red()>c.green()*1.5&&c.red()>c.blue()*1.4))++colored;
                }
            QVERIFY2(colored>100,"Molecule must remain visible after changing application theme");
        }
        if(!capture.isEmpty())QVERIFY(shot.save(capture+QString("/theme-%1.png").arg(i)));
    }
    Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,1,"H",{1,0,0}},{3,8,"O",{6,0,0}},{4,1,"H",{6,1,0}}};
    molecule.setBond(0,1,1);molecule.setBond(2,3,1);for(auto &a:molecule.atoms)a.autoHydrogenLocked=true;
    window.newDrawingDocument(molecule);window.m_geometrySelection={0,2};
    QTimer::singleShot(0,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());QVERIFY(dialog);
        auto *distance=dialog->findChild<QDoubleSpinBox *>("ApproachDistance");QVERIFY(distance);distance->setValue(2.8);dialog->accept();});
    window.arrangeScanApproach();QVERIFY(qAbs(GeometryEditor::distance(window.m_view->molecule(),0,2)-2.8)<1e-5);
    window.undoDrawingEdit();for(int i=0;i<4;++i)QCOMPARE(window.m_view->molecule().atoms[i].position,molecule.atoms[i].position);
}
void SmokeTest::labLigandBrowserBuildsSelectedMetal(){
    MainWindow window;window.resize(1400,960);window.show();
    QTimer::singleShot(0,[&]{
        auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());QVERIFY(dialog);
        auto close=qScopeGuard([&]{if(dialog->isVisible())dialog->reject();});
        auto *category=dialog->findChild<QComboBox *>("FragmentCategory");QVERIFY(category);
        QCOMPARE(category->currentData().toString(),QString("lab-ligand"));QCOMPARE(category->currentText(),QStringLiteral("Ligands for BIOCC"));
        auto *list=dialog->findChild<QListWidget *>("FragmentList");QVERIFY(list);QVERIFY(list->count()>=5);
        auto matches=list->findItems("MDAP",Qt::MatchExactly);QCOMPARE(matches.size(),1);list->setCurrentItem(matches.first());
        auto *mode=dialog->findChild<QComboBox *>("FragmentInsertionMode");QVERIFY(mode);QCOMPARE(mode->currentData().toInt(),1);
        auto *metal=dialog->findChild<QComboBox *>("FragmentMetal");QVERIFY(metal);metal->setCurrentIndex(metal->findData(27));
        auto *charge=dialog->findChild<QSpinBox *>("FragmentTotalCharge");auto *spin=dialog->findChild<QSpinBox *>("FragmentMultiplicity");QVERIFY(charge);QVERIFY(spin);
        charge->setValue(1);spin->setValue(3);
        auto *preview=dialog->findChild<MoleculeView *>();QVERIFY(preview);QCOMPARE(preview->molecule().atoms[0].atomicNumber,27);QCOMPARE(preview->molecule().charge,1);
        const QString capture=qEnvironmentVariable("YOUNGCRAFT_EDITOR_REVIEW_DIR");
        if(!capture.isEmpty()){QDir().mkpath(capture);QTest::qWait(100);QVERIFY(dialog->grab().save(capture+"/lab-ligands-cobalt.png"));}
        dialog->accept();
    });
    window.openFragmentBrowser(true);
    QCOMPARE(window.m_view->molecule().atoms.size(),41);QCOMPARE(window.m_view->molecule().atoms[0].atomicNumber,27);
    QCOMPARE(window.m_view->molecule().charge,1);QCOMPARE(window.m_view->molecule().multiplicity,3);
    int donors=0;for(auto bond:window.m_view->molecule().bonds)if(bond.coordination)++donors;QCOMPARE(donors,4);
}
void SmokeTest::continuousSketchClicksStrokeRingAndUndo(){
    MainWindow window;window.resize(1500,1000);window.show();window.newDrawingDocument();
    auto *view=window.m_view;window.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();auto *toggle=window.m_drawBar->findChild<QPushButton *>("ContinuousDrawingButton");QVERIFY(toggle);QVERIFY(toggle->isChecked());
    auto style=StyleManager::preset("Youngcraft Studio");style.perspective=false;view->setStyle(style);view->m_rotation={};view->m_orthoScale=5;
    auto point=[&](const QVector3D &world){auto p=view->project(world,view->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    auto click=[&](const QPoint &p,Qt::KeyboardModifiers modifiers=Qt::NoModifier){QTest::mouseClick(view,Qt::LeftButton,modifiers,p);};
    click(point({0,0,0}));QCOMPARE(view->molecule().atoms.size(),1);QCOMPARE(view->m_chainAtom,0);
    view->m_orthoScale=5;
    auto mouse=[&](QPoint p,Qt::MouseButtons buttons=Qt::NoButton,Qt::KeyboardModifiers modifiers=Qt::NoModifier){
        QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,buttons,modifiers);QApplication::sendEvent(view,&event);};
    mouse(point({1.5f,.17f,0}));QVERIFY(view->hasGhostBond());
    QVERIFY(qAbs(view->ghostPosition().y())<.02f);click(point({1.5f,.17f,0}));
    QCOMPARE(view->molecule().atoms.size(),2);QCOMPARE(view->molecule().bonds.size(),1);
    QVERIFY(qAbs(GeometryEditor::distance(view->molecule(),0,1)-1.54)<1e-4);
    click(point(view->molecule().atoms[1].position+QVector3D(-.77f,1.3337f,0)));
    QCOMPARE(view->molecule().atoms.size(),3);QCOMPARE(view->molecule().bonds.size(),2);
    click(point(view->molecule().atoms[0].position));QCOMPARE(view->molecule().atoms.size(),3);QCOMPARE(view->molecule().bonds.size(),3);
    QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(view->m_chainAtom,-1);QVERIFY(!view->hasGhostBond());
    window.undoDrawingEdit();QCOMPARE(view->molecule().bonds.size(),2);QCOMPARE(view->m_chainAtom,-1);
    window.redoDrawingEdit();QCOMPARE(view->molecule().bonds.size(),3);
    // Empty-space drag starts a disconnected two-atom bond in one transaction.
    const auto start=point({-3,1,0}),end=point({-1.46f,1,0});
    QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);mouse(end,Qt::LeftButton);
    QVERIFY(view->hasGhostBond());QCOMPARE(view->molecule().atoms.size(),3);
    QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,end);QCOMPARE(view->molecule().atoms.size(),5);QCOMPARE(view->molecule().bonds.size(),4);
    window.undoDrawingEdit();QCOMPARE(view->molecule().atoms.size(),3);QCOMPARE(view->molecule().bonds.size(),3);
    // Auto-H reindexing must retain the heavy-atom anchor for subsequent clicks.
    window.newDrawingDocument();view=window.m_view;window.m_drawBar->findChild<QPushButton *>("ArmDrawingTool")->click();view->setStyle(style);view->m_rotation={};view->m_orthoScale=5;
    window.m_autoHButton->setChecked(true);click(point({0,0,0}));view->m_orthoScale=5;
    QCOMPARE(view->molecule().atoms.size(),5);click(point({1.54f,0,0}));QCOMPARE(view->molecule().atoms.size(),8);
    QVERIFY(view->m_chainAtom>=0);QCOMPARE(view->molecule().atoms[view->m_chainAtom].atomicNumber,6);
    QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(view->m_chainAtom,-1);
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_EDITOR_REVIEW_DIR");if(!capture.isEmpty()){QDir().mkpath(capture);QTest::qWait(100);QVERIFY(window.grab().save(capture+"/continuous-drawing.png"));}
}

void SmokeTest::pastedXYZWorkflow() {
    const auto raw=QStringLiteral("O 0 0 0\nH 0.95 0 0\nH -0.3 0.9 0");
    const auto parsed=XYZParser::parseText(raw);QVERIFY(parsed.success);QCOMPARE(parsed.data.molecule.atoms.size(),3);
    QVERIFY(XYZParser::parseText("3\nwater\n"+raw).success);
    QVERIFY(XYZParser::parseText("3\n\n"+raw).success);
    QVERIFY(!XYZParser::parseText("4\nwater\n"+raw).success);
    QVERIFY(!XYZParser::parseText("O nan 0 0").success);
    QVERIFY(!XYZParser::parseText("O 1e99 0 0").success);
    QVERIFY(!XYZParser::parseText("Bad 0 0 0").success);
    QVERIFY(!XYZParser::parseText("O 0 0 0\nbroken").success);
    QVERIFY(!XYZParser::parseText("").success);
    if(QGuiApplication::platformName()=="offscreen")return;
    MainWindow window;window.show();window.openFile("tests/data/mvp.out");
    QTRY_VERIFY_WITH_TIMEOUT(!window.m_data.molecule.isEmpty(),15000);
    const auto original=window.m_activeDocument;const auto originalData=window.m_data;
    auto *panel=window.m_analysis;
    auto *input=panel->findChild<QPlainTextEdit *>("CoordinatePasteInput");
    auto *render=panel->findChild<QPushButton *>("RenderPastedXYZ");QVERIFY(input && render);
    input->setPlainText("broken");render->click();QCOMPARE(window.m_activeDocument,original);
    input->setPlainText(raw);panel->setCoordinateMolecule(originalData.molecule);QCOMPARE(input->toPlainText(),raw);
    render->click();QTRY_VERIFY_WITH_TIMEOUT(window.m_activeDocument!=original && window.m_data.molecule.atoms.size()==3,15000);
    QCOMPARE(window.m_view->molecule().atoms.size(),3);QCOMPARE(window.m_data.format,QString("XYZ"));
    QCOMPARE(original->data.molecule.atoms.size(),originalData.molecule.atoms.size());
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_SI_PREVIEW");
    if(!capture.isEmpty()){auto *tabs=window.m_analysis->findChild<QTabWidget *>();tabs->setCurrentIndex(tabs->count()-1);QTest::qWait(200);QVERIFY(window.grab().save(capture+".xyz.png"));}
}
#include "reaction_capture_workflow.inc"



void SmokeTest::vibrationVectorLengthControl(){
    MainWindow window;window.show();Molecule molecule;
    molecule.atoms={{1,6,"C",{-1,0,0}},{2,8,"O",{1,0,0}}};molecule.setBond(0,1,1);
    window.newDrawingDocument(molecule);window.setDrawingMode(false);
    auto *panel=window.m_analysis;auto *length=panel->findChild<QDoubleSpinBox *>("FrequencyVectorLength");
    auto *cycle=panel->findChild<QDoubleSpinBox *>("FrequencyCycleTime");QVERIFY(length);QVERIFY(cycle);
    QCOMPARE(length->value(),1.0);QCOMPARE(length->minimum(),0.1);QCOMPARE(length->maximum(),20.0);
    bool correctlyPlaced=false;
    for(auto *form:panel->findChildren<QFormLayout *>()){
        int cycleRow=-1,lengthRow=-1;QFormLayout::ItemRole role;
        form->getWidgetPosition(cycle,&cycleRow,&role);form->getWidgetPosition(length,&lengthRow,&role);
        if(cycleRow>=0&&lengthRow==cycleRow+1)correctlyPlaced=true;
    }
    QVERIFY(correctlyPlaced);
    FrequencyMode mode;mode.displacement={{0,0.3f,0},{0,-0.3f,0}};
    window.m_view->setFrequencyMode(&mode);window.m_view->m_rotation=QQuaternion();
    auto style=window.m_view->style();style.showVibrationVectors=true;style.showAxes=false;style.showLabels=false;style.background=Qt::white;style.vectorColor=QColor(15,65,170);window.m_view->setStyle(style);
    auto span=[](const QImage &image){int top=image.height(),bottom=-1;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){auto c=image.pixelColor(x,y);if(c.blue()>110&&c.red()<70&&c.green()<110){top=qMin(top,y);bottom=qMax(bottom,y);}}return bottom-top;};
    const double amplitude=panel->frequencyAmplitude(),period=panel->frequencyCycleTime();
    QTRY_VERIFY(window.m_view->isValid());
    const int shortSpan=span(window.m_view->imageAtSize({560,420}));QVERIFY(shortSpan>20);
    length->setValue(2);QCOMPARE(window.m_view->m_vibrationVectorLengthFactor,2.0);
    const int longSpan=span(window.m_view->imageAtSize({560,420}));QVERIFY(longSpan>shortSpan*1.5);
    QCOMPARE(panel->frequencyAmplitude(),amplitude);QCOMPARE(panel->frequencyCycleTime(),period);
    for(int i=0;i<molecule.atoms.size();++i)QCOMPARE(window.m_view->molecule().atoms[i].position,molecule.atoms[i].position);
    window.newDrawingDocument(molecule);auto *second=window.m_analysis->findChild<QDoubleSpinBox *>("FrequencyVectorLength");
    QCOMPARE(second->value(),1.0);second->setValue(0.5);
    window.m_documentTabs->setCurrentIndex(window.m_documentTabs->count()-2);
    QCOMPARE(window.m_analysis,panel);QCOMPARE(length->value(),2.0);QCOMPARE(window.m_view->m_vibrationVectorLengthFactor,2.0);
    length->setValue(3);QCOMPARE(window.m_view->m_vibrationVectorLengthFactor,3.0);
    window.m_analysis->findChild<QCheckBox *>("FrequencyScaledVectors")->setChecked(true);
    QCOMPARE(window.m_view->m_vibrationVectorLengthFactor,3.0);QCOMPARE(window.m_view->m_vibrationVectorScaleFactor,amplitude);
}

void SmokeTest::vibrationVectorContrast(){
    MoleculeView view;view.resize(560,420);view.show();QTRY_VERIFY(view.isValid());
    Molecule molecule;molecule.atoms={{1,6,"C",{-1,0,0}},{2,8,"O",{1,0,0}}};
    molecule.setBond(0,1,1);view.setMolecule(molecule);view.m_rotation=QQuaternion();
    FrequencyMode mode;mode.displacement={{0,0.9f,0},{0,-0.9f,0}};view.setFrequencyMode(&mode);
    QImage montage(1120,840,QImage::Format_ARGB32);montage.fill(Qt::white);
    for(int theme=0;theme<2;++theme)for(int scaled=0;scaled<2;++scaled){
        auto style=StyleManager::preset("Youngcraft Studio");style.background=theme?QColor(18,24,32):QColor(Qt::white);
        style.vectorColor=style.background;style.showAxes=false;style.showLabels=false;style.shadows=false;
        style.showVibrationVectors=false;view.setStyle(style);view.setVibrationVectorScaleFactor(scaled?0.5:1.0);
        const auto baseline=view.imageAtSize({560,420});
        style.showVibrationVectors=true;view.setStyle(style);const auto rendered=view.imageAtSize({560,420});
        int colored=0,changed=0;
        for(int y=0;y<rendered.height();++y)for(int x=0;x<rendered.width();++x){
            const auto c=rendered.pixelColor(x,y);
            if(theme?(c.green()>150&&c.blue()>180&&c.red()<130):(c.blue()>110&&c.red()<70&&c.green()<110))++colored;
            if(c!=baseline.pixelColor(x,y))++changed;
        }
        QVERIFY2(colored>40,qPrintable(QString("theme=%1 scaled=%2 colored=%3").arg(theme).arg(scaled).arg(colored)));
        QVERIFY(changed>100);
        QPainter painter(&montage);painter.drawImage(scaled*560,theme*420,rendered);
    }
    auto style=view.m_style;style.background=Qt::white;style.vectorColor=QColor(135,15,35);view.setStyle(style);
    auto rendered=view.imageAtSize({560,420});int red=0;
    for(int y=0;y<rendered.height();++y)for(int x=0;x<rendered.width();++x){auto c=rendered.pixelColor(x,y);if(c.red()>100&&c.red()<170&&c.green()<45&&c.blue()<65)++red;}
    QVERIFY(red>40); // A sufficiently contrasting user color is preserved.
    style.background=Qt::black;style.vectorColor=QColor(255,255,255,5);view.setStyle(style);
    rendered=view.imageAtSize({560,420},true);int blue=0;
    for(int y=0;y<rendered.height();++y)for(int x=0;x<rendered.width();++x){auto c=rendered.pixelColor(x,y);if(c.alpha()>200&&c.blue()>110&&c.red()<70&&c.green()<110)++blue;}
    QVERIFY(blue>40); // Transparent exports remain visible when pasted on white.
    const auto path=qEnvironmentVariable("YOUNGCRAFT_VECTOR_PREVIEW");if(!path.isEmpty())QVERIFY(montage.save(path));
}

void SmokeTest::ligandReplacementWorkflow(){
    const auto library=FragmentLibrary::load();const Fragment *ligand=nullptr,*adduct=nullptr;
    for(const auto &f:library){if(f.category=="lab-ligand"&&!ligand)ligand=&f;if(f.category=="lab-adduct"&&!adduct)adduct=&f;}
    QVERIFY(ligand);QVERIFY(adduct);Molecule source;QString error;
    QVERIFY(LabComplexBuilder::assemble(*ligand,*adduct,26,3,1,6,source,&error));
    MainWindow window;window.newDrawingDocument(source);window.setDrawingMode(false);
    auto *action=window.findChild<QAction *>("ReplaceLigandTemplate");QVERIFY(action);
    const int initialTabs=window.m_documentTabs->count();
    // Cancel must leave the original tab and molecule untouched.
    QTimer::singleShot(0,&window,[&]{auto *d=window.findChild<QDialog *>("LigandReplacementDialog");QVERIFY(d);d->reject();});
    action->trigger();QCOMPARE(window.m_documentTabs->count(),initialTabs);QCOMPARE(window.m_view->molecule().atoms.size(),source.atoms.size());
    Molecule expected;QSet<int> removed;
    QTimer::singleShot(0,&window,[&]{
        auto *d=window.findChild<QDialog *>("LigandReplacementDialog");QVERIFY(d);
        auto *candidate=d->findChild<QComboBox *>("ReplacementCandidate");auto *templates=d->findChild<QComboBox *>("ReplacementTemplate");
        auto *apply=d->findChild<QPushButton *>("ApplyLigandReplacement");QVERIFY(candidate);QVERIFY(templates);QVERIFY(apply);
        const auto candidates=LigandReplacement::recognize(source,0,library);
        QVERIFY(candidate->currentIndex()>=0);const auto c=candidates[candidate->currentIndex()];removed=c.atoms;
        int target=-1;for(int i=0;i<templates->count();++i){const auto &f=library[templates->itemData(i).toInt()];if(f.category=="lab-ligand"&&f.name!=ligand->name&&f.attachments.size()==c.donors.size()){target=i;break;}}
        QVERIFY(target>=0);templates->setCurrentIndex(target);QVERIFY(apply->isEnabled());
        QVERIFY(LigandReplacement::replace(source,c,library[templates->currentData().toInt()],{},expected,&error));
        const QString capture=qEnvironmentVariable("YOUNGCRAFT_LIGAND_PREVIEW");
        if(!capture.isEmpty()){QTest::qWait(150);QVERIFY(d->grab().save(capture));}
        auto *spin=d->findChild<QSpinBox *>("ReplacementMultiplicity");QVERIFY(spin);spin->setValue(7);
        apply->click();
    });
    action->trigger();QCOMPARE(window.m_documentTabs->count(),initialTabs+1);
    QCOMPARE(window.m_view->molecule().atoms.size(),expected.atoms.size());QCOMPARE(window.m_view->molecule().multiplicity,7);
    for(int i=0;i<expected.atoms.size();++i)QCOMPARE(window.m_view->molecule().atoms[i].position,expected.atoms[i].position);
    window.m_documentTabs->setCurrentIndex(initialTabs-1);
    QCOMPARE(window.m_view->molecule().atoms.size(),source.atoms.size());
    for(int i=0;i<source.atoms.size();++i)QCOMPARE(window.m_view->molecule().atoms[i].position,source.atoms[i].position);
    // Selected reaction atoms are protected by default.
    window.m_view->selection().atoms={*removed.begin()};
    QTimer::singleShot(0,&window,[&]{auto *d=window.findChild<QDialog *>("LigandReplacementDialog");QVERIFY(d);
        auto *candidate=d->findChild<QComboBox *>("ReplacementCandidate");
        const auto candidates=LigandReplacement::recognize(source,0,library);
        for(int i=0;i<candidates.size();++i)if(candidates[i].atoms==removed)candidate->setCurrentIndex(i);
        QVERIFY(!d->findChild<QPushButton *>("ApplyLigandReplacement")->isEnabled());d->reject();
    });
    action->trigger();QCOMPARE(window.m_documentTabs->count(),initialTabs+1);
}

void SmokeTest::orbitalCacheAndDetachedWindow() {
    MainWindow owner; owner.resize(1200,800); owner.show();
    auto *open=owner.m_analysis->findChild<QPushButton*>("OpenOrbitalWindow"); QVERIFY(open);
    auto *dialog=owner.m_analysis->findChild<QDialog*>("OrbitalManagerWindow"); QVERIFY(dialog); QVERIFY(!dialog->isVisible());
    CalculationData data;data.format="FCHK";data.filePath="sample.fchk";
    data.orbitals={{1,-13.6,2,{},"Alpha"},{2,1.2,0,{},"Alpha"}};
    owner.m_analysis->setData(data);
    open->click(); QVERIFY(dialog->isVisible()); QVERIFY(!dialog->isModal());
    QTRY_VERIFY(dialog->screen()->availableGeometry().contains(dialog->frameGeometry()));
    const auto frame=dialog->frameGeometry();
    for(int repeat=0;repeat<4;++repeat){open->click();QTest::qWait(20);QCOMPARE(dialog->frameGeometry(),frame);}
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_ORBITAL_WINDOW_PREVIEW");
    if(!preview.isEmpty()){QTest::qWait(100);QVERIFY(dialog->grab().save(preview));}
    dialog->resize(420,480);QTest::qWait(50);
    QVERIFY(dialog->screen()->availableGeometry().contains(dialog->geometry()));
    if(!preview.isEmpty())QVERIFY(dialog->grab().save(preview+".small.png"));
    const auto smallSize=dialog->size();
    dialog->close(); QVERIFY(!dialog->isVisible()); open->click(); QVERIFY(dialog->isVisible());
    QTRY_VERIFY(dialog->screen()->availableGeometry().contains(dialog->frameGeometry()));
    QCOMPARE(dialog->size(),smallSize);
    // A user-moved window must be recovered from outside the screen without growing.
    dialog->move(-600,-200);open->click();
    QTRY_VERIFY(dialog->screen()->availableGeometry().contains(dialog->frameGeometry()));
    QCOMPARE(dialog->size(),smallSize);
    MoleculeView view;
    auto grid=std::make_shared<CubeGrid>(); grid->valid=true;grid->nx=grid->ny=grid->nz=2;
    grid->axisX={1,0,0};grid->axisY={0,1,0};grid->axisZ={0,0,1};grid->values={-1,1,-1,1,-1,1,-1,1};
    view.setCubeGrid(grid,.02f);
    QTRY_VERIFY_WITH_TIMEOUT(!view.m_surfaceBuilding,10000); QVERIFY(view.m_positiveSurface);
    const auto positive=view.m_positiveSurface,negative=view.m_negativeSurface;
    auto other=std::make_shared<CubeGrid>(*grid); view.setCubeGrid(other,.02f);
    QTRY_VERIFY_WITH_TIMEOUT(!view.m_surfaceBuilding,10000);
    view.setCubeGrid(grid,.02f); QVERIFY(!view.m_surfaceBuilding);
    QCOMPARE(view.m_positiveSurface,positive); QCOMPARE(view.m_negativeSurface,negative);
    auto style=view.style();style.isoValue=.04f;view.setStyle(style);
    QTRY_VERIFY_WITH_TIMEOUT(!view.m_surfaceBuilding,10000);
    style.isoValue=.02f;view.setStyle(style);QVERIFY(!view.m_surfaceBuilding);QCOMPARE(view.m_positiveSurface,positive);
    // Coalesce a burst; the final visible mesh must belong to the final grid/isovalue.
    for(int i=0;i<30;++i){style.isoValue=.03f+i*.001f;view.setStyle(style);}
    view.setCubeGrid(grid,.02f); QTRY_VERIFY_WITH_TIMEOUT(!view.m_surfaceBuilding,10000);
    QCOMPARE(view.m_positiveSurface,positive);
    view.clearCubeGrid();QVERIFY(!view.m_positiveSurface);
}

void SmokeTest::automaticRemoteConnectionAndHomo() {
    const auto source=qEnvironmentVariable("YOUNGCRAFT_AUTOMATIC_CHK");if(source.isEmpty())QSKIP("Automatic real-server test not requested");
    const auto org=QCoreApplication::organizationName(), app=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("AutomaticServerUi");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(org);QCoreApplication::setApplicationName(app);});
    QSettings s;s.setValue("orbitals/remoteEnabled",true);s.setValue("orbitals/autoGenerateHomo",true);s.setValue("orbitals/remoteHost",qEnvironmentVariable("YOUNGCRAFT_REMOTE_HOST"));
    s.setValue("orbitals/remotePort",10022);s.setValue("orbitals/remoteConfig",qEnvironmentVariable("YOUNGCRAFT_REMOTE_CONFIG"));
    s.setValue("orbitals/remoteGaussianDirectory","/opt/gaussian/g16");s.setValue("orbitals/remoteCheckpointDirectory","/home/skylark/chk");
    MainWindow window;window.resize(1450,1000);window.show();window.openFile(source);
    QTRY_VERIFY_WITH_TIMEOUT(window.m_view->m_cubeGrid&&window.m_view->m_positiveSurfaceVertexCount>0&&window.m_view->m_negativeSurfaceVertexCount>0,180000);
    auto *panel=window.m_analysis->findChild<OrbitalStudyWidget*>();QVERIFY(panel);
    QTRY_VERIFY_WITH_TIMEOUT(panel->findChild<QLabel*>("GaussianConnectionStatus")->property("connected").toBool(),30000);
    QVERIFY(panel->findChild<QLabel*>("OrbitalStatus")->text().contains(QStringLiteral("생성 완료")));
    auto *runner=panel->findChild<GaussianTools*>();QSignalSpy ready(runner,&GaussianTools::ready);QSignalSpy failed(runner,&GaussianTools::failed);
    auto *table=panel->findChild<QTableWidget*>("OrbitalTable");const auto homo=table->item(table->currentRow(),0)->text().toInt();
    panel->findChild<QPushButton*>("ShowLumo")->click();
    QCOMPARE(table->item(table->currentRow(),0)->text().toInt(),homo+1);
    QTRY_VERIFY_WITH_TIMEOUT(ready.size()==1||!failed.isEmpty(),180000);
    QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));QCOMPARE(ready.size(),1);
    const auto dest=qEnvironmentVariable("YOUNGCRAFT_AUTOMATIC_PREVIEW");if(!dest.isEmpty()){QTest::qWait(200);QVERIFY(window.grab().save(dest));}
}

void SmokeTest::realRemoteOrbitalResults() {
    const auto dir=qEnvironmentVariable("YOUNGCRAFT_REMOTE_RESULTS");if(dir.isEmpty())QSKIP("Real server results not supplied");
    const auto org=QCoreApplication::organizationName(), app=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("RemoteOrbitalsGui");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(org);QCoreApplication::setApplicationName(app);});
    QSettings().setValue("orbitals/autoGenerateHomo",false);QSettings().setValue("orbitals/remoteEnabled",true);QSettings().setValue("orbitals/remoteHost",qEnvironmentVariable("YOUNGCRAFT_REMOTE_HOST"));
    QSettings().setValue("orbitals/remotePort",10022);QSettings().setValue("orbitals/remoteConfig",qEnvironmentVariable("YOUNGCRAFT_REMOTE_CONFIG"));
    MainWindow w;w.resize(1450,1000);w.show();
    const auto loaded=FileDispatcher::load(dir+"/converted.fchk",{});QVERIFY2(loaded.parseResult.success,qPrintable(loaded.parseResult.error));w.onFileLoaded(loaded);
    auto *panel=w.m_analysis->findChild<OrbitalStudyWidget*>();QVERIFY(panel);
    QVERIFY(panel->findChild<QGroupBox*>("RemoteGaussianEnabled")->isChecked());
    QCOMPARE(panel->findChild<QLineEdit*>("remoteHost")->text(),qEnvironmentVariable("YOUNGCRAFT_REMOTE_HOST"));
    QCOMPARE(panel->findChild<QTableWidget*>("OrbitalTable")->rowCount(),loaded.parseResult.data.orbitals.size());
    QVERIFY(panel->findChild<QLabel*>("OrbitalStatus")->text().contains(QStringLiteral("계산 서버")));
    auto *runner=panel->findChild<GaussianTools*>();QVERIFY(runner);
    auto homo=std::make_shared<CubeGrid>(CubeParser::parse(dir+"/homo.cube"));QVERIFY(homo->valid);
    runner->ready(homo,{});QTRY_VERIFY_WITH_TIMEOUT(w.m_view->m_positiveSurfaceVertexCount>0 && w.m_view->m_negativeSurfaceVertexCount>0,10000);
    QTest::qWait(150);QVERIFY(w.m_view->image().save(dir+"/actual-homo-preview.png"));
    auto density=std::make_shared<CubeGrid>(CubeParser::parse(dir+"/density.cube"));auto potential=std::make_shared<CubeGrid>(CubeParser::parse(dir+"/potential.cube"));
    QVERIFY(density->valid&&potential->valid);runner->ready(density,potential);
    QTRY_VERIFY_WITH_TIMEOUT(w.m_view->m_positiveSurfaceVertexCount>0,10000);QVERIFY(w.m_view->m_potentialGrid);
    QTest::qWait(150);QVERIFY(w.m_view->image().save(dir+"/actual-mep-preview.png"));QVERIFY(w.grab().save(dir+"/actual-workspace.png"));
    auto *remote=panel->findChild<QGroupBox*>("RemoteGaussianEnabled");remote->setChecked(false);QVERIFY(!QSettings().value("orbitals/remoteEnabled").toBool());
    remote->setChecked(true);QVERIFY(QSettings().value("orbitals/remoteEnabled").toBool());
}

void SmokeTest::orbitalThemesAndPatterns() {
    MoleculeView view;view.resize(640,480);view.show();
    auto style=StyleManager::preset("Youngcraft Studio");
    style.background=Qt::white;style.isoValue=.04f;style.isoOpacity=.85f;
    view.setStyle(style);
    Molecule molecule;Atom atom;atom.index=1;atom.atomicNumber=6;atom.element="C";atom.radiusScale=.4f;
    molecule.atoms.push_back(atom);view.setMolecule(molecule);
    OrbitalStudyWidget panel;panel.setView(&view);
    auto *palette=panel.findChild<QComboBox*>("OrbitalPalette");
    auto *pattern=panel.findChild<QComboBox*>("OrbitalSurfacePattern");
    QVERIFY(palette);QVERIFY(pattern);QCOMPARE(palette->count(),9);QCOMPARE(pattern->count(),5);
    QSignalSpy changed(&panel,&OrbitalStudyWidget::surfaceStyleChanged);
    auto grid=std::make_shared<CubeGrid>();grid->valid=true;
    grid->nx=grid->ny=grid->nz=41;grid->origin={-4,-4,-4};
    grid->axisX={.2f,0,0};grid->axisY={0,.2f,0};grid->axisZ={0,0,.2f};
    for(int z=0;z<41;++z)for(int y=0;y<41;++y)for(int x=0;x<41;++x){
        const auto p=grid->origin+x*grid->axisX+y*grid->axisY+z*grid->axisZ;
        grid->values.push_back(p.x()*std::exp(-p.lengthSquared()));
    }
    auto *runner=panel.findChild<GaussianTools*>();QVERIFY(runner);runner->ready(grid,{});
    QTRY_VERIFY_WITH_TIMEOUT(view.m_positiveSurfaceVertexCount>0 && view.m_negativeSurfaceVertexCount>0,10000);
    QVERIFY(view.isValid());QVERIFY(view.m_program->isLinked());
    const auto generation=view.m_surfaceGeneration;
    for(int i=0;i<8;++i){
        palette->setCurrentIndex(i);QCOMPARE(palette->currentIndex(),i);
        QVERIFY(view.style().positiveSurfaceColor!=view.style().negativeSurfaceColor);
    }
    palette->setCurrentIndex(1);QCOMPARE(view.style().positiveSurfaceColor,QColor("#159a9c"));
    QCOMPARE(view.style().negativeSurfaceColor,QColor("#eb7865"));
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_ORBITAL_THEMES_PREVIEW");
    QImage sheet(1280,1440,QImage::Format_RGB32);sheet.fill(Qt::white);
    QPainter painter(&sheet);painter.setPen(QColor("#323a46"));
    const QStringList titles={"Solid / Classic","Dotted / Lagoon","Fine Dotted / Iris","Mesh / Botanical","Contour / Nordic","Dotted / Candy"};
    QList<QImage> images;
    for(int i=0;i<6;++i){
        palette->setCurrentIndex(i);pattern->setCurrentIndex(i<5?i:1);
        QCOMPARE(int(view.style().surfacePattern),i<5?i:1);
        const auto shot=view.imageAtSize(QSize(640,440),false,true);
        QVERIFY(!shot.isNull());
        if(!images.isEmpty())QVERIFY(shot!=images.last());
        images.append(shot);
        const int x=(i%2)*640,y=(i/2)*480;
        painter.drawText(QRect(x+20,y,600,40),Qt::AlignVCenter,titles[i]);
        painter.drawImage(x,y+40,shot);
    }
    painter.end();if(!capture.isEmpty())QVERIFY(sheet.save(capture));
    palette->setCurrentIndex(1);
    QList<QImage> patternImages;
    for(int i=0;i<5;++i){
        pattern->setCurrentIndex(i);
        const auto shot=view.imageAtSize(QSize(640,440),false,true);
        for(const auto &previous:patternImages)QVERIFY(shot!=previous);
        patternImages.append(shot);
    }
    QCOMPARE(view.m_surfaceGeneration,generation); // Appearance changes never recompute the cube.
    QVERIFY(changed.count()>8);
    auto custom=view.style();custom.positiveSurfaceColor=QColor("#123456");custom.surfacePattern=SurfacePattern::Contour;
    view.setStyle(custom);panel.setView(&view);QCOMPARE(palette->currentIndex(),8);QCOMPARE(pattern->currentIndex(),4);
    palette->setCurrentIndex(2);QCOMPARE(view.style().surfacePattern,SurfacePattern::Contour);
    // MEP retains its potential mapping while the same pattern controls apply.
    auto potential=std::make_shared<CubeGrid>(*grid);for(auto &value:potential->values)value=.025f;
    runner->ready(grid,potential);pattern->setCurrentIndex(1);
    QVERIFY(view.m_potentialGrid);QVERIFY(!view.style().showNegativeSurface);
    QVERIFY(!view.imageAtSize(QSize(1280,960),true,true).isNull());
    view.makeCurrent();QCOMPARE(view.glGetError(),GLenum(GL_NO_ERROR));view.doneCurrent();
}

void SmokeTest::orbitalResponsiveSidebar(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("OrbitalResponsiveSidebar");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow owner;owner.resize(1200,900);owner.show();Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.3f,0,0}}};molecule.setBond(0,1,1);
    owner.newDrawingDocument(molecule);owner.setDrawingMode(false);
    owner.m_data.filePath="a_very_long_checkpoint_filename_for_sidebar_width_validation.fchk";owner.m_data.format="FCHK";
    owner.m_data.orbitals={{1,-13.6,2,{},"Alpha"},{2,1.2,0,{},"Alpha"},{1,-12.1,1,{},"Beta"},{2,2.3,0,{},"Beta"}};
    owner.m_analysis->setData(owner.m_data);owner.m_analysis->setCurrentTab(6);
    auto *panel=owner.m_analysis->findChild<OrbitalStudyWidget*>();QVERIFY(panel);
    owner.m_analysis->findChild<QPushButton*>("OpenOrbitalWindow")->click();
    auto *scroll=owner.m_currentSidebar->findChild<QScrollArea*>("SidebarAnalysisScroll");QVERIFY(scroll);
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_ORBITAL_RESPONSIVE_REVIEW");if(!capture.isEmpty())QDir().mkpath(capture);
    for(int theme=0;theme<6;++theme){
        owner.findChild<QAction*>(QString("PastelTheme%1").arg(theme))->trigger();
        for(int width:{230,280,360,230}){
            owner.resizeDocks({owner.m_explorerDock},{width},Qt::Horizontal);QTest::qWait(60);
            QCOMPARE(scroll->horizontalScrollBar()->maximum(),0);
            QVERIFY(panel->width()<=panel->parentWidget()->width());
            for(auto *button:panel->findChildren<QPushButton*>()){
                if(!button->property("orbitalResponsiveButton").toBool())continue;
                QCOMPARE(button->height(),32);QVERIFY(!button->icon().isNull());QVERIFY(!button->toolTip().isEmpty());QVERIFY(!button->accessibleName().isEmpty());
                if(panel->width()<300){QCOMPARE(button->width(),32);QVERIFY(button->text().isEmpty());}
                const auto bounds=QRect(button->mapTo(panel,QPoint()),button->size());QVERIFY(bounds.left()>=0);QVERIFY(bounds.right()<panel->width());
            }
            auto *tabs=panel->findChild<QTabWidget*>("OrbitalAnalysisTables");QVERIFY(tabs->usesScrollButtons());QVERIFY(tabs->width()<=panel->width());
            if(!capture.isEmpty())QVERIFY(owner.grab().save(capture+QString("/theme-%1-width-%2.png").arg(theme).arg(width)));
        }
    }
    auto *original=panel->findChild<QTableWidget*>("OrbitalTable");QCOMPARE(original->columnCount(),5);QCOMPARE(original->rowCount(),4);
    panel->findChild<QPushButton*>("ExpandOrbitalTable")->click();
    auto *dialog=panel->findChild<QDialog*>("ExpandedOrbitalTableDialog");QVERIFY(dialog&&dialog->isVisible());
    auto *expanded=dialog->findChild<QTableWidget*>("ExpandedOrbitalTable");QCOMPARE(expanded->columnCount(),5);QCOMPARE(expanded->rowCount(),4);
    for(int row=0;row<4;++row)for(int col=0;col<5;++col)QCOMPARE(expanded->item(row,col)->text(),original->item(row,col)->text());
    expanded->setCurrentCell(2,0);QCOMPARE(original->currentRow(),2);
    if(!capture.isEmpty())QVERIFY(dialog->grab().save(capture+"/expanded-table.png"));dialog->close();
}

void SmokeTest::orbitalStudyControlsAndRendering() {
    MainWindow w;w.resize(1450,1000);w.show();
    auto loaded=FileDispatcher::load("tests/data/mvp.out",{});QVERIFY(loaded.parseResult.success);
    loaded.parseResult.data.format="FCHK";loaded.parseResult.data.route.clear();loaded.parseResult.data.scanDetected=false;
    loaded.parseResult.data.frequencies.clear();loaded.parseResult.data.scanPoints.clear();loaded.parseResult.data.ircPoints.clear();loaded.parseResult.data.excitedStates.clear();
    loaded.parseResult.data.orbitals={{1,-13.6,2,{},"Alpha"},{2,1.2,0,{},"Alpha"},{1,-12.1,1,{},"Beta"},{2,2.3,0,{},"Beta"}};
    w.newDrawingDocument(loaded.parseResult.data.molecule);w.setDrawingMode(false);
    w.onFileLoaded(loaded);QCOMPARE(w.m_analysis->currentTab(),6);
    auto *panel=w.m_analysis->findChild<OrbitalStudyWidget*>();QVERIFY(panel);
    auto *orbitals=panel->findChild<QTableWidget*>("OrbitalTable");QVERIFY(orbitals);QCOMPARE(orbitals->rowCount(),4);QCOMPARE(orbitals->currentRow(),0);
    auto *toolbar=w.findChild<QToolBar*>("MainToolBar");QVERIFY(toolbar);QCOMPARE(toolbar->toolButtonStyle(),Qt::ToolButtonIconOnly);
    for (auto *action : toolbar->actions()) {
        if (action->isSeparator()) continue;
        QVERIFY(!action->icon().isNull());
        QVERIFY(!action->toolTip().isEmpty());
    }
    auto grid=std::make_shared<CubeGrid>();grid->valid=true;grid->nx=grid->ny=grid->nz=33;grid->origin={-4,-4,-4};grid->axisX={.25f,0,0};grid->axisY={0,.25f,0};grid->axisZ={0,0,.25f};grid->molecule=w.m_view->molecule();
    auto potential=std::make_shared<CubeGrid>(*grid);
    for(int z=0;z<33;++z)for(int y=0;y<33;++y)for(int x=0;x<33;++x){const auto p=grid->origin+x*grid->axisX+y*grid->axisY+z*grid->axisZ;grid->values.push_back(std::exp(-2*p.lengthSquared()));potential->values.push_back(p.x()*.03f);}
    auto *runner=panel->findChild<GaussianTools*>();QVERIFY(runner);runner->ready(grid,potential);
    QTRY_VERIFY_WITH_TIMEOUT(w.m_view->m_positiveSurfaceVertexCount>0,10000);
    QVERIFY(w.m_view->m_potentialGrid);QVERIFY(!w.m_view->style().showNegativeSurface);QVERIFY(qAbs(w.m_view->style().isoValue-.001)<1e-6);
    auto *iso=panel->findChild<QDoubleSpinBox*>("OrbitalIsovalue");QVERIFY(iso);iso->setValue(.01);QVERIFY(qAbs(w.m_view->style().isoValue-.01)<1e-6);QCOMPARE(w.m_style.isoValue,w.m_view->style().isoValue);
    runner->ready(grid,potential);QCOMPARE(iso->value(),.01);
    runner->ready(grid,{});QCOMPARE(iso->value(),.02);
    iso->setValue(.035);
    runner->ready(grid,{});QCOMPARE(iso->value(),.035);
    runner->ready(grid,potential);QCOMPARE(iso->value(),.01);
    QTest::qWait(200);const QImage first=w.m_view->image();QVERIFY(!first.isNull());
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_ORBITALS_PREVIEW");if(!capture.isEmpty()){QVERIFY(w.grab().save(capture));QVERIFY(first.save(capture+".mep.png"));}
    const int old=w.m_documentTabs->currentIndex();auto *oldView=w.m_view;w.newDrawingDocument(grid->molecule);w.setDrawingMode(false);
    auto mo=std::make_shared<CubeGrid>(*grid);
    for(int z=0;z<33;++z)for(int y=0;y<33;++y)for(int x=0;x<33;++x){const auto p=mo->origin+x*mo->axisX+y*mo->axisY+z*mo->axisZ;mo->values[(z*33+y)*33+x]=p.x()*std::exp(-p.lengthSquared());}
    runner->ready(mo,{});QVERIFY(!oldView->m_potentialGrid);QVERIFY(!w.m_view->m_cubeGrid);
    QCOMPARE(iso->value(),.035);
    w.m_documentTabs->setCurrentIndex(old);QVERIFY(w.m_view==oldView);QCOMPARE(w.m_analysis->currentTab(),6);
    QTRY_VERIFY_WITH_TIMEOUT(oldView->m_positiveSurfaceVertexCount>0 && oldView->m_negativeSurfaceVertexCount>0,10000);QVERIFY(oldView->style().showNegativeSurface);
    if(!capture.isEmpty()){QTest::qWait(100);QVERIFY(oldView->image().save(capture+".fmo.png"));}
}


void SmokeTest::projectLibraryOpensOnDemand() {
    MainWindow window;
    window.show();
    QAction *action = window.findChild<QAction *>(QStringLiteral("ProjectLibraryAction"));
    QVERIFY(action != nullptr);
    QVERIFY(window.findChild<QDialog *>(QStringLiteral("ProjectLibraryDialog")) == nullptr);
    bool appeared = false;
    QTimer::singleShot(0, &window, [&]() {
        QDialog *dialog = window.findChild<QDialog *>(QStringLiteral("ProjectLibraryDialog"));
        appeared = dialog && dialog->isVisible();
        if (dialog) dialog->accept();
    });
    action->trigger();
    QVERIFY(appeared);
}

void SmokeTest::atomFocusAcrossWindowsAndTabs() {
    MainWindow source, other;
    Molecule molecule;
    molecule.atoms = {{1,6,"C",{}},{2,8,"O",{1.4f,0,0}},{3,1,"H",{-1,0,0}}};
    molecule.setBond(0,1,1); molecule.setBond(0,2,1);
    source.newDrawingDocument(molecule); source.setDrawingMode(false);
    auto *origin = source.m_view;
    source.newDrawingDocument(molecule); source.setDrawingMode(false);
    auto *inactive = origin;
    origin = source.m_view;
    other.newDrawingDocument(molecule); other.setDrawingMode(false);
    auto *matching = other.m_view;
    auto reordered = molecule;
    std::swap(reordered.atoms[0], reordered.atoms[1]);
    other.newDrawingDocument(reordered); other.setDrawingMode(false);
    auto *wrongOrder = other.m_view;
    auto shorter = molecule; shorter.atoms.removeLast(); shorter.bonds.clear();
    other.newDrawingDocument(shorter); other.setDrawingMode(false);
    auto *wrongCount = other.m_view;
    const auto camera = matching->cameraState();
    QVector<QVector3D> live{{2,3,4},{3.4f,3,4},{1,3,4}};
    matching->setCoordinates(live);
    source.applyAtomSelection({0,2});
    auto *hide = source.findChild<QAction *>("AtomFocusHideOthers");
    auto *dim = source.findChild<QAction *>("AtomFocusDimOthers");
    auto *reset = source.findChild<QAction *>("AtomFocusRestore");
    QVERIFY(hide); QVERIFY(dim); QVERIFY(reset);
    QVERIFY(source.m_atomFocusAllWindows->isChecked());
    hide->trigger();
    for (auto *view : {origin,inactive,matching}) {
        QVERIFY(view->molecule().atoms[0].visible);
        QVERIFY(!view->molecule().atoms[1].visible);
        QVERIFY(view->molecule().atoms[2].visible);
    }
    for (auto *view : {wrongOrder,wrongCount}) for (const auto &atom : view->molecule().atoms) QVERIFY(atom.visible);
    dim->trigger();
    for (auto *view : {origin,inactive,matching}) {
        QCOMPARE(view->molecule().atoms[0].displayOpacity, 1.0f);
        QCOMPARE(view->molecule().atoms[1].displayOpacity, 0.2f);
        QCOMPARE(view->molecule().atoms[2].displayOpacity, 1.0f);
        for (const auto &atom : view->molecule().atoms) QVERIFY(atom.visible);
    }
    QCOMPARE(matching->cameraState().rotation, camera.rotation);
    QCOMPARE(matching->cameraState().distance, camera.distance);
    for (int i=0;i<live.size();++i) QCOMPARE(matching->molecule().atoms[i].position,live[i]);
    QCOMPARE(origin->selection().atoms, QSet<int>({0,2}));
    if (const auto path=qEnvironmentVariable("YOUNGCRAFT_ATOM_FOCUS_PREVIEW"); !path.isEmpty()) {
        source.resize(1400,900); source.show(); QTest::qWait(150);
        QVERIFY(source.grab().save(path+"-window.png"));
    }
    // Switching tabs must retain the inactive document's new flags.
    const int first = source.m_documentTabs->currentIndex() == 0 ? 1 : 0;
    source.activateDocument(first);
    QCOMPARE(source.m_data.molecule.atoms[1].displayOpacity, 0.2f);
    source.m_atomFocusAllWindows->setChecked(false);
    reset->trigger();
    QCOMPARE(source.m_view->molecule().atoms[1].displayOpacity, 1.0f);
    QCOMPARE(matching->molecule().atoms[1].displayOpacity, 0.2f);
    // No selection is a no-op for emphasis; restore needs no selection.
    source.applyAtomSelection({}); source.clearGeometrySelection();
    hide->trigger();
    for (const auto &atom : source.m_view->molecule().atoms) QVERIFY(atom.visible);
    source.m_atomFocusAllWindows->setChecked(true);
    reset->trigger();
    for (auto *view : {origin,inactive,matching,wrongOrder,wrongCount})
        for (const auto &atom : view->molecule().atoms) { QVERIFY(atom.visible); QCOMPARE(atom.displayOpacity,1.0f); }
    // Selection from normal measurement clicks is also usable.
    source.handleGeometryPick(1); hide->trigger();
    QVERIFY(!source.m_view->molecule().atoms[0].visible);
    QVERIFY(source.m_view->molecule().atoms[1].visible);
    Frame frame; frame.coordinates=live; source.m_data.frames.push_back(frame);
    source.showFrame(0);
    QVERIFY(!source.m_view->molecule().atoms[0].visible);
    QCOMPARE(source.m_view->molecule().atoms[1].position,live[1]);
}

void SmokeTest::atomFocusTransparentExport() {
    MoleculeView view;
    view.resize(400,300); view.show();
    Molecule molecule; molecule.atoms = {{1,8,"O",{}}};
    auto style = view.style(); style.showAxes=false; style.showLabels=false;
    style.showProperties=false; style.shadows=false; style.antiAliasing=false;
    view.setStyle(style); view.setMolecule(molecule);
    QTest::qWait(100);
    if (!view.isValid()) QSKIP("OpenGL context unavailable");
    const auto solid = view.imageAtSize({400,300},true,true);
    QVERIFY(!solid.isNull());
    molecule.atoms[0].displayOpacity=0.2f; view.setAtomDisplay(molecule);
    const auto faded = view.imageAtSize({400,300},true,true);
    QVERIFY(!faded.isNull());
    const QPoint center(200,150);
    QVERIFY(qAlpha(solid.pixel(center)) > 250);
    QVERIFY(qAlpha(faded.pixel(center)) >= 45 && qAlpha(faded.pixel(center)) <= 58);
    const auto solidColor=solid.pixelColor(center), fadedColor=faded.pixelColor(center);
    QVERIFY(qAbs(solidColor.red()-fadedColor.red()) < 12);
    QVERIFY(qAbs(solidColor.green()-fadedColor.green()) < 12);
    QVERIFY(qAbs(solidColor.blue()-fadedColor.blue()) < 12);
    if (const auto path=qEnvironmentVariable("YOUNGCRAFT_ATOM_FOCUS_PREVIEW"); !path.isEmpty()) {
        QVERIFY(solid.save(path+"-solid.png")); QVERIFY(faded.save(path+"-faded.png"));
    }
    molecule.atoms[0].visible=false; view.setAtomDisplay(molecule);
    const auto hidden = view.imageAtSize({400,300},true,true);
    QCOMPARE(qAlpha(hidden.pixel(center)),0);
}

void SmokeTest::reactionCoordinatePresentationWorkflow() {
    MainWindow window;window.resize(1450,1000);window.show();
    window.findChild<QAction *>("OpenMechanismAnimation")->trigger();
    auto *dialog=window.findChild<MechanismAnimationDialog *>();QVERIFY(dialog);
    MechanismStep a;a.name="Reactant";a.seconds=1;
    a.molecule.atoms={{1,6,"C",{}},{2,8,"O",{1.4f,0,0}}};a.molecule.setBond(0,1,1);
    auto b=a;b.name="TS";b.molecule.atoms[1].position={2,0,0};auto c=a;c.name="Product";c.molecule.atoms[1].position={1.2f,1,0};
    QVERIFY(dialog->setSteps({a,b,c}));QVERIFY(!dialog->m_coordinateMode->isEnabled());QCOMPARE(dialog->m_coordinate->mappedTime(.5),.5);
    QImage artwork(320,200,QImage::Format_ARGB32);artwork.fill(Qt::white);
    {QPainter p(&artwork);p.setPen(QPen(QColor(55,60,80),4));p.drawLine(70,100,230,100);p.setFont(QFont("Arial",24));p.drawText(30,110,"C");p.drawText(240,110,"O");}
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);QVERIFY(artwork.save(&buffer,"PNG"));
    const QString encoded=QString::fromLatin1(png.toBase64());
    a.presentation={{"image",encoded},{"energy",0},{"coordinate",0}};
    b.presentation={{"image",encoded},{"energy",35},{"coordinate",1},{"dashed",true},{"mode",1},
        {"annotations",QJsonArray{QJsonObject{{"type","form"},{"points",QJsonArray{.2,.5,.7,.5}}},QJsonObject{{"type","break"},{"points",QJsonArray{.2,.6,.7,.6}}},QJsonObject{{"type","density"},{"points",QJsonArray{.2,.3,.7,.3}}},QJsonObject{{"type","orbital"},{"points",QJsonArray{.2,.8,.7,.8}}}}}};
    c.presentation={{"image",encoded},{"energy",-8},{"coordinate",2}};
    QVERIFY(dialog->setSteps({a,b,c}));QVERIFY(dialog->m_coordinateMode->isEnabled());QVERIFY(!dialog->m_coordinate->active());
    dialog->m_coordinateMode->click();QVERIFY(dialog->m_coordinate->active());
    QTest::qWait(80);QVERIFY(dialog->m_player->isVisible());QCOMPARE(dialog->m_coordinate->window(),dialog->m_player);
    QVERIFY(!dialog->m_player->isModal());
    QVERIFY(dialog->m_playerView->isVisible());QCOMPARE(dialog->m_playerView->window(),dialog->m_player);
    QVERIFY(dialog->m_player->screen()->availableGeometry().contains(dialog->m_player->frameGeometry()));
    auto *hide=dialog->findChild<QCheckBox *>("MechanismHideHydrogen");QVERIFY(hide);hide->setChecked(true);
    QVERIFY(!dialog->m_view->style().showHydrogen);QVERIFY(!dialog->m_playerView->style().showHydrogen);
    QCOMPARE(dialog->m_view->maximumHeight(),QWIDGETSIZE_MAX);
    dialog->m_playerSlider->setValue(2500);QCOMPARE(dialog->m_slider->value(),2500);
    dialog->m_playerLoop->setChecked(true);QVERIFY(dialog->m_loop->isChecked());dialog->m_loop->setChecked(false);QVERIFY(!dialog->m_playerLoop->isChecked());
    dialog->m_player->findChild<QPushButton *>("ReactionPlaybackStart")->click();QCOMPARE(dialog->m_time,0.);
    dialog->m_playerPlay->click();QTest::qWait(60);QVERIFY(dialog->m_timer.isActive());
    dialog->m_player->close();QVERIFY(!dialog->m_timer.isActive());QVERIFY(!dialog->m_player->isVisible());QVERIFY(dialog->m_coordinate->active());
    const auto pausedTime=dialog->m_time;dialog->m_playerOpen->click();QVERIFY(dialog->m_player->isVisible());QCOMPARE(dialog->m_time,pausedTime);
    dialog->m_player->showMaximized();QTest::qWait(60);QVERIFY(dialog->m_player->isMaximized());dialog->m_player->showNormal();
    QCOMPARE(dialog->m_coordinate->mappedTime(0),0.);QCOMPARE(dialog->m_coordinate->mappedTime(1),1.);QCOMPARE(dialog->m_coordinate->mappedTime(2),2.);
    QCOMPARE(dialog->m_coordinate->mappedTime(.5),.5);
    QVERIFY(!dialog->m_coordinate->position(.5).collision);
    b.presentation["impactBarrier"]=true;
    QVERIFY(dialog->setSteps({a,b,c}));
    QVERIFY(std::abs(dialog->m_coordinate->mappedTime(.5)-.5)>.1);
    QVERIFY(dialog->m_coordinate->position(.5).collision);QVERIFY(!dialog->m_coordinate->position(1.5).collision);
    dialog->seek(.5);const auto sampled=dialog->m_timeline.sample(dialog->m_coordinate->mappedTime(.5));
    QCOMPARE(dialog->m_view->molecule().atoms[1].position,sampled.atoms[1].position);
    QCOMPARE(dialog->m_playerView->molecule().atoms[1].position,sampled.atoms[1].position);
    QCOMPARE(qAlpha(dialog->m_coordinate->imageAtTime({900,420},.5,true).pixel(0,0)),0);
    const auto start=dialog->m_coordinate->imageAtTime({900,420},0),middle=dialog->m_coordinate->imageAtTime({900,420},.5),end=dialog->m_coordinate->imageAtTime({900,420},2);
    QVERIFY(start!=middle);QVERIFY(middle!=end);QVERIFY(qAlpha(middle.pixel(10,10))==255);
    QTemporaryDir dir;QVERIFY(dialog->saveProject(dir.filePath("coordinate.ycmechanism")));
    dialog->m_coordinateMode->click();QVERIFY(!dialog->m_coordinate->active());
    QVERIFY(dialog->openProject(dir.filePath("coordinate.ycmechanism")));QVERIFY(dialog->m_coordinate->active());QVERIFY(hide->isChecked());
    QCOMPARE(dialog->m_steps[1].presentation["annotations"].toArray().size(),4);QCOMPARE(dialog->m_coordinate->imageAtTime({900,420},.5),middle);
    auto invalid=dialog->m_steps;invalid[1].presentation.remove("energy");invalid[1].presentation.remove("impactBarrier");QVERIFY(dialog->setSteps(invalid));QVERIFY(dialog->m_coordinateMode->isEnabled());QVERIFY(!dialog->m_coordinate->position(.5).collision);
    invalid[1]=b;invalid[1].presentation["coordinate"]=0;QVERIFY(dialog->setSteps(invalid));QVERIFY(!dialog->m_coordinateMode->isEnabled());
    // Either input is sufficient, including partially populated presentations.
    auto imagesOnly=QVector<MechanismStep>{a,b,c};for(auto &step:imagesOnly)step.presentation.remove("energy");
    imagesOnly[1].presentation.remove("impactBarrier");
    QVERIFY(dialog->setSteps(imagesOnly));QVERIFY(dialog->m_coordinateMode->isEnabled());QVERIFY(dialog->m_coordinate->active());
    QVERIFY(dialog->m_coordinate->hasImages());QVERIFY(!dialog->m_coordinate->hasEnergies());QCOMPARE(dialog->m_coordinate->mappedTime(.5),.5);
    const auto pictureOnly=dialog->m_coordinate->imageAtTime({900,420},.5);QVERIFY(pictureOnly!=middle);
    QVERIFY(dialog->saveProject(dir.filePath("pictures-only.ycmechanism")));QVERIFY(dialog->openProject(dir.filePath("pictures-only.ycmechanism")));QVERIFY(dialog->m_coordinate->active());QVERIFY(!dialog->m_coordinate->hasEnergies());
    if(dialog->m_view->isValid())QVERIFY(dialog->exportAnimation(dir.filePath("pictures-only.gif"),"gif",480,3));
    auto energiesOnly=QVector<MechanismStep>{a,b,c};for(auto &step:energiesOnly)step.presentation.remove("image");
    QVERIFY(dialog->setSteps(energiesOnly));QVERIFY(dialog->m_coordinateMode->isEnabled());QVERIFY(dialog->m_coordinate->active());
    QVERIFY(!dialog->m_coordinate->hasImages());QVERIFY(dialog->m_coordinate->hasEnergies());QVERIFY(dialog->m_coordinate->position(.5).collision);
    const auto energyOnly=dialog->m_coordinate->imageAtTime({900,420},.5);QVERIFY(energyOnly!=pictureOnly);
    QVERIFY(dialog->saveProject(dir.filePath("energies-only.ycmechanism")));QVERIFY(dialog->openProject(dir.filePath("energies-only.ycmechanism")));QVERIFY(dialog->m_coordinate->active());QVERIFY(!dialog->m_coordinate->hasImages());
    if(dialog->m_view->isValid())QVERIFY(dialog->exportAnimation(dir.filePath("energies-only.gif"),"gif",480,3));
    auto partial=imagesOnly;partial[1].presentation.remove("image");partial[2].presentation.remove("image");
    QVERIFY(dialog->setSteps(partial));QVERIFY(dialog->m_coordinateMode->isEnabled());QVERIFY(dialog->m_coordinate->active());
    for(auto &step:partial)step.presentation.remove("image");partial[1].presentation["energy"]=35;
    QVERIFY(dialog->setSteps(partial));QVERIFY(dialog->m_coordinateMode->isEnabled());QVERIFY(dialog->m_coordinate->active());QVERIFY(!dialog->m_coordinate->position(.5).collision);
    for(auto &step:partial)step.presentation.remove("energy");
    QVERIFY(dialog->setSteps(partial));QVERIFY(!dialog->m_coordinateMode->isEnabled());QVERIFY(!dialog->m_coordinate->active());
    if(const auto path=qEnvironmentVariable("YOUNGCRAFT_COORDINATE_PREVIEW");!path.isEmpty()){QVERIFY(pictureOnly.save(path+".pictures-only.png"));QVERIFY(energyOnly.save(path+".energies-only.png"));}
    QVERIFY(dialog->setSteps({a,b,c}));dialog->m_coordinate->setEnabledPresentation(true);
    // Exercise the actual two-click annotation editor and transactional cancel.
    QTimer::singleShot(0,dialog,[&]{auto *editor=dialog->findChild<QDialog *>("ReactionCoordinateEditor");QVERIFY(editor);auto *canvas=editor->findChild<QWidget *>("ReactionAnnotationCanvas");QVERIFY(canvas);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(canvas->width()/3,canvas->height()/2));QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(2*canvas->width()/3,canvas->height()/2));editor->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();});
    dialog->m_coordinate->edit();QCOMPARE(dialog->m_steps[0].presentation["annotations"].toArray().size(),1);
    QTimer::singleShot(0,dialog,[&]{auto *editor=dialog->findChild<QDialog *>("ReactionCoordinateEditor");editor->findChild<QTableWidget *>("ReactionCoordinateStages")->item(0,2)->setText("999");editor->reject();});
    dialog->m_coordinate->edit();QCOMPARE(dialog->m_steps[0].presentation["energy"].toDouble(),0.);
    dialog->seek(.5);QTest::qWait(150);
    if(const auto path=qEnvironmentVariable("YOUNGCRAFT_COORDINATE_PREVIEW");!path.isEmpty()){QVERIFY(dialog->grab().save(path));QVERIFY(dialog->m_player->grab().save(path+".player.png"));QVERIFY(middle.save(path+".graph.png"));}
    if(dialog->m_view->isValid()){
        QVERIFY(dialog->exportAnimation(dir.filePath("coordinate.gif"),"gif",480,3));QImageReader reader(dir.filePath("coordinate.gif"));QVERIFY(reader.canRead());QCOMPARE(reader.imageCount(),7);
        const auto frame=reader.read();
        QVERIFY(!frame.isNull());QCOMPARE(qAlpha(frame.pixel(0,0)),0);
        const int split=frame.height()*5/9;
        auto hasContent=[&](int top,int bottom) {
            for(int y=top;y<bottom;++y)for(int x=0;x<frame.width();++x)
                if(qAlpha(frame.pixel(x,y)))return true;
            return false;
        };
        QVERIFY(hasContent(0,split));QVERIFY(hasContent(split,frame.height()));
    }
    dialog->close();
}

void SmokeTest::hydrogenRemovalActions() {
    Molecule source;source.atoms={{1,6,"C",{}},{2,1,"H",{1,0,0}},{3,8,"O",{0,1,0}},{4,1,"H",{0,2,0}},{5,7,"N",{0,0,1}},{6,1,"H",{0,0,2}},{7,1,"H",{3,3,3}}};
    source.setBond(0,1,1);source.setBond(0,2,1);source.setBond(2,3,1);source.setBond(0,4,1);source.setBond(4,5,1);
    auto carbon=source;MoleculeEditController::hydrogens(carbon,HydrogenAction::RemoveCarbonBound);QCOMPARE(carbon.atoms.size(),6);QCOMPARE(carbon.atoms[1].element,QString("O"));QCOMPARE(carbon.atoms[2].position,source.atoms[3].position);QVERIFY(MoleculeEditController::validate(carbon));
    auto keep=source;MoleculeEditController::hydrogens(keep,HydrogenAction::RemoveExceptSelected,{3,0});QCOMPARE(keep.atoms.size(),4);QCOMPARE(keep.atoms[2].element,QString("H"));QCOMPARE(keep.atoms[2].position,source.atoms[3].position);QVERIFY(MoleculeEditController::validate(keep));
    auto none=source;MoleculeEditController::hydrogens(none,HydrogenAction::RemoveExceptSelected,{0,-1,100});QCOMPARE(none.atoms.size(),source.atoms.size());
    auto all=source;MoleculeEditController::hydrogens(all,HydrogenAction::RemoveAll);QCOMPARE(all.atoms.size(),3);QVERIFY(MoleculeEditController::validate(all));
    MainWindow window;window.newDrawingDocument(source);const int count=window.m_view->molecule().atoms.size();
    auto *carbonAction=window.findChild<QAction *>("RemoveCarbonHydrogens"),*keepAction=window.findChild<QAction *>("RemoveUnselectedHydrogens");QVERIFY(carbonAction);QVERIFY(keepAction);
    window.applyAtomSelection({});keepAction->trigger();QCOMPARE(window.m_view->molecule().atoms.size(),count);
    {const QSignalBlocker block(window.m_autoHButton);window.m_autoHButton->setChecked(true);}carbonAction->trigger();
    QCOMPARE(window.m_view->molecule().atoms.size(),count-1);
    window.undoDrawingEdit();QCOMPARE(window.m_view->molecule().atoms.size(),count);window.redoDrawingEdit();QCOMPARE(window.m_view->molecule().atoms.size(),count-1);
    window.undoDrawingEdit();window.applyAtomSelection({3});keepAction->trigger();QCOMPARE(window.m_view->molecule().atoms.size(),4);window.undoDrawingEdit();QCOMPARE(window.m_view->molecule().atoms.size(),count);
}

void SmokeTest::wholeMechanismAnimationWorkflow() {
    MainWindow window;window.resize(1450,900);window.show();
    auto *action=window.findChild<QAction *>("OpenMechanismAnimation");QVERIFY(action);action->trigger();
    auto *dialog=window.findChild<MechanismAnimationDialog *>("MechanismAnimationDialog");QVERIFY(dialog);
    Molecule molecule;molecule.atoms={{1,6,"C",{}},{2,8,"O",{1.4f,0,0}},{3,7,"N",{0,1.4f,0}}};molecule.setBond(0,1,1);molecule.setBond(0,2,1);
    MechanismStep a;a.name="Reactant";a.molecule=molecule;a.seconds=.2;
    auto b=a;b.name="Intermediate";b.molecule.atoms[1].position={1.8f,0,.4f};
    auto c=b;c.name="Product";c.molecule.atoms[1].position={2.1f,0,.8f};
    auto withHydrogens=a;
    withHydrogens.molecule.atoms.append({4,1,"H",{-1,0,0}});
    withHydrogens.molecule.atoms.append({5,1,"H",{0,-1,0}});
    withHydrogens.molecule.setBond(0,3,1);withHydrogens.molecule.setBond(0,4,1);
    auto cleavage=withHydrogens;cleavage.name="Cleavage";cleavage.molecule.removeBond(0,1);
    QVERIFY(dialog->setSteps({withHydrogens,cleavage}));
    QVERIFY(!dialog->m_coordinate->active());
    dialog->m_playerOpen->click();QVERIFY(dialog->m_player->isVisible());
    dialog->m_table->setCurrentCell(1,0);QCOMPARE(dialog->m_time,.2);
    QVERIFY(dialog->m_playerBarrier->isEnabled());
    dialog->m_playerBarrier->click();
    const double hit=.2*.39;
    dialog->seek(hit);
    QVERIFY(dialog->m_coordinate->mappedTime(hit)<hit);
    QVERIFY(dialog->m_playerView->molecule().atoms[0].displayFlash>.5f);
    dialog->m_playerBarrier->click();QCOMPARE(dialog->m_coordinate->mappedTime(hit),hit);
    dialog->seek(hit);QCOMPARE(dialog->m_playerView->molecule().atoms[0].displayFlash,0.f);
    dialog->m_playerView->selection().atoms={0,3};
    auto *keepHydrogens=dialog->m_player->findChild<QPushButton *>("ReactionPlaybackKeepHydrogens");
    QVERIFY(keepHydrogens);keepHydrogens->click();
    for(auto *view:{dialog->m_view,dialog->m_playerView}) {
        QVERIFY(view->molecule().atoms[0].visible);QVERIFY(view->molecule().atoms[3].visible);
        QVERIFY(!view->molecule().atoms[4].visible);
    }
    auto *photo=qobject_cast<QPushButton*>(dialog->m_table->cellWidget(0,5));QVERIFY(photo);
    QTimer::singleShot(0,dialog,[&]{
        auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if(!menu)return;
        for(auto *candidate:menu->actions())if(candidate->isCheckable()) {
            menu->setActiveAction(candidate);QTest::keyClick(menu,Qt::Key_Return);return;
        }
        menu->close();
    });
    photo->click();
    QCOMPARE(dialog->m_time,0.);
    QCOMPARE(dialog->m_steps[0].presentation["photoCleavages"].toArray().size(),1);
    QVERIFY(dialog->playbackFrame(.1).atoms[1].displayFlash>0);
    QTemporaryDir options;
    QVERIFY(dialog->saveProject(options.filePath("selection.ycmechanism")));
    QVERIFY(dialog->openProject(options.filePath("selection.ycmechanism")));
    QVERIFY(!dialog->m_view->molecule().atoms[4].visible);
    dialog->m_player->close();
    QVERIFY(dialog->setSteps({a,b,c}));QTest::qWait(100);
    QVERIFY(dialog->m_caption->text().startsWith("Reactant"));
    QTimer::singleShot(0, dialog, [&] {
        auto *editor = dialog->findChild<QDialog *>("ReactionCaptionsDialog");
        if (!editor) return;
        const auto dismiss = qScopeGuard([&] { if (editor->isVisible()) editor->reject(); });
        auto *add = editor->findChild<QPushButton *>("AddReactionCaptionRange");
        auto *ranges = editor->findChild<QTableWidget *>("ReactionCaptionRanges");
        auto *buttons = editor->findChild<QDialogButtonBox *>();
        QVERIFY(add && ranges && buttons);
        add->click();
        ranges->setCurrentCell(0, 1);
        ranges->item(0, 0)->setText("Oxidation");
        qobject_cast<QDoubleSpinBox *>(ranges->cellWidget(0, 1))->setValue(.05);
        qobject_cast<QDoubleSpinBox *>(ranges->cellWidget(0, 2))->setValue(.15);
        buttons->button(QDialogButtonBox::Ok)->click();
    });
    dialog->m_reactionCaptionEdit->click();
    dialog->seek(.05); QVERIFY(dialog->m_caption->text().startsWith("Oxidation"));
    QVERIFY(dialog->m_playerCaption->text().startsWith("Oxidation"));
    dialog->seek(.15); QVERIFY(dialog->m_caption->text().startsWith("Reactant"));
    dialog->seek(0);
    const auto camera=dialog->m_view->cameraState();const auto first=dialog->m_view->molecule();
    dialog->seek(.3);QVERIFY((dialog->m_view->molecule().atoms[1].position-first.atoms[1].position).length()>.1f);
    QCOMPARE(dialog->m_view->cameraState().rotation,camera.rotation);QCOMPARE(dialog->m_view->cameraState().centerOffset,camera.centerOffset);
    dialog->seek(0);dialog->play(true);QTRY_VERIFY_WITH_TIMEOUT(dialog->m_time>0,3000);dialog->play(false);
    dialog->m_view->selection().atoms={0};dialog->emphasize(1);dialog->seek(.2);QCOMPARE(dialog->m_view->molecule().atoms[1].displayOpacity,.2f);
    QTemporaryDir dir;QVERIFY(dialog->saveProject(dir.filePath("mechanism.ycmechanism")));
    QVERIFY(dialog->openProject(dir.filePath("mechanism.ycmechanism")));QCOMPARE(dialog->m_focusMode,1);QCOMPARE(dialog->m_view->molecule().atoms[1].displayOpacity,.2f);
    dialog->seek(.1); QVERIFY(dialog->m_caption->text().startsWith("Oxidation"));
    dialog->seek(.2); QVERIFY(!dialog->m_caption->text().startsWith("Oxidation"));
    if(dialog->m_view->isValid()) {
        const auto captionCamera = dialog->m_view->cameraState();
        const auto withName = dialog->renderOutputFrame(.1, {320, 180}, true, captionCamera);
        dialog->m_useReactionCaptions = false;
        const auto withStructure = dialog->renderOutputFrame(.1, {320, 180}, true, captionCamera);
        dialog->m_useReactionCaptions = true;
        QVERIFY(!withName.isNull());
        QVERIFY(withName != withStructure);
        const auto gif=dir.filePath("mechanism.gif");QVERIFY2(dialog->exportAnimation(gif,"gif",320,10),qPrintable(dialog->m_status->text()));
        QImageReader reader(gif);QVERIFY(reader.canRead());QCOMPARE(reader.imageCount(),5);const auto firstImage=reader.read();reader.jumpToImage(4);const auto lastImage=reader.read();QVERIFY(!firstImage.isNull());QVERIFY(!lastImage.isNull());QVERIFY(firstImage!=lastImage);
        QVERIFY(dialog->exportAnimation(dir.path(),"png",320,10));const auto directories=QDir(dir.path()).entryList({"Youngcraft-Mechanism-*"},QDir::Dirs);QCOMPARE(directories.size(),1);QCOMPARE(QDir(dir.filePath(directories.first())).entryList({"*.png"},QDir::Files).size(),5);
        QTemporaryDir portrait;
        dialog->m_shotAspect=2;dialog->m_shotArrangement=2;dialog->m_shotAtoms={0,1};
        const auto base=dialog->m_view->cameraState();
        const auto molecule=dialog->playbackFrame(.2);
        const auto camera=dialog->shotCamera(molecule,base);
        const auto focus=(molecule.atoms[0].position+molecule.atoms[1].position)/2;
        QVERIFY((camera.centerOffset+dialog->m_fittedCenter-focus).length()<1e-5f);
        QVERIFY(camera.orthoScale<base.orthoScale);
        QVERIFY(dialog->saveProject(portrait.filePath("shot.ycmechanism")));
        QVERIFY(dialog->openProject(portrait.filePath("shot.ycmechanism")));
        QCOMPARE(dialog->m_shotAspect,2);
        QCOMPARE(dialog->m_shotAtoms,QSet<int>({0,1}));
        QVERIFY(dialog->exportAnimation(portrait.path(),"png",320,10));
        const auto portraitDirs=QDir(portrait.path()).entryList({"Youngcraft-Mechanism-*"},QDir::Dirs);
        QCOMPARE(portraitDirs.size(),1);
        const QImage portraitFrame(portrait.filePath(portraitDirs.first()+"/frame_000000.png"));
        QCOMPARE(portraitFrame.size(),QSize(320,568));
        dialog->m_shotAspect=0;dialog->m_shotArrangement=0;dialog->m_shotAtoms.clear();dialog->applyShotLayout();
        QFile original(gif);QVERIFY(original.open(QIODevice::ReadOnly));const auto bytes=original.readAll();original.close();
        QTimer cancel;cancel.setInterval(1);connect(&cancel,&QTimer::timeout,dialog,[&]{for(auto *progress:dialog->findChildren<QProgressDialog *>())progress->cancel();});cancel.start();
        QVERIFY(!dialog->exportAnimation(gif,"gif",320,10));cancel.stop();QVERIFY(original.open(QIODevice::ReadOnly));QCOMPARE(original.readAll(),bytes);
    }
    a.molecule.atoms[1].visible=false;a.molecule.atoms[2].displayOpacity=.2f;
    QVERIFY(dialog->setSteps({a,b,c}));QVERIFY(!dialog->m_view->molecule().atoms[1].visible);
    dialog->emphasize(2);QVERIFY(dialog->m_view->molecule().atoms[1].visible);
    dialog->m_parked={1};dialog->m_parkStart=0;dialog->m_timeline.park({1},0);dialog->fitTimeline();dialog->seek(.3);
    const auto savedTarget=dialog->m_view->m_target;
    QVERIFY(dialog->saveProject(dir.filePath("restored.ycmechanism")));QVERIFY(dialog->openProject(dir.filePath("restored.ycmechanism")));
    QVERIFY(dialog->m_view->molecule().atoms[1].visible);QCOMPARE(dialog->m_view->molecule().atoms[2].displayOpacity,1.f);
    QCOMPARE(dialog->m_parked,QSet<int>({1}));QVERIFY((dialog->m_view->m_target-savedTarget).length()<1e-5f);
    QFile input(dir.filePath("reaction.xyz"));QVERIFY(input.open(QIODevice::WriteOnly));
    const auto xyz=StructureTools::xyz(molecule).toUtf8();QCOMPARE(input.write(xyz),xyz.size());input.close();
    dialog->setSteps({});dialog->addFiles({input.fileName()});
    QTRY_VERIFY_WITH_TIMEOUT(!dialog->m_loading,5000);QVERIFY(dialog->m_valid);QCOMPARE(dialog->m_steps.size(),1);
    QCOMPARE(dialog->m_steps.first().molecule.atoms.size(),3);
    dialog->close();
}

void SmokeTest::mechanismPowerPointWorkflow() {
    const auto path=qEnvironmentVariable("YOUNGCRAFT_PPTX_TEST_OUTPUT");
    if(path.isEmpty())QSKIP("Optional Windows presentation export integration test");
    MainWindow window;window.show();window.openMechanismAnimation();
    auto *dialog=window.findChild<MechanismAnimationDialog *>();
    QVERIFY(dialog);
    MechanismStep a;a.name="Reactant";
    a.molecule.atoms={{1,6,"C",{}},{2,8,"O",{1.4f,0,0}}};
    a.molecule.setBond(0,1,1);a.presentation["energy"]=0;
    auto b=a;b.name="Intermediate";b.seconds=1;b.presentation["energy"]=15;
    b.molecule.atoms[1].position={2,0,0};b.molecule.removeBond(0,1);
    auto c=b;c.name="Product";c.seconds=1;c.presentation["energy"]=5;
    c.molecule.atoms[1].position={2,1,0};
    QVERIFY(dialog->setSteps({a,b,c}));
    QVERIFY(!dialog->m_coordinate->active());
    dialog->showPlaybackWindow();QTest::qWait(100);
    QVERIFY(dialog->m_pptx->isEnabled());
    QCOMPARE(dialog->presentationBoundaries(),QVector<double>({0,1,2}));
    dialog->seek(.5);
    auto *add=dialog->m_player->findChild<QPushButton *>("MechanismPresentationStops");
    QVERIFY(add);
    QTimer::singleShot(0,dialog,[&]{
        auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if(!menu)return;
        for(auto *action:menu->actions())if(action->isEnabled()&&!action->menu()&&!action->isSeparator()) {
            menu->setActiveAction(action);QTest::keyClick(menu,Qt::Key_Return);return;
        }
        menu->close();
    });
    add->click();
    QCOMPARE(dialog->presentationBoundaries(),QVector<double>({0,.5,1,2}));
    QTemporaryDir project;
    const auto projectPath=project.filePath("stops.ycmechanism");
    QVERIFY(dialog->saveProject(projectPath));QVERIFY(dialog->openProject(projectPath));
    QCOMPARE(dialog->presentationBoundaries(),QVector<double>({0,.5,1,2}));
    dialog->m_shotArrangement=2;
    const int width=qEnvironmentVariableIntValue("YOUNGCRAFT_PPTX_TEST_WIDTH");
    QVERIFY2(dialog->exportPowerPoint(path,width>0?width:1920,15),qPrintable(dialog->m_status->text()));
    QVERIFY(QFileInfo(path).size()>0);
    for(int slide=2;slide<=4;++slide) {
        QProcess unpack;
        unpack.start("tar",{"-xOf",path,QStringLiteral("ppt/slides/slide%1.xml").arg(slide)});
        QVERIFY(unpack.waitForFinished(10000));
        QCOMPARE(unpack.exitCode(),0);
        const auto xml=unpack.readAllStandardOutput();
        QVERIFY(xml.contains("playFrom"));
        QVERIFY(xml.contains("nodeType=\"withEffect\""));
    }
    const auto bundle=project.filePath("scenes.zip");
    QVERIFY2(dialog->exportPowerPoint(bundle,1280,15,true),qPrintable(dialog->m_status->text()));
    QProcess poster;
    poster.start("tar",{"-xOf",bundle,"003-end.png"});
    QVERIFY(poster.waitForFinished(10000));
    QCOMPARE(poster.exitCode(),0);
    QCOMPARE(QImage::fromData(poster.readAllStandardOutput(),"PNG").size(),QSize(1280,720));
}

void SmokeTest::suppliedMechanismPreview() {
    const auto project=qEnvironmentVariable("YOUNGCRAFT_MECHANISM_PROJECT");if(project.isEmpty())QSKIP("User preview project is optional");
    MainWindow window;window.openMechanismAnimation();auto *dialog=window.findChild<MechanismAnimationDialog *>("MechanismAnimationDialog");QVERIFY(dialog);QVERIFY(dialog->openProject(project));
    dialog->resize(1420,860);dialog->show();QTest::qWait(200);QVERIFY(dialog->m_view->isValid());
    QSet<int> keep;const auto molecule=dialog->m_timeline.sample(0);
    for(int i=0;i<molecule.atoms.size();++i)if(isMetal(molecule.atoms[i].atomicNumber)||molecule.atoms[i].atomicNumber==8)keep.insert(i);
    const auto core=keep;for(const auto &bond:molecule.bonds){if(core.contains(bond.first))keep.insert(bond.second);if(core.contains(bond.second))keep.insert(bond.first);}
    dialog->m_view->selection().atoms=keep;dialog->emphasize(1);dialog->m_view->selection().clear();
    QVERIFY(dialog->saveProject(project));
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_MECHANISM_PREVIEW");
    if(!preview.isEmpty()) {
        dialog->seek(dialog->m_timeline.duration()*.5);QTest::qWait(100);QVERIFY(dialog->grab().save(preview+"-window.png"));
        for(int i=0;i<3;++i){dialog->seek(dialog->m_timeline.duration()*i/2.);QVERIFY(dialog->m_view->imageAtSize({960,540},false,true).save(preview+QStringLiteral("-%1.png").arg(i)));}
        QVERIFY2(dialog->exportAnimation(preview+".gif","gif",640,25),qPrintable(dialog->m_status->text()));
    }
    dialog->close();
}


void SmokeTest::unlimitedSelectionAndHoverElements() {
    MainWindow w;w.resize(1380,900);w.show();
    Molecule m;for(int i=0;i<7;++i)m.atoms<<Atom{i+1,6,"C",{float(i*1.5),float(i%2),0}};
    w.newDrawingDocument(m);w.setDrawingMode(false);auto *view=w.m_view;view->bestView();QTest::qWait(100);
    auto screen=[&](int i){auto p=view->project(view->molecule().atoms[i].position,view->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    const int height=view->height();
    for(int i=0;i<7;++i)QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,screen(i));
    QCOMPARE(w.m_geometrySelection.size(),7);QCOMPARE(view->selection().atoms.size(),7);QCOMPARE(view->height(),height);
    QVERIFY(!w.m_geometryBar->isVisible());
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,screen(2));
    QCOMPARE(w.m_geometrySelection,QVector<int>({0,1,3,4,5,6}));QVERIFY(!view->selection().atoms.contains(2));
    // Tiny hand movement remains a click, not a drag.
    const auto pos=screen(2);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,pos);
    QMouseEvent move(QEvent::MouseMove,QPointF(pos+QPoint(2,1)),QPointF(view->mapToGlobal(pos+QPoint(2,1))),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(view,&move);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,pos+QPoint(2,1));
    QCOMPARE(view->selection().atoms.size(),7);
    w.setDrawingMode(true);view->setDrawingArmed(false);
    const auto target=screen(0);
    QMouseEvent hover(QEvent::MouseMove,QPointF(target),QPointF(view->mapToGlobal(target)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(view,&hover);QCOMPARE(view->hoveredAtom(),0);
    QTest::keyClick(&w,Qt::Key_N);QCOMPARE(view->molecule().atoms[0].atomicNumber,7);
    QTest::keyClick(&w,Qt::Key_C);QCOMPARE(view->molecule().atoms[0].atomicNumber,6);
    QTest::keyClick(&w,Qt::Key_C);QCOMPARE(view->molecule().atoms[0].atomicNumber,17);
    QCOMPARE(view->molecule().atoms.size(),7);
    w.undoDrawingEdit();QCOMPARE(view->molecule().atoms[0].atomicNumber,6);
    // Text inputs keep ordinary typing semantics.
    auto *input=new QLineEdit(&w);input->show();input->setFocus();QTest::keyClick(input,Qt::Key_O);
    QCOMPARE(input->text(),QString("o"));QCOMPARE(view->molecule().atoms[0].atomicNumber,6);delete input;
}
void SmokeTest::sidebarOptimizationFixedSelection() {
    MainWindow w;w.resize(1380,900);w.show();
    Molecule water;water.atoms={{1,8,"O",{2,3,1}},{2,1,"H",{3.2f,3,1}},{3,1,"H",{1.7f,4.1f,1}}};water.setBond(0,1,1);water.setBond(0,2,1);
    w.newDrawingDocument(water);w.setDrawingMode(false);w.startMMOptimization();
    QVERIFY(!QApplication::activeModalWidget());auto *panel=w.findChild<QWidget *>("MMOptimizationPanel");QVERIFY(panel);QVERIFY(w.m_structureDock->isVisible());
    QVERIFY(!w.m_moleculeWorkspace->isAncestorOf(w.m_mmBar));
    // Change selection *after* opening options: the editor must use the live set.
    QTest::qWait(30);auto *view=w.m_view;
    auto screen=[view](int i){auto p=view->project(view->molecule().atoms[i].position,view->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    const QPoint first=screen(0),second=screen(1);
    const QPoint start(qMin(first.x(),second.x())-35,qMin(first.y(),second.y())-35);
    const QPoint end(qMax(first.x(),second.x())+35,qMax(first.y(),second.y())+35);
    QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);
    QMouseEvent drag(QEvent::MouseMove,QPointF(end),QPointF(view->mapToGlobal(end)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(view,&drag);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,end);
    QCOMPARE(view->selection().atoms,QSet<int>({0,1}));
    auto *editor=static_cast<ConstraintEditor *>(panel->findChild<QWidget *>("ConstraintEditor"));
    editor->findChild<QPushButton *>("FreezeSelectedAtoms")->click();QVector<GeometryConstraint> entries;QString error;
    QVERIFY2(editor->read(entries,&error),qPrintable(error));QCOMPARE(entries.size(),2);
    panel->findChild<QPushButton *>("MMStart")->click();
    QTRY_VERIFY_WITH_TIMEOUT(w.m_mmState==MainWindow::MMState::Finished || w.m_mmState==MainWindow::MMState::Failed,30000);
    QVERIFY2(w.m_mmState==MainWindow::MMState::Finished,qPrintable(w.m_mmLabel->text()));
    for(int i:{0,1})QVERIFY((w.m_view->molecule().atoms[i].position-water.atoms[i].position).length()<1e-5);
    QVERIFY((w.m_view->molecule().atoms[2].position-water.atoms[2].position).length()>0.01);
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_SIDEBAR_PREVIEW");if(!preview.isEmpty())QVERIFY(w.grab().save(preview+"-mm.png"));
    for(int method:{0,2}) {
        w.newDrawingDocument(water);w.setDrawingMode(false);w.startXtbCalculation(method);
        auto *qm=w.findChild<XtbDialog *>();QVERIFY(qm);QVERIFY(!qm->isWindow());QVERIFY(w.m_structureDock->isAncestorOf(qm));
        w.applyAtomSelection({0,1});auto *constraints=static_cast<ConstraintEditor *>(qm->findChild<QWidget *>("ConstraintEditor"));
        constraints->findChild<QPushButton *>("FreezeSelectedAtoms")->click();
        QVERIFY(constraints->read(entries,&error));QCOMPARE(entries.size(),2);
        qm->findChild<QPushButton *>("XtbStart")->click();
        QTRY_VERIFY_WITH_TIMEOUT(!qm->calculationActive(),45000);
        QVERIFY2(qm->findChild<QPushButton *>("XtbApply")->isEnabled(),qPrintable(qm->findChild<QLabel *>("XtbStatus")->text()));
        for(int i:{0,1})QVERIFY((w.m_view->molecule().atoms[i].position-water.atoms[i].position).length()<1e-5);
        QVERIFY((w.m_view->molecule().atoms[2].position-water.atoms[2].position).length()>0.01);
        if(!preview.isEmpty())QVERIFY(w.grab().save(preview+QString("-qm%1.png").arg(method)));
        qm->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    }
    // Metal-containing MM takes GFN-FF directly, retaining a frozen atom.
    Molecule salt;salt.atoms={{1,11,"Na",{0,0,0}},{2,17,"Cl",{3.2f,0,0}}};salt.atoms[0].formalCharge=1;salt.atoms[1].formalCharge=-1;salt.setBond(0,1,1,true);
    w.newDrawingDocument(salt);w.setDrawingMode(false);w.startMMOptimization();panel=w.findChild<QWidget *>("MMOptimizationPanel");
    w.applyAtomSelection({0});editor=static_cast<ConstraintEditor *>(panel->findChild<QWidget *>("ConstraintEditor"));editor->findChild<QPushButton *>("FreezeSelectedAtoms")->click();
    panel->findChild<QPushButton *>("MMStart")->click();auto *metal=w.findChild<XtbDialog *>();QVERIFY(metal);
    QCOMPARE(metal->findChild<QComboBox *>("XtbMethod")->currentData().toInt(),0);
    QTRY_VERIFY_WITH_TIMEOUT(metal->findChild<QLabel *>("XtbStatus")->text()!=QStringLiteral("계산 준비"),3000);
    QTRY_VERIFY_WITH_TIMEOUT(!metal->calculationActive(),45000);
    QVERIFY2(metal->findChild<QPushButton *>("XtbApply")->isEnabled(),qPrintable(metal->findChild<QLabel *>("XtbStatus")->text()));
    QVERIFY2(!metal->findChild<QLabel *>("XtbStatus")->text().contains(QStringLiteral("失敗")),qPrintable(metal->findChild<QLabel *>("XtbStatus")->text()));
    QVERIFY((w.m_view->molecule().atoms[0].position-salt.atoms[0].position).length()<1e-5);
    metal->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    // Switching documents cannot start an old options panel on a new molecule.
    w.startMMOptimization();auto *oldPanel=w.findChild<QWidget *>("MMOptimizationPanel");w.newDrawingDocument(water);
    oldPanel->findChild<QPushButton *>("MMStart")->click();QVERIFY(!w.m_mmProcess);
}
void SmokeTest::bilingualCategorizedHelp() {
    MainWindow w;w.resize(1280,850);w.show();auto *toolbar=w.findChild<QToolBar *>("MainToolBar");
    auto *action=w.findChild<QAction *>("YoungcraftHelpAction");QVERIFY(action);QVERIFY(toolbar->actions().contains(action));
    QCOMPARE(action->shortcut(),QKeySequence::HelpContents);action->trigger();
    auto *help=w.findChild<QDialog *>("YoungcraftHelp");QVERIFY(help);
    auto *language=help->findChild<QComboBox *>("HelpLanguage");auto *tree=help->findChild<QTreeWidget *>("HelpCategories");auto *content=help->findChild<QTextBrowser *>("HelpContent");
    QVERIFY(language);QVERIFY(tree);QVERIFY(content);QCOMPARE(tree->topLevelItemCount(),11);
    language->setCurrentIndex(0);tree->setCurrentItem(tree->topLevelItem(0));
    QVERIFY(content->toPlainText().contains(QStringLiteral("할 수 있습니다.")));QVERIFY(content->toPlainText().contains("(Ctrl+N)"));
    language->setCurrentIndex(1);QVERIFY(content->toPlainText().contains("New molecule (Ctrl+N)"));QVERIFY(content->toPlainText().contains("You can"));
    auto *search=help->findChild<QLineEdit *>("HelpSearch");search->setText("GFN-FF");
    QVERIFY(!tree->topLevelItem(5)->isHidden());QVERIFY(tree->topLevelItem(0)->isHidden());
    search->clear();language->setCurrentIndex(0);tree->setCurrentItem(tree->topLevelItem(5));
    const auto path=qEnvironmentVariable("YOUNGCRAFT_SIDEBAR_PREVIEW");if(!path.isEmpty())QVERIFY(help->grab().save(path+"-help-ko.png"));
    language->setCurrentIndex(1);if(!path.isEmpty())QVERIFY(help->grab().save(path+"-help-en.png"));
    action->trigger();QCOMPARE(w.findChildren<QDialog *>("YoungcraftHelp").size(),1);
    help->close();
}

void SmokeTest::periodicTableGrouping() {
    MainWindow w;w.show();w.newDrawingDocument();
    bool inspected=false;
    QTimer::singleShot(0,&w,[&]{
        auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());QVERIFY(dialog);
        auto find=[dialog](const QString &symbol){for(auto *button:dialog->findChildren<QPushButton *>())if(button->text()==symbol)return button;return static_cast<QPushButton *>(nullptr);};
        auto *tin=find("Sn"),*silicon=find("Si"),*carbon=find("C"),*chlorine=find("Cl");
        inspected=tin && silicon && carbon && chlorine;
        if(inspected)inspected=tin->styleSheet().contains("#d7e8fa") && silicon->styleSheet().contains("#eadcf5") && carbon->styleSheet().contains("#d9efdf") && dialog->width()<1000;
        const auto path=qEnvironmentVariable("YOUNGCRAFT_SIDEBAR_PREVIEW");if(!path.isEmpty())inspected=inspected && dialog->grab().save(path+"-periodic.png");
        if(chlorine)chlorine->click();else dialog->reject();
    });
    w.m_drawBar->findChild<QPushButton *>("DrawPeriodicTable")->click();
    QVERIFY(inspected);QCOMPARE(w.m_drawElement->currentText(),QString("Cl"));
}

int main(int argc, char **argv) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication application(argc, argv);
    SmokeTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_smoke.moc"

