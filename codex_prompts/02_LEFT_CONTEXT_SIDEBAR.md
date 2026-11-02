# 02 — 하단 패널 제거 및 좌측 Context Sidebar 정리

## 작업 위치
`E:\03\_프로젝트\_개발\01\_진행중\_개발프로젝트\Youngcraft`

## 최우선 원칙
- `docs/MODULE_MAP.md`를 먼저 읽는다.
- 아래 파일부터 확인하고 저장소 전수조사하지 않는다.
- 기능을 삭제하거나 동작을 바꾸는 작업이 아니라 **현재 기능의 배치만 정리**한다.

## 수정 허용 범위
- `src/app/mainwindow/MainWindow_Ui.cpp`
- `src/app/MainWindow.h`
- `src/ui/AnalysisPanel.cpp`
- `src/ui/AnalysisPanel.h`
- 필요 시 `src/app/mainwindow/MainWindow_Navigation.cpp`

그 외 파일은 컴파일 의존성이 확인되지 않는 한 수정 금지.

## 목표
현재 `계산 결과 탐색기`와 `분석 패널`이 따로 존재하고 분석 패널이 하단 공간을 크게 차지한다. 이를 **화면 좌측에 고정된 단일 작업 영역(Context Sidebar)** 중심으로 정리한다. 중앙은 가능한 넓게 유지한다.

## 구현 요구
1. 하단 `AnalysisDock` 배치를 제거한다.
2. 분석 UI는 좌측 영역에 배치한다.
3. `계산 결과 탐색기`와 분석 기능을 좌측에서 자연스럽게 전환할 수 있게 한다. 권장 방식은 하나의 좌측 dock 내부에서 상단 탐색 + 하단 context controls 또는 compact tab/stack 구조이다.
4. 현재 존재하는 모든 signal/slot 연결은 유지한다.
5. Frequency, Scan, IRC, TD-DFT, Atom, Orbital, Coord 기능을 하나도 제거하지 않는다.
6. 선택한 분석 종류에 필요한 컨트롤만 보이도록 공간 낭비를 줄인다.
7. 좌측 sidebar 폭은 기본 약 300~360 px 수준으로 하고 사용자가 resize 가능해야 한다.
8. 중앙 3D viewport는 sidebar 때문에 불필요하게 축소되지 않게 한다.
9. window state 저장/복원 기능이 계속 정상 동작해야 한다.
10. UI 문구는 한글을 유지한다.

## UX 기준
- 사용자가 분석 기능을 찾기 위해 화면 아래를 볼 필요가 없어야 한다.
- 동일 데이터를 표시하는 중복 UI가 있으면 제거가 아니라 **같은 기존 위젯을 재배치/재사용**하는 방향을 우선한다.
- 기능 위치를 바꾼다는 이유로 계산 로직이나 렌더링 로직을 수정하지 않는다.

## 검증
- 파일 열기 → 구조 → Frequency → Scan → IRC → TD-DFT 탭 전환 확인.
- 선택 signal이 기존과 동일하게 3D/그래프에 반영되는지 확인.
- 앱 재시작 후 dock/sidebar 상태 복원이 깨지지 않는지 확인.
- 마지막 보고는 수정한 파일만 명시한다.
