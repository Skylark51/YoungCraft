#pragma once
#include "render/MoleculeView.h"
#include <QDialog>
class QListWidget;
class QGridLayout;
class QLabel;
class QDoubleSpinBox;
class SmokeTest;
namespace youngcraft {
struct CaptureSource { QString name; Molecule molecule; RenderStyle style; CameraState camera; };
// Own snapshots so source tabs can close or continue animating safely.
class MultiCaptureDialog final : public QDialog {
    friend class ::SmokeTest;
public:
    MultiCaptureDialog(const QVector<CaptureSource> &sources,QWidget *parent=nullptr);
private:
    void synchronize(MoleculeView *source);
    void arrange();
    QVector<QImage> renderPanels();
    void exportPanels(bool save,const QString &targetDirectory={});
    QVector<CaptureSource> m_sources;
    QVector<MoleculeView *> m_views;
    QVector<QLabel *> m_names;
    QListWidget *m_list=nullptr;
    QGridLayout *m_grid=nullptr;
    QLabel *m_status=nullptr;
    QDoubleSpinBox *m_width=nullptr;
};
}
