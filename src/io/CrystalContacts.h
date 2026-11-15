#pragma once
#include "io/StructureReader.h"
#include <array>

namespace youngcraft {
struct CrystalContact {
    int first = -1, second = -1; // unit-cell atom indices
    std::array<int, 3> translation{};
    double distance = 0;
};

// Enumerate unique atom-pair/translation contacts, including covalent neighbors.
// Throws on invalid input or a search too large for an interactive operation.
QVector<CrystalContact> crystalContacts(const CrystalData &crystal, double cutoff);
}
