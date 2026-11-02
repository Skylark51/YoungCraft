#include "compute/GaussianConnection.h"
#include <QGroupBox>
#include "ui/OutputSourceView.h"
#include "ui/AppPattern.h"
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
#include <QDir>
#include <QListWidget>
#include <QSignalBlocker>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSettings>
#include <QComboBox>
#include <QMenuBar>
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
#include <QScopeGuard>
#include <QFileInfo>
#include <QProcess>
#include <QProgressBar>
#include <QStandardPaths>
#include <QDialog>
#include <QTimer>
#include <QtTest>
#include "export/ImageExporter.h"

using namespace youngcraft;

class SmokeTest final : public QObject {
    Q_OBJECT
private slots:
    void projectLibraryOpensOnDemand();
    void additionalInteractionRequests();
    void directGeometryToolsAndFixedCore();
    void selectAllAndFragmentRotation();
    void optimizationGraphWindow();
    void patternTemplatesWorkflow();
    void patternViewportContinuity();
    void measurementBarPatternContinuity();
    void sidebarPatternContinuity();
    void dialogThemeSynchronization();
    void orbitalStudyControlsAndRendering();
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
    void crystalModesSwitchStructures();
    void styleEditorPreviewsImmediately();
    void editorInputTransactionsAndSelection();
    void editorPreservesCalculationFrames();
    void editorFragmentUndoAndKeyboard();
    void editorVisualPreview();
    void atomNumbersToolbarLabelsEveryAtom();
    void atomClicksMeasureDistanceAngleAndDihedral();
};

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
    auto *main=w.findChild<QToolBar *>("MainToolBar");auto *qm=w.findChild<QAction *>("QMOptimizationAction");QVERIFY(qm);const auto actions=main->actions();int index=actions.indexOf(qm);QVERIFY(index>0);QCOMPARE(actions[index-1]->text(),QStringLiteral("MM opt"));
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
    QAction *reset = nullptr;
    for (auto *action : window.findChildren<QAction *>())
        if (action->text() == QStringLiteral("수소 숨기기 초기화")) reset = action;
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
        auto *options=qobject_cast<QDialog *>(QApplication::activeModalWidget());QVERIFY(options);
        options->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
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
        auto *options=qobject_cast<QDialog *>(QApplication::activeModalWidget());QVERIFY(options);
        auto *editor=options->findChild<QWidget *>("ConstraintEditor");QVERIFY(editor);
        static_cast<ConstraintEditor *>(editor)->setEntries(entries);
        options->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
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
    QCOMPARE(window.m_analysis->currentTab(),0); QVERIFY(window.m_explorerDock->isVisible());
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
    const QStringList expected = {QStringLiteral("새 구조"),QStringLiteral("열기"),QStringLiteral("구조 그리기"),
        "MM opt",QStringLiteral("이미지 복사"),QStringLiteral("맞춤 캡처"),QStringLiteral("수소 숨기기"),QStringLiteral("수소 숨기기 초기화"),
        QStringLiteral("가운데 배치"),QStringLiteral("베스트 뷰"),QStringLiteral("뷰 동기화")};
    const auto actions=toolbar->actions();
    for (int i=0;i<expected.size();++i) QCOMPARE(actions[i]->text(),expected[i]);
    auto *xyz=window.findChild<QAction *>("AnalysisPanelTab7"); QVERIFY(xyz);
    QVERIFY(!toolbar->actions().contains(xyz));
    QVERIFY(window.findChild<QToolBar *>("AnalysisToolBar")->actions().contains(xyz));
    for (auto *action:actions) QVERIFY(action->text()!=QStringLiteral("TS 애니메이션 복사"));
}

