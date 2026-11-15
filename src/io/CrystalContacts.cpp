#include "io/CrystalContacts.h"
#include <gemmi/unitcell.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace youngcraft {
QVector<CrystalContact> crystalContacts(const CrystalData &crystal, double cutoff) {
    if (!std::isfinite(cutoff) || cutoff <= 0 || cutoff > 10)
        throw std::runtime_error("Contact cutoff must be between 0 and 10 Angstrom");
    gemmi::UnitCell cell(crystal.a, crystal.b, crystal.c, crystal.alpha, crystal.beta, crystal.gamma);
    if (!std::isfinite(cell.volume) || cell.volume <= 0)
        throw std::runtime_error("Invalid unit cell");
    QVector<gemmi::Fractional> sites;
    for (const auto &atom : crystal.unitCell.atoms) {
        const auto p = atom.position;
        if (!std::isfinite(p.x()) || !std::isfinite(p.y()) || !std::isfinite(p.z()))
            throw std::runtime_error("Invalid atom coordinate");
        sites.push_back(cell.fractionalize({p.x(), p.y(), p.z()}));
    }
    if (sites.size() > 2000) throw std::runtime_error("Contact search is limited to 2000 unit-cell atoms");
    QVector<CrystalContact> result;
    qint64 candidates = 0;
    for (int i = 0; i < sites.size(); ++i) for (int j = i; j < sites.size(); ++j) {
        const auto delta = sites[j] - sites[i];
        const double radius[] = {cutoff * cell.ar, cutoff * cell.br, cutoff * cell.cr};
        const double d[] = {delta.x, delta.y, delta.z};
        int low[3], high[3];
        for (int axis = 0; axis < 3; ++axis) {
            if (!std::isfinite(radius[axis]) || radius[axis] > 100000 || std::abs(d[axis]) > 100000)
                throw std::runtime_error("Cell or coordinates exceed interactive contact search limits");
            low[axis] = int(std::ceil(-d[axis] - radius[axis] - 1e-8));
            high[axis] = int(std::floor(-d[axis] + radius[axis] + 1e-8));
        }
        // Reciprocal-vector bounds include all images inside the Cartesian sphere,
        // including skew cells and cutoffs spanning more than one lattice period.
        for (int x = low[0]; x <= high[0]; ++x)
            for (int y = low[1]; y <= high[1]; ++y)
                for (int z = low[2]; z <= high[2]; ++z) {
                    if (++candidates > 20000000) throw std::runtime_error("Contact search exceeds interactive limit");
                    if (i == j && (x < 0 || (x == 0 && y < 0) || (x == 0 && y == 0 && z <= 0))) continue;
                    const double distance = cell.orthogonalize_difference(delta + gemmi::Fractional(x, y, z)).length();
                    if (distance > 1e-6 && distance <= cutoff + 1e-8) {
                        result.push_back({i, j, {x, y, z}, distance});
                        if (result.size() > 20000) throw std::runtime_error("Contact list exceeds 20000 entries; reduce cutoff");
                    }
                }
    }
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return a.distance < b.distance; });
    return result;
}
}
