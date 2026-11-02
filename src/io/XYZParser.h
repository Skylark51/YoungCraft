#pragma once

#include "io/GaussianParser.h"

namespace youngcraft {

class XYZParser {
public:
    static ParseResult parseText(const QString &text);
    static ParseResult parse(const QString &filePath);
};

} // namespace youngcraft
