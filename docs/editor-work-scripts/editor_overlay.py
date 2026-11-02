from pathlib import Path
p=Path('src/render/moleculeview/MoleculeView_Overlays.cpp');s=p.read_text(encoding='utf-8');s=s.replace('        if (m_ghostActive) drawEditorPreview();','        drawEditorPreview();')
a=s.index('void MoleculeView::drawEditorPreview()')
b=s.index('void MoleculeView::drawLabels',a)
s=s[:a]+'''void MoleculeView::drawEditorPreview() {
    const auto vp=viewProjection();QPainter painter(m_previewOverlay);painter.setRenderHint(QPainter::Antialiasing,true);
    auto point=[&](const QVector3D&p){auto v=project(p,vp);return QPointF(v.x(),v.y());};
    painter.setBrush(Qt::NoBrush);painter.setPen(QPen(m_style.selectionColor,2));
    for(int i:m_selection.atoms)if(i>=0&&i<m_molecule.atoms.size())painter.drawEllipse(point(m_molecule.atoms[i].position),13,13);
    for(int i:m_selection.bonds)if(i>=0&&i<m_molecule.bonds.size()){auto b=m_molecule.bonds[i];painter.drawLine(point(m_molecule.atoms[b.first].position),point(m_molecule.atoms[b.second].position));}
    if(m_hoverAtom>=0&&m_hoverAtom<m_molecule.atoms.size()){painter.setPen(QPen(QColor(255,190,80,170),2));painter.drawEllipse(point(m_molecule.atoms[m_hoverAtom].position),16,16);}
    if(m_hoverBond>=0&&m_hoverBond<m_molecule.bonds.size()){auto b=m_molecule.bonds[m_hoverBond];painter.setPen(QPen(QColor(255,190,80,150),5));painter.drawLine(point(m_molecule.atoms[b.first].position),point(m_molecule.atoms[b.second].position));}
    if(m_rectangle){painter.setPen(QPen(m_style.selectionColor,1,Qt::DashLine));painter.setBrush(QColor(80,160,250,30));painter.drawRect(QRect(m_pressMouse,m_lastMouse).normalized());painter.setBrush(Qt::NoBrush);}
    if(m_groupDrag&&!m_groupTranslation.isNull()){painter.setPen(QPen(m_style.selectionColor,2,Qt::DashLine));for(auto b:m_molecule.bonds)if(m_selection.atoms.contains(b.first)&&m_selection.atoms.contains(b.second))painter.drawLine(point(m_molecule.atoms[b.first].position+m_groupTranslation),point(m_molecule.atoms[b.second].position+m_groupTranslation));for(int i:m_selection.atoms)painter.drawEllipse(point(m_molecule.atoms[i].position+m_groupTranslation),8,8);}
    painter.setPen(QPen(QColor(120,180,230,180),1,Qt::DashLine));
    for(const auto &site:m_molecule.coordinationSites)if(!site.occupied&&site.center>=0&&site.center<m_molecule.atoms.size()){auto origin=m_molecule.atoms[site.center].position;auto end=point(origin+site.direction*2);painter.drawLine(point(origin),end);painter.drawEllipse(end,6,6);}
    for(const auto &h:m_molecule.hapticContacts){if(h.metal<0||h.metal>=m_molecule.atoms.size())continue;QVector3D c;for(int i:h.atoms)c+=m_molecule.atoms[i].position;if(!h.atoms.isEmpty()){c/=h.atoms.size();painter.drawLine(point(m_molecule.atoms[h.metal].position),point(c));painter.drawText(point(c)+QPointF(8,0),h.name);}}
    if(m_bondCentric&&m_activeBond>=0&&m_activeBond<m_molecule.bonds.size()){auto b=m_molecule.bonds[m_activeBond];auto a=point(m_molecule.atoms[b.first].position),c=point(m_molecule.atoms[b.second].position);painter.setPen(QPen(m_style.selectionColor,3));painter.drawLine(a,c);painter.setPen(QPen(m_style.selectionColor,1,Qt::DashLine));painter.drawEllipse((a+c)/2,32,18);painter.drawText((a+c)/2+QPointF(12,-25),m_centricText);}
    if(m_insertPreview){painter.setPen(QPen(QColor(80,170,245,190),2,Qt::DashLine));
        if(m_templatePreview>=0){for(auto dir:FragmentLibrary::directions(m_templatePreview)){auto p=point(m_insertPosition+dir*2);painter.drawLine(point(m_insertPosition),p);painter.drawEllipse(p,5,5);}}
        else{auto q=QQuaternion::rotationTo(m_insertFragment.direction,m_insertDirection);auto pos=[&](int i){return point(m_insertPosition+q.rotatedVector(m_insertFragment.atoms[i].position-m_insertFragment.anchor));};for(auto b:m_insertFragment.bonds)if(m_insertFragment.atoms[b.first].atomicNumber>0&&m_insertFragment.atoms[b.second].atomicNumber>0)painter.drawLine(pos(b.first),pos(b.second));for(int i=0;i<m_insertFragment.atoms.size();++i)if(m_insertFragment.atoms[i].atomicNumber>0)painter.drawEllipse(pos(i),5,5);painter.drawText(point(m_insertPosition)+QPointF(12,-15),m_insertFragment.name);}
    }
    if(!m_ghostActive||m_drawStartAtom<0||m_drawStartAtom>=m_molecule.atoms.size())return;
    auto start=point(m_molecule.atoms[m_drawStartAtom].position),end=point(m_ghostPosition);
    painter.setPen(QPen(m_ghostSnapAtom>=0?m_style.selectionColor:elementColor(m_drawAtomicNumber),3,Qt::DashLine,Qt::RoundCap));painter.drawLine(start,end);
    painter.setBrush(QColor(elementColor(m_drawAtomicNumber).red(),elementColor(m_drawAtomicNumber).green(),elementColor(m_drawAtomicNumber).blue(),170));painter.drawEllipse(end,9,9);
    painter.setPen(m_style.labelColor);painter.drawText(end+QPointF(12,-8),QString("%1 · %2 · %3 Å").arg(m_ghostSnapAtom>=0?m_molecule.atoms[m_ghostSnapAtom].element:elementSymbol(m_drawAtomicNumber)).arg(m_previewOrder).arg((m_ghostPosition-m_molecule.atoms[m_drawStartAtom].position).length(),0,'f',2));
}

'''+s[b:]
p.write_text(s,encoding='utf-8')
# Redraw overlay whenever edited or camera changes (selection and template sites).
p=Path('src/render/moleculeview/MoleculeView_State.cpp');s=p.read_text(encoding='utf-8').replace('    update();','    update();\n    if(m_previewOverlay)m_previewOverlay->update();');p.write_text(s,encoding='utf-8')
# Package notice and data beside embedded resources for downstream reuse.
p=Path('tools/package_portable.ps1');s=p.read_text(encoding='utf-8');s=s.replace("    $licenses = Join-Path $stage 'licenses'", "    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'resources/fragments') -Destination (Join-Path $stage 'resources/fragments') -Recurse -Force\n    $licenses = Join-Path $stage 'licenses'")
s=s.replace("    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'third_party/gemmi/LICENSE.txt')", "    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'licenses/Avogadro-fragments-BSD-3-Clause.txt') -Destination $licenses -Force\n    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'third_party/gemmi/LICENSE.txt')")
s=s.replace("    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'resources/fragments')", "    New-Item -ItemType Directory -Path (Join-Path $stage 'resources') -Force | Out-Null\n    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'resources/fragments')")
p.write_text(s,encoding='utf-8')
