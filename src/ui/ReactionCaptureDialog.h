#pragma once
#include "model/Molecule.h"
#include "render/CameraState.h"
#include "settings/StyleManager.h"
#include <QDialog>
#include <QSet>

class QLabel;
class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QDoubleSpinBox;
class SmokeTest;
class QListWidget;
class QGridLayout;
namespace youngcraft {
class MoleculeView;
class AnimationController;
class ReactionCaptureDialog final : public QDialog {
    friend class ::SmokeTest;
public:
    ReactionCaptureDialog(const CalculationData &data, MoleculeView *source,
                          double amplitude, double cycle, bool recount, QWidget *parent, bool si = true);
private:
    void updateCaptureMode();
    void updateCaption();
    void exportStaticPanels(int kind, const QString &directory);
    QCheckBox *m_si=nullptr, *m_includeTs=nullptr;
    QLineEdit *m_figureNumber=nullptr, *m_structureName=nullptr;
    QPlainTextEdit *m_caption=nullptr;
    void synchronize(const CameraState &camera);
    QVector<QImage> renderPanels();
    void exportPanels(int kind);
    void updateHydrogenVisibility();
    enum class HydrogenVisibility { Original, HideCarbon, SelectedOnly, ShowAll };
    HydrogenVisibility m_hydrogenVisibility=HydrogenVisibility::Original;
    QVector<bool> m_initialVisibility;
    QSet<int> m_carbonHydrogens;
    QSet<int> m_keptHydrogens;
    QListWidget *m_hydrogens=nullptr;
    CalculationData m_data;
    MoleculeView *m_views[3]{};
    QGridLayout *m_previewGrid=nullptr;
    QLabel *m_names[3]{};
    QLabel *m_status=nullptr;
    QDoubleSpinBox *m_width=nullptr;
    QVector<QPushButton *> m_exports;
    AnimationController *m_animation=nullptr;
    double m_amplitude, m_cycle;
    bool m_recount=false, m_ready=false, m_ppt=false;
    int m_mode=-1;
    CameraState m_camera;
};
}
