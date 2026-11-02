#pragma once

#include "model/Molecule.h"

#include <QColor>
#include <QString>

namespace youngcraft {

enum class GeometryMoveMode { OneSide, BothSides };

struct MeasurementTextStyle {
    QString fontFamily;
    int pointSize = 12;
    bool bold = false;
    QColor color = Qt::black;
};

class GeometryEditor {
public:
    // Rigid approach geometry: preserve each fragment's internal coordinates.
    static bool arrangeApproach(Molecule &molecule, int a, int b, double separation,
                                QString *error = nullptr);
    static double distance(const Molecule &molecule, int a, int b);
    static double angleDegrees(const Molecule &molecule, int a, int b, int c);
    static double dihedralDegrees(const Molecule &molecule, int a, int b, int c, int d);

    static bool setDistance(Molecule &molecule, int a, int b, double targetAngstrom,
                            GeometryMoveMode mode, QString *warning = nullptr);
    static bool setAngle(Molecule &molecule, int a, int b, int c, double targetDegrees,
                         GeometryMoveMode mode, QString *warning = nullptr);
    static bool setDihedral(Molecule &molecule, int a, int b, int c, int d,
                            double targetDegrees, GeometryMoveMode mode,
                            QString *warning = nullptr);
};

} // namespace youngcraft
