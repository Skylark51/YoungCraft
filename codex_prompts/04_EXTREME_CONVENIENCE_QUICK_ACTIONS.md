# 04 — 계산화학 작업을 1-click으로 만드는 Quick Actions

## 작업 위치
`E:\03\_프로젝트\_개발\01\_진행중\_개발프로젝트\Youngcraft`

## 최우선 원칙
- `docs/MODULE_MAP.md`를 먼저 읽는다.
- 새로운 기능을 무작정 늘리지 말고 **기존 기능을 자동 연결하여 클릭 수를 줄이는 것**이 목적이다.
- 아래 파일만 우선 확인한다. 전수조사 금지.

## 수정 허용 범위
- `src/app/mainwindow/MainWindow_File.cpp`
- `src/app/mainwindow/MainWindow_Navigation.cpp`
- `src/app/mainwindow/MainWindow_Animation.cpp`
- `src/app/mainwindow/MainWindow_Bonds.cpp`
- `src/app/mainwindow/MainWindow_Toolbar.cpp`
- `src/app/mainwindow/MainWindow_Menus.cpp`
- `src/ui/analysispanel/AnalysisPanel_Frequency.cpp`
- `src/ui/AnalysisPanel.h`
- 필요 시 `src/app/MainWindow.h`

렌더러/파서의 계산 로직은 수정 금지.

## 목표
Youngcraft의 핵심 UX는 `극도의 편리성`이다. 계산화학자가 자주 하는 작업을 메뉴 탐색 없이 한 번의 선택 또는 클릭으로 실행되게 한다.

## 필수 구현 1 — Frequency 즉시 실행
Frequency 목록에서 특정 진동수를 클릭하면 별도 재생 버튼 없이 다음이 즉시 수행되어야 한다.
1. 해당 mode 선택.
2. 해당 mode의 vibration animation 즉시 시작.
3. `Recount Bonds`가 켜져 있으면 애니메이션 프레임마다 현재 기존 방식으로 자동 적용.
4. vibration vector 표시가 켜져 있으면 즉시 표시.
5. scaled vector 옵션도 현재 amplitude와 동기화.

## 필수 구현 2 — TS 확인 Quick Action
`TS 확인` 버튼을 하나 추가한다.
- imaginary frequency가 있으면 가장 대표적인 imaginary mode를 자동 선택한다. 기본은 가장 음수인 mode.
- 해당 frequency 화면으로 이동.
- animation 즉시 재생.
- 사용자가 설정한 recount/vector 옵션을 그대로 적용.
- imaginary frequency가 없으면 짧은 한글 status message만 표시.

## 필수 구현 3 — 자주 쓰는 Quick Actions
toolbar 또는 좌측 context area에 다음을 1-click으로 제공한다.
- `최종 구조`
- `TS 확인`
- `최적 보기`
- `PNG 저장`
- `이미지 복사`

기존 함수가 있으면 반드시 재사용하고 중복 구현하지 않는다.

## 필수 구현 4 — 파일을 열자마자 핵심 상태 표시
파일 로딩 완료 후 기존 데이터만 이용하여 상태바 또는 작은 summary 영역에 다음을 즉시 알려준다.
- optimization frame 존재 여부/개수
- frequency 존재 여부
- imaginary frequency 개수
- Scan 존재 여부
- IRC 존재 여부
- TD-DFT 존재 여부

새로운 계산은 하지 않는다.

## 메뉴 정리
현재 클릭해도 실제 기능 없이 안내 메시지만 나오는 placeholder 메뉴는 기본 UI에서 숨길 수 있다. 단, 실제 구현된 기능은 절대 숨기거나 제거하지 않는다. 나중에 재활성화하기 쉽도록 코드 삭제보다 visible/exposure 정리를 우선한다.

## 금지
- 동일 기능을 새 함수로 복제하지 말 것.
- 거대한 `MainWindow.cpp`로 코드를 다시 합치지 말 것.
- 새 외부 dependency 추가 금지.
- 기존 단축키/마우스 조작을 깨뜨리지 말 것.

## 검증
- Frequency 1회 클릭으로 실제 animation이 시작되는지 확인.
- imaginary mode가 있는 샘플에서 `TS 확인` 1회로 mode 선택+재생까지 되는지 확인.
- Quick Action이 기존 함수와 같은 결과를 내는지 확인.
- 정상 frequency가 없는 파일/TD-DFT 없는 파일에서도 crash 없어야 한다.