void SmokeTest::contextualTsCopyButton() {
    AnalysisPanel panel; panel.show(); panel.setCurrentTab(1);
    auto *copy=panel.findChild<QPushButton *>("CopyFrequencyAnimation"); QVERIFY(copy);
    QVERIFY(!copy->isVisible());
    CalculationData data; FrequencyMode mode; mode.wavenumber=100;
    data.frequencies={mode}; panel.setData(data); QVERIFY(!copy->isVisible());
    mode.imaginary=true; mode.wavenumber=-200;
    data.frequencies={mode}; panel.setData(data); QVERIFY(copy->isVisible());
    QSignalSpy requested(&panel,&AnalysisPanel::animationCopyRequested);
    copy->click(); QCOMPARE(requested.count(),1);
    panel.setCurrentTab(0); QVERIFY(!copy->isVisible());
    panel.setCurrentTab(1); QVERIFY(copy->isVisible());
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
    QAction *removeHydrogen = nullptr, *resetHydrogen = nullptr;
    for (auto *action : toolbarActions) {
        if (action->text() == QStringLiteral("수소 숨기기")) removeHydrogen = action;
        if (action->text() == QStringLiteral("수소 숨기기 초기화")) resetHydrogen = action;
    }
    QVERIFY(removeHydrogen);
    QVERIFY(resetHydrogen);
    QCOMPARE(removeHydrogen->text(), QStringLiteral("수소 숨기기"));
    QCOMPARE(resetHydrogen->text(), QStringLiteral("수소 숨기기 초기화"));
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
    auto *toolbar = w.findChild<QToolBar *>("AnalysisToolBar"); QVERIFY(toolbar);
    QCOMPARE(w.toolBarArea(toolbar), Qt::TopToolBarArea);
    QCOMPARE(w.dockWidgetArea(w.m_explorerDock), Qt::LeftDockWidgetArea);
    QVERIFY(toolbar->geometry().top() > w.findChild<QToolBar *>("MainToolBar")->geometry().top());
    Molecule m; m.atoms = {{1,6,"C",{}}}; w.newDrawingDocument(m); w.setDrawingMode(false);
    const int first = w.m_documentTabs->currentIndex();
    auto *frequency = w.findChild<QAction *>("AnalysisPanelTab1"); QVERIFY(frequency);
    frequency->trigger(); QCOMPARE(w.m_analysis->currentTab(),1); QVERIFY(frequency->isChecked());
    auto *splitter = w.m_currentSidebar->findChild<QSplitter *>("SidebarPanelSplitter");
    // The sidebar itself is the splitter.
    if (!splitter) splitter = qobject_cast<QSplitter *>(w.m_currentSidebar);
    QVERIFY(splitter); QCOMPARE(splitter->orientation(), Qt::Vertical);
    auto *firstPanel = w.m_analysis;
    w.newDrawingDocument(m); w.setDrawingMode(false);
    auto *atoms = w.findChild<QAction *>("AnalysisPanelTab5"); QVERIFY(atoms);
    atoms->trigger(); QCOMPARE(w.m_analysis->currentTab(),5); QVERIFY(atoms->isChecked());
    w.m_documentTabs->setCurrentIndex(first);
    QCOMPARE(w.m_analysis, firstPanel); QCOMPARE(w.m_analysis->currentTab(),1);
    QVERIFY(frequency->isChecked()); QVERIFY(!atoms->isChecked());
    frequency->trigger(); QVERIFY(!w.m_explorerDock->isVisible()); QVERIFY(!frequency->isChecked());
    frequency->trigger(); QVERIFY(w.m_explorerDock->isVisible()); QVERIFY(frequency->isChecked());
    QVERIFY(w.m_view->height() > 200);
    const auto capture = qEnvironmentVariable("YOUNGCRAFT_TOP_PANEL_CAPTURE");
    if (!capture.isEmpty()) {
        w.openFile("tests/data/mvp.out");
        QTRY_VERIFY_WITH_TIMEOUT(w.m_workspace->property("loadFinished").toBool(), 10000);
        w.m_analysis->setCurrentTab(1); w.m_explorerDock->show();
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
    auto *toolbar = w.findChild<QToolBar *>("AnalysisToolBar"); QVERIFY(toolbar);
    const QStringList labels = {"Opt","Freq","Scan","IRC","TD-DFT","Atom Properties","Orbitals","XYZ"};
    for (int i=0; i<labels.size(); ++i) {
        auto *a=w.findChild<QAction *>(QString("AnalysisPanelTab%1").arg(i));
        QVERIFY(a); QCOMPARE(a->text(), labels[i]);
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
    auto *toolbar = window.findChild<QToolBar *>(QStringLiteral("AnalysisToolBar"));
    QVERIFY(action); QVERIFY(toolbar);
    auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
    QVERIFY(button);
    QCOMPARE(button->text(), QStringLiteral("Atomic number"));
    QVERIFY(!button->isChecked());
    auto *view = window.m_view;
    view->bestView();
    QTest::qWait(150);
    const bool visual = QGuiApplication::platformName() != "offscreen";
    QImage before;
    if (visual) { QVERIFY(view->isValid()); before = view->image(); }
    QTest::mouseClick(button, Qt::LeftButton);
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
    QVERIFY(!button->isChecked());
    window.m_documentTabs->setCurrentIndex(numberedTab);
    QVERIFY(button->isChecked());
    QVERIFY(window.m_view->style().showLabels);
    action->trigger(); // The menu and toolbar share the same action.
    QVERIFY(!button->isChecked());
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
    QCOMPARE(window.m_geometrySelection, QVector<int>({4}));
    QVERIFY(!readout->text().contains(QStringLiteral("거리 ")));
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
        auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); QVERIFY(dialog);
        dialog->findChild<QSpinBox *>()->setValue(120); dialog->accept();
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
    auto begin=[&]{QTimer::singleShot(10,&window,[&]{auto *d=qobject_cast<QDialog *>(QApplication::activeModalWidget());QVERIFY(d);d->findChild<QSpinBox *>()->setValue(120);d->accept();});window.startMMOptimization();};
    bool paused=false;
    connect(window.m_mmProgress,&QProgressBar::valueChanged,&window,[&](int step){if(step==48&&!paused){paused=true;window.m_mmPauseButton->click();}});
    begin();QTRY_VERIFY_WITH_TIMEOUT(paused,3000);QCOMPARE(window.m_mmState,MainWindow::MMState::Paused);QTest::qWait(200);QCOMPARE(window.m_mmStep,48);
    window.m_mmPauseButton->click();QTRY_VERIFY_WITH_TIMEOUT(window.m_mmStep>=64,3000);window.m_mmStopButton->click();QTRY_COMPARE_WITH_TIMEOUT(window.m_mmState,MainWindow::MMState::Finished,3000);int stopped=window.m_mmStep;QTest::qWait(100);QCOMPARE(window.m_mmStep,stopped);QVERIFY(!window.m_mmProcess);
    qputenv("YOUNGCRAFT_FAKE_CONVERGE","1");begin();QTRY_VERIFY_WITH_TIMEOUT(!window.m_mmProcess,3000);QCOMPARE(window.m_mmStep,64);QCOMPARE(window.m_mmState,MainWindow::MMState::Finished);QVERIFY(window.m_mmLabel->text().contains(QStringLiteral("수렴")));
    qunsetenv("YOUNGCRAFT_FAKE_CONVERGE");qunsetenv("YOUNGCRAFT_MM_TEST_WORKER");
}
void SmokeTest::xtbGeometryActions() {
    qputenv("YOUNGCRAFT_XTB_TEST_RUNTIME",QDir::current().filePath("xtb_runtime").toUtf8());
    MainWindow window;window.show();Molecule m;m.atoms={{1,8,"O",{0,0,0}},{2,1,"H",{1.2f,0,0}},{3,1,"H",{-0.3f,1.1f,0}}};m.rebuildBonds();window.newDrawingDocument(m);
    window.startXtbCalculation();auto *dialog=window.findChild<XtbDialog *>();QVERIFY(dialog);dialog->findChild<QPushButton *>("XtbStart")->click();
    auto *apply=dialog->findChild<QPushButton *>("XtbApply");QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(),15000);QVERIFY(dialog->findChild<QLabel *>("XtbStatus")->text().contains(QStringLiteral("최적화 수렴")));
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
    for (QAction *menuAction : window.menuBar()->actions()) {
        if (!menuAction->menu()) continue;
        for (QAction *action : menuAction->menu()->actions()) {
            if (action->text() == QStringLiteral("새 구조 그리기")) draw = action;
            if (action->text() == QStringLiteral("구조 최적화...")) optimize = action;
        }
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
    for (QAction *menuAction : window.menuBar()->actions()) {
        if (!menuAction->menu()) continue;
        for (QAction *action : menuAction->menu()->actions()) {
            if (action->text() == QStringLiteral("새 구조 그리기")) newDrawing = action;
            if (action->text() == QStringLiteral("실행 취소")) undo = action;
            if (action->text() == QStringLiteral("다시 실행")) redo = action;
        }
    }
    QVERIFY(newDrawing); QVERIFY(undo); QVERIFY(redo);
    newDrawing->trigger();
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("DocumentTabs"));
    QVERIFY(tabs);
    auto *view = tabs->currentWidget()->findChild<MoleculeView *>();
    auto *autoH = tabs->currentWidget()->findChild<QPushButton *>(QStringLiteral("AutoHydrogenButton"));
    auto *order = tabs->currentWidget()->findChild<QComboBox *>(QStringLiteral("DrawBondOrder"));
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
    QPushButton *deleteAtom = nullptr;
    for (QPushButton *button : tabs->currentWidget()->findChildren<QPushButton *>())
        if (button->text() == QStringLiteral("선택 원자 삭제")) deleteAtom = button;
    QVERIFY(deleteAtom);
    view->setSelectedAtom(1);
    deleteAtom->click();
    QCOMPARE(view->molecule().atoms.size(), 5); // Remaining C becomes CH4.
    undo->trigger();
    QCOMPARE(view->molecule().atoms.size(), 4);
    QAction *clearBonds = nullptr;
    for (QAction *menuAction : window.menuBar()->actions()) {
        if (!menuAction->menu()) continue;
        for (QAction *action : menuAction->menu()->actions())
            if (action->text() == QStringLiteral("모든 결합 지우기")) clearBonds = action;
    }
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
    QAction *draw = nullptr;
    for (QAction *action : toolbar->actions())
        if (action->text() == QStringLiteral("구조 그리기")) draw = action;
    QVERIFY(draw);
    QWidget *button = toolbar->widgetForAction(draw);
    QVERIFY(button);

    QTest::mouseClick(button, Qt::LeftButton);
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
    QTest::mouseClick(button, Qt::LeftButton);
    QVERIFY(window.isVisible());
    QCOMPARE(tabs->count(), 2);
    view = tabs->currentWidget()->findChild<MoleculeView *>();
    QVERIFY(view);
    QVERIFY(view->isVisible());
    QVERIFY(!view->molecule().atoms.isEmpty());
}

