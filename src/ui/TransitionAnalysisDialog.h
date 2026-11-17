#pragma once
#include "analysis/TransitionAnalysis.h"
#include <QDialog>
#include <QMap>
class QLabel;
class QTableWidget;
class QComboBox;
class QPushButton;
class QDoubleSpinBox;
namespace youngcraft {
class GaussianTools;
class MoleculeView;
class TransitionAnalysisDialog final : public QDialog {
public:
    TransitionAnalysisDialog(const CalculationData &data, int row, QWidget *parent=nullptr);
private:
    void generate();
    void nextGrid();
    void showGrids();
    CalculationData m_data;
    TransitionAnalysis m_analysis;
    QString m_source;
    QStringList m_pending;
    int m_next=0;
    QMap<QString,std::shared_ptr<const CubeGrid>> m_grids;
    GaussianTools *m_runner;
    QLabel *m_status, *m_sourceLabel;
    QComboBox *m_mode;
    QDoubleSpinBox *m_iso;
    QPushButton *m_generate, *m_cancel;
    MoleculeView *m_hole, *m_particle;
};
}
