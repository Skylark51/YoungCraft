from pathlib import Path
p=Path('src/render/moleculeview/MoleculeView_Interaction.cpp');s=p.read_text(encoding='utf-8').replace('''    if (m_ghostSnapAtom == m_drawStartAtom ||
        (m_ghostSnapAtom >= 0 && m_molecule.atoms[m_ghostSnapAtom].autoHydrogen))''','    if (m_ghostSnapAtom == m_drawStartAtom)')
s=s.replace('m_rectangle=false;m_centricDrag=false;','m_rectangle=false;m_centricDrag=false;m_centricRejected=false;')
s=s.replace('            m_centricDrag=std::isfinite(m_centricStart)&&!sa.contains(c);m_centricValue=m_centricStart;','''            m_centricDrag=std::isfinite(m_centricStart)&&!sa.contains(c);m_centricValue=m_centricStart;
            m_centricRejected=!m_centricDrag;
            if(m_centricRejected)m_centricText=sa.contains(c)?"Ring bond: fragment cannot be separated safely":"Angle / torsion needs adjacent atoms";
            m_centricAtoms=MoleculeEditController::side(m_molecule,c,a,c);m_centricProbe={};
            // Only four probe atoms are passed through the existing geometry functions.
            // The molecular graph and coordinates remain untouched throughout preview.
            for(int i:{oa,a,c,oc}){Atom atom;if(i>=0)atom=m_molecule.atoms[i];m_centricProbe.atoms<<atom;}
            m_centricProbe.setBond(0,1,1);m_centricProbe.setBond(1,2,1);m_centricProbe.setBond(2,3,1);
            m_centricOrigin=m_molecule.atoms[a].position;m_centricTranslation={};m_centricRotation={};''')
needle='    else if(m_dragButton==Qt::MiddleButton'
pos=s.index(needle)
s=s[:pos]+'''    if(m_centricDrag){
        Molecule probe=m_centricProbe;bool ok=false;
        if(m_centricKind==0)ok=GeometryEditor::setDistance(probe,1,2,m_centricValue,GeometryMoveMode::OneSide);
        else if(m_centricKind==1)ok=GeometryEditor::setAngle(probe,0,1,2,m_centricValue,GeometryMoveMode::OneSide);
        else ok=GeometryEditor::setDihedral(probe,0,1,2,3,m_centricValue,GeometryMoveMode::OneSide);
        if(ok){if(m_centricKind==0)m_centricTranslation=probe.atoms[2].position-m_centricProbe.atoms[2].position;
            else{QVector3D before=m_centricProbe.atoms[m_centricKind==1?2:3].position-m_centricOrigin,after=probe.atoms[m_centricKind==1?2:3].position-m_centricOrigin;
                if(m_centricKind==2){auto axis=(m_centricProbe.atoms[2].position-m_centricOrigin).normalized();before-=axis*QVector3D::dotProduct(before,axis);after-=axis*QVector3D::dotProduct(after,axis);}
                m_centricRotation=QQuaternion::rotationTo(before,after);
            }
        }
    }
''' + s[pos:]
s=s.replace('else if(event->buttons()&Qt::RightButton)rotateBy(delta);','else if((event->buttons()&Qt::RightButton)&&!m_centricRejected)rotateBy(delta);')
p.write_text(s,encoding='utf-8')
p=Path('src/render/moleculeview/MoleculeView_Overlays.cpp');s=p.read_text(encoding='utf-8');needle='    if(m_insertPreview){';pos=s.index(needle);s=s[:pos]+'''    if(m_centricDrag&&m_dragging){painter.setPen(QPen(m_style.selectionColor,2,Qt::DashLine));
        auto ghost=[&](int i){auto p=m_molecule.atoms[i].position;if(m_centricAtoms.contains(i))p=m_centricOrigin+m_centricRotation.rotatedVector(p-m_centricOrigin)+m_centricTranslation;return point(p);};
        for(auto b:m_molecule.bonds)if(m_centricAtoms.contains(b.first)||m_centricAtoms.contains(b.second))painter.drawLine(ghost(b.first),ghost(b.second));
        for(int i:m_centricAtoms)painter.drawEllipse(ghost(i),7,7);
    }
'''+s[pos:];s=s.replace('QColor(255,190,80,170)','(m_dragButton==Qt::RightButton&&m_drawingMode?QColor(240,70,70,220):QColor(255,190,80,170))').replace('QColor(255,190,80,150)','(m_dragButton==Qt::RightButton&&m_drawingMode?QColor(240,70,70,220):QColor(255,190,80,150))');p.write_text(s,encoding='utf-8')