void SmokeTest::drawingModeCreatesAtomsAndBonds() {
    MainWindow window;
    window.resize(1000, 700);
    window.show();
    QAction *drawAction = nullptr;
    QAction *optimizeAction = nullptr;
    for (QAction *menuAction : window.menuBar()->actions()) {
        if (!menuAction->menu()) continue;
        for (QAction *action : menuAction->menu()->actions()) {
            if (action->text() == QStringLiteral("새 구조 그리기")) drawAction = action;
            if (action->text() == QStringLiteral("구조 최적화...")) optimizeAction = action;
        }
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
        if (candidate->isVisible() && candidate->text() == QStringLiteral("선택 원자 → 현재 원소")) replace = candidate;
    QVERIFY(replace);
    replace->click();
    QCOMPARE(view->molecule().atoms[1].atomicNumber, 7);
    QCOMPARE(view->molecule().atoms[1].element, QStringLiteral("N"));
    QVERIFY(!view->molecule().atoms[1].colorOverride.isValid());
    QPushButton *finish = nullptr;
    for (QPushButton *candidate : window.findChildren<QPushButton *>())
        if (candidate->isVisible() && candidate->text() == QStringLiteral("그리기 종료")) finish = candidate;
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
    QTest::qWait(1700);QVERIFY(splash.isVisible());QVERIFY(!ready.isVisible());QVERIFY(!completed);
    QTRY_VERIFY_WITH_TIMEOUT(!splash.isVisible(),1800);QVERIFY(ready.isVisible());QVERIFY(completed);QVERIFY(elapsed.elapsed()>=2000);QVERIFY(elapsed.elapsed()<3000);
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
            if (action->text() == QStringLiteral("분석 패널 표시")) restore = action;
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
    for (QPushButton *button : panel.findChildren<QPushButton *>()) buttonTexts.push_back(button->text());
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
    auto *toolbar = w.findChild<QToolBar *>(QStringLiteral("MainToolBar"));
    QVERIFY(toolbar->actions().contains(sync));
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
    window.findChild<QToolButton*>("PatternTemplate0")->click();
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
        window.findChild<QToolButton*>(QString("PastelTheme%1").arg(i))->click();
        for(QDialog *dialog:{static_cast<QDialog*>(xtb),static_cast<QDialog*>(&style),&nested,static_cast<QDialog*>(capture),static_cast<QDialog*>(multi)}){
            QCOMPARE(dialog->property("appPattern").toInt(),0);
            QCOMPARE(dialog->property("appPatternPaper"),window.property("appPatternPaper"));
            QCOMPARE(dialog->palette().color(QPalette::Window),window.palette().color(QPalette::Window));
            QVERIFY(dialog->property("sidebarPaperWatched").toBool());
        }
        aligned();
    }
    window.findChild<QToolButton*>("PastelTheme2")->click();
    capture->resize(1000,820);multi->resize(1000,820);QTest::qWait(50);aligned();
    capture->m_includeTs->setChecked(false);QTest::qWait(30);
    QCOMPARE(capture->m_views[0]->y(),capture->m_views[2]->y());QVERIFY(capture->m_views[0]->width()>capture->width()*.4);
    capture->m_includeTs->setChecked(true);capture->resize(1160,800);multi->resize(1200,800);QTest::qWait(50);aligned();
    const auto path=qEnvironmentVariable("YOUNGCRAFT_DIALOG_THEME_REVIEW");
    if(!path.isEmpty()){QTest::qWait(70);QVERIFY(xtb->grab().save(path));QVERIFY(style.grab().save(path+".style.png"));QVERIFY(capture->grab().save(path+".capture.png"));QVERIFY(multi->grab().save(path+".multi.png"));}
    window.findChild<QToolButton*>("PatternTemplate0")->click();QVERIFY(!uiPattern::active(xtb));QVERIFY(!uiPattern::active(&nested));
    nested.hide();style.hide();xtb->hide();
}

