#include "ui/SidebarButton.h"
#include "ui/TransitionAnalysisIcon.h"
#include "app/MainWindow.h"
#include "io/CrystalContacts.h"
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include "app/DocumentTabs.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/PlotWidget.h"
#include "ui/StyleEditorDialog.h"
#include "ui/GeometryMeasureBar.h"
#include "settings/AppPattern.h"
#include "ui/SidebarPaper.h"
#include "ui/OutputSourceView.h"
#include <QShortcut>
#include <QTableWidget>

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
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <QKeyEvent>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QProgressBar>
#include <QSettings>
#include <QScrollArea>
#include <QResizeEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include "ui/CenteredIconTabs.h"
#include <QTimer>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <functional>

namespace youngcraft {
namespace {
class WorkspacePaper final : public QWidget {
public:
    explicit WorkspacePaper(QWidget *parent):QWidget(parent){}
protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);QPainter painter(this);
        if(!uiPattern::paintBackdrop(painter,this))painter.fillRect(rect(),window()->palette().color(QPalette::Window));
    }
};

// The interactive viewport fills the available workspace; capture sizes are independent.
class MoleculeViewport final : public QWidget {
public:
    explicit MoleculeViewport(MoleculeView *view, QWidget *parent)
        : QWidget(parent), m_view(view) {
        setObjectName(QStringLiteral("MoleculeViewport"));
        setAutoFillBackground(true);
        auto colors = palette();
        colors.setColor(QPalette::Window, QColor(230, 232, 235));
        setPalette(colors);
        view->setParent(this);
        setMinimumSize(120, 120);
    }
protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);
        QPainter painter(this);uiPattern::paintBackdrop(painter,this);
    }
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        m_view->setGeometry(rect());
        m_view->update();
    }
private:
    MoleculeView *m_view;
};
}

void MainWindow::updateOutputSource(const QString &section,int index,bool reveal) {
    if(!m_outputSource)return;
    m_outputSource->setData(m_data);
    if(reveal){m_outputDock->show();m_outputDock->raise();}
    QString key=section;int row=index;
    if(key.isEmpty())switch(m_analysis->currentTab()) {
    case 0:key="frame_geometry";row=qMax(0,m_currentFrame);break;
    case 1:key="frequency_modes";row=qMax(0,m_currentFrequency);break;
    case 2:if(!m_data.scanPoints.isEmpty()){const int i=qBound(0,m_analysis->findChild<QListWidget*>("ScanList")?m_analysis->findChild<QListWidget*>("ScanList")->currentRow():0,int(m_data.scanPoints.size()-1));row=m_data.scanPoints[i].frameIndex;key="frame_geometry";}break;
    case 3:key="irc";break;
    case 4:key="excited_states";row=qMax(0,m_analysis->findChild<QTableWidget*>("TddftStatesTable")->currentRow());break;
    case 5:key="mulliken_charges";row=qMax(0,int(m_data.index.sectionOffsets.value(key).size())-1);break;
    case 6:key="orbital_energies";row=qMax(0,int(m_data.index.sectionOffsets.value(key).size())-1);break;
    default:key="frame_geometry";row=qMax(0,m_currentFrame);break;
    }
    if(!key.isEmpty())m_outputSource->jumpSection(key,row);
}

