#pragma once

#include "model/Molecule.h"
#include "compute/GeometryConstraints.h"
#include "render/CameraState.h"
#include "model/MoleculeEditController.h"
#include "model/FragmentLibrary.h"
#include "model/GeometryEditor.h"
#include "settings/StyleManager.h"
#include "io/FileDispatcher.h"

#include <QMainWindow>
#include <QHash>
#include <QByteArray>

class QAction;
class QLabel;
class QMenu;
class QTreeWidget;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QDockWidget;
class QProcess;
class QComboBox;
class QPushButton;
class QProgressBar;
class QStackedWidget;
class QTabWidget;
class QTimer;
class QCheckBox;

class SmokeTest;

namespace youngcraft {

class AnalysisPanel;
class AnimationController;
class MoleculeView;
class PlotWidget;
class GeometryMeasureBar;
class OutputSourceView;

class MainWindow final : public QMainWindow {
    friend class ::SmokeTest;
    friend class DocumentTabBar;
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void openFile(const QString &path = {});

signals:
    void documentLoaded(const QString &path, bool success);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onFileLoaded(const youngcraft::LoadedFile &loaded);
    void showFrame(int index);
    void showFrequency(int index);
    void showScanPoint(int index);
    void showIrcPoint(int index);
    void showAtom(int index);
    void updateAtomVisibility();
    void setCarbonHydrogensVisible(bool visible);
    void updatePanelActions();
    void toggleFrameAnimation();
    void toggleFrequencyAnimation();
    void restoreFrequencyGeometry();
    void openStyleEditor();
    void showCrystalMode(int mode);
    void applyStyle(const RenderStyle &style);
    void choosePreset(const QString &name);
    void copyImage();
    void openReactionCapture();
    void openMultiCapture();
    void openProjectLibrary();
    void saveImage();
    void exportAnimation();
    void copyAnimation();
    void toggleSelectedBond();
    void toggleBondEditMode();
    void handleBondPick(int index);
    void recountBonds(bool clearManual = false);
    void toggleGeometryMode();
    void handleGeometryPick(int index);
    void applyGeometryMeasurement(int kind, double value);
    void arrangeScanApproach();
    void saveLigandTemplate();

private:
    QVector<QImage> renderFrequencyAnimation(int frequency, const QSize &size, int frameCount, bool recount, QString *error);
    void synchronizeViews(MoleculeView *source);
    void applySynchronizedView(MoleculeView *view);
    void updateOutputSource(const QString &section={},int index=0,bool reveal=false);
    OutputSourceView *m_outputSource=nullptr;
    QDockWidget *m_outputDock=nullptr;
    void createUi();
    void createDocumentUi();
    void connectDocumentUi();
    void connectEditorControls();
    bool moveDocumentTo(int index, MainWindow *target);
    void detachDocument(int index);
    struct DocumentSession {
        QVector<QMetaObject::Connection> connections;
        CalculationData data;
        QString path, status;
        std::shared_ptr<CubeGrid> cube;
        MoleculeView *view = nullptr;
        AnalysisPanel *analysis = nullptr;
        QTreeWidget *explorer = nullptr;
        AnimationController *animation = nullptr;
        QStackedWidget *workspace = nullptr;
        QWidget *sidebar = nullptr, *moleculeWorkspace = nullptr, *tddftWorkspace = nullptr;
        PlotWidget *tddftPlot = nullptr;
        QLabel *tddftInfo = nullptr;
        GeometryMeasureBar *geometryBar = nullptr;
        QLabel *measurementReadout = nullptr;
        QWidget *drawBar = nullptr, *mmBar = nullptr;
        QWidget *crystalBar = nullptr;
        QComboBox *crystalMode = nullptr;
        QCheckBox *cellToggle = nullptr;
        std::shared_ptr<CrystalData> crystal;
        QPushButton *autoHButton = nullptr;
        QComboBox *drawElement = nullptr, *drawBondOrder = nullptr;
        QLabel *mmLabel = nullptr;
        QPushButton *mmPauseButton = nullptr, *mmStopButton = nullptr, *mmRestoreButton = nullptr;
        PlotWidget *mmPlot = nullptr;
        QProgressBar *mmProgress = nullptr;
        QVector<QPointF> mmSeries;
        QVector<QVector3D> mmOriginalCoords;
        QVector<Molecule> undoMolecules, redoMolecules;
        RenderStyle style;
        int frame = -1, frequency = -1, firstBondAtom = -1;
        bool dragAtoms = false, bondEdit = false, geometryMode = false, drawingMode = false, resumeAnimation = false;
        QVector<int> geometrySelection;
    };
    void saveDocumentState();
    void bindDocument(const std::shared_ptr<DocumentSession> &document);
    void activateDocument(int index);
    void closeDocument(int index);
    std::shared_ptr<DocumentSession> captureDocument();
    void createMenus();
    void createToolBar();
    void populateExplorer();
    void addRecentFile(const QString &path);
    void clearDocument();
    void setStatus(const QString &message, int timeout = 4000);
    void showToolMessage(const QString &title, const QString &message);
    void updateWindowTitle();
    void setPerspective(bool perspective);
    bool isTextInputFocused() const;
    void clearGeometrySelection();
    void updateGeometryReadout();
    void syncEditedGeometry(const Molecule &molecule, const QString &warning);
    void newDrawingDocument(const Molecule &initial = {});
    void setDrawingMode(bool enabled);
    void addDrawnAtom(const QVector3D &position, int bondFrom = -1);
    void commitEditedMolecule(const Molecule &molecule, bool topologyChanged, bool adjustHydrogens = true);
    DrawState drawState() const;
    void setupEditorControls();
    void createBuildMenu();
    void ensureEditableDocument();
    void deleteSelection();
    void changeSelectionElement();
    void manipulateSelection();
    QSet<int> selectedStructureAtoms() const;
    void applyAtomSelection(const QSet<int> &ids);
    void copyStructureFragment();
    void pasteStructureFragment();
    void extractStructureFragments();
    void showZMatrix();
    void createGaussianInput();
    void openFragmentBrowser(bool labOnly = false);
    void insertCoordinationDialog();
    bool consumeInsertion(int target, const QVector3D &position, const QVector3D &direction = {1,0,0});
    void undoDrawingEdit();
    void redoDrawingEdit();
    void setAtomDragMode(bool enabled);
    void startXtbCalculation(int method=2, bool automatic=false, const QVector<GeometryConstraint> &constraints={}, int cycles=500);
    void buildLabComplex();
    enum class MMState { Idle, Starting, Running, Paused, Stopping, Finished, Failed };
    MMState m_mmState = MMState::Idle;
    void failMMOptimization(const QString &message);
    void startMMOptimization();
    void stopMMOptimization(const QString &message = {});
    void scheduleMMStep();
    void handleMMOutput();

