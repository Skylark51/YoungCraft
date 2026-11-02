from pathlib import Path
p=Path('src/model/MoleculeEditController.cpp');s=p.read_text(encoding='utf-8').replace('a.colorOverride={};','a.colorOverride=QColor();').replace('m.atoms<<a;return','m.charge+=s.formalCharge;m.atoms<<a;return').replace('auto &a=m.atoms[i];a.atomicNumber=z;', 'auto &a=m.atoms[i];m.charge+=charge-a.formalCharge;a.atomicNumber=z;');p.write_text(s,encoding='utf-8')
p=Path('src/model/Molecule.cpp');s=p.read_text(encoding='utf-8').replace('#include <QSet>','#include <QSet>\n#include <QQuaternion>');s=s.replace('    atoms.removeAt(index);','    charge-=atoms[index].formalCharge;\n    atoms.removeAt(index);')
s=s.replace('        double used = 0.0;','        double used = 0.0, maxOrder=1.0;')
s=s.replace('if (!bond.coordination && !isMetal(atoms[neighbor].atomicNumber)) used += bond.order;', 'if (!bond.coordination && !isMetal(atoms[neighbor].atomicNumber)) { used += bond.order; maxOrder=qMax(maxOrder,bond.order); }')
a=s.index('        for (int h = 0; h < count; ++h)')
b=s.index('            Atom hydrogen;',a)
s=s[:a]+'''        QVector<QVector3D> candidates;
        if(maxOrder>=2.5) candidates={{1,0,0},{-1,0,0}};
        else if(maxOrder>=1.5) candidates={{1,0,0},{-0.5f,0.8660254f,0},{-0.5f,-0.8660254f,0}};
        else if(target>4) candidates={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        else candidates={{1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1}};
        for(auto &v:candidates)v.normalize();
        if(!occupied.isEmpty()){auto align=QQuaternion::rotationTo(candidates.first(),occupied.first());for(auto &v:candidates)v=align.rotatedVector(v);}
        for (int h = 0; h < count; ++h) {
            QVector3D best(1, 0, 0);float bestScore=-1;
            for(auto candidate:candidates){float score=2.0f;for(const auto &other:occupied)score=qMin(score,1-QVector3D::dotProduct(candidate,other));if(score>bestScore){bestScore=score;best=candidate;}}
'''+s[b:]
p.write_text(s,encoding='utf-8')
p=Path('src/app/mainwindow/MainWindow_MM.cpp');s=p.read_text(encoding='utf-8').replace('    edited.atoms.push_back(atom);','    edited.charge+=atom.formalCharge;\n    edited.atoms.push_back(atom);',1).replace('a.colorOverride!=b.colorOverride','a.colorOverride!=b.colorOverride||a.radiusScale!=b.radiusScale||a.autoHydrogenLocked!=b.autoHydrogenLocked');s=s.replace('if(same)return;','if(same && committed.charge==m_data.molecule.charge)return;');p.write_text(s,encoding='utf-8')
p=Path('src/model/FragmentLibrary.cpp');s=p.read_text(encoding='utf-8').replace('edited.removeAtom(target);if(neighbor>target)--neighbor;target=neighbor;','edited.removeAtom(target);if(neighbor>target)--neighbor;target=neighbor;coordination=isMetal(edited.atoms[target].atomicNumber);');p.write_text(s,encoding='utf-8')
p=Path('src/app/mainwindow/MainWindow_Editor.cpp');s=p.read_text(encoding='utf-8');s=s.replace('MoleculeEditController::transform(edited,m_view->selection().atoms,t,{},{ });commitEditedMolecule(edited,false);','MoleculeEditController::transform(edited,m_view->selection().atoms,t,{},{ });for(int i:m_view->selection().atoms)if(edited.atoms[i].atomicNumber==1)edited.atoms[i].autoHydrogen=false;commitEditedMolecule(edited,false);');p.write_text(s,encoding='utf-8')
