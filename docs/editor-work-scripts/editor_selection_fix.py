from pathlib import Path
p=Path('src/app/mainwindow/MainWindow_Editor.cpp');s=p.read_text(encoding='utf-8');s=s.replace('Molecule edited=m_view->molecule();auto s=drawState();MoleculeEditController::replace(edited,{i},s.element,s.formalCharge);commitEditedMolecule(edited,true);','''Molecule edited=m_view->molecule();auto s=drawState();MoleculeEditController::replace(edited,{i},s.element,s.formalCharge);
        int selected=i;if(s.autoHydrogen){selected=0;for(int j=0;j<i;++j)if(!edited.atoms[j].autoHydrogen)++selected;}
        commitEditedMolecule(edited,true);if(selected>=0&&selected<m_view->molecule().atoms.size()){m_view->selection().atoms={selected};m_view->setSelectedAtom(selected);}''');p.write_text(s,encoding='utf-8')
p=Path('src/render/moleculeview/MoleculeView_Interaction.cpp');s=p.read_text(encoding='utf-8').replace('emit atomEditRequested(atom);emit atomPicked(atom);','emit atomEditRequested(atom);if(m_selectedAtom>=0)emit atomPicked(m_selectedAtom);');p.write_text(s,encoding='utf-8')
# Make the repeatable local build entry point incremental after the clean final build.
p=Path('tools/editor_release_build.cmd');s=p.read_text(encoding='utf-8').replace('cmake --fresh -S .','cmake -S .').replace('--clean-first --parallel 6','--parallel 6');p.write_text(s,encoding='utf-8')
