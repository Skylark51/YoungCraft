# Chemcraft 대응 기능 5개 업그레이드 — 2026-10-16

## 확인한 근거와 한계

Computer Use 스킬의 @oai/sky로 `C:\Chemcraft\Chemcraft.exe`를 실행했다. 파일 존재와 실행 프로세스를 확인했지만 도구의 `list_apps()`는 Chemcraft의 windows 배열을 비워 반환했고 `launch_app`은 `launched app did not expose a targetable window`를 반환했다. 따라서 이번 작업에서 Chemcraft의 실제 메뉴 클릭이나 기능 결과를 직접 확인했다고 주장하지 않는다.

대신 이 프로젝트에 보관된 Chemcraft 메뉴 캡처 `edit.png`, `tools.png`를 읽고 현재 `src/app/mainwindow/MainWindow_Menus.cpp`의 핸들러와 대조했다. Youngcraft는 Computer Use로 창을 열고 화면과 메뉴 트리를 확인했다. 파일 대화상자 요소는 스냅샷에 나타나지만 입력 시 cached state 오류가 있었으므로 GUI 동작의 최종 검증은 QtTest로 수행했다.

## 선정 기준과 개선

| 순위 | 기능 | 이전 상태 | 개선 |
|---|---|---|---|
| 1 | 선택 기능 | 확장·반전·연결 선택은 상태 문구만 표시 | 한 단계 확장, 반전, 연결 성분 전체 선택, 원소/수소 필터. 배위 및 haptic 연결 포함 |
| 2 | 조각 복사·붙여넣기 | 상태 문구만 표시 | 전용 클립보드 형식+XYZ 텍스트, 원자 속성/결합 차수/배위 정보 보존, 옆에 배치, undo/redo |
| 3 | 분자 조각 추출기 | 상태 문구만 표시 | 연결된 조각 목록 또는 선택 영역을 새 탭으로 추출 |
| 4 | Z-matrix | 이름과 달리 원소·번호·Cartesian 좌표 나열 | 앞선 원자 참조, 거리 Å·각/이면각 ° 생성, 선형/겹침 오류 보고, 복사/저장 |
| 5 | 계산 입력파일 생성 | route 문자열만 보여줌 | 현재 좌표·총전하·다중도·route를 포함한 Gaussian 입력 미리보기/복사/GJF 저장 |

1–3은 구조 편집의 기본 흐름과 조각 재사용에 필요하다. 4–5는 기존 메뉴명이 약속하는 출력과 실제 출력이 달라 잘못된 계산 입력으로 이어질 수 있어 선정했다. 이는 메뉴 캡처와 코드 분석을 바탕으로 한 개발 우선순위이며 Chemcraft 전체 기능의 실사용 비교 평가는 아니다.

## 구현 및 검증

- C++20 / MSVC 2022 / Qt 6.8.3 Release.
- 핵심 구현: `src/model/StructureTools.{h,cpp}`, `src/app/mainwindow/MainWindow_Chemcraft.cpp`.
- 계산 문서를 편집하면 기존 `commitEditedMolecule` 경로로 새 편집 탭을 만들며 원본 프레임은 보존한다.
- 부분 조각은 내부 결합만 유지한다. 전체가 포함되지 않은 haptic contact는 복사하지 않고, 빠진 배위 donor는 빈 자리로 처리한다. 부분 조각의 전하는 형식 전하 합, 다중도는 기본값 1이다. 최종 스핀 결합은 추정하지 않는다.
- Gaussian 입력은 Cartesian Å 기준의 단일 계산이다. 사용자 지정 Gen/GenECP, 체크포인트 의존, 다른 좌표 단위 등은 일부 검사하여 거부한다. 모든 Gaussian 키워드를 검증하는 파서는 아니며 Gaussian 계산 자체는 실행하지 않았다.
- Z-matrix는 dummy를 자동 생성하지 않는다. 앞선 원자들이 선형이어서 이면각을 정의할 수 없으면 명시적 오류를 반환한다.
- 구조 테스트: 6 passed, 0 failed (`model-tests.txt`). 클립보드 왕복/배위 재매핑/손상 데이터 거부, 선택/연결 성분, Z-matrix 수치, 입력파일 전하/다중도 검사.
- Windows OpenGL 전체 GUI 회귀: 36 passed, 0 failed, 1 skipped (`full-gui-tests.txt`). 건너뛴 기존 `realScanGuiInputs`는 별도 YOUNGCRAFT_SCAN_INPUT_DIR 데이터가 필요하다.
- 마지막 haptic 연결 보완 후 핵심 구조 테스트와 신규 GUI 시나리오 재검증 (`final-gui-tests.txt`).
- CTest: structure_tools / structure_files / parser_gaussian / model_atoms_bonds / xtb_runner / molecular_editor 6개 묶음 모두 통과 (`ctest.txt`).

## 배포와 복원

루트 `Youngcraft.exe`와 `dist/Youngcraft.exe`를 갱신했다. Qt 런타임은 기존 파일과 동일한 SDK를 사용한다. 기존 portable ZIP은 갱신하지 않았다.

수정 전 실행 파일, dist 실행 파일, src/CMakeLists/README 스냅샷은 `.build/before-chemcraft-tools-20261016/`에 보관한다. 빌드 폴더는 `build-chemcraft-tools`, 검토용 실행 파일은 `.build/chemcraft-tools/app/Youngcraft.exe`다.
