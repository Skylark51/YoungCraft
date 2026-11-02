from pathlib import Path
p=Path('tests/smoke/test_smoke.cpp');s=p.read_text(encoding='utf-8');s=s.replace('QString capture=qEnvironmentVariable("YOUNGCRAFT_EDITOR_CAPTURE");if(!capture.isEmpty())','''auto projected=v->project(v->molecule().atoms[0].position,v->viewProjection());
    QPoint probe=v->mapTo(&w,QPoint(qRound(projected.x())+20,qRound(projected.y())+15));
    QImage composed=w.grab().toImage();QColor pixel=composed.pixelColor(probe);
    QVERIFY(pixel.value()>0);QVERIFY(pixel!=v->style().background); // Transparent editor overlay must retain the molecule below it.
    QString capture=qEnvironmentVariable("YOUNGCRAFT_EDITOR_CAPTURE");if(!capture.isEmpty())''');p.write_text(s,encoding='utf-8')
Path('resources/fragments/SOURCE.txt').write_text('Avogadro2 2.0.0 installed resource data, copied 2026-09-30.\n166 functional groups and 76 ligands (CJSON only); no Avogadro binaries.\nCopyright (c) 2022, Open Chemistry. BSD 3-Clause.\nSee licenses/Avogadro-fragments-BSD-3-Clause.txt in the distribution.\nDummy sites are removed by the Youngcraft resource loader before insertion.\n',encoding='utf-8')
