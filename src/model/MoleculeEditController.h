#pragma once
#include "model/Molecule.h"
#include <QSet>

namespace youngcraft {
struct DrawState { int element = 6; double bondOrder = 0; int formalCharge = 0; bool autoHydrogen = false; };
enum class HydrogenAction { Adjust, Add, RemoveExtra, RemoveAll };
struct SelectionModel {
    QSet<int> atoms;
    QSet<int> bonds;
    void click(int index, bool add, bool toggle);
    void connected(const Molecule &m, int start);
    void clear() { atoms.clear(); bonds.clear(); }
};
class MoleculeEditController {
public:
    static bool validate(const Molecule &m, QString *error = nullptr);
    static double idealLength(int a, int b, double order);
    static double predictOrder(int a, int b, double distance);
    static QVector3D snappedPosition(const QVector3D &origin, const QVector3D &cursor,
                                    int a, int b, double order, bool alt);
    static int addAtom(Molecule &m, const QVector3D &p, const DrawState &s);
    static bool replace(Molecule &m, const QSet<int> &selection, int element, int charge);
    static bool drawBond(Molecule &m, int a, int b, const DrawState &s);
    static void hydrogens(Molecule &m, HydrogenAction action);
    static void transform(Molecule &m, const QSet<int> &ids, const QVector3D &translation,
                          const QVector3D &rotation, const QVector3D &center);
    static QSet<int> side(const Molecule &m, int start, int a, int b);
    static bool bondGeometry(Molecule &m, int a, int b, int kind, double value,
                             bool opposite, QString *error = nullptr);
};
}
