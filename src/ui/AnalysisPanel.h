#pragma once

#include "model/Molecule.h"

#include <QListWidget>
#include <QPointF>
#include <QTabWidget>

class QDialog;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QSlider;
class QTableWidget;
class QSpinBox;

namespace youngcraft {

class PlotWidget;
class OrbitalStudyWidget;
class MoleculeView;
class ScanResultsDialog;

class AnalysisPanel final : public QWidget {
    Q_OBJECT
public:
    explicit AnalysisPanel(QWidget *parent = nullptr);
    void setData(const CalculationData &data);
    void setOrbitalView(MoleculeView *view);
    void setCurrentFrame(int index);
    void setCurrentScanPoint(int index);
    void setCurrentFrequency(int index);
    int currentFrame() const { return m_frameList ? m_frameList->currentRow() : -1; }
    int currentFrequency() const { return m_frequencyList ? m_frequencyList->currentRow() : -1; }
    int currentTab() const { return m_tabs ? m_tabs->currentIndex() : 0; }
    double frequencyAmplitude() const;
    double frequencyCycleTime() const;
    bool recountBondsEnabled() const;
    bool scaledVectorsEnabled() const;
    bool coordinateViewActive() const;
    void setCoordinateMolecule(const Molecule &molecule);
    void selectDefaultTab();
    void selectNext();
    void selectPrevious();
    void setCurrentTab(int index);
    QStringList tabTitles() const;
    void setTabBarVisible(bool visible);

signals:
    void orbitalStyleChanged();
    void orbitalFileRequested(const QString &path);
    void currentTabChanged(int index);
    void frameRequested(int index);
    void frequencyRequested(int index);
    void scanPointRequested(int index);
    void ircPointRequested(int index);
    void atomRequested(int index);
    void frequencyPlayRequested();
    void frequencyRestoreRequested();
    void framePlayRequested();
    void frequencyAmplitudeChanged(double value);
    void frequencyCycleTimeChanged(double seconds);
    void recountBondsChanged(bool enabled);
    void vibrationVectorsChanged(bool enabled, bool scaled);
    void animationSaveRequested();
    void animationCopyRequested();
    void coordinateViewRequested();
    void coordinateImportRequested(const QString &xyz);
    void broadeningChanged(double value);
    void workspaceModeRequested(bool tddft);
    void tddftSpectrumChanged(const QVector<QPointF> &spectrum,
                              const QVector<QPointF> &sticks,
                              const QString &summary);
    void tddftStateInfoChanged(const QString &summary);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *makeGeometryTab();
    QWidget *makeFrequencyTab();
    QWidget *makeScanTab();
    QWidget *makeIrcTab();
    QWidget *makeTddftTab();
    QWidget *makeAtomsTab();
    QWidget *makeOrbitalsTab();
    QWidget *makeCoordinateTab();
    void fillFrameList();
    void fillFrequencyList();
    void fillScanPlot();
    void fillIrcPlot();
    void fillTddft();
    void fillAtoms();
    void fillOrbitals();

    CalculationData m_data;
    QTabWidget *m_tabs = nullptr;
    QListWidget *m_frameList = nullptr;
    QListWidget *m_frequencyList = nullptr;
    QListWidget *m_scanList = nullptr;
    QListWidget *m_ircList = nullptr;
    QTableWidget *m_tddftTable = nullptr;
    QTableWidget *m_atomTable = nullptr;
    OrbitalStudyWidget *m_orbitalStudy = nullptr;
    QLabel *m_frameInfo = nullptr;
    QLabel *m_frequencyInfo = nullptr;
    QPushButton *m_copyFrequencyAnimation = nullptr;
    QSlider *m_frameSlider = nullptr;
    QDoubleSpinBox *m_amplitude = nullptr;
    QDoubleSpinBox *m_cycleTime = nullptr;
    QCheckBox *m_recountBonds = nullptr;
    QCheckBox *m_vectors = nullptr;
    QCheckBox *m_scaledVectors = nullptr;
    QComboBox *m_broadeningShape = nullptr;
    QDoubleSpinBox *m_broadening = nullptr;
    QCheckBox *m_normalizeSpectrum = nullptr;
    QCheckBox *m_showSpectrumSticks = nullptr;
    PlotWidget *m_frequencyPlot = nullptr;
    PlotWidget *m_framePlot = nullptr;
    QDialog *m_optimizationResults = nullptr;
    QPushButton *m_openOptimizationGraph = nullptr;
    QVector<int> m_energyFrames;
    QLabel *m_scanStatus = nullptr;
    ScanResultsDialog *m_scanResults = nullptr;
    PlotWidget *m_ircPlot = nullptr;
    QLabel *m_coordinateTitle = nullptr;
    QPlainTextEdit *m_coordinateText = nullptr;
    QPushButton *m_coordinateCopy = nullptr;
    QString m_coordinateClipboardText;
    int m_coordinateTabIndex = -1;
    int m_tddftTabIndex = -1;
    bool m_frequencySelectionChanged = false;
};

} // namespace youngcraft
