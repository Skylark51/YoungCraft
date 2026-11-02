#pragma once

#include "model/GeometryEditor.h"

#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace youngcraft {

enum class MeasurementKind { Distance, Angle, Dihedral };

class GeometryMeasureBar final : public QWidget {
    Q_OBJECT
public:
    explicit GeometryMeasureBar(QWidget *parent = nullptr);
    void setSelection(const Molecule &molecule, const QVector<int> &indices);
    void setMeasurementStyle(MeasurementKind kind, const MeasurementTextStyle &style);
    MeasurementTextStyle measurementStyle(MeasurementKind kind) const;
    GeometryMoveMode moveMode() const;
    bool moveFirstSide() const;
    void setMovement(GeometryMoveMode mode, bool firstSide);

signals:
    void distanceApplyRequested(double value);
    void angleApplyRequested(double value);
    void dihedralApplyRequested(double value);
    void distanceVisibilityChanged(bool visible);
    void angleVisibilityChanged(bool visible);
    void dihedralVisibilityChanged(bool visible);
    void moveModeChanged(youngcraft::GeometryMoveMode mode);
    void clearSelectionRequested();
    void measurementStyleChanged(youngcraft::MeasurementKind kind,
                                 const youngcraft::MeasurementTextStyle &style);

private:
    struct Row {
        QWidget *widget{};
        QLabel *atoms{};
        QDoubleSpinBox *value{};
        QPushButton *show{};
        QPushButton *style{};
    };
    Row createRow(MeasurementKind kind, const QString &unit, int decimals,
                  double minimum, double maximum, double step);
    void editStyle(MeasurementKind kind);
    Row &row(MeasurementKind kind);
    const Row &row(MeasurementKind kind) const;

    Row m_distance;
    Row m_angle;
    Row m_dihedral;
    QComboBox *m_moveMode{};
    MeasurementTextStyle m_distanceStyle;
    MeasurementTextStyle m_angleStyle;
    MeasurementTextStyle m_dihedralStyle;
};

} // namespace youngcraft
