#pragma once
#include "model/MoleculeEditController.h"
#include <QByteArray>

namespace youngcraft {
class StructureTools {
public:
    static QSet<int> selection(const Molecule &m, const QSet<int> &ids);
    static QSet<int> expand(const Molecule &m, const QSet<int> &ids, bool connected = false);
    static QSet<int> invert(const Molecule &m, const QSet<int> &ids);
    static Molecule fragment(const Molecule &m, const QSet<int> &ids);
    static QVector<QSet<int>> components(const Molecule &m);
    static QByteArray encode(const Molecule &m);
    static bool decode(const QByteArray &bytes, Molecule &m, QString *error = nullptr);
    static bool append(Molecule &target, const Molecule &fragment, const QVector3D &offset, QString *error = nullptr);
    static QString xyz(const Molecule &m);
    static QString zMatrix(const Molecule &m, QString *error = nullptr);
    static QString gaussianInput(const Molecule &m, const QString &route, int charge, int multiplicity,
                                 QString *error = nullptr);
};
}
