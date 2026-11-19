#pragma once

#include "model/NboData.h"
#include <QHash>

namespace youngcraft {
struct CalculationData;
class NboParser {
public:
    static NboData parse(const QString &path);
    static void assignFrameCharges(CalculationData &data, const QHash<qint64, int> &chargeFrames);
};
}
