#pragma once
#include "model/Molecule.h"
#include "compute/GaussianTools.h"
#include <QWidget>
#include <QPointer>
class QTableWidget;
class QTabWidget;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QPushButton;
class QCheckBox;
namespace youngcraft {
class MoleculeView;
class OrbitalStudyWidget final : public QWidget {
    Q_OBJECT
public:
    explicit OrbitalStudyWidget(QWidget *parent=nullptr);
    void setData(const CalculationData &data);
    void setView(MoleculeView *view);
    void startDefaultOrbital();
    QSize minimumSizeHint() const override;
protected:
    void resizeEvent(QResizeEvent *event) override;
public:
signals:
    void surfaceStyleChanged();
    void openRequested(const QString &path);
private:
    void updateResponsiveControls();
    void expandTable();
    void generate(bool mep);
    void applyStyle();
    void showSurface(std::shared_ptr<const CubeGrid> surface, std::shared_ptr<const CubeGrid> potential);
    void exportTable();
    bool chooseFrontier(bool lumo);
    void updateGenerationButtons();
    CalculationData m_data;
    QPointer<MoleculeView> m_view;
    GaussianTools *m_runner;
    QTableWidget *m_orbitals=nullptr;
    QTabWidget *m_tables=nullptr;
    QComboBox *m_resolution, *m_density, *m_palette, *m_surfacePattern;
    QPushButton *m_plusButton, *m_minusButton;
    void syncPalette();
    QDoubleSpinBox *m_iso, *m_opacity, *m_min, *m_max;
    QCheckBox *m_positive, *m_negative;
    QLabel *m_status, *m_source, *m_legend;
    QPushButton *m_generateMo, *m_generateMep, *m_cancel, *m_homo, *m_lumo;
    QColor m_plusColor{60,110,235}, m_minusColor{220,65,65}, m_lowColor{220,35,35}, m_highColor{40,65,220};
    std::shared_ptr<const CubeGrid> m_surface, m_potential;
    bool m_mep = false;
    double m_moIsovalue = 0.02, m_mepIsovalue = 0.001;
};
}