void SmokeTest::sidebarPatternContinuity(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("SidebarContinuity");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();window.m_explorer->show();QTest::qWait(100);
    auto verify=[&](QWidget *widget){
        QVERIFY(widget);const QImage actual=widget->grab().toImage();
        QImage expected(widget->size(),QImage::Format_ARGB32);expected.fill(Qt::transparent);
        {QPainter painter(&expected);uiPattern::paintBackdrop(painter,widget);}
        const auto ratio=widget->devicePixelRatioF();
        // Bottom half avoids list items and native header labels.
        for(int y=widget->height()/2;y<widget->height()-8;y+=23)for(int x=12;x<widget->width()-12;x+=29){
            const auto a=actual.pixelColor(qRound(x*ratio),qRound(y*ratio)),b=expected.pixelColor(x,y);
            QVERIFY2(qAbs(a.red()-b.red())<8 && qAbs(a.green()-b.green())<8,"Sidebar motif must use the same window-space origin as the molecular view");
        }
    };
    for(int i=0;i<6;++i){
        QTest::mouseClick(window.findChild<QToolButton*>(QString("PatternTemplate%1").arg(i)),Qt::LeftButton);QTest::qWait(30);
        verify(window.m_explorer->viewport());
        auto *list=window.m_analysis->findChild<QListWidget*>("FrameList");QVERIFY(list);verify(list->viewport());
    }
    window.resize(1350,860);QTest::qWait(60);verify(window.m_explorer->viewport());
    QTest::mouseClick(window.findChild<QToolButton*>("PatternTemplate0"),Qt::LeftButton);
    const auto path=qEnvironmentVariable("YOUNGCRAFT_SIDEBAR_CONTINUITY_REVIEW");
    if(!path.isEmpty())QVERIFY(window.grab().save(path));
    QTest::mouseClick(window.findChild<QToolButton*>("PatternTemplate0"),Qt::LeftButton);QVERIFY(!uiPattern::active(window.m_explorer));
}

