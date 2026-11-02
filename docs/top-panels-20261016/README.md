# 원자 표시 상태 유지 및 상단 패널

2026-10-16 / MSVC 2022 / Qt 6.8.3 / Release

원자 숨기기에서 setMolecule()을 호출하면 bestView()가 회전·pan·zoom을 재설정하고 원본 좌표로 돌아갔다. 새 setAtomVisibility()는 표시 플래그만 변경한다. 현재 좌표, 재계산된 결합, 카메라와 경계, 측정/선택 및 패널 위젯을 다시 만들지 않는다. 선택 집합과 결과 목록의 단일 선택 모두 지원한다.

주 도구 모음 아래 고정된 AnalysisToolBar를 추가했다. 기존 좌측 도크를 상단 도크로 옮기고 결과 트리와 분석 내용을 가로로 배치했다. 분석 선택 탭은 도구 모음 버튼으로 옮겼다. 같은 버튼으로 패널을 접고 펼칠 수 있으며, 문서 전환과 프로그램 내부 패널 전환에 선택 표시를 동기화한다. 분석 내용은 스크롤 영역에서 표시하여 작은 창에서도 접근 가능하다. 기존 좌측 도크 저장값의 재적용을 피하기 위해 window/topPanelState에 새 배치를 저장한다.

## 검증

- 최종 Windows OpenGL GUI 전체 회귀: 39 passed, 0 failed, 1 skipped (`full-gui-tests.txt`). 기존 realScanGuiInputs는 별도 실제 파일 입력 경로가 없어 생략했다.
- 원자 숨기기 5가지 동작을 직교/원근 보기 각각에서 검증했다. 회전/이동/확대, 투영행렬, 현재 프레임 좌표, 목록 스크롤, 분석 선택이 그대로 유지됨을 확인했다. 다중 선택 및 결과 트리 단일 선택을 확인했다.
- 상단 툴바 줄, 도크 방향, 문서별 패널 선택, 접기/펼치기를 검증했다.
- CTest GUI 외 6개 묶음 모두 통과: structure_tools, structure_files, parser_gaussian, model_atoms_bonds, xtb_runner, molecular_editor.
- QtTest로 생성한 실제 화면 `top-panels.png`를 열어 새 툴바와 상단 가로 배치 및 스크롤 표시를 확인했다.
- 소스 변경 비교: `source.patch`.

## 실행 파일 및 복원

루트 Youngcraft.exe와 dist/Youngcraft.exe를 갱신하고 빌드 결과와 SHA-256 일치를 확인했다. 기존 portable ZIP은 갱신하지 않았다.

수정 전 src/README/루트 실행 파일과 dist 실행 파일은 `.build/before-top-panels-20261016/`에 보관한다. 빌드 폴더는 기존 `build-chemcraft-tools`를 사용한다.
