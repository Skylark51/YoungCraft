#pragma once
#include "model/Molecule.h"
#include "io/CubeParser.h"
#include <memory>

namespace youngcraft {
struct NtoPair {
    double weight = 0; // Normalized squared singular value.
    QVector<double> hole, particle; // Signed MO coefficients.
};
struct TransitionAnalysis {
    QStringList occupied, virtuals;
    QVector<NtoPair> pairs;
    QVector<double> holePopulation, particlePopulation;
    double printedNorm = 0;
    QString error;
    static TransitionAnalysis calculate(const ExcitedState &state);
    // Grids must follow occupied / virtuals and share a voxel lattice.
    std::shared_ptr<CubeGrid> grid(const QVector<std::shared_ptr<const CubeGrid>> &mos,
                                 bool particle, int pair, QString *error) const;
};
}
