#pragma once
#include "model/Molecule.h"

namespace youngcraft {
// A sidebar stays open while the user works in the viewport. Reject stale inputs
// before launching a calculation, including charge and edited bond-order changes.
inline bool sameOptimizationInput(const Molecule &a,const Molecule &b) {
    if(a.charge!=b.charge || a.multiplicity!=b.multiplicity || a.atoms.size()!=b.atoms.size() || a.bonds.size()!=b.bonds.size())return false;
    for(int i=0;i<a.atoms.size();++i){
        const auto &x=a.atoms[i];const auto &y=b.atoms[i];
        if(x.atomicNumber!=y.atomicNumber || x.position!=y.position || x.formalCharge!=y.formalCharge || x.optimizationFixed!=y.optimizationFixed)return false;
    }
    for(int i=0;i<a.bonds.size();++i){
        const auto &x=a.bonds[i];const auto &y=b.bonds[i];
        if(x.first!=y.first || x.second!=y.second || x.order!=y.order || x.coordination!=y.coordination)return false;
    }
    return true;
}
}
