#pragma once
#include "animation/MechanismTimeline.h"
#include "render/CameraState.h"
#include "settings/StyleManager.h"
#include <QDialog>
#include <QElapsedTimer>
#include <QTimer>

class QTableWidget;
class QLabel;
class QSlider;
class QPushButton;
class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QCloseEvent;
class SmokeTest;
namespace youngcraft {
class MoleculeView;
class ReactionCoordinateWidget;
class MechanismAnimationDialog final : public QDialog {
    friend class ::SmokeTest;
    Q_OBJECT
public:
    explicit MechanismAnimationDialog(const RenderStyle &style, const QVector<MechanismStep> &openSteps={}, QWidget *parent=nullptr);
    void addFiles(const QStringList &paths);
    bool setSteps(const QVector<MechanismStep> &steps);
    bool openProject(const QString &path);
    bool saveProject(const QString &path);
    // Also used by regression tests and reproducible previews.
    bool exportAnimation(const QString &path, const QString &format, int width=960, int fps=30);
protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
private:
    void populate();
    bool rebuild(bool fit=true);
    void seek(double time);
    void play(bool on);
    void moveStep(int delta);
    void editMapping();
    void emphasize(int mode);
    void updateControls();
    void fitTimeline();
    void showPlaybackWindow();
    QVector<MechanismStep> m_steps, m_openSteps;
    MechanismTimeline m_timeline;
    MechanismOptions m_options;
    MoleculeView *m_view=nullptr;
    ReactionCoordinateWidget *m_coordinate=nullptr;
    QPushButton *m_coordinateMode=nullptr,*m_coordinateEdit=nullptr;
    QDialog *m_player=nullptr;
    QPushButton *m_playerOpen=nullptr,*m_playerPlay=nullptr;
    QSlider *m_playerSlider=nullptr;
    QCheckBox *m_playerLoop=nullptr;
    QLabel *m_playerCaption=nullptr;
    QTableWidget *m_table=nullptr;
    QLabel *m_status=nullptr,*m_caption=nullptr;
    QWidget *m_controls=nullptr;
    QSlider *m_slider=nullptr;
    QPushButton *m_play=nullptr,*m_gif=nullptr,*m_mp4=nullptr,*m_png=nullptr;
    QCheckBox *m_align=nullptr,*m_preserve=nullptr,*m_keep=nullptr,*m_path=nullptr,*m_loop=nullptr,*m_labels=nullptr,*m_transparent=nullptr,*m_captions=nullptr;
    QDoubleSpinBox *m_lift=nullptr;
    QSpinBox *m_fps=nullptr;
    QComboBox *m_resolution=nullptr;
    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_time=0,m_startedAt=0;
    bool m_loading=false,m_exporting=false,m_valid=false;
    QSet<int> m_focus;
    int m_focusMode=-1; // -1 inherits the source; 2 explicitly restores all atoms.
    QSet<int> m_parked;
    double m_parkStart=0;
    QVector3D m_fittedCenter;
};
}
