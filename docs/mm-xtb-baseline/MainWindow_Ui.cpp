#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include "ui/StyleEditorDialog.h"
#include "ui/GeometryMeasureBar.h"

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
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <functional>

namespace youngcraft {

void MainWindow::createUi() {
    m_documentArea = new QStackedWidget(this);
    m_documentTabs = new QTabWidget(m_documentArea);
    m_documentTabs->setObjectName(QStringLiteral("DocumentTabs"));
    m_documentTabs->setTabsClosable(true);
    m_documentTabs->setMovable(true);
    m_documentArea->addWidget(m_documentTabs);
    setCentralWidget(m_documentArea);
    m_explorerDock = new QDockWidget(QStringLiteral("Context Sidebar"), this);
    m_explorerDock->setObjectName(QStringLiteral("ResultExplorerDock"));
    m_sidebars = new QStackedWidget(m_explorerDock);
    m_explorerDock->setMinimumWidth(300);
    m_explorerDock->setWidget(m_sidebars);
    addDockWidget(Qt::LeftDockWidgetArea, m_explorerDock);
    resizeDocks({m_explorerDock}, {340}, Qt::Horizontal);
    m_statusLabel = new QLabel(QStringLiteral("준비"), this);
    statusBar()->addPermanentWidget(m_statusLabel, 1);
    createDocumentUi();
    m_emptyDocument = captureDocument();
    m_activeDocument = m_emptyDocument;
    m_documentArea->addWidget(m_workspace);
    m_documentArea->setCurrentWidget(m_workspace);
    m_analysis->setData({});
    populateExplorer();
    connect(m_documentTabs, &QTabWidget::currentChanged, this, &MainWindow::activateDocument);
    connect(m_documentTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeDocument);
}

void MainWindow::createDocumentUi() {
    m_view = new MoleculeView(this);
    m_view->setStyle(m_style);
    m_moleculeWorkspace = new QWidget(this);auto *moleculeLayout=new QVBoxLayout(m_moleculeWorkspace);moleculeLayout->setContentsMargins(0,0,0,0);moleculeLayout->setSpacing(2);moleculeLayout->addWidget(m_view,1);m_geometryBar=new GeometryMeasureBar(m_moleculeWorkspace);m_geometryBar->setVisible(false);moleculeLayout->addWidget(m_geometryBar);
    m_crystalBar = new QWidget(m_moleculeWorkspace);
    auto *crystalLayout = new QHBoxLayout(m_crystalBar);
    crystalLayout->setContentsMargins(8, 3, 8, 3);
    crystalLayout->addWidget(new QLabel(QStringLiteral("결정 표시"), m_crystalBar));
    m_crystalMode = new QComboBox(m_crystalBar);
    m_crystalMode->setObjectName(QStringLiteral("CrystalDisplayMode"));
    m_crystalMode->addItems({QStringLiteral("비대칭 단위"), QStringLiteral("단위격자"),
                             QStringLiteral("2 × 2 × 2 Packing")});
    crystalLayout->addWidget(m_crystalMode);
    m_cellToggle = new QCheckBox(QStringLiteral("단위격자 표시"), m_crystalBar);
    m_cellToggle->setObjectName(QStringLiteral("CrystalCellVisible"));
    m_cellToggle->setChecked(true);
    crystalLayout->addWidget(m_cellToggle);
    crystalLayout->addStretch();
    m_crystalBar->hide();
    moleculeLayout->addWidget(m_crystalBar);
    connect(m_crystalMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::showCrystalMode);
    connect(m_cellToggle, &QCheckBox::toggled, this, [this](bool visible) {
        if (m_crystal) m_view->setCrystalCell(m_crystal->axisA, m_crystal->axisB,
                                              m_crystal->axisC, m_crystalMode->currentIndex() == 2 ? 2 : 1,
                                              visible);
    });
    m_drawBar = new QWidget(m_moleculeWorkspace);
    auto *drawLayout = new QHBoxLayout(m_drawBar);
    drawLayout->setContentsMargins(8, 3, 8, 3);
    drawLayout->addWidget(new QLabel(QStringLiteral("원소"), m_drawBar));
    m_drawElement = new QComboBox(m_drawBar);
    m_drawElement->setObjectName(QStringLiteral("DrawElementSelector"));
    for (int z = 1; z <= 118; ++z) m_drawElement->addItem(elementSymbol(z), z);
    m_drawElement->setEditable(true);
    m_drawElement->setInsertPolicy(QComboBox::NoInsert);
    m_drawElement->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    m_drawElement->setCurrentText(QStringLiteral("C"));
    m_drawElement->setFixedWidth(72);
    drawLayout->addWidget(m_drawElement);
    const QStringList quickElements{QStringLiteral("H"), QStringLiteral("C"), QStringLiteral("N"),
        QStringLiteral("O"), QStringLiteral("B"), QStringLiteral("F"), QStringLiteral("Si"),
        QStringLiteral("P"), QStringLiteral("S"), QStringLiteral("Cl"), QStringLiteral("Br"), QStringLiteral("I")};
    QVector<QPushButton *> quickButtons;
    for (const QString &symbol : quickElements) {
        auto *button = new QPushButton(symbol, m_drawBar);
        button->setCheckable(true);
        button->setFixedWidth(symbol.size() == 1 ? 28 : 32);
        button->setObjectName(QStringLiteral("QuickElement_%1").arg(symbol));
        drawLayout->addWidget(button);
        quickButtons.push_back(button);
        connect(button, &QPushButton::clicked, m_drawElement, [selector = m_drawElement, symbol] {
            selector->setCurrentText(symbol);
        });
    }
    connect(m_drawElement, &QComboBox::currentTextChanged, m_drawBar,
            [this, quickButtons](const QString &symbol) {
        const int z = atomicNumberFor(symbol);
        if (z > 0) m_view->setDrawingElement(z);
        for (QPushButton *button : quickButtons) button->setChecked(button->text() == symbol);
    });
    for (QPushButton *button : quickButtons) button->setChecked(button->text() == QStringLiteral("C"));
    drawLayout->addWidget(new QLabel(QStringLiteral("결합"), m_drawBar));
    m_drawBondOrder = new QComboBox(m_drawBar);
    m_drawBondOrder->setObjectName(QStringLiteral("DrawBondOrder"));
    m_drawBondOrder->addItem(QStringLiteral("단일"), 1.0);
    m_drawBondOrder->addItem(QStringLiteral("이중"), 2.0);
    m_drawBondOrder->addItem(QStringLiteral("삼중"), 3.0);
    m_drawBondOrder->addItem(QStringLiteral("방향족"), 1.5);
    m_drawBondOrder->setFixedWidth(76);
    drawLayout->addWidget(m_drawBondOrder);
    for (int i = 0; i < 4; ++i) {
        auto *button = new QPushButton(i == 3 ? QStringLiteral("Ar") : QString::number(i + 1), m_drawBar);
        button->setFixedWidth(30);
        drawLayout->addWidget(button);
        connect(button, &QPushButton::clicked, m_drawBondOrder, [selector = m_drawBondOrder, i] {
            selector->setCurrentIndex(i);
        });
    }
    m_autoHButton = new QPushButton(QStringLiteral("수소 자동 추가"), m_drawBar);
    m_autoHButton->setObjectName(QStringLiteral("AutoHydrogenButton"));
    m_autoHButton->setCheckable(true);
    m_autoHButton->setChecked(false);
    m_autoHButton->setToolTip(QStringLiteral("구조 편집이 확정될 때 부족한 수소만 자동으로 보정합니다."));
    drawLayout->addWidget(m_autoHButton);
    connect(m_autoHButton, &QPushButton::toggled, this, [this](bool enabled) {
        if (enabled && !m_view->molecule().isEmpty())
            commitEditedMolecule(m_view->molecule(), true);
    });
    auto *replaceAtom = new QPushButton(QStringLiteral("선택 원자 → 현재 원소"), m_drawBar);
    drawLayout->addWidget(replaceAtom);
    connect(replaceAtom, &QPushButton::clicked, this, [this] {
        const int selected = m_view->selectedAtom();
        const int z = atomicNumberFor(m_drawElement->currentText());
        if (selected < 0 || selected >= m_view->molecule().atoms.size() || z <= 0) {
            setStatus(QStringLiteral("원자와 유효한 원소를 선택하세요.")); return;
        }
        Molecule edited = m_view->molecule();
        Atom &atom = edited.atoms[selected];
        atom.atomicNumber = z; atom.element = elementSymbol(z); atom.colorOverride = QColor();
        atom.autoHydrogen = false;
        commitEditedMolecule(edited, true);
        setStatus(QStringLiteral("선택 원자를 %1(으)로 변경했습니다.").arg(atom.element));
    });
    auto *deleteAtom = new QPushButton(QStringLiteral("선택 원자 삭제"), m_drawBar);
    drawLayout->addWidget(deleteAtom);
    connect(deleteAtom, &QPushButton::clicked, this, [this] {
        Molecule edited = m_view->molecule();
        if (!edited.removeAtom(m_view->selectedAtom())) { setStatus(QStringLiteral("삭제할 원자를 선택하세요.")); return; }
        m_view->setSelectedAtom(-1);
        commitEditedMolecule(edited, true);
        setStatus(QStringLiteral("선택 원자를 삭제했습니다."));
    });
    drawLayout->addStretch();
    auto *finishDrawing = new QPushButton(QStringLiteral("그리기 종료"), m_drawBar);
    drawLayout->addWidget(finishDrawing);
    connect(finishDrawing, &QPushButton::clicked, this, [this] { setDrawingMode(false); });
    m_drawBar->hide(); moleculeLayout->addWidget(m_drawBar);
    m_mmBar = new QWidget(m_moleculeWorkspace);
    auto *mmLayout = new QVBoxLayout(m_mmBar);
    mmLayout->setContentsMargins(8, 3, 8, 3);
    auto *mmControls = new QHBoxLayout;
    m_mmLabel = new QLabel(m_mmBar); mmLayout->addWidget(m_mmLabel);
    m_mmLabel->setWordWrap(true);
    mmLayout->addLayout(mmControls);
    m_mmProgress = new QProgressBar(m_mmBar);
    m_mmProgress->setObjectName(QStringLiteral("MMProgress"));
    m_mmProgress->setTextVisible(true);
    mmLayout->addWidget(m_mmProgress);
    m_mmPauseButton = new QPushButton(QStringLiteral("일시정지"), m_mmBar);
    m_mmPauseButton->setObjectName(QStringLiteral("MMPauseButton"));
    mmControls->addWidget(m_mmPauseButton);
    m_mmStopButton = new QPushButton(QStringLiteral("중단"), m_mmBar);
    m_mmStopButton->setObjectName(QStringLiteral("MMStopButton"));
    mmControls->addWidget(m_mmStopButton);
    m_mmRestoreButton = new QPushButton(QStringLiteral("시작 구조 복원"), m_mmBar);
    mmControls->addWidget(m_mmRestoreButton);
    mmControls->addStretch();
    m_mmPlot = new PlotWidget(m_mmBar);
    m_mmPlot->setObjectName(QStringLiteral("MMEnergyPlot"));
    m_mmPlot->setMinimumHeight(140);
    m_mmPlot->setEmptyMessage(QStringLiteral("MM energy trajectory"));
    mmLayout->addWidget(m_mmPlot);
    connect(m_mmPauseButton, &QPushButton::clicked, this, [this] {
        m_mmPaused = !m_mmPaused;
        m_mmPauseButton->setText(m_mmPaused ? QStringLiteral("재개") : QStringLiteral("일시정지"));
        if (m_mmAnimationTimer) {
            if (m_mmPaused) m_mmAnimationTimer->stop();
            else { m_mmAnimationTimer->start(); scheduleMMStep(); }
        }
    });
    connect(m_mmStopButton, &QPushButton::clicked, this, [this] {
        if (m_mmProcess) stopMMOptimization(QStringLiteral("MM 최적화를 중단했습니다."));
        else m_mmBar->hide();
    });
    connect(m_mmRestoreButton, &QPushButton::clicked, this, [this] {
        if (m_mmProcess) stopMMOptimization(QStringLiteral("MM 최적화를 중단했습니다."));
        if (m_mmOriginalCoords.size() != m_view->molecule().atoms.size()) return;
        m_view->setCoordinates(m_mmOriginalCoords);
        commitEditedMolecule(m_view->molecule(), false);
        setStatus(QStringLiteral("MM 시작 전 구조를 복원했습니다."));
    });
    m_mmBar->hide(); moleculeLayout->addWidget(m_mmBar);
    m_scanWorkspace = new QWidget(m_moleculeWorkspace);
    m_scanWorkspace->setObjectName(QStringLiteral("ScanWorkspace"));
    auto *scanLayout = new QVBoxLayout(m_scanWorkspace);
    scanLayout->setContentsMargins(4, 0, 4, 0);
    m_scanWorkspaceStatus = new QLabel(QStringLiteral("Scan 데이터 없음"), m_scanWorkspace);
    m_scanWorkspaceStatus->setWordWrap(true);
    scanLayout->addWidget(m_scanWorkspaceStatus);
    m_scanWorkspacePlot = new PlotWidget(m_scanWorkspace);
    m_scanWorkspacePlot->setObjectName(QStringLiteral("ScanWorkspacePlot"));
    m_scanWorkspacePlot->setMinimumHeight(260);
    scanLayout->addWidget(m_scanWorkspacePlot, 1);
    moleculeLayout->insertWidget(1, m_scanWorkspace, 1);
    moleculeLayout->setStretch(0, 2);
    m_scanWorkspacePlot->setEmptyMessage(QStringLiteral("Scan 데이터 없음"));
    connect(m_scanWorkspacePlot, &PlotWidget::pointClicked, this, &MainWindow::showScanPoint);
    m_workspace = new QStackedWidget(this);
    m_workspace->addWidget(m_moleculeWorkspace);
    m_tddftWorkspace = new QWidget(m_workspace);
    auto *spectrumLayout = new QVBoxLayout(m_tddftWorkspace);
    auto *spectrumTitle = new QLabel(QStringLiteral("UV‑Vis Spectrum"), m_tddftWorkspace);
    QFont spectrumTitleFont = spectrumTitle->font();
    spectrumTitleFont.setPointSize(spectrumTitleFont.pointSize() + 3);
    spectrumTitleFont.setBold(true);
    spectrumTitle->setFont(spectrumTitleFont);
    m_tddftWorkspaceInfo = new QLabel(QStringLiteral("TD-DFT 데이터 없음"), m_tddftWorkspace);
    m_tddftWorkspaceInfo->setWordWrap(true);
    m_tddftWorkspacePlot = new PlotWidget(m_tddftWorkspace);
    m_tddftWorkspacePlot->setMinimumHeight(360);
    m_tddftWorkspacePlot->setEmptyMessage(QStringLiteral("TD-DFT 데이터 없음"));
    m_tddftWorkspacePlot->setPlotStyle(QColor(205, 36, 45), 2.75, Qt::black, Qt::white);
    auto *spectrumButtons = new QHBoxLayout;
    spectrumButtons->addStretch();
    auto *saveSpectrum = new QPushButton(QStringLiteral("Save PNG"), m_tddftWorkspace);
    auto *copySpectrum = new QPushButton(QStringLiteral("Copy"), m_tddftWorkspace);
    spectrumButtons->addWidget(saveSpectrum);
    spectrumButtons->addWidget(copySpectrum);
    spectrumLayout->addWidget(spectrumTitle);
    spectrumLayout->addWidget(m_tddftWorkspaceInfo);
    spectrumLayout->addWidget(m_tddftWorkspacePlot, 1);
    spectrumLayout->addLayout(spectrumButtons);
    connect(saveSpectrum, &QPushButton::clicked, this, [this] {
        QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Save TD-DFT Spectrum"), QStringLiteral("spectrum.png"),
            QStringLiteral("PNG image (*.png)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) path += QStringLiteral(".png");
        if (!m_tddftWorkspacePlot->image(true).save(path, "PNG"))
            QMessageBox::warning(this, QStringLiteral("Save TD-DFT Spectrum"),
                                 QStringLiteral("PNG 파일을 저장하지 못했습니다."));
    });
    connect(copySpectrum, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setImage(m_tddftWorkspacePlot->image(true));
    });
    m_workspace->addWidget(m_tddftWorkspace);
    auto *contextSplitter = new QSplitter(Qt::Vertical, m_sidebars);
    m_currentSidebar = contextSplitter;
    m_explorer = new QTreeWidget(contextSplitter);
    m_explorer->setHeaderLabels({QStringLiteral("계산 결과")});
    m_analysis = new AnalysisPanel(contextSplitter);
    contextSplitter->addWidget(m_explorer);
    contextSplitter->addWidget(m_analysis);
    contextSplitter->setStretchFactor(0, 2);
    contextSplitter->setStretchFactor(1, 3);
    contextSplitter->setSizes({260, 430});
    m_sidebars->addWidget(contextSplitter);
    m_animation = new AnimationController(m_workspace);
    connect(m_view, &MoleculeView::atomPicked, this, [this](int index) { showAtom(index);if(m_bondEditMode)handleBondPick(index);else if(m_geometryMode)handleGeometryPick(index); });
    connect(m_view, &MoleculeView::canvasClicked, this, [this](const QVector3D &position) {
        if (m_drawingMode) addDrawnAtom(position);
    });
    connect(m_view, &MoleculeView::bondDrawn, this, [this](int first, int second, const QVector3D &position) {
        if (!m_drawingMode) return;
        if (second < 0) addDrawnAtom(position, first);
        else if (first != second) {
            Molecule edited = m_view->molecule();
            if (edited.atoms[first].atomicNumber == 1) edited.atoms[first].autoHydrogen = false;
            if (edited.atoms[second].atomicNumber == 1) edited.atoms[second].autoHydrogen = false;
            const double order = m_drawBondOrder->currentData().toDouble();
            edited.setBond(first, second, order, false,
                           order == 1.5 ? BondStyle::Dashed : BondStyle::Solid, true);
            commitEditedMolecule(edited, true);
            setStatus(QStringLiteral("결합을 그렸습니다."));
        }
    });
    connect(m_view, &MoleculeView::atomMoved, this, [this](int index, const QVector3D &) {
        Molecule edited = m_view->molecule();
        if (index >= 0 && index < edited.atoms.size()) edited.atoms[index].autoHydrogen = false;
        commitEditedMolecule(edited, false);
        setStatus(QStringLiteral("원자 위치를 적용했습니다."));
    });
    connect(m_geometryBar,&GeometryMeasureBar::distanceApplyRequested,this,[this](double value){applyGeometryMeasurement(0,value);});
    connect(m_geometryBar,&GeometryMeasureBar::angleApplyRequested,this,[this](double value){applyGeometryMeasurement(1,value);});
    connect(m_geometryBar,&GeometryMeasureBar::dihedralApplyRequested,this,[this](double value){applyGeometryMeasurement(2,value);});
    connect(m_geometryBar,&GeometryMeasureBar::distanceVisibilityChanged,this,[this](bool visible){m_view->setMeasurementVisible(0,visible);});
    connect(m_geometryBar,&GeometryMeasureBar::angleVisibilityChanged,this,[this](bool visible){m_view->setMeasurementVisible(1,visible);});
    connect(m_geometryBar,&GeometryMeasureBar::dihedralVisibilityChanged,this,[this](bool visible){m_view->setMeasurementVisible(2,visible);});
    connect(m_geometryBar,&GeometryMeasureBar::clearSelectionRequested,this,&MainWindow::clearGeometrySelection);
    connect(m_geometryBar,&GeometryMeasureBar::measurementStyleChanged,this,[this](MeasurementKind kind,const MeasurementTextStyle&style){m_view->setMeasurementStyle(int(kind),style);});
    connect(m_analysis, &AnalysisPanel::frameRequested, this, &MainWindow::showFrame);
    connect(m_analysis, &AnalysisPanel::frequencyRequested, this, &MainWindow::showFrequency);
    connect(m_analysis, &AnalysisPanel::scanPointRequested, this, &MainWindow::showScanPoint);
    connect(m_analysis, &AnalysisPanel::ircPointRequested, this, &MainWindow::showIrcPoint);
    connect(m_analysis, &AnalysisPanel::atomRequested, this, &MainWindow::showAtom);
    connect(m_analysis, &AnalysisPanel::framePlayRequested, this, &MainWindow::toggleFrameAnimation);
    connect(m_analysis, &AnalysisPanel::frequencyPlayRequested, this, &MainWindow::toggleFrequencyAnimation);
    connect(m_analysis, &AnalysisPanel::frequencyRestoreRequested, this, &MainWindow::restoreFrequencyGeometry);
    connect(m_analysis, &AnalysisPanel::frequencyAmplitudeChanged, this, [this](double amplitude) {
        m_animation->setAmplitude(amplitude);
        m_view->setVibrationVectorScaleFactor(m_analysis->scaledVectorsEnabled() ? amplitude : 1.0);
    });
    connect(m_analysis, &AnalysisPanel::frequencyCycleTimeChanged, m_animation, &AnimationController::setCycleTime);
    connect(m_analysis, &AnalysisPanel::animationSaveRequested, this, &MainWindow::exportAnimation);
    connect(m_analysis, &AnalysisPanel::coordinateViewRequested, this, [this] {
        m_analysis->setCoordinateMolecule(m_view->molecule());
    });
    connect(m_analysis, &AnalysisPanel::workspaceModeRequested, this, [this](bool tddft) {
        m_workspace->setCurrentWidget(tddft ? m_tddftWorkspace : m_moleculeWorkspace);
    });
    connect(m_analysis, &AnalysisPanel::tddftSpectrumChanged, this,
            [this](const QVector<QPointF> &spectrum, const QVector<QPointF> &sticks, const QString &summary) {
        m_tddftWorkspacePlot->setSeries(spectrum, QStringLiteral("Wavelength (nm)"), QStringLiteral("Relative Absorption"));
        m_tddftWorkspacePlot->setMarkers({});
        m_tddftWorkspacePlot->setStickSeries(sticks);
        m_tddftWorkspacePlot->setEmptyMessage(
            summary == QStringLiteral("TD-DFT 데이터 없음")
                ? summary
                : QStringLiteral("No spectrum to display (enable Sticks)"));
        m_tddftWorkspaceInfo->setText(summary);
    });
    connect(m_analysis, &AnalysisPanel::tddftStateInfoChanged, m_tddftWorkspaceInfo, &QLabel::setText);
    connect(m_analysis, &AnalysisPanel::vibrationVectorsChanged, this, [this](bool enabled, bool scaled) {
        m_style.showVibrationVectors = enabled;
        m_view->setStyle(m_style);
        m_view->setVibrationVectorScaleFactor(scaled ? m_analysis->frequencyAmplitude() : 1.0);
    });
    connect(m_analysis, &AnalysisPanel::recountBondsChanged, this, [this](bool enabled) {
        if (!enabled) m_view->setBonds(m_data.molecule.bonds);
    });
    connect(m_animation, &AnimationController::coordinatesReady, this, [this](const QVector<QVector3D> &coordinates) {
        m_view->setCoordinates(coordinates);
        if (m_animation->kind() == AnimationController::Kind::Frequency ||
            m_analysis->recountBondsEnabled())
            m_view->rebuildBonds(m_style.recountCutoffScale, m_style.recountHydrogen, m_style.recountMetalCoordination);
        if (m_analysis->coordinateViewActive()) m_analysis->setCoordinateMolecule(m_view->molecule());
    });
    connect(m_animation, &AnimationController::frameChanged, this, [this](int index) {
        if (m_animation->kind() == AnimationController::Kind::Frames) {
            m_currentFrame = index;
            m_analysis->setCurrentFrame(index);
        }
    });
    connect(m_animation, &AnimationController::playingChanged, this, [this](bool playing) {
        setStatus(playing ? QStringLiteral("애니메이션 재생 중") : QStringLiteral("애니메이션 일시정지"));
    });
    connect(m_explorer, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        const QString category = item->data(0, Qt::UserRole).toString();
        const int index = item->data(0, Qt::UserRole + 1).toInt();
        if (category == QStringLiteral("frame")) showFrame(index);
        else if (category == QStringLiteral("frequency")) showFrequency(index);
        else if (category == QStringLiteral("scan")) showScanPoint(index);
        else if (category == QStringLiteral("irc")) showIrcPoint(index);
        else if (category == QStringLiteral("atom")) showAtom(index);
        else if (category == QStringLiteral("tddft")) m_analysis->setCurrentTab(4);
        else if (category == QStringLiteral("section")) m_analysis->setCurrentTab(index);
    });
}

} // namespace youngcraft
