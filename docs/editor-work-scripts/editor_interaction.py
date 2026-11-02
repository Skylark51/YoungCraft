from pathlib import Path
p=Path('src/render/moleculeview/MoleculeView_Interaction.cpp');s=p.read_text(encoding='utf-8')
a=s.index('            const float length =')
b=s.index('\n        }',a)
s=s[:a]+'''            m_ghostPosition = MoleculeEditController::snappedPosition(origin, m_ghostPosition,
                m_molecule.atoms[m_drawStartAtom].atomicNumber,m_drawAtomicNumber,m_drawOrder,false);'''+s[b:]
s=s.replace('    m_ghostActive = true;', '''    int z=m_ghostSnapAtom>=0?m_molecule.atoms[m_ghostSnapAtom].atomicNumber:m_drawAtomicNumber;
    m_previewOrder=m_drawOrder==0?MoleculeEditController::predictOrder(m_molecule.atoms[m_drawStartAtom].atomicNumber,z,(m_ghostPosition-origin).length()):m_drawOrder;
    m_ghostActive = true;''')
a=s.index('void MoleculeView::mousePressEvent')
b=s.index('void MoleculeView::wheelEvent',a)
s=s[:a]+'''int MoleculeView::pickBond(const QPoint &point) const {
    auto vp=viewProjection();int picked=-1;double best=9;
    for(int i=0;i<m_molecule.bonds.size();++i){auto b=m_molecule.bonds[i];if(!m_molecule.atoms[b.first].visible||!m_molecule.atoms[b.second].visible)continue;
        if(!m_style.showHydrogen&&(m_molecule.atoms[b.first].atomicNumber==1||m_molecule.atoms[b.second].atomicNumber==1))continue;
        auto a=project(m_molecule.atoms[b.first].position,vp),c=project(m_molecule.atoms[b.second].position,vp);QPointF u(a.x(),a.y()),v(c.x()-a.x(),c.y()-a.y()),p(point);double den=v.x()*v.x()+v.y()*v.y();if(den<1)continue;double t=((p-u).x()*v.x()+(p-u).y()*v.y())/den;if(t<0.1||t>0.9)continue;double d=QLineF(p,u+v*t).length();if(d<best){best=d;picked=i;}}
    return picked;
}
int MoleculeView::pickSite(const QPoint &point) const {
    auto vp=viewProjection();int picked=-1;double best=16;
    for(int i=0;i<m_molecule.coordinationSites.size();++i){auto s=m_molecule.coordinationSites[i];if(s.occupied||s.center<0||s.center>=m_molecule.atoms.size())continue;auto p=project(m_molecule.atoms[s.center].position+s.direction*2.0f,vp);double d=QLineF(point,QPointF(p.x(),p.y())).length();if(d<best){best=d;picked=i;}}
    return picked;
}
void MoleculeView::mousePressEvent(QMouseEvent *event) {
    setFocus();m_lastMouse=event->pos();m_pressMouse=event->pos();m_dragButton=event->button();m_dragging=false;
    int atom=pickAtom(event->pos());m_drawStartAtom=m_drawingMode&&event->button()==Qt::LeftButton&&!m_insertPreview?atom:-1;
    m_dragAtom=m_atomDragMode&&event->button()==Qt::LeftButton?atom:-1;
    m_groupDrag=false;m_groupTranslation={};m_rectangle=false;m_centricDrag=false;
    if(m_dragAtom>=0){m_dragStartPosition=m_molecule.atoms[m_dragAtom].position;if(!m_selection.atoms.contains(m_dragAtom))m_selection.click(m_dragAtom,false,false);m_groupDrag=true;}
    if(m_bondCentric&&m_activeBond>=0&&m_activeBond<m_molecule.bonds.size()&&atom>=0){
        auto b=m_molecule.bonds[m_activeBond];if(atom==b.first||atom==b.second){
            m_centricKind=event->button()==Qt::RightButton?0:event->modifiers()&Qt::ControlModifier?2:1;
            int a=b.first,c=b.second;auto sa=MoleculeEditController::side(m_molecule,a,a,c),sc=MoleculeEditController::side(m_molecule,c,a,c);
            bool moveA=sa.size()<sc.size();if(event->modifiers()&Qt::AltModifier)moveA=!moveA;if(moveA)std::swap(a,c);
            int oa=-1,oc=-1;for(auto bond:m_molecule.bonds){int n=bond.first==a?bond.second:bond.second==a?bond.first:-1;if(n>=0&&n!=c)oa=n;n=bond.first==c?bond.second:bond.second==c?bond.first:-1;if(n>=0&&n!=a)oc=n;}
            m_centricStart=m_centricKind==0?GeometryEditor::distance(m_molecule,a,c):m_centricKind==1&&oa>=0?GeometryEditor::angleDegrees(m_molecule,oa,a,c):m_centricKind==2&&oa>=0&&oc>=0?GeometryEditor::dihedralDegrees(m_molecule,oa,a,c,oc):kNaN;
            m_centricDrag=std::isfinite(m_centricStart)&&!sa.contains(c);m_centricValue=m_centricStart;
        }
    }
    m_ghostActive=false;if(m_previewOverlay)m_previewOverlay->update();
    if(event->button()==Qt::RightButton||event->button()==Qt::MiddleButton)emit interactionStarted();
}
void MoleculeView::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta=event->pos()-m_lastMouse;
    m_hoverAtom=pickAtom(event->pos());m_hoverBond=m_hoverAtom<0?pickBond(event->pos()):-1;
    if(m_insertPreview){int site=pickSite(event->pos());m_insertDirection={1,0,0};m_insertPosition=pointOnDrawingPlane(event->pos(),m_target);
        if(site>=0){auto s=m_molecule.coordinationSites[site];m_insertPosition=m_molecule.atoms[s.center].position;m_insertDirection=s.direction;}
        else if(m_hoverAtom>=0){m_insertPosition=m_molecule.atoms[m_hoverAtom].position;for(auto b:m_molecule.bonds)if(b.first==m_hoverAtom||b.second==m_hoverAtom){int n=b.first==m_hoverAtom?b.second:b.first;if(m_molecule.atoms[m_hoverAtom].atomicNumber==1){m_insertDirection=(m_insertPosition-m_molecule.atoms[n].position).normalized();m_insertPosition=m_molecule.atoms[n].position;}else if(m_molecule.atoms[n].atomicNumber==1)m_insertDirection=(m_molecule.atoms[n].position-m_insertPosition).normalized();}}
    }
    if(m_centricDrag){int pixels=(event->pos()-m_pressMouse).x();m_centricValue=m_centricStart+pixels*(m_centricKind==0?0.01:0.5);if(m_centricKind==0)m_centricValue=qMax(0.1,m_centricValue);if(m_centricKind==1)m_centricValue=qBound(0.0,m_centricValue,180.0);if(m_centricKind>0&&(event->modifiers()&Qt::ShiftModifier))m_centricValue=std::round(m_centricValue/15)*15;m_centricText=QString("%1: %2 %3").arg(m_centricKind==0?"Distance":m_centricKind==1?"Angle":"Dihedral").arg(m_centricValue,0,'f',m_centricKind==0?3:1).arg(m_centricKind==0?"Å":"°");}
    else if(m_dragButton==Qt::MiddleButton||((event->buttons()&Qt::RightButton)&&(event->modifiers()&Qt::ControlModifier)))panBy(delta);
    else if(event->buttons()&Qt::RightButton)rotateBy(delta);
    if((event->buttons()&Qt::LeftButton)&&(event->pos()-m_pressMouse).manhattanLength()>3&&!m_centricDrag){
        if(m_drawingMode&&m_drawStartAtom>=0&&!m_insertPreview&&!(event->modifiers()&(Qt::ControlModifier|Qt::ShiftModifier)))updateDrawingPreview(event->pos(),event->modifiers());
        else if(m_groupDrag)m_groupTranslation=pointOnDrawingPlane(event->pos(),m_dragStartPosition)-m_dragStartPosition;
        else if(pickAtom(m_pressMouse)<0&&!m_insertPreview)m_rectangle=true;
    }
    m_lastMouse=event->pos();m_dragging=(event->pos()-m_pressMouse).manhattanLength()>3;
    if(m_previewOverlay)m_previewOverlay->update();
}
void MoleculeView::mouseReleaseEvent(QMouseEvent *event) {
    int atom=pickAtom(event->pos()),bond=atom<0?pickBond(event->pos()):-1;
    bool add=event->modifiers()&Qt::ShiftModifier,toggle=event->modifiers()&Qt::ControlModifier;
    if(m_centricDrag&&m_dragging){auto b=m_molecule.bonds[m_activeBond];emit bondManipulated(b.first,b.second,m_centricKind,m_centricValue,event->modifiers()&Qt::AltModifier);}
    else if(m_rectangle){if(!add&&!toggle)m_selection.clear();QRect rect=QRect(m_pressMouse,event->pos()).normalized();auto vp=viewProjection();for(int i=0;i<m_molecule.atoms.size();++i){auto a=m_molecule.atoms[i];if(!a.visible||(!m_style.showHydrogen&&a.atomicNumber==1))continue;auto p=project(a.position,vp);if(rect.contains(QPoint(qRound(p.x()),qRound(p.y()))))m_selection.click(i,true,toggle);}}
    else if(m_groupDrag&&m_dragging){emit groupMoved(m_groupTranslation);}
    else if(m_drawingMode&&event->button()==Qt::RightButton&&!m_dragging){emit deleteRequested(atom,bond);}
    else if(m_drawingMode&&event->button()==Qt::LeftButton&&!add&&!toggle){
        int site=pickSite(event->pos());
        if(m_insertPreview&&site>=0){auto s=m_molecule.coordinationSites[site];emit attachmentClicked(s.center,s.direction);}
        else if(m_drawStartAtom>=0&&m_dragging){updateDrawingPreview(event->pos(),event->modifiers());emit bondDrawn(m_drawStartAtom,m_ghostSnapAtom,m_ghostPosition);}
        else if(!m_dragging){if(atom>=0){setSelectedAtom(atom);emit atomEditRequested(atom);emit atomPicked(atom);}else if(bond>=0&&!m_insertPreview)emit bondEditRequested(bond);else emit canvasClicked(pointOnDrawingPlane(event->pos(),m_target));}
    }else if(event->button()==Qt::LeftButton&&!m_dragging){
        if(m_bondCentric&&bond>=0){m_activeBond=bond;m_centricText="Drag endpoint: angle · right: length · Ctrl: torsion";}
        else{m_selection.click(atom,add,toggle);if(bond>=0){if(toggle&&m_selection.bonds.contains(bond))m_selection.bonds.remove(bond);else m_selection.bonds.insert(bond);}setSelectedAtom(atom);if(atom>=0)emit atomPicked(atom);}
    }
    m_groupTranslation={};m_dragButton=Qt::NoButton;m_ghostActive=false;m_drawStartAtom=-1;m_dragAtom=-1;m_groupDrag=false;m_rectangle=false;m_centricDrag=false;
    update();if(m_previewOverlay)m_previewOverlay->update();
    if(event->button()==Qt::RightButton||event->button()==Qt::MiddleButton)emit interactionFinished();
}
void MoleculeView::mouseDoubleClickEvent(QMouseEvent *event) {
    if(event->button()!=Qt::LeftButton)return;int atom=pickAtom(event->pos());
    if(atom>=0){m_selection.connected(m_molecule,atom);m_selectedAtom=atom;m_target=m_molecule.atoms[atom].position;update();if(m_previewOverlay)m_previewOverlay->update();emit atomPicked(atom);}
}

'''+s[b:]
p.write_text(s,encoding='utf-8')
