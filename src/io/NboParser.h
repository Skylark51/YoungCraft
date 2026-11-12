#pragma once

#include "model/NboData.h"

namespace youngcraft {
class NboParser {
public:
    static NboData parse(const QString &path);
};
}
