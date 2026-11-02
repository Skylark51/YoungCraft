#pragma once

#include "model/Molecule.h"

#include <QString>

namespace youngcraft {

struct ParseOptions {
    bool parseHeavySections = true;
    bool preferStandardOrientation = true;
};

struct ParseResult {
    bool success = false;
    CalculationData data;
    QString error;
};

class GaussianParser {
public:
    static ParseResult parse(const QString &filePath, const ParseOptions &options = {});
};

} // namespace youngcraft
