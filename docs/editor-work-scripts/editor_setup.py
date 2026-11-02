from pathlib import Path
r=Path('.')
p=r/'src/model/Molecule.cpp'
s=p.read_text(encoding='utf-8')
s=s.replace('atomicNumber == 55 || atomicNumber == 56 ||','atomicNumber == 55 || atomicNumber == 56 || (atomicNumber >= 87 && atomicNumber <= 112) ||')
s=s.replace('    Atom', '    Atom')
s=s.replace('    if (first == second || first < 0', '    if (!std::isfinite(order) || order < 1.0 || order > 3.0) return;\n    if (first == second || first < 0')
s=s.replace('    atoms.removeAt(index);','''    for (int j = coordinationSites.size()-1; j>=0; --j) {
        if(coordinationSites[j].center==index) coordinationSites.removeAt(j);
        else if(coordinationSites[j].center>index) --coordinationSites[j].center;
    }
    for(int j=hapticContacts.size()-1;j>=0;--j){
        auto &h=hapticContacts[j];
        if(h.metal==index||h.atoms.contains(index)){hapticContacts.removeAt(j);continue;}
        if(h.metal>index)--h.metal;for(int &i:h.atoms)if(i>index)--i;
    }
    atoms.removeAt(index);''')
s=s.replace('void Molecule::reconcileAutoHydrogens() {','''namespace {
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
void Molecule::reconcileAutoHydrogens(bool addOnly, bool removeOnly) {''')
a=s.index('    // Keep every hydrogen')
b=s.index('    const int heavyCount',a)
s=s[:a]+'''    if (removeOnly) {
        QSet<int> remove;
        for(int i=0;i<atoms.size();++i){
            if(atoms[i].atomicNumber==1)continue;
            double used=0;QVector<int> hs;
            for(auto bond:bonds)if(bond.first==i||bond.second==i){
                int n=bond.first==i?bond.second:bond.first;
                if(bond.coordination||isMetal(atoms[n].atomicNumber))continue;
                used+=bond.order;if(atoms[n].atomicNumber==1&&atoms[n].autoHydrogen)hs<<n;
            }
            int excess=qMax(0,int(std::ceil(used-hydrogenValence(atoms[i],used))));
            for(int n:hs){if(excess--<=0)break;remove.insert(n);}
        }
        for(int i=atoms.size()-1;i>=0;--i)if(remove.contains(i))removeAtom(i);
        return;
    }
    if (!addOnly) for (int i = atoms.size() - 1; i >= 0; --i)
        if (atoms[i].atomicNumber == 1 && atoms[i].autoHydrogen) removeAtom(i);

'''+s[b:]
a=s.index('        if (isMetal(heavy.atomicNumber)')
b=s.index('        double used =',a)
s=s[:a]+'''        if (isMetal(heavy.atomicNumber) || heavy.atomicNumber==1) continue;
'''+s[b:]
s=s.replace('            if (bond.coordination) { used = target + 1.0; break; }','')
s=s.replace('            used += bond.order;', '            if (!bond.coordination && !isMetal(atoms[neighbor].atomicNumber)) used += bond.order;')
s=s.replace('        const double missing = target - used;', '        const int target = hydrogenValence(heavy, used);\n        const double missing = target - used;')
s=s.replace('#include <QHash>','#include <QHash>\n#include <QSet>')
p.write_text(s,encoding='utf-8')
# Copy only licensed resource data (no binaries).
import shutil
src=Path('C:/Program Files/Avogadro2/share/avogadro2/fragments')
for category in ('groups','ligands'):
    for f in (src/category).rglob('*.cjson'):
        dst=r/'resources/fragments'/f.relative_to(src);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,dst)
(r/'licenses').mkdir(exist_ok=True)
shutil.copy2(src/'LICENSE',r/'licenses/Avogadro-fragments-BSD-3-Clause.txt')
print('Resources:',len(list((r/'resources/fragments').rglob('*.cjson'))))
