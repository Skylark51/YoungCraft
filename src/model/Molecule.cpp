#include "model/Molecule.h"

#include <QHash>
#include <QSet>
#include <QQuaternion>
#include <QtMath>

#include <iterator>
#include <cmath>

namespace youngcraft {

namespace {

struct ElementData {
    const char *symbol;
    double mass;
    double covalent;
    double vdw;
    QColor color;
};

const ElementData kElements[] = {
    {"", 0.0, 0.0, 0.0, QColor(180, 180, 180)},
    {"H", 1.008, 0.31, 1.20, QColor(245, 245, 245)},
    {"He", 4.003, 0.28, 1.40, QColor(217, 255, 255)},
    {"Li", 6.941, 1.28, 1.82, QColor(204, 128, 255)},
    {"Be", 9.012, 0.96, 1.53, QColor(194, 255, 0)},
    {"B", 10.811, 0.84, 1.92, QColor(255, 181, 181)},
    {"C", 12.011, 0.76, 1.70, QColor(70, 70, 70)},
    {"N", 14.007, 0.71, 1.55, QColor(48, 80, 248)},
    {"O", 15.999, 0.66, 1.52, QColor(235, 40, 40)},
    {"F", 18.998, 0.57, 1.47, QColor(144, 224, 80)},
    {"Ne", 20.180, 0.58, 1.54, QColor(179, 227, 245)},
    {"Na", 22.990, 1.66, 2.27, QColor(171, 92, 242)},
    {"Mg", 24.305, 1.41, 1.73, QColor(138, 255, 0)},
    {"Al", 26.982, 1.21, 1.84, QColor(191, 166, 166)},
    {"Si", 28.086, 1.11, 2.10, QColor(240, 200, 160)},
    {"P", 30.974, 1.07, 1.80, QColor(255, 128, 0)},
    {"S", 32.066, 1.05, 1.80, QColor(255, 200, 50)},
    {"Cl", 35.453, 1.02, 1.75, QColor(31, 240, 31)},
    {"Ar", 39.948, 1.06, 1.88, QColor(128, 209, 227)},
    {"K", 39.098, 2.03, 2.75, QColor(143, 64, 212)},
    {"Ca", 40.078, 1.76, 2.31, QColor(61, 255, 0)},
    {"Sc", 44.956, 1.70, 2.30, QColor(230, 230, 230)},
    {"Ti", 47.867, 1.60, 2.15, QColor(191, 194, 199)},
    {"V", 50.942, 1.53, 2.05, QColor(166, 166, 171)},
    {"Cr", 51.996, 1.39, 2.05, QColor(138, 153, 199)},
    {"Mn", 54.938, 1.39, 2.05, QColor(156, 122, 199)},
    {"Fe", 55.845, 1.32, 2.00, QColor(224, 102, 51)},
    {"Co", 58.933, 1.26, 2.00, QColor(240, 144, 160)},
    {"Ni", 58.693, 1.24, 1.63, QColor(80, 208, 80)},
    {"Cu", 63.546, 1.32, 1.40, QColor(200, 128, 51)},
    {"Zn", 65.39, 1.22, 1.39, QColor(125, 128, 176)},
    {"Ga", 69.723, 1.22, 1.87, QColor(194, 143, 143)},
    {"Ge", 72.64, 1.20, 2.11, QColor(102, 143, 143)},
    {"As", 74.922, 1.19, 1.85, QColor(189, 128, 227)},
    {"Se", 78.96, 1.20, 1.90, QColor(255, 161, 0)},
    {"Br", 79.904, 1.20, 1.85, QColor(166, 41, 41)},
    {"Kr", 83.80, 1.16, 2.02, QColor(92, 184, 209)},
    {"I", 126.904, 1.39, 1.98, QColor(148, 0, 148)},
    {"Pt", 195.084, 1.36, 1.75, QColor(208, 208, 224)},
    {"Au", 196.967, 1.36, 1.66, QColor(255, 209, 35)},
    {"Hg", 200.59, 1.32, 1.55, QColor(184, 184, 208)}
};

constexpr const char *kSymbols[] = {
    "", "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca",
    "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y",
    "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce",
    "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W", "Re", "Os",
    "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U", "Np", "Pu",
    "Am", "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn",
    "Nh", "Fl", "Mc", "Lv", "Ts", "Og"
};
static_assert(std::size(kSymbols) == 119);

const ElementData &elementData(int atomicNumber) {
    if (atomicNumber >= 1 && atomicNumber <= 36) return kElements[atomicNumber];
    switch (atomicNumber) {
    case 53: return kElements[37];
    case 78: return kElements[38];
    case 79: return kElements[39];
    case 80: return kElements[40];
    default: return kElements[0];
    }
}

} // namespace

QString elementSymbol(int atomicNumber) {
    return atomicNumber >= 1 && atomicNumber <= 118 ? QString::fromLatin1(kSymbols[atomicNumber]) : QString();
}

int atomicNumberFor(QString symbol) {
    symbol = symbol.trimmed();
    if (symbol.isEmpty()) {
        return 0;
    }
    symbol[0] = symbol[0].toUpper();
    for (int i = 1; i < symbol.size(); ++i) {
        symbol[i] = symbol[i].toLower();
    }
    for (int i = 1; i <= 118; ++i) {
        if (symbol == QLatin1String(kSymbols[i])) {
            return i;
        }
    }
    return 0;
}

double atomicMass(int atomicNumber) {
    const double known=elementData(atomicNumber).mass;
    if(known>0.0)return known;
    // Supplement the original element table with CIAAW 2024 abridged standard weights.
    // https://www.ciaaw.org/abridged-atomic-weights.htm
    static constexpr double standardWeights[]={
        0.0, 1.008, 4.0026, 6.94, 9.0122, 10.81, 12.011, 14.007, 15.999, 18.998,
        20.18, 22.99, 24.305, 26.982, 28.085, 30.974, 32.06, 35.45, 39.95, 39.098,
        40.078, 44.956, 47.867, 50.942, 51.996, 54.938, 55.845, 58.933, 58.693, 63.546,
        65.38, 69.723, 72.63, 74.922, 78.971, 79.904, 83.798, 85.468, 87.62, 88.906,
        91.222, 92.906, 95.95, 0.0, 101.07, 102.91, 106.42, 107.87, 112.41, 114.82,
        118.71, 121.76, 127.6, 126.9, 131.29, 132.91, 137.33, 138.91, 140.12, 140.91,
        144.24, 0.0, 150.36, 151.96, 157.25, 158.93, 162.5, 164.93, 167.26, 168.93,
        173.05, 174.97, 178.49, 180.95, 183.84, 186.21, 190.23, 192.22, 195.08, 196.97,
        200.59, 204.38, 207.2, 208.98, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        232.04, 231.04, 238.03, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    };
    return atomicNumber>=0&&atomicNumber<int(std::size(standardWeights))?standardWeights[atomicNumber]:0.0;
}

double covalentRadius(int atomicNumber) {
    const double value = elementData(atomicNumber).covalent;
    return value > 0.0 ? value : 1.3;
}

double vanDerWaalsRadius(int atomicNumber) {
    const double value = elementData(atomicNumber).vdw;
    return value > 0.0 ? value : 2.0;
}

QColor elementColor(int atomicNumber, bool monochrome) {
    if (monochrome) {
        return QColor(210, 210, 210);
    }
    return elementData(atomicNumber).color;
}

bool isMetal(int atomicNumber) {
    return (atomicNumber >= 3 && atomicNumber <= 4) ||
           (atomicNumber >= 11 && atomicNumber <= 13) ||
           (atomicNumber >= 19 && atomicNumber <= 31) ||
           (atomicNumber >= 37 && atomicNumber <= 48) ||
           atomicNumber == 49 || atomicNumber == 50 || (atomicNumber >= 81 && atomicNumber <= 83) ||
           (atomicNumber >= 113 && atomicNumber <= 116) ||
           atomicNumber == 55 || atomicNumber == 56 || (atomicNumber >= 87 && atomicNumber <= 112) ||
           (atomicNumber >= 57 && atomicNumber <= 80);
}

void inferBonds(Molecule &molecule, double cutoffScale, bool includeHydrogen,
                bool includeMetalCoordination, bool preserveManual) {
    const QVector<Bond> previous = molecule.bonds;
    molecule.bonds.clear();
    const auto &atoms = molecule.atoms;
    for (int i = 0; i < atoms.size(); ++i) {
        if (!atoms[i].visible) {
            continue;
        }
        for (int j = i + 1; j < atoms.size(); ++j) {
            if (!atoms[j].visible || (!includeHydrogen &&
                                      (atoms[i].atomicNumber == 1 || atoms[j].atomicNumber == 1))) {
                continue;
            }
            const bool metalPair = isMetal(atoms[i].atomicNumber) || isMetal(atoms[j].atomicNumber);
            if (metalPair && !includeMetalCoordination) {
                continue;
            }
            const double cutoff = (covalentRadius(atoms[i].atomicNumber) +
                                   covalentRadius(atoms[j].atomicNumber)) *
                                  cutoffScale;
            const double distance = (atoms[i].position - atoms[j].position).length();
            if (distance > 0.15 && distance <= cutoff) {
                Bond inferred{i, j, 1.0, metalPair, metalPair ? BondStyle::Dashed : BondStyle::Solid, false};
                if (preserveManual) {
                    for (const Bond &old : previous) {
                        if (old.manual && ((old.first == i && old.second == j) ||
                                           (old.first == j && old.second == i))) {
                            inferred = old;
                            break;
                        }
                    }
                }
                molecule.bonds.push_back(inferred);
            }
        }
    }
    if (preserveManual) {
        for (const Bond &old : previous) {
            if (!old.manual || molecule.bondIndex(old.first, old.second) >= 0) continue;
            if (old.first >= 0 && old.second >= 0 && old.first < molecule.atoms.size() &&
                old.second < molecule.atoms.size()) molecule.bonds.push_back(old);
        }
    }
}

void Molecule::rebuildBonds(double cutoffScale, bool includeHydrogen,
                            bool includeMetalCoordination, bool preserveManual) {
    inferBonds(*this, cutoffScale, includeHydrogen, includeMetalCoordination, preserveManual);
}

int Molecule::bondIndex(int first, int second) const {
    for (int i = 0; i < bonds.size(); ++i) {
        const Bond &bond = bonds[i];
        if ((bond.first == first && bond.second == second) ||
            (bond.first == second && bond.second == first)) return i;
    }
    return -1;
}

void Molecule::setBond(int first, int second, double order, bool coordination,
                       BondStyle style, bool manual) {
    if (!std::isfinite(order) || order < 1.0 || order > 3.0) return;
    if (first == second || first < 0 || second < 0 || first >= atoms.size() || second >= atoms.size()) return;
    const int existing = bondIndex(first, second);
    Bond bond{first, second, order, coordination, style, manual};
    if (existing >= 0) bonds[existing] = bond;
    else bonds.push_back(bond);
}

void Molecule::removeBond(int first, int second) {
    const int existing = bondIndex(first, second);
    if (existing >= 0) bonds.removeAt(existing);
}

bool Molecule::removeAtom(int index) {
    if (index < 0 || index >= atoms.size()) return false;
    for (int j = coordinationSites.size()-1; j>=0; --j) {
        if(coordinationSites[j].center==index) coordinationSites.removeAt(j);
        else {
            if(coordinationSites[j].center>index) --coordinationSites[j].center;
            if(coordinationSites[j].donor==index){coordinationSites[j].occupied=false;coordinationSites[j].donor=-1;}
            else if(coordinationSites[j].donor>index)--coordinationSites[j].donor;
        }
    }
    for(int j=hapticContacts.size()-1;j>=0;--j){
        auto &h=hapticContacts[j];
        if(h.metal==index||h.atoms.contains(index)){hapticContacts.removeAt(j);continue;}
        if(h.metal>index)--h.metal;for(int &i:h.atoms)if(i>index)--i;
    }
    charge-=atoms[index].formalCharge;
    atoms.removeAt(index);
    for (int i = bonds.size() - 1; i >= 0; --i) {
        Bond &bond = bonds[i];
        if (bond.first == index || bond.second == index) { bonds.removeAt(i); continue; }
        if (bond.first > index) --bond.first;
        if (bond.second > index) --bond.second;
    }
    for (int i = 0; i < atoms.size(); ++i) atoms[i].index = i + 1;
    return true;
}

namespace {
int hydrogenValence(const Atom &a, double used) {
    const int q=a.formalCharge;
    switch(a.atomicNumber){
    case 6: case 14: return q==0?4:std::abs(q)==1?3:0;
    case 7: return q==0?3:q==1?4:q==-1?2:0;
    case 8: return q==0?2:q==1?3:q==-1?1:q==-2?0:0;
    case 15: return q==0?(used>3.1?5:3):q==1?4:q==-1?2:0;
    case 16: return q==0?(used>4.1?6:used>2.1?4:2):q==1?3:q==-1?1:0;
    case 9: case 17: case 35: case 53: return q==0?1:0;
    default:return 0;
    }
}
}
void Molecule::reconcileAutoHydrogens(bool addOnly, bool removeOnly) {
    reconcileAutoHydrogens(addOnly, removeOnly, {});
}

void Molecule::reconcileAutoHydrogens(bool addOnly, bool removeOnly, const QSet<int> &onlyOnAtoms) {
    // Track scope through atom removals; original selection indices may shift.
    QVector<bool> editable;
    for (int i = 0; i < atoms.size(); ++i)
        editable << ((onlyOnAtoms.isEmpty() || onlyOnAtoms.contains(i)) &&
                     atoms[i].atomicNumber != 1 && !isMetal(atoms[i].atomicNumber) &&
                     !atoms[i].optimizationFixed && !atoms[i].autoHydrogenLocked);
    auto removableHydrogen = [&](int h) {
        if (atoms[h].atomicNumber != 1 || !atoms[h].autoHydrogen || atoms[h].optimizationFixed) return false;
        int owner = -1, count = 0;
        bool ordinary = false;
        for (const auto &bond : bonds) if (bond.first == h || bond.second == h) {
            ++count; owner = bond.first == h ? bond.second : bond.first;
            ordinary = !bond.coordination && bond.order == 1.0;
        }
        // Global automatic editing also clears generated H orphaned by deleting a heavy atom.
        if (count == 0) return onlyOnAtoms.isEmpty();
        return count == 1 && ordinary && owner >= 0 && editable[owner];
    };
    if (removeOnly) {
        QSet<int> remove;
        for(int i=0;i<atoms.size();++i){
            if(!editable[i])continue;
            double used=0;QVector<int> hs;
            for(auto bond:bonds)if(bond.first==i||bond.second==i){
                int n=bond.first==i?bond.second:bond.first;
                if(bond.coordination||isMetal(atoms[n].atomicNumber))continue;
                used+=bond.order;if(removableHydrogen(n))hs<<n;
            }
            int excess=qMax(0,int(std::ceil(used-hydrogenValence(atoms[i],used))));
            for(int n:hs){if(excess--<=0)break;remove.insert(n);}
        }
        for(int i=atoms.size()-1;i>=0;--i)if(remove.contains(i))removeAtom(i);
        return;
    }
    if (!addOnly) for (int i = atoms.size() - 1; i >= 0; --i)
        if (removableHydrogen(i)) { removeAtom(i); editable.removeAt(i); }

    const int heavyCount = atoms.size();
    for (int i = 0; i < heavyCount; ++i) {
        const Atom &heavy = atoms[i];
        if (!editable[i]) continue;
        double used = 0.0, maxOrder=1.0;
        QVector<QVector3D> occupied;
        for (const Bond &bond : bonds) {
            if (bond.first != i && bond.second != i) continue;

            const int neighbor = bond.first == i ? bond.second : bond.first;
            if (neighbor < 0 || neighbor >= atoms.size()) continue;
            if (!bond.coordination && !isMetal(atoms[neighbor].atomicNumber)) { used += bond.order; maxOrder=qMax(maxOrder,bond.order); }
            QVector3D direction = atoms[neighbor].position - heavy.position;
            if (direction.lengthSquared() > 1e-8f) occupied.push_back(direction.normalized());
        }
        const int target = hydrogenValence(heavy, used);
        const double missing = target - used;
        const int count = qRound(missing);
        if (count <= 0 || count > 4 || std::abs(missing - count) > 0.05) continue;
        const QVector3D origin = heavy.position;
        const float distance = float(covalentRadius(heavy.atomicNumber) + covalentRadius(1));
        QVector<QVector3D> candidates;
        if(maxOrder>=2.5) candidates={{1,0,0},{-1,0,0}};
        else if(maxOrder>=1.5) candidates={{1,0,0},{-0.5f,0.8660254f,0},{-0.5f,-0.8660254f,0}};
        else if(target>4) candidates={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        else candidates={{1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1}};
        for(auto &v:candidates)v.normalize();
        if(!occupied.isEmpty()){auto align=QQuaternion::rotationTo(candidates.first(),occupied.first());for(auto &v:candidates)v=align.rotatedVector(v);}
        for (int h = 0; h < count; ++h) {
            QVector3D best(1, 0, 0);float bestScore=-1;
            for(auto candidate:candidates){float score=2.0f;for(const auto &other:occupied)score=qMin(score,1-QVector3D::dotProduct(candidate,other));if(score>bestScore){bestScore=score;best=candidate;}}
            Atom hydrogen;
            hydrogen.index = atoms.size() + 1;
            hydrogen.atomicNumber = 1;
            hydrogen.element = QStringLiteral("H");
            hydrogen.position = origin + best * distance;
            hydrogen.autoHydrogen = true;
            atoms.push_back(hydrogen);
            setBond(i, atoms.size() - 1, 1.0, false, BondStyle::Solid, true);
            occupied.push_back(best);
        }
    }
}

} // namespace youngcraft
