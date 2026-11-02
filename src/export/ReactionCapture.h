#pragma once
#include "model/Molecule.h"
#include <QImage>
#include <QStringList>

namespace youngcraft {
class ReactionCapture {
public:
    static QStringList companionPaths(const QString &tsPath);
    // Atom identity is the Gaussian atom order; never infer a permutation.
    static bool alignTo(Molecule &endpoint, const Molecule &ts);
    static QVector<QImage> equalPanels(const QImage &left, const QVector<QImage> &ts,
                                      const QImage &right, int width, double widthCm);
};
}
