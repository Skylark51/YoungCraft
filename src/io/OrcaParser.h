#pragma once

#include "io/GaussianParser.h"

namespace youngcraft {

// Lightweight ORCA text-output reader. No external runtime dependency.
class OrcaParser {
public:
    static bool isOrcaOutput(const QString &filePath);
    static ParseResult parse(const QString &filePath, const ParseOptions &options = {});
};

} // namespace youngcraft
