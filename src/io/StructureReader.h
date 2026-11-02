#pragma once

#include "io/GaussianParser.h"

#include <memory>

namespace youngcraft {

struct CrystalData {
    double a = 0, b = 0, c = 0;
    double alpha = 90, beta = 90, gamma = 90;
    QVector3D axisA, axisB, axisC;
    QString spaceGroup;
    Molecule asymmetricUnit;
    Molecule unitCell;
    Molecule packing;
};

struct StructureLoad {
    ParseResult result;
    std::shared_ptr<CrystalData> crystal;
};

class StructureReader {
public:
    static StructureLoad read(const QString &path, const QString &format);
};

} // namespace youngcraft