void SmokeTest::measurementBarPatternContinuity(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("MeasurementPaper");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,950);window.show();
    window.findChild<QToolButton*>("PatternTemplate0")->click();QTest::qWait(70);
    auto *row=window.findChild<QWidget*>("WorkspaceMeasurementRow");QVERIFY(row);QVERIFY(row->height()<60);
    const auto actual=row->grab().toImage();QImage expected(row->size(),QImage::Format_ARGB32);
    expected.fill(Qt::transparent);{QPainter painter(&expected);uiPattern::paintBackdrop(painter,row);}
    const auto dpr=row->devicePixelRatioF();
    for(int x=5;x<row->width()-5;x+=13){const auto a=actual.pixelColor(qRound(x*dpr),qRound(2*dpr)),b=expected.pixelColor(x,2);QVERIFY(qAbs(a.red()-b.red())<3&&qAbs(a.green()-b.green())<3);}
    const auto path=qEnvironmentVariable("YOUNGCRAFT_MEASUREMENT_PAPER_PREVIEW");if(!path.isEmpty())QVERIFY(window.grab().save(path));
}

void SmokeTest::patternViewportContinuity(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("PatternViewport");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();QTest::qWait(150);
    auto *button=window.findChild<QToolButton*>("PatternTemplate0");QVERIFY(button);
    QTest::mouseClick(button,Qt::LeftButton);QTest::qWait(100);
    auto *view=window.m_view;QVERIFY(view->isValid());
    const auto paper=window.property("appPatternPaper").value<QColor>();
    const auto frame=view->grabFramebuffer();QVERIFY(!frame.isNull());
    QVERIFY(qAbs(frame.pixelColor(2,2).red()-paper.red())<10);
    auto *surface=window.findChild<QWidget*>("MoleculeViewport");QVERIFY(surface);
    // Compare the GL background to the same window-space motif rendered as a widget.
    QImage expected(surface->size(),QImage::Format_ARGB32);expected.fill(paper);
    {QPainter painter(&expected);uiPattern::paintBackdrop(painter,surface);}
    const auto shot=surface->grab().toImage();const qreal dpr=surface->devicePixelRatioF();
    const auto debugPath=qEnvironmentVariable("YOUNGCRAFT_VIEWPORT_PATTERN_REVIEW");
    if(!debugPath.isEmpty()){shot.save(debugPath+".surface.png");expected.save(debugPath+".expected.png");}
    for(int y=10;y<surface->height();y+=37)for(int x=10;x<surface->width();x+=41){
        const auto inView=view->geometry().adjusted(0,0,-1,-1);
        if(inView.contains(x,y) && qAbs(y-surface->height()/2)<30)continue;
        const auto a=shot.pixelColor(qRound(x*dpr),qRound(y*dpr)),b=expected.pixelColor(x,y);
        QVERIFY2(qAbs(a.red()-b.red())<9 && qAbs(a.green()-b.green())<9,qPrintable(QString("Pattern continuity at %1,%2: actual %3 expected %4").arg(x).arg(y).arg(a.name(),b.name())));
    }
    Molecule molecule;molecule.atoms={{1,6,"C",{0,0,0}},{2,8,"O",{1.3f,0,0}}};molecule.setBond(0,1,1);
    view->setMolecule(molecule);view->bestView();QTest::qWait(120);
    QVERIFY(!view->grabFramebuffer().isNull());
    const auto transparent=view->image(true,true);QVERIFY(!transparent.isNull());QVERIFY(transparent.pixelColor(0,0).alpha()<8);
    const QString path=qEnvironmentVariable("YOUNGCRAFT_VIEWPORT_PATTERN_REVIEW");
    if(!path.isEmpty())QVERIFY(window.grab().save(path));
    QTest::mouseClick(button,Qt::LeftButton);QVERIFY(!uiPattern::active(view));
    QVERIFY(view->image(true,true)==transparent);
}

