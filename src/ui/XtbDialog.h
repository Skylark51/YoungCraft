#pragma once
#include "compute/XtbRunner.h"
#include <QDialog>
#include <functional>
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLabel;
class QPushButton;
namespace youngcraft {
class PlotWidget;
class ConstraintEditor;
class XtbDialog final : public QDialog {
    Q_OBJECT
public:
    XtbDialog(const Molecule &, const QString &runtime, QWidget *parent=nullptr,
              int initialMethod=2, bool automatic=false, const QVector<int> &selection = {});
    void setBondRetention(bool enabled);
    void embedInSidebar();
    void setSelectionProvider(std::function<QVector<int>()> provider);
    void setInputValidator(std::function<bool()> validator) { inputValidator=std::move(validator); }
    void configureOptimization(const QVector<GeometryConstraint> &, int maximum);
    bool calculationActive() const { return runner.state == XtbRunner::State::Starting || runner.state == XtbRunner::State::Running || runner.state == XtbRunner::State::Stopping; }
    void reject() override { close(); }
signals:
    void previewCoordinates(const QVector<QVector3D> &);
    void applyGeometry(const youngcraft::Molecule &);
    void openGeometry(const youngcraft::Molecule &);
    void restorePreview();
protected:
    void closeEvent(QCloseEvent *) override;
private:
    void startCalculation();
    void refreshPlot();
    void finishCalculation();
    Molecule original;
    QString runtime;
    XtbRunner runner;
    QComboBox *kind,*method,*solvation,*solvent,*level,*threads,*energyMode;
    QSpinBox *charge,*multiplicity,*cycles;
    QCheckBox *preserveFramework, *preserveBonds;
    ConstraintEditor *constraints;
    QLabel *status;
    QPushButton *start,*cancel,*apply,*open,*save;
    QWidget *settings;
    PlotWidget *plot;
    QVector<QPointF> series;
    std::function<bool()> inputValidator;
    bool closing=false, applied=false;
    bool automatic=false;
};
}
