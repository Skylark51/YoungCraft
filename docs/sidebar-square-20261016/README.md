# 상단 버튼 정리, 좌측 사이드바와 정방형 구조 뷰

2026-10-16 / MSVC 2022 / Qt 6.8.3 / Release

상단 분석 선택 버튼은 유지하고 요청한 영문/한글 명칭으로 정리했다. 재생은 애니메이션으로 표기했다. 기존과 동일하게 Freq에서 진동, 나머지 패널에서 구조 프레임의 재생/일시정지를 수행한다. Atomic number는 사용자가 요청한 툴바 명칭이며 기존 원자 번호 표시 기능은 그대로다.

실제 분석 내용은 좌측 도크에 배치하고 기본 320px, 최소 280px, 최대 360px로 제한했다. 도크 이동과 분리는 막고 접기/펼치기는 유지했다. 결과 트리와 분석 내용은 세로 분할이며 Freq/Scan/IRC 내부도 세로로 배치한다. 기존 상단 도크 저장값이 적용되지 않도록 window/sidebarPanelState를 사용한다.

정방형 컨테이너가 실제 OpenGL 위젯의 너비와 높이를 같게 유지한다. 남는 영역은 회색 여백이다. 렌더링, 원자 선택과 이미지 캡처가 동일한 정방형 좌표계를 사용한다. Scan 그래프는 scanDetected이며 scanPoints가 있을 때만 보이고 초기/새 구조/일반 출력/빈 Scan에서는 숨긴다.

## 검증

- 최종 Windows OpenGL 전체 GUI 회귀: 40 passed, 0 failed, 1 skipped (`full-gui-tests.txt`). 생략한 기존 realScanGuiInputs는 별도 실제 파일 경로가 필요하다.
- 일반 Gaussian Opt/Freq 출력과 Scan 출력 간 문서 전환, 완료 point가 없는 Scan, 새 구조의 하단 그래프 표시를 확인했다.
- 900×700, 1400×900, 1600×950 창에서 정방형 뷰와 사이드바 폭 제한을 검증했다.
- 버튼명, 두 줄 툴바, 좌측 도크, 문서별 분석 선택과 접기/펼치기를 확인했다.
- 원자 숨기기 시 직교/원근 카메라, 현재 좌표와 스크롤 유지 회귀도 통과했다.
- QtTest 화면 `sidebar.png`, `sidebar.png.scan.png`를 직접 열어 레이아웃을 확인했다.

루트와 dist의 Youngcraft.exe를 갱신하고 빌드 파일과 SHA-256 일치를 확인했다. portable ZIP은 갱신하지 않았다. 수정 전 소스, GUI 테스트, README 및 실행 파일은 `.build/before-sidebar-square-20261016/`에 보관한다. 소스/테스트 변경 비교는 `source.patch`다.