void SmokeTest::patternTemplatesWorkflow(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("PatternTemplates");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();QTest::qWait(120);
    auto *top=window.findChild<QToolBar*>("MainToolBar");auto *bottom=window.findChild<QToolBar*>("AnalysisToolBar");
    auto *selector=window.findChild<QWidget*>("AppPatternSelector");QVERIFY(selector);QVERIFY(top);QVERIFY(bottom);
    QCOMPARE(selector->findChildren<QToolButton*>().size(),6);
    const auto style=window.m_view->style();
    QString capture=qEnvironmentVariable("YOUNGCRAFT_PATTERN_REVIEW");if(!capture.isEmpty())QDir().mkpath(capture);
    const QImage plain=top->grab().toImage();
    const QImage sidebarPlain=window.m_analysis->grab().toImage();
    for(int i=0;i<6;++i){
        auto *button=window.findChild<QToolButton*>(QString("PatternTemplate%1").arg(i));QVERIFY(button);QVERIFY(button->isVisible());
        auto *color=window.findChild<QToolButton*>(QString("PastelTheme%1").arg(i));QVERIFY(color);
        QCOMPARE(button->mapTo(&window,QPoint()).x(),color->mapTo(&window,QPoint()).x());
        QVERIFY(button->mapTo(&window,QPoint()).y()>color->mapTo(&window,QPoint()).y());
        QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(top->property("patternTemplate").toInt(),i);QCOMPARE(bottom->property("patternTemplate").toInt(),i);
        QCOMPARE(QSettings().value("ui/patternTemplate").toInt(),i);QVERIFY(top->grab().toImage()!=plain);
        QVERIFY(window.m_analysis->grab().toImage()!=sidebarPlain);
        for(int j=0;j<6;++j)QCOMPARE(window.findChild<QToolButton*>(QString("PatternTemplate%1").arg(j))->isChecked(),j==i);
        QTest::mouseClick(color,Qt::LeftButton);QCOMPARE(top->property("patternTemplate").toInt(),i);
        QCOMPARE(window.m_view->style().background,style.background);
        QTest::qWait(40);if(!capture.isEmpty())QVERIFY(window.grab().save(capture+QString("/pattern-%1.png").arg(i+1)));
    }
    {MainWindow restored;QCOMPARE(restored.findChild<QToolBar*>("MainToolBar")->property("patternTemplate").toInt(),5);}
    QTest::mouseClick(window.findChild<QToolButton*>("PatternTemplate5"),Qt::LeftButton);
    QCOMPARE(top->property("patternTemplate").toInt(),-1);QCOMPARE(bottom->property("patternTemplate").toInt(),-1);
}

