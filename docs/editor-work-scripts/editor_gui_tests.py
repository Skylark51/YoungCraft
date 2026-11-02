from pathlib import Path
p=Path('tools/editor_release_build.cmd');s=p.read_text(encoding='utf-8').replace('build-editor-release','build-editor-ninja');p.write_text(s,encoding='utf-8')
p=Path('tests/smoke/test_smoke.cpp');s=p.read_text(encoding='utf-8').replace('    void styleEditorPreviewsImmediately();','    void styleEditorPreviewsImmediately();\n    void editorInputTransactionsAndSelection();\n    void editorPreservesCalculationFrames();\n    void editorFragmentUndoAndKeyboard();')
s=s.replace('    QVERIFY((view->molecule().atoms[0].position - beforeDrag).length() > 0.1f);','    QCOMPARE(view->molecule().atoms[0].position, beforeDrag);\n    QVERIFY(view->m_groupTranslation.length() > 0.1f);')
s=s.replace('    QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, moved);','    QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, moved);\n    QVERIFY((view->molecule().atoms[0].position-beforeDrag).length()>0.1f);',1)
idx=s.index('QTEST_MAIN')
s=s[:idx]+'''void SmokeTest::editorInputTransactionsAndSelection() {
    MainWindow w;w.show();w.newDrawingDocument();QTest::qWait(30);auto *v=w.m_view;
    QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,v->rect().center());QCOMPARE(v->molecule().atoms.size(),1);
    auto screen=[&](int i){auto p=v->project(v->molecule().atoms[i].position,v->viewProjection());return QPoint(qRound(p.x()),qRound(p.y()));};
    auto move=[&](const QPoint &p,Qt::MouseButton button,Qt::KeyboardModifiers mods){QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(v->mapToGlobal(p)),Qt::NoButton,button,mods);QApplication::sendEvent(v,&e);};
    w.m_drawElement->setCurrentText("N");w.m_drawBar->findChild<QSpinBox *>("DrawFormalCharge")->setValue(1);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,screen(0));QCOMPARE(v->molecule().atoms.size(),1);QCOMPARE(v->molecule().atoms[0].atomicNumber,7);QCOMPARE(v->molecule().atoms[0].formalCharge,1);
    w.undoDrawingEdit();QCOMPARE(v->molecule().atoms[0].atomicNumber,6);w.redoDrawingEdit();QCOMPARE(v->molecule().atoms[0].atomicNumber,7);
    auto before=v->molecule();QPoint start=screen(0),end=start+QPoint(130,0);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,start);move(end,Qt::LeftButton,Qt::AltModifier);QCOMPARE(v->molecule().atoms.size(),1);QVERIFY(v->hasGhostBond());QTest::mouseRelease(v,Qt::LeftButton,Qt::AltModifier,end);QCOMPARE(v->molecule().atoms.size(),2);QCOMPARE(v->molecule().bonds.size(),1);
    w.undoDrawingEdit();QCOMPARE(v->molecule().atoms.size(),1);w.redoDrawingEdit();QCOMPARE(v->molecule().atoms.size(),2);
    // Existing target is used, and Automatic preserves explicit order without a new undo step.
    int steps=w.m_undoMolecules.size();start=screen(0);end=screen(1);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,start);move(end,Qt::LeftButton,Qt::NoModifier);QTest::mouseRelease(v,Qt::LeftButton,Qt::NoModifier,end);QCOMPARE(v->molecule().atoms.size(),2);QCOMPARE(v->molecule().bonds.size(),1);QCOMPARE(w.m_undoMolecules.size(),steps);
    QPoint midpoint=(screen(0)+screen(1))/2;w.m_drawBondOrder->setCurrentIndex(w.m_drawBondOrder->findData(0.0));QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().bonds[0].order,2.0);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().bonds[0].order,3.0);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().bonds[0].order,1.0);
    QTest::mouseClick(v,Qt::RightButton,Qt::NoModifier,midpoint);QCOMPARE(v->molecule().atoms.size(),2);QVERIFY(v->molecule().bonds.isEmpty());w.undoDrawingEdit();
    w.setDrawingMode(false);QTest::mouseClick(v,Qt::LeftButton,Qt::NoModifier,screen(0));QTest::mouseClick(v,Qt::LeftButton,Qt::ShiftModifier,screen(1));QCOMPARE(v->selection().atoms.size(),2);QTest::mouseClick(v,Qt::LeftButton,Qt::ControlModifier,screen(0));QCOMPARE(v->selection().atoms.size(),1);QTest::mouseDClick(v,Qt::LeftButton,Qt::NoModifier,screen(0));QCOMPARE(v->selection().atoms.size(),2);
    QPoint min=screen(0)-QPoint(40,40),max=screen(1)+QPoint(40,40);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,min);move(max,Qt::LeftButton,Qt::NoModifier);QTest::mouseRelease(v,Qt::LeftButton,Qt::NoModifier,max);QCOMPARE(v->selection().atoms.size(),2);
    auto positions=v->molecule();double length=GeometryEditor::distance(positions,0,1);w.setAtomDragMode(true);steps=w.m_undoMolecules.size();start=screen(0);end=start+QPoint(0,50);QTest::mousePress(v,Qt::LeftButton,Qt::NoModifier,start);for(int i=1;i<=10;++i)move(start+QPoint(0,i*5),Qt::LeftButton,Qt::NoModifier);QCOMPARE(v->molecule().atoms[0].position,positions.atoms[0].position);QTest::mouseRelease(v,Qt::LeftButton,Qt::NoModifier,end);QCOMPARE(w.m_undoMolecules.size(),steps+1);QVERIFY(qAbs(GeometryEditor::distance(v->molecule(),0,1)-length)<1e-5);w.undoDrawingEdit();QCOMPARE(v->molecule().atoms[0].position,positions.atoms[0].position);
    w.setDrawingMode(true);QTest::mouseClick(v,Qt::RightButton,Qt::NoModifier,screen(0));QCOMPARE(v->molecule().atoms.size(),1);QVERIFY(v->molecule().bonds.isEmpty());
}
void SmokeTest::editorPreservesCalculationFrames() {
    MainWindow w;w.show();w.openFile("tests/fixtures/gaussian_sample.log");
    QTRY_VERIFY_WITH_TIMEOUT(!w.m_data.molecule.isEmpty(),5000);
    auto original=w.m_activeDocument;auto frames=w.m_data.frames;auto originalMolecule=w.m_data.molecule;auto active=w.m_view->molecule();int count=w.m_documentTabs->count();
    w.setDrawingMode(true);QCOMPARE(w.m_documentTabs->count(),count+1);QVERIFY(w.m_activeDocument!=original);QVERIFY(w.m_data.frames.isEmpty());QCOMPARE(w.m_data.molecule.atoms[0].position,active.atoms[0].position);
    w.addDrawnAtom({10,0,0});w.undoDrawingEdit();QCOMPARE(w.m_data.molecule.atoms.size(),active.atoms.size());QCOMPARE(original->data.frames.size(),frames.size());for(int i=0;i<frames.size();++i)QCOMPARE(original->data.frames[i].coordinates,frames[i].coordinates);QCOMPARE(original->data.molecule.atoms.size(),originalMolecule.atoms.size());
}
void SmokeTest::editorFragmentUndoAndKeyboard(){
    MainWindow w;w.show();w.newDrawingDocument();QTest::keyClick(&w,Qt::Key_F);QTest::keyClick(&w,Qt::Key_E);QCOMPARE(w.m_drawElement->currentText(),QString("Fe"));QTest::keyClick(&w,Qt::Key_2);QCOMPARE(w.m_drawBondOrder->currentData().toDouble(),2.0);QTest::keyClick(&w,Qt::Key_A);QCOMPARE(w.m_drawBondOrder->currentData().toDouble(),0.0);
    auto *v=w.m_view;v->setProperty("pendingCenter",27);v->setProperty("pendingCharge",2);v->setProperty("pendingGeometry",7);QVERIFY(w.consumeInsertion(-1,{}));QCOMPARE(v->molecule().atoms.size(),1);QCOMPARE(v->molecule().coordinationSites.size(),6);w.undoDrawingEdit();QVERIFY(v->molecule().isEmpty());w.redoDrawingEdit();QCOMPARE(v->molecule().coordinationSites.size(),6);
    v->setProperty("pendingFragment",":/fragments/ligands/bidentate/bpy-bipyridine.cjson");w.m_autoHButton->setChecked(true);int count=v->molecule().atoms.size();QVERIFY(w.consumeInsertion(0,{}, {1,0,0}));int coordination=0;for(auto b:v->molecule().bonds)if(b.coordination)++coordination;QCOMPARE(coordination,2);QVERIFY(MoleculeEditController::validate(v->molecule()));w.undoDrawingEdit();QCOMPARE(v->molecule().atoms.size(),count);w.redoDrawingEdit();QVERIFY(v->molecule().atoms.size()>count);
}

'''+s[idx:]
# Resolve the existing fixture path from this repository.
s=s.replace('tests/fixtures/gaussian_sample.log','tests/fixtures/sample.log')
p.write_text(s,encoding='utf-8')
