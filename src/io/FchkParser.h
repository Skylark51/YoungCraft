#pragma once

#include "io/GaussianParser.h"

namespace youngcraft {

class FchkParser {
public:
    static ParseResult parse(const QString &filePath);
};

} // namespace youngcraft
