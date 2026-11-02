from pathlib import Path
p=Path('src/app/mainwindow/MainWindow_Ui.cpp');s=p.read_text(encoding='utf-8');a=s.index('    connect(replaceAtom,');b=s.index('    auto *deleteAtom',a);s=s[:a]+'''    connect(replaceAtom, &QPushButton::clicked, this, [this] {
        auto ids=m_view->selection().atoms;if(ids.isEmpty()&&m_view->selectedAtom()>=0)ids.insert(m_view->selectedAtom());
        if(ids.isEmpty())return;auto edited=m_view->molecule();auto state=drawState();
        MoleculeEditController::replace(edited,ids,state.element,state.formalCharge);commitEditedMolecule(edited,true);
    });
'''+s[b:];a=s.index('    connect(deleteAtom,');b=s.index('    setupEditorControls()',a);s=s[:a]+'''    connect(deleteAtom, &QPushButton::clicked, this, &MainWindow::deleteSelection);
'''+s[b:]
s=s.replace('    m_drawBar->hide(); moleculeLayout->addWidget(m_drawBar);','''    // Keep editor controls readable at ordinary window widths.
    QVector<QLayoutItem *> controls;
    while(drawLayout->count())controls<<drawLayout->takeAt(0);
    delete drawLayout;
    auto *editorRows=new QVBoxLayout(m_drawBar);editorRows->setContentsMargins(8,3,8,3);editorRows->setSpacing(3);
    auto *row1=new QHBoxLayout,*row2=new QHBoxLayout,*row3=new QHBoxLayout;
    for(int i=0;i<controls.size();++i){auto *item=controls[i];if(!item->widget()){delete item;continue;}
        if(item->widget()->objectName()=="DrawStateSummary")row3->addItem(item);
        else if(i<20)row1->addItem(item);else row2->addItem(item);
    }
    row1->addStretch();row2->addStretch();row3->addStretch();editorRows->addLayout(row1);editorRows->addLayout(row2);editorRows->addLayout(row3);
    m_drawBar->hide(); moleculeLayout->addWidget(m_drawBar);''')
p.write_text(s,encoding='utf-8')
p=Path('src/app/mainwindow/MainWindow_MM.cpp');s=p.read_text(encoding='utf-8');s=s.replace('    m_undoMolecules.push_back(m_data.molecule);\n    m_redoMolecules.clear();','''    bool same=committed.atoms.size()==m_data.molecule.atoms.size()&&committed.bonds.size()==m_data.molecule.bonds.size()&&committed.coordinationSites.size()==m_data.molecule.coordinationSites.size()&&committed.hapticContacts.size()==m_data.molecule.hapticContacts.size();
    if(same) for(int i=0;i<committed.atoms.size();++i){const auto&a=committed.atoms[i];const auto&b=m_data.molecule.atoms[i];if(a.position!=b.position||a.atomicNumber!=b.atomicNumber||a.formalCharge!=b.formalCharge||a.autoHydrogen!=b.autoHydrogen||a.colorOverride!=b.colorOverride){same=false;break;}}
    if(same) for(int i=0;i<committed.bonds.size();++i){const auto&a=committed.bonds[i];const auto&b=m_data.molecule.bonds[i];if(a.first!=b.first||a.second!=b.second||a.order!=b.order||a.coordination!=b.coordination||a.style!=b.style||a.manual!=b.manual){same=false;break;}}
    if(same) for(int i=0;i<committed.coordinationSites.size();++i){const auto&a=committed.coordinationSites[i];const auto&b=m_data.molecule.coordinationSites[i];if(a.direction!=b.direction||a.center!=b.center||a.occupied!=b.occupied){same=false;break;}}
    if(same)return;
    m_undoMolecules.push_back(m_data.molecule);
    m_redoMolecules.clear();''',1)
s=s.replace('    m_view->setEditedMolecule(m_data.molecule);','    m_view->selection().clear();\n    m_view->setSelectedAtom(-1);\n    m_view->setEditedMolecule(m_data.molecule);')
p.write_text(s,encoding='utf-8')
p=Path('src/model/FragmentLibrary.cpp');s=p.read_text(encoding='utf-8').replace('ring(3,0.75,0);ring(3,-0.75,0)','ring(3,0.75f,0);ring(3,-0.75f,0)').replace('ring(4,0.6,0);ring(4,-0.6,45)','ring(4,0.6f,0);ring(4,-0.6f,45)');p.write_text(s,encoding='utf-8')