void MainWindow::createUi() {
    setProperty("solidWorkspace", true);
    m_documentArea = new QStackedWidget(this);
    // Hidden workspace pages must not impose their height on the active document.
    m_documentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    m_documentArea->setMinimumHeight(120);
    m_documentTabs = new DocumentTabs(this);
    m_documentTabs->setObjectName(QStringLiteral("DocumentTabs"));
    m_documentTabs->setTabsClosable(true);
    m_documentTabs->setMovable(true);
    m_documentTabs->setUsesScrollButtons(true);
    m_documentTabs->setElideMode(Qt::ElideMiddle);
    m_documentArea->addWidget(m_documentTabs);
    setCentralWidget(m_documentArea);
    m_explorerDock = new QDockWidget(QStringLiteral("계산 결과"), this);
    m_explorerDock->setObjectName(QStringLiteral("ResultExplorerDock"));
    auto *sidebar = new QWidget(m_explorerDock);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(8, 4, 8, 0);
    sidebarLayout->setSpacing(8);
    m_analysisShortcuts = new QWidget(sidebar);
    sidebarLayout->addWidget(m_analysisShortcuts);
    m_sidebars = new QStackedWidget(sidebar);
    sidebarLayout->addWidget(m_sidebars, 1);
    m_explorerDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_explorerDock->setFeatures(QDockWidget::DockWidgetClosable);
    m_explorerDock->setMinimumWidth(230);
    m_explorerDock->setMaximumWidth(360);
    m_explorerDock->setWidget(sidebar);
    addDockWidget(Qt::LeftDockWidgetArea, m_explorerDock);
    m_structureDock = new QDockWidget(QStringLiteral("구조 도구"), this);
    m_structureDock->setObjectName(QStringLiteral("StructureToolsDock"));
    m_structureDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_structureDock->setFeatures(QDockWidget::DockWidgetClosable);
    m_structureDock->setMinimumWidth(188);
    m_structureDock->setMaximumWidth(360);
    m_structureTabs = new CenteredIconTabs(m_structureDock);
    m_structureTabs->setIconSize(QSize(sidebarIconSize,sidebarIconSize));
    m_structureTabs->setObjectName(QStringLiteral("StructureToolsTabs"));
    auto *displayPage = new QWidget;
    displayPage->setObjectName("StructureDisplayPage");
    auto *displayLayout = new QVBoxLayout(displayPage);
    displayLayout->setContentsMargins(0, 4, 0, 0);
    m_structureTabs->addTab(displayPage, QStringLiteral("표시"));
    auto *editScroll = new QScrollArea;
    editScroll->setWidgetResizable(true);
    editScroll->setFrameShape(QFrame::NoFrame);
    auto *editPage = new QWidget;
    editPage->setObjectName("StructureEditPage");
    auto *editLayout = new QVBoxLayout(editPage);
    editLayout->setContentsMargins(0, 4, 0, 0);
    m_drawPanels = new QStackedWidget(editPage);
    editLayout->addWidget(m_drawPanels);
    editLayout->addStretch();
    editScroll->setWidget(editPage);
    m_structureTabs->addTab(editScroll, QStringLiteral("편집"));
    auto *structureContainer = new QWidget(m_structureDock);
    structureContainer->setObjectName("StructureToolsContainer");
    auto *structureLayout = new QVBoxLayout(structureContainer);
    structureLayout->setContentsMargins(8, 4, 8, 8);
    structureLayout->addWidget(m_structureTabs);
    m_structureDock->setWidget(structureContainer);
    addDockWidget(Qt::RightDockWidgetArea, m_structureDock);
    resizeDocks({m_explorerDock, m_structureDock}, {280, 208}, Qt::Horizontal);
    m_outputDock=new QDockWidget(QStringLiteral("Output · 원문 / 결과 출처"),this);
    m_outputDock->setObjectName(QStringLiteral("OutputSourceDock"));
    m_outputSource=new OutputSourceView(m_outputDock);m_outputDock->setWidget(m_outputSource);
    addDockWidget(Qt::BottomDockWidgetArea,m_outputDock);m_outputDock->hide();
    auto *findOutput=new QShortcut(QKeySequence::Find,this);
    connect(findOutput,&QShortcut::activated,this,[this]{if(!m_outputDock->isVisible())updateOutputSource({},0,true);m_outputSource->focusFind();});

    m_statusLabel = new QLabel(QStringLiteral("파일을 이 창에 끌어 놓거나 파일 열기를 누르세요."), this);
    m_statusLabel->setToolTip(m_statusLabel->text());
    m_statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
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
    m_documentConnections.clear();
    m_view = new MoleculeView(this);
    m_view->setStyle(m_style);

    m_moleculeWorkspace = new WorkspacePaper(this);auto *moleculeLayout=new QVBoxLayout(m_moleculeWorkspace);moleculeLayout->setContentsMargins(0,0,0,0);moleculeLayout->setSpacing(2);moleculeLayout->addWidget(new MoleculeViewport(m_view, m_moleculeWorkspace),1);m_geometryBar=new GeometryMeasureBar(m_moleculeWorkspace);m_geometryBar->setVisible(false);moleculeLayout->addWidget(m_geometryBar);
    auto *measurementRow = new WorkspacePaper(m_moleculeWorkspace);
    measurementRow->setObjectName(QStringLiteral("WorkspaceMeasurementRow"));
    measurementRow->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    measurementRow->setFixedHeight(qMax(sidebarButtonSize+8,measurementRow->fontMetrics().height()*2+8));
    auto *measurementLayout = new QHBoxLayout(measurementRow);
    measurementLayout->setContentsMargins(8, 4, 8, 4);
    m_measurementReadout = new QLabel(QStringLiteral("원자를 순서대로 클릭하세요: 2개 거리 · 3개 각도 · 4개 이면각"), measurementRow);
    m_measurementReadout->setObjectName(QStringLiteral("AtomMeasurementReadout"));
    m_measurementReadout->setWordWrap(true);
    m_measurementReadout->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_measurementReadout->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *clearMeasurement = new QPushButton(QStringLiteral("측정 초기화"), measurementRow);
    clearMeasurement->setObjectName(QStringLiteral("ClearAtomMeasurement"));
    measurementLayout->addWidget(m_measurementReadout, 1);
    makeSidebarIconButton(clearMeasurement, QStyle::SP_DialogResetButton);
    auto *editMeasurement = new QPushButton(QStringLiteral("거리 / 각도 / 이면각 편집"), measurementRow);
    editMeasurement->setObjectName("EditAtomMeasurement");
    makeSidebarIconButton(editMeasurement, QStyle::SP_FileDialogDetailedView);
    measurementLayout->addWidget(editMeasurement);
    connect(editMeasurement, &QPushButton::clicked, m_geometryBar, [bar=m_geometryBar] {
        const bool expanded=!bar->property("editExpanded").toBool();
        bar->setProperty("editExpanded",expanded);bar->setVisible(expanded);
    });
    measurementLayout->addWidget(clearMeasurement);
    auto *approach = new QPushButton(QStringLiteral("Scan용 구조 조정…"), measurementRow);
    approach->setObjectName(QStringLiteral("ScanApproachButton"));
    makeSidebarIconButton(approach, QStyle::SP_ArrowForward);
    measurementLayout->addWidget(approach);
    auto *frequencyControls=new QWidget(measurementRow);frequencyControls->setObjectName("ViewportFrequencyControls");
    auto *frequencyRow=new QHBoxLayout(frequencyControls);frequencyRow->setContentsMargins(0,0,0,0);
    auto *frequencyToggle=new QPushButton(QStringLiteral("진동 재개"),frequencyControls);frequencyToggle->setObjectName("ViewportFrequencyToggle");
    auto *frequencyRestore=new QPushButton(QStringLiteral("원래 구조"),frequencyControls);frequencyRestore->setObjectName("ViewportFrequencyRestore");
    makeSidebarIconButton(frequencyToggle, QStyle::SP_MediaPlay);
    makeSidebarIconButton(frequencyRestore, QStyle::SP_MediaStop);
    frequencyRow->addWidget(frequencyToggle);frequencyRow->addWidget(frequencyRestore);measurementLayout->addWidget(frequencyControls);frequencyControls->hide();

    moleculeLayout->addWidget(measurementRow);

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
    auto *contactsButton = new QPushButton(QStringLiteral("주기적 접촉…"), m_crystalBar);
    contactsButton->setObjectName(QStringLiteral("CrystalContacts"));
    crystalLayout->addWidget(contactsButton);
    connect(contactsButton, &QPushButton::clicked, this, [this] {
        if (!m_crystal) return;
        auto crystal = m_crystal;
        if (crystal->displayMode == 0 && !crystal->sourceCif.isEmpty()) {
            QString error;
            crystal = StructureReader::rebuildCif(*crystal, m_view->molecule(), &error);
            if (!crystal) { setStatus(error); return; }
        }
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("주기적 원자 접촉"));
        dialog.resize(700, 500);
        auto *layout = new QVBoxLayout(&dialog);
        auto *note = new QLabel(QStringLiteral("단위격자 기준 원자쌍과 격자 이동입니다. 결합을 포함한 기하학적 접촉이며 수소결합·에너지 판정은 수행하지 않습니다."));
        note->setWordWrap(true);
        layout->addWidget(note);
        auto *cutoff = new QDoubleSpinBox;
        cutoff->setRange(0.1, 10); cutoff->setValue(3.5); cutoff->setSuffix(QStringLiteral(" Å"));
        layout->addWidget(cutoff);
        auto *refresh = new QPushButton(QStringLiteral("접촉 계산"));
        layout->addWidget(refresh);
        auto *table = new QTableWidget(0, 4);
        table->setHorizontalHeaderLabels({QStringLiteral("원자 1"), QStringLiteral("원자 2"), QStringLiteral("격자 이동 a,b,c"), QStringLiteral("거리 (Å)")});
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        layout->addWidget(table);
        connect(refresh, &QPushButton::clicked, &dialog, [=] {
            table->setRowCount(0);
            try {
                const auto contacts = crystalContacts(*crystal, cutoff->value());
                for (const auto &contact : contacts) {
                    const int row = table->rowCount(); table->insertRow(row);
                    const auto label = [&](int index) {
                        const auto &atom = crystal->unitCell.atoms[index];
                        return QStringLiteral("%1 [%2]").arg(atom.label.isEmpty() ? atom.element : atom.label).arg(index + 1);
                    };
                    table->setItem(row, 0, new QTableWidgetItem(label(contact.first)));
                    table->setItem(row, 1, new QTableWidgetItem(label(contact.second)));
                    table->setItem(row, 2, new QTableWidgetItem(QStringLiteral("%1, %2, %3").arg(contact.translation[0]).arg(contact.translation[1]).arg(contact.translation[2])));
                    table->setItem(row, 3, new QTableWidgetItem(QString::number(contact.distance, 'f', 5)));
                }
                note->setText(QStringLiteral("접촉 %1개 · 결합 포함 · 동일 접촉의 역방향 중복 제외").arg(contacts.size()));
            } catch (const std::exception &e) { note->setText(QString::fromUtf8(e.what())); }
        });
        dialog.exec();
    });
    crystalLayout->addStretch();
    m_crystalBar->hide();
    moleculeLayout->addWidget(m_crystalBar);

    m_drawBar = new QWidget(m_drawPanels);
    auto *editorRows = new QVBoxLayout(m_drawBar);
    editorRows->setContentsMargins(0, 4, 0, 0);
    auto *elementRow = new QHBoxLayout;
    editorRows->addLayout(elementRow);
    auto *elements = new QGridLayout;
    editorRows->addLayout(elements);
    elements->setSpacing(4);
    elements->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *elementLabel = new QLabel(QStringLiteral("원소"), m_drawBar);
    elementRow->addWidget(elementLabel);
    m_drawElement = new QComboBox(m_drawBar);
    m_drawElement->setObjectName(QStringLiteral("DrawElementSelector"));
    for (int z = 1; z <= 118; ++z) m_drawElement->addItem(elementSymbol(z), z);
    m_drawElement->setEditable(true);
    m_drawElement->setInsertPolicy(QComboBox::NoInsert);
    m_drawElement->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    m_drawElement->setCurrentText(QStringLiteral("C"));
    m_drawElement->setFixedWidth(72);
    elementRow->addWidget(m_drawElement, 1);
    const QStringList quickElements{QStringLiteral("H"), QStringLiteral("C"), QStringLiteral("N"),
        QStringLiteral("O"), QStringLiteral("B"), QStringLiteral("F"), QStringLiteral("Si"),
        QStringLiteral("P"), QStringLiteral("S"), QStringLiteral("Cl"), QStringLiteral("Br"), QStringLiteral("I")};
    QVector<QPushButton *> quickButtons;
    for (const QString &symbol : quickElements) {
        auto *button = new QPushButton(symbol, m_drawBar);
        button->setCheckable(true);
        sizeSidebarButton(button);
        button->setToolTip(QStringLiteral("원소 선택 · %1").arg(symbol));
        button->setObjectName(QStringLiteral("QuickElement_%1").arg(symbol));
        const int position = quickButtons.size();
        elements->addWidget(button, position / 4, position % 4);
        quickButtons.push_back(button);
        connect(button, &QPushButton::clicked, m_drawElement, [selector = m_drawElement, symbol] {
            selector->setCurrentText(symbol);
        });
    }

    for (QPushButton *button : quickButtons) button->setChecked(button->text() == QStringLiteral("C"));
    auto *bondLabel = new QLabel(QStringLiteral("결합"), m_drawBar);
    auto *bondRow = new QHBoxLayout;
    bondRow->addWidget(bondLabel);
    editorRows->addLayout(bondRow);
    m_drawBondOrder = new QComboBox(m_drawBar);
    m_drawBondOrder->setObjectName(QStringLiteral("DrawBondOrder"));
    m_drawBondOrder->addItem(QStringLiteral("단일"), 1.0);
    m_drawBondOrder->addItem(QStringLiteral("이중"), 2.0);
    m_drawBondOrder->addItem(QStringLiteral("삼중"), 3.0);
    m_drawBondOrder->addItem(QStringLiteral("방향족"), 1.5);
    m_drawBondOrder->addItem(QStringLiteral("자동"), 0.0);
    m_drawBondOrder->setCurrentIndex(0);
    m_drawBondOrder->setFixedWidth(100);
    bondRow->addWidget(m_drawBondOrder, 1);
    auto *bondPresets = new QHBoxLayout;
    editorRows->addLayout(bondPresets);
    bondPresets->setSpacing(4);
    for (int i = 0; i < 4; ++i) {
        auto *button = new QPushButton(i == 3 ? QStringLiteral("Ar") : QString::number(i + 1), m_drawBar);
        sizeSidebarButton(button);
        button->setToolTip(m_drawBondOrder->itemText(i));
        bondPresets->addWidget(button);
        connect(button, &QPushButton::clicked, m_drawBondOrder, [selector = m_drawBondOrder, i] {
            selector->setCurrentIndex(i);
        });
    }
    bondPresets->addStretch();
    m_autoHButton = new QPushButton(QStringLiteral("수소 자동 추가"), m_drawBar);
    m_autoHButton->setObjectName(QStringLiteral("AutoHydrogenButton"));
    m_autoHButton->setCheckable(true);
    m_autoHButton->setChecked(false);
    m_autoHButton->setToolTip(QStringLiteral("구조 편집이 확정될 때 부족한 수소만 자동으로 보정합니다."));
    auto *drawTools = new QGridLayout;
    drawTools->setSpacing(4);
    drawTools->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    editorRows->addLayout(drawTools);
    sizeSidebarButton(m_autoHButton);
    m_autoHButton->setAccessibleName(m_autoHButton->text());
    m_autoHButton->setText(QStringLiteral("H+"));
    drawTools->addWidget(m_autoHButton, 1, 0);

    auto *replaceAtom = new QPushButton(QStringLiteral("선택 원자 → 현재 원소"), m_drawBar);
    replaceAtom->setObjectName(QStringLiteral("Document_replaceAtom"));
    makeSidebarIconButton(replaceAtom, QStyle::SP_BrowserReload);
    drawTools->addWidget(replaceAtom, 1, 1);

    auto *deleteAtom = new QPushButton(QStringLiteral("선택 원자 삭제"), m_drawBar);
    deleteAtom->setObjectName(QStringLiteral("Document_deleteAtom"));
    makeSidebarIconButton(deleteAtom, QStyle::SP_TrashIcon);
    drawTools->addWidget(deleteAtom, 1, 2);

    setupEditorControls(drawTools);
    auto *finishDrawing = new QPushButton(QStringLiteral("그리기 종료"), m_drawBar);
    finishDrawing->setObjectName(QStringLiteral("Document_finishDrawing"));

    makeSidebarIconButton(finishDrawing, QStyle::SP_DialogApplyButton);
    drawTools->addWidget(finishDrawing, 1, 3);
    m_drawPanels->addWidget(m_drawBar);
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
    makeSidebarIconButton(m_mmPauseButton, QStyle::SP_MediaPause);
    mmControls->addWidget(m_mmPauseButton);
    m_mmStopButton = new QPushButton(QStringLiteral("중단"), m_mmBar);
    m_mmStopButton->setObjectName(QStringLiteral("MMStopButton"));
    makeSidebarIconButton(m_mmStopButton, QStyle::SP_MediaStop);
    mmControls->addWidget(m_mmStopButton);
    m_mmRestoreButton = new QPushButton(QStringLiteral("시작 구조 복원"), m_mmBar);
    makeSidebarIconButton(m_mmRestoreButton, QStyle::SP_DialogResetButton);
    mmControls->addWidget(m_mmRestoreButton);
    mmControls->addStretch();
    m_mmPlot = new PlotWidget(m_mmBar);
    m_mmPlot->setObjectName(QStringLiteral("MMEnergyPlot"));
    m_mmPlot->setMinimumHeight(140);
    m_mmPlot->setEmptyMessage(QStringLiteral("MM 에너지 변화"));
    mmLayout->addWidget(m_mmPlot);

    m_mmBar->hide(); moleculeLayout->addWidget(m_mmBar);

    m_workspace = new QStackedWidget(this);
    m_workspace->addWidget(m_moleculeWorkspace);
    m_tddftWorkspace = new QWidget(m_workspace);
    auto *spectrumLayout = new QVBoxLayout(m_tddftWorkspace);
    auto *spectrumTitle = new QLabel(QStringLiteral("자외선·가시광선 스펙트럼"), m_tddftWorkspace);
    QFont spectrumTitleFont = spectrumTitle->font();
    spectrumTitleFont.setPointSize(spectrumTitleFont.pointSize() + 3);
    spectrumTitleFont.setBold(true);
    spectrumTitle->setFont(spectrumTitleFont);
    m_tddftWorkspaceInfo = new QLabel(QStringLiteral("TD-DFT 데이터 없음"), m_tddftWorkspace);
    m_tddftWorkspaceInfo->setWordWrap(true);
    m_tddftWorkspacePlot = new PlotWidget(m_tddftWorkspace);
    m_tddftWorkspacePlot->setEmptyMessage(QStringLiteral("TD-DFT 데이터 없음"));
    m_tddftWorkspacePlot->setObjectName(QStringLiteral("TddftPublicationPlot"));
    PlotWidget::StyleOptions spectrumStyle;
    spectrumStyle.showPoints=false;
    spectrumStyle.lineColor=QColor("#d62027");
    spectrumStyle.background=Qt::transparent;
    spectrumStyle.axisThickness=1.8;
    spectrumStyle.axisFont=QFont(QStringLiteral("Times New Roman"),14,QFont::Bold);
    spectrumStyle.tickFontFollowsAxis=true;
    spectrumStyle.nonnegativeX=true;
    spectrumStyle.thickness=2.5;
    spectrumStyle.stickColor=QColor("#a8b2bc");
    spectrumStyle.stickThickness=0.7;
    spectrumStyle.zeroBaseline=true;
    spectrumStyle.minorTicks=1;
    spectrumStyle.majorTicks=6;
    spectrumStyle.axisColor=spectrumStyle.tickColor=QColor("#252b31");
    m_tddftWorkspacePlot->setStyleOptions(spectrumStyle);
    m_tddftWorkspacePlot->setToolTip(QStringLiteral("실선: 선폭 확장 스펙트럼 · 회색 막대: 개별 전이의 진동자 세기. 정규화 시 각각의 최댓값을 1로 맞춥니다."));
    auto *spectrumButtons = new QHBoxLayout;
    auto *axisSettings=new QPushButton(QStringLiteral("축·범위·눈금 설정…"),m_tddftWorkspace);
    axisSettings->setObjectName(QStringLiteral("TddftAxisSettings"));
    connect(axisSettings,&QPushButton::clicked,m_tddftWorkspacePlot,&PlotWidget::showAxisSettings);
    makeSidebarIconButton(axisSettings, QStyle::SP_FileDialogDetailedView);
    spectrumButtons->addWidget(axisSettings);
    auto *autoAxes=new QPushButton(QStringLiteral("자동 범위"),m_tddftWorkspace);
    autoAxes->setObjectName(QStringLiteral("TddftAutoRange"));
    connect(autoAxes,&QPushButton::clicked,m_tddftWorkspacePlot,&PlotWidget::clearManualRange);
    makeSidebarIconButton(autoAxes, QStyle::SP_DialogResetButton);
    spectrumButtons->addWidget(autoAxes);
    auto *sourceResult=new QPushButton(QStringLiteral("선택 전이 원문 위치"),m_tddftWorkspace);
    sourceResult->setObjectName(QStringLiteral("TddftShowSource"));
    makeSidebarIconButton(sourceResult, QStyle::SP_FileIcon);
    spectrumButtons->addWidget(sourceResult);
    connect(sourceResult,&QPushButton::clicked,this,[this]{updateOutputSource("excited_states",qMax(0,m_analysis->findChild<QTableWidget*>("TddftStatesTable")->currentRow()),true);});
    auto *ntoAnalysis=new QPushButton(QStringLiteral("NTO 분석"),m_tddftWorkspace);
    ntoAnalysis->setObjectName(QStringLiteral("TddftNtoAnalysis"));
    ntoAnalysis->setIcon(transitionAnalysisIcon());
    ntoAnalysis->setToolTip(QStringLiteral("선택 상태의 NTO · Hole / Particle 분석"));
    spectrumButtons->addWidget(ntoAnalysis);
    connect(ntoAnalysis,&QPushButton::clicked,this,[this]{m_analysis->showTransitionAnalysis();});

    spectrumButtons->addStretch();
    auto *saveSpectrum = new QPushButton(QStringLiteral("내보내기…"), m_tddftWorkspace);
    saveSpectrum->setObjectName(QStringLiteral("Document_saveSpectrum"));
    auto *copySpectrum = new QPushButton(QStringLiteral("그림 복사"), m_tddftWorkspace);
    copySpectrum->setObjectName(QStringLiteral("Document_copySpectrum"));
    saveSpectrum->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveSpectrum->setToolTip(QStringLiteral("현재 TD-DFT 그래프를 PNG 또는 PDF로 저장합니다. 크기·해상도·투명 배경을 설정할 수 있습니다."));
    spectrumButtons->addWidget(saveSpectrum);
    copySpectrum->setIcon(sidebarCopyIcon());
    copySpectrum->setToolTip(QStringLiteral("현재 TD-DFT 그래프를 600 dpi 이미지로 클립보드에 복사합니다."));
    spectrumButtons->addWidget(copySpectrum);
    spectrumLayout->addWidget(spectrumTitle);
    spectrumLayout->addWidget(m_tddftWorkspaceInfo);
    spectrumLayout->addLayout(spectrumButtons);
    spectrumLayout->addWidget(m_tddftWorkspacePlot, 1);

    m_workspace->addWidget(m_tddftWorkspace);
    auto *contextSplitter = new QSplitter(Qt::Vertical, m_sidebars);
    contextSplitter->setObjectName(QStringLiteral("SidebarPanelSplitter"));
    m_currentSidebar = contextSplitter;
    m_explorer = new QTreeWidget(contextSplitter);
    m_explorer->setVisible(QSettings().value(QStringLiteral("ui/showResultNavigator"),false).toBool());
    m_explorer->setHeaderLabels({QStringLiteral("결과 탐색")});
    m_explorer->setHeaderHidden(true);
    m_explorer->setAccessibleName(QStringLiteral("계산 결과 탐색 트리"));
    auto *analysisScroll = new QScrollArea(contextSplitter);
    analysisScroll->setObjectName(QStringLiteral("SidebarAnalysisScroll"));
    analysisScroll->setWidgetResizable(true);
    analysisScroll->setFrameShape(QFrame::NoFrame);
    m_analysis = new AnalysisPanel(analysisScroll);
    m_analysis->setTabBarVisible(false);
    m_analysis->setProperty("sidebarPatternPaper",true);
    if(auto *tabs=m_analysis->findChild<QTabWidget*>()) {
        tabs->setObjectName(QStringLiteral("SidebarPatternTabs"));
        for(int i=0;i<tabs->count();++i)tabs->widget(i)->setProperty("sidebarPatternPaper",true);
    }

    m_analysis->setOrbitalView(m_view);
    connect(m_analysis, &AnalysisPanel::orbitalStyleChanged, this, [this, view=m_view] {
        if (view==m_view) m_style=view->style();
        for(const auto &document:m_documents) if(document->view==view) document->style=view->style();
    });
    connect(m_analysis, &AnalysisPanel::orbitalFileRequested, this, &MainWindow::openFile);
    analysisScroll->setWidget(m_analysis);
    contextSplitter->addWidget(m_explorer);
    contextSplitter->addWidget(analysisScroll);
    contextSplitter->setStretchFactor(0, 0);
    contextSplitter->setStretchFactor(1, 1);
    contextSplitter->setSizes({180, 540});

    m_sidebars->addWidget(contextSplitter);
    uiPattern::watchSidebar(m_explorerDock);
    m_animation = new AnimationController(m_workspace);

    connect(m_analysis, &AnalysisPanel::frequencyCycleTimeChanged, m_animation, &AnimationController::setCycleTime);

    connect(m_analysis, &AnalysisPanel::tddftStateInfoChanged, m_tddftWorkspaceInfo, &QLabel::setText);

    connectDocumentUi();
}

