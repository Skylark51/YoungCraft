from pathlib import Path
p=Path('src/model/MoleculeEditController.cpp');s=p.read_text(encoding='utf-8');s=s.replace('    // Mark singly bound explicit H as adjustable for an explicit user command.','    if(action==HydrogenAction::Add){m.reconcileAutoHydrogens(true,false);return;}\n    // Explicit adjustment can replace singly bound main-group hydrogen.');s=s.replace('int count=0;bool metal=false;','int count=0;bool metal=false,locked=false;').replace('++count;metal|=isMetal(m.atoms[b.first==i?b.second:b.first].atomicNumber);','++count;auto &neighbor=m.atoms[b.first==i?b.second:b.first];metal|=isMetal(neighbor.atomicNumber);locked|=neighbor.autoHydrogenLocked;').replace('if(count==1&&!metal) m.atoms[i].autoHydrogen=true;','if(count==1&&!metal&&(!locked||action==HydrogenAction::RemoveExtra)) m.atoms[i].autoHydrogen=true;');p.write_text(s,encoding='utf-8')
p=Path('src/app/mainwindow/MainWindow_Geometry.cpp');s=p.read_text(encoding='utf-8');a=s.index('    if(kind==0)ok=');b=s.index('    if(!ok)',a);s=s[:a]+'''    auto ids=m_geometrySelection;
    QSet<int> left,right;
    if(kind==0){left=MoleculeEditController::side(edited,ids[0],ids[0],ids[1]);right=MoleculeEditController::side(edited,ids[1],ids[0],ids[1]);}
    else if(kind==1){left=MoleculeEditController::side(edited,ids[0],ids[0],ids[1]);right=MoleculeEditController::side(edited,ids[2],ids[1],ids[2]);}
    else{left=MoleculeEditController::side(edited,ids[0],ids[1],ids[2]);right=MoleculeEditController::side(edited,ids[3],ids[1],ids[2]);}
    bool overlap=false;for(int i:left)if(right.contains(i)){overlap=true;break;}
    if(overlap){setStatus(QStringLiteral("고리 구조에서는 fragment를 안전하게 분리할 수 없습니다."));return;}
    if(mode==GeometryMoveMode::OneSide && left.size()<right.size()){
        if(kind==0)std::swap(ids[0],ids[1]);else if(kind==1)std::swap(ids[0],ids[2]);else{std::swap(ids[0],ids[3]);std::swap(ids[1],ids[2]);}
    }
    if(kind==0)ok=GeometryEditor::setDistance(edited,ids[0],ids[1],value,mode,&warning);
    else if(kind==1)ok=GeometryEditor::setAngle(edited,ids[0],ids[1],ids[2],value,mode,&warning);
    else ok=GeometryEditor::setDihedral(edited,ids[0],ids[1],ids[2],ids[3],value,mode,&warning);
'''+s[b:];p.write_text(s,encoding='utf-8')
p=Path('src/render/moleculeview/MoleculeView_Camera.cpp');s=p.read_text(encoding='utf-8').replace('    update();','    update();\n    if(m_previewOverlay)m_previewOverlay->update();');p.write_text(s,encoding='utf-8')
p=Path('src/render/moleculeview/MoleculeView_Interaction.cpp');s=p.read_text(encoding='utf-8').replace('    update();\n}\n\n} // namespace', '    update();\n    if(m_previewOverlay)m_previewOverlay->update();\n}\n\n} // namespace');p.write_text(s,encoding='utf-8')