    MoleculeView *m_view = nullptr;
    AnalysisPanel *m_analysis = nullptr;
    QTreeWidget *m_explorer = nullptr;
    QLabel *m_statusLabel = nullptr;
    AnimationController *m_animation = nullptr;
    CalculationData m_data;
    RenderStyle m_style;
    QAction *m_atomLabelsAction = nullptr;
    QAction *m_syncViewsAction = nullptr;
    QHash<int, CameraState> m_sharedCameras;
    QString m_currentFile;
    int m_currentFrequency = -1;
    int m_currentFrame = -1;
    bool m_dragAtoms = false;
    bool m_bondEditMode = false;
    bool m_geometryMode = false;
    bool m_drawingMode = false;
    bool m_mmStepPending = false;
    int m_mmStep = 0;
    int m_mmMaximum = 500;
    QString m_mmField;
    QProcess *m_mmProcess = nullptr;
    QByteArray m_mmOutput;
    QByteArray m_mmStderr;
    QVector<QPointF> m_mmSeries;
    QVector<QVector3D> m_mmOriginalCoords;
    int m_mmDisplayedIteration = 0;
    QString m_mmFinalMessage;
    QVector<Molecule> m_undoMolecules, m_redoMolecules;
    QWidget *m_drawBar = nullptr;
    QWidget *m_crystalBar = nullptr;
    QComboBox *m_crystalMode = nullptr;
    QCheckBox *m_cellToggle = nullptr;
    std::shared_ptr<CrystalData> m_crystal;
    QPushButton *m_autoHButton = nullptr;
    QWidget *m_mmBar = nullptr;
    QComboBox *m_drawElement = nullptr;
    QComboBox *m_drawBondOrder = nullptr;
    QLabel *m_mmLabel = nullptr;
    QPushButton *m_mmPauseButton = nullptr;
    QPushButton *m_mmStopButton = nullptr;
    QPushButton *m_mmRestoreButton = nullptr;
    PlotWidget *m_mmPlot = nullptr;
    QProgressBar *m_mmProgress = nullptr;
    QVector<int> m_geometrySelection;
    int m_firstBondAtom = -1;
    QMenu *m_recentMenu = nullptr;
    QAction *m_geometryAction = nullptr;
    QDockWidget *m_analysisDock = nullptr;
    QDockWidget *m_explorerDock = nullptr;
    QStackedWidget *m_workspace = nullptr;
    QWidget *m_moleculeWorkspace = nullptr;
    GeometryMeasureBar *m_geometryBar = nullptr;
    QLabel *m_measurementReadout = nullptr;
    QWidget *m_tddftWorkspace = nullptr;
    PlotWidget *m_tddftWorkspacePlot = nullptr;
    QLabel *m_tddftWorkspaceInfo = nullptr;
    QTabWidget *m_documentTabs = nullptr;
    QStackedWidget *m_documentArea = nullptr;
    QStackedWidget *m_sidebars = nullptr;
    QWidget *m_currentSidebar = nullptr;
    std::shared_ptr<DocumentSession> m_activeDocument;
    std::shared_ptr<DocumentSession> m_emptyDocument;
    QHash<QWidget *, std::shared_ptr<DocumentSession>> m_documents;
    QVector<QMetaObject::Connection> m_documentConnections;
};

} // namespace youngcraft
