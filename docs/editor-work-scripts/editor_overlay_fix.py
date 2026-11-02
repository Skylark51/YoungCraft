from pathlib import Path
p=Path('src/render/moleculeview/MoleculeView_Overlays.cpp');s=p.read_text(encoding='utf-8');s=s.replace('''        {
            QPainter clear(m_previewOverlay);
            clear.setCompositionMode(QPainter::CompositionMode_Source);
            clear.fillRect(m_previewOverlay->rect(), Qt::transparent);
        }
''','');p.write_text(s,encoding='utf-8')
p=Path('tests/smoke/test_smoke.cpp');s=p.read_text(encoding='utf-8').replace('if(!capture.isEmpty())QVERIFY(w.grab().save(capture));','if(!capture.isEmpty()){QVERIFY(w.grab().save(capture));QVERIFY(v->image().save(capture+".scene.png"));}');p.write_text(s,encoding='utf-8')
