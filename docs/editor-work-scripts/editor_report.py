from pathlib import Path
import zipfile,hashlib,json
root=Path('.')
changed=[]
with zipfile.ZipFile(root/'docs/drawing-before.zip') as z:
    baseline={n.replace('\\','/'):z.read(n) for n in z.namelist() if not n.endswith('/')}
    for f in sorted((root/'src').rglob('*')):
        if f.is_file():
            name=f.as_posix()
            if name not in baseline or f.read_bytes()!=baseline[name]:changed.append(name)
    for name in ('CMakeLists.txt','tests/smoke/test_smoke.cpp','tools/package_portable.ps1'):
        if (root/name).read_bytes()!=baseline.get(name):changed.append(name)
changed.extend(['tests/editor/test_editor.cpp','resources/fragments.qrc','licenses/Avogadro-fragments-BSD-3-Clause.txt','tools/build_editor_release.ps1'])
(root/'docs/editor-modified-files.txt').write_text('\n'.join(sorted(set(changed)))+'\nresources/fragments/: 166 groups + 76 ligands, SOURCE.txt\n',encoding='utf-8')
pkg=json.loads((root/'docs/editor-package-verification.json').read_text(encoding='utf-8'))
report='''# Youngcraft Molecular Editor 완료 보고 — 2026-09-30

작업 경로: `D:\\이영섭\\프로젝트 개발\\개발프로젝트\\Youngcraft`

1. **실제 기존 구조:** `MoleculeView_Interaction.cpp`가 hit-test와 mouse 입력, `MoleculeView_Overlays.cpp`가 ghost 표시를 담당했다. `canvasClicked / bondDrawn / atomMoved / atomPicked` → `MainWindow_Ui.cpp` 연결 → `MainWindow_MM.cpp`의 drawing commit → model/view/analysis 갱신 흐름이었다. 기존 전체 MainWindow 및 렌더러 구조는 유지했다.
2. **기존 기능:** 118개 원소 selector, quick element 버튼, explicit bond order/방향족, AutoHydrogenButton, ghost overlay, 기존 atom으로 bond drag, Ctrl+B, Ctrl+D, Alt 길이 snap 해제, GeometryMeasureBar/GeometryEditor, 단순 molecule undo 목록이 있었다. 기존 클릭은 picking이었으며 atom 치환과 우클릭 삭제는 없었다. undo는 새 문서의 topology 편집에만 제한됐고 coordinate 편집/계산 frame 보존이 부족했다.
3. **추가 기능:** 클릭 치환·결합 순환/explicit 변경·우클릭 삭제·거리/order preview, Automatic mode, formal charge -4..+4, 최근 원소 8개/주기율표, 원소 typing buffer, Shift/Ctrl/rectangle/connected-fragment selection, 그룹 이동/회전, Build 메뉴 및 아래 template/fragment/centric 기능.
4. **수정 파일:** `editor-modified-files.txt`에 전체 목록. 새 주요 파일은 `MoleculeEditController.{h,cpp}`, `FragmentLibrary.{h,cpp}`, `MainWindow_Editor.cpp`, `tests/editor/test_editor.cpp`, `resources/fragments.qrc`, `tools/build_editor_release.ps1`이다. 기존 drawing 관련 파일과 package/CMake 연결만 확장했으며 parser·MM·xTB·frequency·TD-DFT·PES·IRC 알고리즘은 수정하지 않았다.
5. **editor/model 구조:** `DrawState`는 원소/order/전하/Auto H를 전달하고, `SelectionModel`은 atom/bond 집합을 관리한다. controller는 mutation·valence·snap·geometry 검증을 담당한다. `Atom::formalCharge`는 기존 partial charge와 분리됐다. coordination site와 haptic contact는 chemical atom이 아닌 metadata다. 파일/trajectory의 편집은 현재 표시 구조를 새 편집 탭으로 복사해 원본 frame을 보존한다.
6. **Auto H:** commit 때만 처리한다. C/N/O/P/S/halogen의 formal charge별 target을 적용하고 coordination bond를 일반 valence에서 제외한다. 금속/lanthanide/actinide에 자동 H를 생성하지 않는다. 기존 수동 H는 자동 처리에서 유지하며 명시적 Adjust/Add/Remove Extra/Remove All 메뉴도 제공한다. 완성된 library ligand의 explicit H topology를 잠가 CO/CN 등에 잘못된 H가 추가되지 않는다. tetrahedral/planar/linear 방향을 이용한다.
7. **Undo/Redo:** 기존 문서별 snapshot stack을 동등한 transaction stack으로 확장했다. 모든 drawing·coordinate·geometry·template·fragment·group edit를 기록한다. mouseMove에는 model mutation/전체 molecule copy/H recount가 없고 release에 한 번 commit한다. 무변경 편집은 history에 추가하지 않는다. Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z 지원.
8. **Template/Ligand/Fragment:** 11개 배위 geometry. site는 원자가 아니며 donor와 연결되면 occupied 처리하고 donor 삭제 시 해제한다. 166 functional groups + 76 ligands를 category/search/작은 MoleculeView preview로 탐색한다. H/dummy 치환, orientation, 30° 회전 후보의 clash 평가, 여러 donor 연결, haptic metadata/centroid contact 표시를 제공한다. dummy는 삽입 후 남지 않는다. resources는 Qt resource로 실행파일에 내장되고 배포본에도 CJSON이 포함된다.
9. **Avogadro/라이선스:** 첨부 ZIP은 접근 가능한 경로에서 찾지 못했다. 설치된 Avogadro2 2.0.0의 동일한 fragments 자료에서 CJSON만 복사했다. BSD 3-Clause 원문을 `licenses/Avogadro-fragments-BSD-3-Clause.txt`와 portable ZIP에 포함했다. Avogadro exe/DLL/lib는 복사하거나 link하지 않았다. upstream: https://github.com/OpenChemistry/fragments .
10. **Bond-Centric:** bond 선택 후 endpoint drag=angle, right drag=distance, Ctrl drag=torsion, Shift=15° snap, Alt=반대쪽. graph 분리로 작은 fragment를 기본 선택한다. 기존 GeometryEditor를 재사용하며 mouseMove는 최대 4개 probe 원자로 preview transform을 계산하고 실제 molecule에는 적용하지 않는다. ring은 안전하게 거부한다. 기존 숫자 입력 bar도 유지하며 geometry commit 역시 undo 가능하다.
11. **테스트:** CTest 6/6 통과(structure, Gaussian parser, model, GUI, xTB runner, molecular editor). editor test 10 passed, 0 failed. Windows GUI 28 passed, 0 failed; 환경 의존 realScan 항목은 별도 실제 Gaussian 파일로 실행해 11개 point 표시/선택까지 통과했다. offscreen에서 OpenGL test는 skip되므로 Windows 플랫폼에서 별도 확인했다. methane/ethane/ethylene/acetylene/charged N/O/금속, 중복 결합/삭제/치환, template 방향, 242개 resource 삽입·dummy 제거, bpy topology, haptic metadata, snap/Alt, group rigid motion, ring 차단, atom/bond mouse 입력, undo/redo, trajectory 보존을 확인했다. transparent overlay가 분자를 가리지 않는 pixel regression도 추가했다.
12. **Release:** Ninja + MSVC + Qt6 Release 완료. 기존 `build`는 다른 PC의 E: compiler 경로를 참조해 `build-editor-final`을 사용했다. 한국어 MSVC show-includes 출력의 인코딩을 UTF-8로 맞춰 header dependency 추적을 정상화했고 긴 Qt SDK 경로는 build-only junction으로 단축했다. 재현: `tools/build_editor_release.ps1`, `ctest --test-dir build-editor-final --output-on-failure`.
13. **Portable:** `dist/Youngcraft.exe`, `Youngcraft-portable.zip` 생성. ZIP의 242개 CJSON/라이선스/MM/xTB runtime과 Release exe의 SHA-256 일치를 검사했다. Avogadro binary 없음. 별도 `.build/portable-editor-check` 경로에서 배포 Qt DLL과 test-only QtTest DLL을 이용해 model 10 passed / GUI 5 passed, 0 failed로 relocation을 검증했다. 실제 portable exe도 시작 후 responding 상태를 확인했다. ZIP 크기: SIZE bytes. 실행파일 SHA-256: HASH.
14. **미구현:** P2 coordinate text editor, structure clipboard, SMILES/InChI integration은 추가하지 않았다. haptic은 native model metadata와 centroid contact 표시로 지원하며 별도의 화학 파일 serialization 확장은 하지 않았다. fragment preview는 기본 방향을 보여주고 최종 clash 회전은 commit 때 선택한다. peptide/DNA/polymer/protein/layer 기능은 이번 작업에 포함하지 않았다.

검증 로그: `editor-ctest-final.log`, `editor-gui-windows.txt`, `editor-real-scan-test.txt`, `editor-portable-model.txt`, `editor-portable-gui.txt`, `editor-package-verification.json`.
시각 검토: `editor-windows-preview.png`, `editor-windows-preview.png.scene.png`.
수정 전 백업: `drawing-before.zip`.
'''.replace('SIZE',str(pkg['zip_size'])).replace('HASH',pkg['exe_sha256'])
(root/'docs/molecular-editor-report.md').write_text(report,encoding='utf-8')
print((root/'docs/editor-modified-files.txt').read_text(encoding='utf-8'))