void SmokeTest::themesEmptySidebarAndScanApproach(){
    const auto oldOrg=QCoreApplication::organizationName(),oldApp=QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("YoungcraftTest");QCoreApplication::setApplicationName("OctoberUpdate");QSettings().clear();
    auto cleanup=qScopeGuard([&]{QSettings().clear();QCoreApplication::setOrganizationName(oldOrg);QCoreApplication::setApplicationName(oldApp);});
    MainWindow window;window.resize(1500,960);window.show();QTest::qWait(80);
    auto *plot=window.findChild<PlotWidget *>("OptimizationEnergyPlot");QVERIFY(plot);QVERIFY(plot->isHidden());
    auto *themes=window.findChild<QWidget *>("AppThemeSelector");QVERIFY(themes);
    const auto swatches=themes->findChildren<QToolButton *>();QCOMPARE(swatches.size(),6);
    QString errors;const auto library=FragmentLibrary::load(&errors);Fragment ligand,adduct;
    for(auto f:library){if(f.name=="12-MTP (Me3-TPADP)")ligand=f;if(f.name.startsWith("peroxo")&&f.name.contains("eta2"))adduct=f;}
    Molecule complex;QVERIFY2(LabComplexBuilder::assemble(ligand,adduct,26,3,1,6,complex,&errors),qPrintable(errors));
    window.newDrawingDocument(complex);window.setDrawingMode(false);window.m_view->bestView();
    const QString capture=qEnvironmentVariable("YOUNGCRAFT_REVIEW_DIR");if(!capture.isEmpty())QDir().mkpath(capture);
    for(int i=0;i<swatches.size();++i){QTest::mouseClick(swatches[i],Qt::LeftButton);QVERIFY(swatches[i]->isChecked());
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
        QCOMPARE(category->currentData().toString(),QString("lab-ligand"));QVERIFY(category->currentText().startsWith("Lab Ligands"));
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
    for(int i=0;i<11;++i){QVERIFY(!toolbar->actions()[i]->icon().isNull());QVERIFY(!toolbar->actions()[i]->toolTip().isEmpty());}
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

int main(int argc, char **argv) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication application(argc, argv);
    SmokeTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_smoke.moc"

