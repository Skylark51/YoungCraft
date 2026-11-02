# 03 — TD-DFT 선택 시 중앙 Workspace를 UV-Vis 그래프로 전환

## 작업 위치
`E:\03\_프로젝트\_개발\01\_진행중\_개발프로젝트\Youngcraft`

## 최우선 원칙
- `docs/MODULE_MAP.md`를 먼저 읽는다.
- TD-DFT parser는 이미 동작하므로 파싱 코드는 수정하지 않는다.
- 새로운 그래프 라이브러리를 추가하지 말고 기존 `PlotWidget`을 재사용한다.
- 저장소 전수조사 금지.

## 수정 허용 범위
- `src/app/mainwindow/MainWindow_Ui.cpp`
- `src/app/mainwindow/MainWindow_Navigation.cpp`
- `src/app/MainWindow.h`
- `src/ui/analysispanel/AnalysisPanel_Tddft.cpp`
- `src/ui/AnalysisPanel.h`
- `src/ui/PlotWidget.cpp`
- `src/ui/PlotWidget.h`

필요한 경우에만 `src/ui/AnalysisPanel.cpp` 추가 수정. 파서/렌더러는 수정 금지.

## 목표
TD-DFT를 확인할 때 중앙의 3D 분자 렌더링은 중요도가 낮다. **TD-DFT를 선택하면 중앙 3D viewport 대신 큰 UV-Vis spectrum workspace가 표시되도록 전환한다.** 다른 구조 기반 항목으로 이동하면 즉시 3D viewport로 돌아온다.

## 구현 요구
1. MainWindow 중앙 영역을 `QStackedWidget` 또는 동등한 가벼운 Qt 구조로 만든다.
2. Page A = 기존 `MoleculeView` 그대로.
3. Page B = 기존 TD-DFT spectrum용 `PlotWidget`을 크게 표시하는 전용 workspace.
4. TD-DFT 항목/탭을 선택하면 Page B로 전환한다.
5. Geometry/Frequency/Scan point/IRC point/Atom 등을 선택하면 Page A로 복귀한다.
6. TD-DFT workspace에는 최소한 다음을 함께 표시한다.
   - UV-Vis broadened spectrum
   - wavelength 축
   - oscillator strength/intensity 정보
   - excited-state table 또는 선택 state 정보
7. 기존 broadening 값 변경 시 중앙 그래프가 즉시 갱신되어야 한다.
8. 기존 AnalysisPanel 내부 TD-DFT 기능을 중복으로 새로 만들지 말고 데이터/위젯을 재사용하거나 최소한의 signal로 연결한다.
9. TD-DFT 데이터가 없는 파일에서는 빈 그래프 대신 `TD-DFT 데이터 없음`을 명확하게 표시한다.
10. 렌더링 상태/카메라는 workspace 전환 전후 그대로 유지한다.

## 금지
- Gaussian parser 수정 금지.
- MoleculeView 렌더 코드 수정 금지.
- TD-DFT 데이터를 다시 계산하거나 별도 데이터 모델로 복제하지 말 것.

## 검증
- TD-DFT 파일 열기 → TD-DFT 선택 → 중앙 그래프 표시.
- broadening 변경 → 그래프 즉시 업데이트.
- Frequency/Geometry 선택 → 동일 카메라 상태의 3D viewport 복귀.
- TD-DFT 없는 파일에서 crash 없음.
