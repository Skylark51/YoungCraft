#pragma once

#include "model/Molecule.h"
#include <QDialog>

class QTableWidget;
class QListWidget;
class QCheckBox;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace youngcraft {
class PlotWidget;

class ScanResultsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ScanResultsDialog(QWidget *parent = nullptr);
    void setData(const CalculationData &data, int selected = -1);
    void setSelectedPoint(int index);
    void setSelectedFrame(int index);
signals:
    void pointSelected(int index);
    void frameSelected(int index);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool navigatePoint(QKeyEvent *event);
    void updateHighlight();
    void updateOptimization();
    QVector<Frame> m_frames;
    QVector<int> m_historyFrames, m_energyFrames;
    PlotWidget *m_optimizationPlot;
    QListWidget *m_history;
    QLabel *m_optimizationStatus;
    void editStyle();
    void editAxisTitles();
    void saveAxisTitles();
    void editFrame();
    void editMechanism();
    QVector<ScanPoint> m_points;
    PlotWidget *m_plot;
    QTableWidget *m_table;
    QCheckBox *m_highlight;
    QDoubleSpinBox *m_threshold;
    QComboBox *m_reference;
    QLabel *m_status;
    QPushButton *m_mechanism;
    bool m_mechanismInitialized = false;
    int m_selected = -1;
    int m_scanDimensions = 1;
    QString m_scanAxisLabel;
    QString m_scanAxisLabel2;
};
} // namespace youngcraft