void MainWindow::connectDocumentUi() {
    auto *frequencyControls=m_workspace->findChild<QWidget *>("ViewportFrequencyControls");
    auto *frequencyToggle=m_workspace->findChild<QPushButton *>("ViewportFrequencyToggle");
    auto *frequencyRestore=m_workspace->findChild<QPushButton *>("ViewportFrequencyRestore");
    auto *approach = m_workspace->findChild<QPushButton *>(QStringLiteral("ScanApproachButton"));
    auto *clearMeasurement = m_workspace->findChild<QPushButton *>(QStringLiteral("ClearAtomMeasurement"));
    auto *replaceAtom = m_drawBar->findChild<QPushButton *>(QStringLiteral("Document_replaceAtom"));
    auto *deleteAtom = m_drawBar->findChild<QPushButton *>(QStringLiteral("Document_deleteAtom"));
    auto *finishDrawing = m_drawBar->findChild<QPushButton *>(QStringLiteral("Document_finishDrawing"));
    auto *saveSpectrum = m_workspace->findChild<QPushButton *>(QStringLiteral("Document_saveSpectrum"));
    auto *copySpectrum = m_workspace->findChild<QPushButton *>(QStringLiteral("Document_copySpectrum"));
    QVector<QPushButton *> quickButtons;
    for (auto *button : m_drawBar->findChildren<QPushButton *>())
        if (button->objectName().startsWith(QStringLiteral("QuickElement_"))) quickButtons.push_back(button);
    m_documentConnections.push_back(connect(m_view, &MoleculeView::cameraChanged, this, [this, view = m_view] {
        if (view == m_view && m_activeDocument && m_activeDocument->view == view &&
            m_activeDocument->workspace->property("loadFinished").toBool()) synchronizeViews(view);
    }));
    m_documentConnections.push_back(connect(approach, &QPushButton::clicked, this, &MainWindow::arrangeScanApproach));
    m_documentConnections.push_back(connect(clearMeasurement, &QPushButton::clicked, this, &MainWindow::clearGeometrySelection));
    m_documentConnections.push_back(connect(m_view, &MoleculeView::geometryChanged, this, [this, view = m_view] {
        if (view == m_view) updateGeometryReadout();
    }));
    m_documentConnections.push_back(connect(m_crystalMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::showCrystalMode));
    m_documentConnections.push_back(connect(m_cellToggle, &QCheckBox::toggled, this, [this](bool visible) {
        if (m_crystal) m_view->setCrystalCell(m_crystal->axisA, m_crystal->axisB,
                                              m_crystal->axisC, m_crystalMode->currentIndex() == 2 ? 2 : 1,
                                              visible);
    }));
    m_documentConnections.push_back(connect(m_drawElement, &QComboBox::currentTextChanged, m_drawBar,
            [this, quickButtons](const QString &symbol) {
        const int z = atomicNumberFor(symbol);
        if (z > 0) m_view->setDrawingElement(z);
        for (QPushButton *button : quickButtons) button->setChecked(button->text() == symbol);
    }));
    m_documentConnections.push_back(connect(m_drawElement,&QComboBox::textActivated,this,[this](const QString &symbol){int z=atomicNumberFor(symbol);if(z>0)m_view->setDrawingElement(z);}));
    for(auto *button:quickButtons)m_documentConnections.push_back(connect(button,&QPushButton::clicked,this,[this,button]{m_view->setDrawingElement(atomicNumberFor(button->text()));}));
    m_documentConnections.push_back(connect(m_drawBondOrder,&QComboBox::activated,this,[this]{m_view->setDrawingElement(atomicNumberFor(m_drawElement->currentText()));}));
    m_documentConnections.push_back(connect(m_autoHButton, &QPushButton::toggled, this, [this](bool enabled) {
        if (enabled && !m_view->molecule().isEmpty())
            commitEditedMolecule(m_view->molecule(), true);
    }));
    m_documentConnections.push_back(connect(replaceAtom, &QPushButton::clicked, this, [this] {
        auto ids=m_view->selection().atoms;if(ids.isEmpty()&&m_view->selectedAtom()>=0)ids.insert(m_view->selectedAtom());
        if(ids.isEmpty())return;auto edited=m_view->molecule();auto state=drawState();
        MoleculeEditController::replace(edited,ids,state.element,state.formalCharge);commitEditedMolecule(edited,true);
    }));
    m_documentConnections.push_back(connect(deleteAtom, &QPushButton::clicked, this, &MainWindow::deleteSelection));
    m_documentConnections.push_back(connect(finishDrawing, &QPushButton::clicked, this, [this] { setDrawingMode(false); }));
    m_documentConnections.push_back(connect(m_mmPauseButton, &QPushButton::clicked, this, [this] {
        if (m_mmState == MMState::Running || m_mmState == MMState::Starting) m_mmState = MMState::Paused;
        else if (m_mmState == MMState::Paused) m_mmState = m_mmField.isEmpty() ? MMState::Starting : MMState::Running;
        const bool paused = m_mmState == MMState::Paused;
        setSidebarButtonIcon(m_mmPauseButton, paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause,
                             paused ? QStringLiteral("재개") : QStringLiteral("일시정지"));
        scheduleMMStep();
    }));
    m_documentConnections.push_back(connect(m_mmStopButton, &QPushButton::clicked, this, [this] {
        if (m_mmProcess) stopMMOptimization(QStringLiteral("MM 최적화를 중단했습니다."));
        else m_mmBar->hide();
    }));
    m_documentConnections.push_back(connect(m_mmRestoreButton, &QPushButton::clicked, this, [this] {
        if (m_mmProcess) stopMMOptimization(QStringLiteral("MM 최적화를 중단했습니다."));
        if (m_mmOriginalCoords.size() != m_view->molecule().atoms.size()) return;
        m_view->setCoordinates(m_mmOriginalCoords);
        commitEditedMolecule(m_view->molecule(), false);
        setStatus(QStringLiteral("MM 시작 전 구조를 복원했습니다."));
    }));
    m_documentConnections.push_back(connect(saveSpectrum, &QPushButton::clicked, this, [this] {
        m_tddftWorkspacePlot->showExportDialog();
    }));
    m_documentConnections.push_back(connect(copySpectrum, &QPushButton::clicked, this, [this] {
        PlotWidget::ExportOptions options;
        options.transparent=m_tddftWorkspacePlot->styleOptions().background.alpha()==0;
        setStatus(m_tddftWorkspacePlot->copyPublication(options)
            ? QStringLiteral("TD-DFT 그래프를 클립보드에 복사했습니다 (600 dpi).")
            : QStringLiteral("TD-DFT 그래프를 복사하지 못했습니다."));
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::dataChanged, this, [this, panel = m_analysis] {
        if (panel == m_analysis) updatePanelActions();
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::currentTabChanged, this, [this, panel = m_analysis] {
        if (panel == m_analysis) {updatePanelActions();if(m_outputDock->isVisible())updateOutputSource();}
    }));
    m_documentConnections.push_back(connect(m_view, &MoleculeView::atomPicked, this, [this](int index) {
        showAtom(index);
        if (m_bondEditMode) handleBondPick(index);
        else if (index<0 || m_geometryMode || !m_drawingMode) handleGeometryPick(index);
    }));
    m_documentConnections.push_back(connect(m_view, &MoleculeView::atomSelectionChanged, this, [this] {
        const auto selected=m_view->selection().atoms;
        QVector<int> ordered;
        for(int id:m_geometrySelection)if(selected.contains(id))ordered<<id;
        auto remaining=selected.values();std::sort(remaining.begin(),remaining.end());
        for(int id:remaining)if(!ordered.contains(id))ordered<<id;
        m_geometrySelection=ordered;m_view->setGeometrySelection(ordered);updateGeometryReadout();
    }));
    m_documentConnections.push_back(connect(m_view, &MoleculeView::canvasClicked, this, [this](const QVector3D &position) {
        if (m_drawingMode && !consumeInsertion(-1,position)) addDrawnAtom(position);
    }));
    m_documentConnections.push_back(connect(m_view, &MoleculeView::bondDrawn, this, [this](int first, int second, const QVector3D &position) {
        if (!m_drawingMode) return;
        if(first<0||first>=m_view->molecule().atoms.size()||second>=m_view->molecule().atoms.size())return;
        if (second < 0) addDrawnAtom(position, first);
        else if (first != second) {
            Molecule edited = m_view->molecule();
            const auto endpoint=edited.atoms[second].position;
            if (edited.atoms[first].atomicNumber == 1) edited.atoms[first].autoHydrogen = false;
            if (edited.atoms[second].atomicNumber == 1) edited.atoms[second].autoHydrogen = false;
            MoleculeEditController::drawBond(edited, first, second, drawState());
            commitEditedMolecule(edited, true);
            m_view->setDrawingAnchorAt(endpoint);
            setStatus(QStringLiteral("결합을 그렸습니다."));
        }
    }));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::bondStrokeDrawn,this,[this](const QVector3D &start,const QVector3D &end){
        if(!m_drawingMode||(start-end).length()<0.1f)return;
        auto edited=m_view->molecule();const auto state=drawState();
        const int first=MoleculeEditController::addAtom(edited,start,state),second=MoleculeEditController::addAtom(edited,end,state);
        MoleculeEditController::drawBond(edited,first,second,state);commitEditedMolecule(edited,true);
        m_view->setDrawingAnchorAt(end);
    }));
    m_documentConnections.push_back(connect(m_view, &MoleculeView::atomMoved, this, [this](int index, const QVector3D &) {
        Molecule edited = m_view->molecule();
        if (index >= 0 && index < edited.atoms.size()) edited.atoms[index].autoHydrogen = false;
        commitEditedMolecule(edited, false);
        setStatus(QStringLiteral("원자 위치를 적용했습니다."));
    }));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::distanceApplyRequested,this,[this](double value){applyGeometryMeasurement(0,value);}));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::angleApplyRequested,this,[this](double value){applyGeometryMeasurement(1,value);}));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::dihedralApplyRequested,this,[this](double value){applyGeometryMeasurement(2,value);}));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::distanceVisibilityChanged,this,[this](bool visible){m_view->setMeasurementVisible(0,visible);}));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::angleVisibilityChanged,this,[this](bool visible){m_view->setMeasurementVisible(1,visible);}));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::dihedralVisibilityChanged,this,[this](bool visible){m_view->setMeasurementVisible(2,visible);}));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::clearSelectionRequested,this,&MainWindow::clearGeometrySelection));
    m_documentConnections.push_back(connect(m_geometryBar,&GeometryMeasureBar::measurementStyleChanged,this,[this](MeasurementKind kind,const MeasurementTextStyle&style){m_view->setMeasurementStyle(int(kind),style);}));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::frameRequested, this, &MainWindow::showFrame));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::frequencyRequested, this, &MainWindow::showFrequency));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::scanPointRequested, this, &MainWindow::showScanPoint));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::ircPointRequested, this, &MainWindow::showIrcPoint));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::atomRequested, this, &MainWindow::showAtom));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::framePlayRequested, this, &MainWindow::toggleFrameAnimation));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::frequencyPlayRequested, this, &MainWindow::toggleFrequencyAnimation));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::frequencyRestoreRequested, this, &MainWindow::restoreFrequencyGeometry));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::frequencyAmplitudeChanged, this, [this](double amplitude) {
        m_animation->setAmplitude(amplitude);
        m_view->setVibrationVectorScaleFactor(m_analysis->scaledVectorsEnabled() ? amplitude : 1.0);
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::frequencyVectorLengthChanged,
        m_view, &MoleculeView::setVibrationVectorLengthFactor));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::animationSaveRequested, this, &MainWindow::exportAnimation));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::animationCopyRequested, this, &MainWindow::copyAnimation));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::coordinateImportRequested, this, [this](const QString &xyz) {
        const auto directory=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/pasted-xyz");
        if(!QDir().mkpath(directory)){setStatus(QStringLiteral("XYZ 저장 폴더를 만들 수 없습니다."));return;}
        const auto path=directory+QStringLiteral("/Pasted-%1.xyz").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QSaveFile file(path);const auto bytes=xyz.toUtf8();
        if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()){setStatus(QStringLiteral("붙여넣은 XYZ를 저장할 수 없습니다."));return;}
        openFile(path);
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::coordinateViewRequested, this, [this] {
        m_analysis->setCoordinateMolecule(m_view->molecule());
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::structureFileRequested, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("계산에 사용한 구조 파일 연결"),
            QFileInfo(m_currentFile).absolutePath(), QStringLiteral("구조 및 계산 파일 (*.xyz *.out *.log *.fchk *.fch *.mol *.sdf);;모든 파일 (*)"));
        if (!path.isEmpty()) attachResultStructure(path);
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::workspaceModeRequested, this, [this](bool tddft) {
        m_workspace->setCurrentWidget(tddft ? m_tddftWorkspace : m_moleculeWorkspace);
    }));
    if(auto *states=m_analysis->findChild<QTableWidget*>("TddftStatesTable"))
        m_documentConnections.push_back(connect(states,&QTableWidget::currentCellChanged,this,[this](int row,int,int,int){
            m_tddftWorkspacePlot->setSelectedPoint(row);
            if (row < 0 || row >= m_data.excitedStates.size()) return;
            const int frame = m_data.excitedStates[row].frameIndex;
            if (frame >= 0 && frame < m_data.frames.size()) {
                m_animation->stop();
                m_currentFrame = frame;
                m_view->setFrequencyMode(nullptr);
                m_view->setCoordinates(m_data.frames[frame].coordinates);
                m_view->setAnalysisFrame(&m_data.frames[frame]);
                m_analysis->setCurrentFrame(frame);
                updateGeometryReadout();
            }
            if(m_outputDock->isVisible())updateOutputSource("excited_states",row);
        }));
    m_documentConnections.push_back(connect(m_tddftWorkspacePlot, &PlotWidget::pointClicked, this, [this](int row) {
        if (auto *states = m_analysis->findChild<QTableWidget*>("TddftStatesTable"))
            if (row >= 0 && row < states->rowCount()) states->setCurrentCell(row, 0);
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::tddftSpectrumChanged, this,
            [this](const QVector<QPointF> &spectrum, const QVector<QPointF> &sticks, const QString &summary) {
        const bool normalized=m_analysis->findChild<QCheckBox *>(QStringLiteral("TddftNormalize"))->isChecked();
        m_tddftWorkspacePlot->setSeries(spectrum, QStringLiteral("Wavelength (nm)"),
            normalized ? QStringLiteral("Normalized absorption (a.u.)") : QStringLiteral("Calculated absorption (a.u.)"));
        m_tddftWorkspacePlot->setMarkers({});
        m_tddftWorkspacePlot->setStickSeries(sticks);
        QVector<QPointF> transitions;
        double maximum = 0;
        for (const auto &state : m_data.excitedStates)
            if (std::isfinite(state.oscillatorStrength)) maximum = std::max(maximum, state.oscillatorStrength);
        for (const auto &state : m_data.excitedStates)
            transitions.push_back({state.wavelengthNm,
                normalized && maximum > 0 ? state.oscillatorStrength / maximum : state.oscillatorStrength});
        m_tddftWorkspacePlot->setSelectablePoints(transitions);
        if (auto *states = m_analysis->findChild<QTableWidget*>("TddftStatesTable"))
            m_tddftWorkspacePlot->setSelectedPoint(states->currentRow());
        m_tddftWorkspacePlot->setEmptyMessage(
            summary == QStringLiteral("TD-DFT 데이터 없음") || summary.startsWith(QStringLiteral("Gaussian 저장 NTO"))
                ? summary
                : QStringLiteral("표시할 스펙트럼이 없습니다(막대 표시를 켜세요)."));
        m_tddftWorkspaceInfo->setText(summary);
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::vibrationVectorsChanged, this, [this](bool enabled, bool scaled) {
        m_style.showVibrationVectors = enabled;
        m_view->setStyle(m_style);
        m_view->setVibrationVectorScaleFactor(scaled ? m_analysis->frequencyAmplitude() : 1.0);
    }));
    m_documentConnections.push_back(connect(m_analysis, &AnalysisPanel::recountBondsChanged, this, [this](bool enabled) {
        if (!enabled) m_view->setBonds(m_data.molecule.bonds);
    }));
    m_documentConnections.push_back(connect(m_animation, &AnimationController::coordinatesReady, this, [this](const QVector<QVector3D> &coordinates) {
        m_view->setCoordinates(coordinates);
        if (m_animation->kind() == AnimationController::Kind::Frequency ||
            m_analysis->recountBondsEnabled())
            m_view->rebuildBonds(m_style.recountCutoffScale, m_style.recountHydrogen, m_style.recountMetalCoordination);
        if (m_analysis->coordinateViewActive()) m_analysis->setCoordinateMolecule(m_view->molecule());
    }));
    m_documentConnections.push_back(connect(m_animation, &AnimationController::frameChanged, this, [this](int index) {
        if (m_animation->kind() == AnimationController::Kind::Frames) {
            m_currentFrame = index;
            m_view->setAnalysisFrame(index >= 0 && index < m_data.frames.size() ? &m_data.frames[index] : nullptr);
            m_analysis->setCurrentFrame(index);
        }
    }));
    m_documentConnections.push_back(connect(frequencyToggle,&QPushButton::clicked,this,&MainWindow::toggleFrequencyAnimation));
    m_documentConnections.push_back(connect(frequencyRestore,&QPushButton::clicked,this,&MainWindow::restoreFrequencyGeometry));
    m_documentConnections.push_back(connect(m_animation, &AnimationController::playingChanged, this, [this,frequencyControls,frequencyToggle](bool playing) {
        const bool frequency=m_animation->kind()==AnimationController::Kind::Frequency&&m_currentFrequency>=0;
        frequencyControls->setVisible(frequency);
        if (auto *play = m_analysis->findChild<QPushButton *>("FrequencyPlay"))
            setSidebarButtonIcon(play, frequency && playing ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay,
                                 frequency && playing ? QStringLiteral("진동 일시정지") : QStringLiteral("진동 재생"));
        setSidebarButtonIcon(frequencyToggle, playing ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay,
                             playing ? QStringLiteral("진동 일시정지") : QStringLiteral("진동 재개"));
        setStatus(playing ? QStringLiteral("애니메이션 재생 중") : QStringLiteral("애니메이션 일시정지"));
    }));
    m_documentConnections.push_back(connect(m_explorer, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        const QString category = item->data(0, Qt::UserRole).toString();
        const int index = item->data(0, Qt::UserRole + 1).toInt();
        if (category == QStringLiteral("frame")) showFrame(index);
        else if (category == QStringLiteral("frequency")) showFrequency(index);
        else if (category == QStringLiteral("scan")) showScanPoint(index);
        else if (category == QStringLiteral("irc")) showIrcPoint(index);
        else if (category == QStringLiteral("atom")) showAtom(index);
        else if (category == QStringLiteral("tddft")) m_analysis->setCurrentTab(4);
        else if (category == QStringLiteral("section")) m_analysis->setCurrentTab(index);
    }));
    connectEditorControls();
}

} // namespace youngcraft
