#pragma once
#include "model/FragmentLibrary.h"
#include <QSet>

namespace youngcraft {
struct LigandCandidate {
    int metal = -1;
    QSet<int> atoms;
    QVector<int> donors;
    QString name;
    bool knownTemplate = false;
    bool protectedCore = false;
};
class LigandReplacement {
public:
    // Indices are zero-based. Every atom outside the selected component is retained.
    static QVector<LigandCandidate> recognize(const Molecule &m, int metal,
        const QVector<Fragment> &library = {}, const QSet<int> &protectedAtoms = {});
    static bool replace(const Molecule &source, const LigandCandidate &candidate,
        const Fragment &replacement, const QSet<int> &protectedAtoms,
        Molecule &result, QString *error = nullptr);
};
}
