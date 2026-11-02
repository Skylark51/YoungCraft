from pathlib import Path
p=Path('src/model/MoleculeEditController.cpp');s=p.read_text(encoding='utf-8').replace('for (int o=2;o<=3;++o) { double l=idealLength', '''for (int o=2;o<=3;++o) {
        if(o==3 && !((a==6 && (b==6||b==7||b==8)) || (b==6 && (a==7||a==8)) || (a==7&&b==7)))continue;
        double l=idealLength''');p.write_text(s,encoding='utf-8')
p=Path('tests/editor/test_editor.cpp');s=p.read_text(encoding='utf-8').replace('QCOMPARE(MoleculeEditController::predictOrder(26,6,1.2),1.0);','QCOMPARE(MoleculeEditController::predictOrder(26,6,1.2),1.0);\n    QCOMPARE(MoleculeEditController::predictOrder(8,8,1.10),1.0);QCOMPARE(MoleculeEditController::predictOrder(7,8,1.10),1.0);');p.write_text(s,encoding='utf-8')
