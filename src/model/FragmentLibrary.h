#pragma once
#include "model/Molecule.h"
namespace youngcraft {
struct OptimizedGeometry {
    int metal=0,oxidation=0,charge=0,multiplicity=0;
    QString adduct,coligand,source;
    QVector<int> atomicNumbers;
    QVector<QVector3D> positions;
};
struct Fragment {
    QString name, category, resource;
    QVector<Atom> atoms; QVector<Bond> bonds;
    QVector<int> attachments, dummyAtoms, hapticAtoms;
    QVector3D anchor, direction{1,0,0};
    int totalCharge = 0;
    bool ligand = false;
    QVector<OptimizedGeometry> optimizedGeometries;
};
class FragmentLibrary {
public:
    static bool metalComplex(const Fragment &fragment, int metal, int oxidation,
                             int totalCharge, int multiplicity, Molecule &result,
                             QString *error = nullptr);
    static bool saveLigand(const Molecule &m, int metal, const QSet<int> &included,
                           const QString &name, const QString &path, QString *error = nullptr);
    static QVector<Fragment> load(QString *error = nullptr, const QString &root = ":/fragments");
    static bool read(const QString &path, Fragment &fragment, QString *error = nullptr);
    static bool insert(Molecule &m, const Fragment &fragment, int target, const QVector3D &position,
                       const QVector3D &direction, QString *error = nullptr);
    static QStringList geometries();
    static QVector<QVector3D> directions(int geometry);
    static int coordinationCenter(Molecule &m, int element, int charge, int geometry, const QVector3D &position);
};
}
