#pragma once

#include "model/Molecule.h"
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

namespace youngcraft {

class AnalysisPanel;
class AnimationController;
class MoleculeView;
class PlotWidget;
class GeometryMeasureBar;

class MainWindow final : public QMainWindow {
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
    void toggleFrameAnimation();
    void toggleFrequencyAnimation();
    void restoreFrequencyGeometry();
    void openStyleEditor();
    void showCrystalMode(int mode);
    void applyStyle(const RenderStyle &style);
    void choosePreset(const QString &name);
    void copyImage();
    void saveImage();
    void exportAnimation();
    void toggleBondEditMode();
    void handleBondPick(int index);
    void recountBonds(bool clearManual = false);
    void toggleGeometryMode();
    void handleGeometryPick(int index);
    void applyGeometryMeasurement(int kind, double value);

private:
    void createUi();
    void createDocumentUi();
    struct DocumentSession {
        CalculationData data;
        QString path, status;
        std::shared_ptr<CubeGrid> cube;
        MoleculeView *view = nullptr;
        AnalysisPanel *analysis = nullptr;
        QTreeWidget *explorer = nullptr;
        AnimationController *animation = nullptr;
        QStackedWidget *workspace = nullptr;
        QWidget *sidebar = nullptr, *moleculeWorkspace = nullptr, *scanWorkspace = nullptr, *tddftWorkspace = nullptr;
        PlotWidget *scanPlot = nullptr, *tddftPlot = nullptr;
        QLabel *scanStatus = nullptr, *tddftInfo = nullptr;
        GeometryMeasureBar *geometryBar = nullptr;
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
    void syncEditedGeometry(const Molecule &molecule, const QString &warning);
    void newDrawingDocument(const Molecule &initial = {});
    void setDrawingMode(bool enabled);
    void addDrawnAtom(const QVector3D &position, int bondFrom = -1);
    void commitEditedMolecule(const Molecule &molecule, bool topologyChanged);
    void undoDrawingEdit();
    void redoDrawingEdit();
    void setAtomDragMode(bool enabled);
    void startMMOptimization();
    void stopMMOptimization(const QString &message = {});
    void scheduleMMStep();
    void handleMMOutput();
    void advanceMMAnimation();

    MoleculeView *m_view = nullptr;
    AnalysisPanel *m_analysis = nullptr;
    QTreeWidget *m_explorer = nullptr;
    QLabel *m_statusLabel = nullptr;
    AnimationController *m_animation = nullptr;
    CalculationData m_data;
    RenderStyle m_style;
    QString m_currentFile;
    int m_currentFrequency = -1;
    int m_currentFrame = -1;
    bool m_dragAtoms = false;
    bool m_bondEditMode = false;
    bool m_geometryMode = false;
    bool m_drawingMode = false;
    bool m_mmPaused = false;
    bool m_mmStepPending = false;
    int m_mmStep = 0;
    int m_mmMaximum = 500;
    QString m_mmField;
    QProcess *m_mmProcess = nullptr;
    QByteArray m_mmOutput;
    QByteArray m_mmStderr;
    QVector<QPointF> m_mmSeries;
    QVector<QVector3D> m_mmOriginalCoords;
    QVector<QVector<QVector3D>> m_mmFrameQueue;
    QVector<int> m_mmQueuedIterations;
    QVector<QVector3D> m_mmDisplayedCoords;
    QVector<QVector3D> m_mmTransitionStart;
    QTimer *m_mmAnimationTimer = nullptr;
    int m_mmTransitionTick = 0;
    int m_mmDisplayedIteration = 0;
    bool m_mmFinalPending = false;
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
    QWidget *m_scanWorkspace = nullptr;
    PlotWidget *m_scanWorkspacePlot = nullptr;
    QLabel *m_scanWorkspaceStatus = nullptr;
    GeometryMeasureBar *m_geometryBar = nullptr;
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
};

} // namespace youngcraft
