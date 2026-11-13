#pragma once

#include "model/Molecule.h"

#include <QPointF>
#include <QString>
#include <QVector>

namespace youngcraft {

enum class SpectrumLineShape {
    Gaussian,
    Lorentzian
};

class AnalysisEngine {
public:
    static QString molecularFormula(const Molecule &molecule);
    static double molecularMass(const Molecule &molecule);
    static QVector3D principalMomentApproximation(const Molecule &molecule);
    static double collisionDiameter(const Molecule &molecule);
    static double molecularAreaApproximation(const Molecule &molecule);
    static QVector<QPointF> uvVisSpectrum(const QVector<ExcitedState> &states,
                                          double broadeningNm = 18.0,
                                          double startNm = 180.0,
                                          double endNm = 900.0,
                                          double stepNm = 2.0,
                                          SpectrumLineShape lineShape = SpectrumLineShape::Gaussian);
    static QString summary(const CalculationData &data);
    static QString formatEnergy(double hartree);
};

} // namespace youngcraft
