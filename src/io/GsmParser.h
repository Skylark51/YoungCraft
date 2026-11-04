#pragma once

#include "io/GaussianParser.h"

namespace youngcraft {

// Read molecularGSM and pyGSM reaction-path coordinates and kcal/mol energies.
class GsmParser {
public:
    static bool isGsmPath(const QString &filePath);
    static bool isMoldenOutput(const QString &filePath);
    static ParseResult parse(const QString &filePath);
};

} // namespace youngcraft
