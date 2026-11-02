from pathlib import Path
import zipfile,hashlib,json
root=Path('.')
with zipfile.ZipFile(root/'Youngcraft-portable.zip') as z:
    names=z.namelist();fragments=[n for n in names if n.replace('\\','/').startswith('resources/fragments/') and n.endswith('.cjson')]
    assert len(fragments)==242,len(fragments)
    assert 'licenses/Avogadro-fragments-BSD-3-Clause.txt' in names
    assert 'mm_runtime/YoungcraftMM.exe' in names and 'xtb_runtime/xtb.exe' in names
    assert not any(n.lower().endswith('.lib') or ('avogadro' in n.lower() and n.lower().endswith(('.dll','.exe'))) for n in names)
    exe=z.read('Youngcraft.exe');assert exe==(root/'Youngcraft.exe').read_bytes()==(root/'dist/Youngcraft.exe').read_bytes()
    assert z.read('licenses/Avogadro-fragments-BSD-3-Clause.txt')==(root/'licenses/Avogadro-fragments-BSD-3-Clause.txt').read_bytes()
    print('ZIP verified: 242 fragments, notice, MM/xTB runtime, exact Release executable; no Avogadro binary')
    summary={'fragments':len(fragments),'exe_sha256':hashlib.sha256(exe).hexdigest(),'zip_size':(root/'Youngcraft-portable.zip').stat().st_size,'zip_sha256':hashlib.sha256((root/'Youngcraft-portable.zip').read_bytes()).hexdigest()}
    (root/'docs/editor-package-verification.json').write_text(json.dumps(summary,indent=2),encoding='utf-8');print(json.dumps(summary))
