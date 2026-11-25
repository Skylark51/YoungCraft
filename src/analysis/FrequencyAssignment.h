#pragma once

#include "model/Molecule.h"

namespace youngcraft {

struct FrequencyAnnotation {
    QString atoms;
    QString label;
    QString tooltip;
};

// Kinematic estimates from normal-mode displacements, not spectroscopic assignments.
QVector<FrequencyAnnotation> frequencyAnnotations(const CalculationData &data);

} // namespace youngcraft
