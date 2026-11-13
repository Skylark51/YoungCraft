# 시작 로고 표시 시간 (2026-11-13)

스플래시의 첫 paintEvent가 완료된 뒤부터 500ms를 계산한다. 마지막 100ms는 페이드 전환이며 그 전에는 작업 창을 표시하지 않는다. 작업 창 준비 시간은 표시 시간에 포함한다. 준비가 늦어지면 준비 후 전환하며, 로고가 그려지기 전 readiness 요청이 와도 표시 시간을 먼저 소모하지 않는다. 클릭으로 스플래시를 건너뛰지 않는다.

Qt 6.8.3 / MSVC Release / Windows 네이티브 GUI 검사에서 전환은 520ms로 관측됐다. startupSplashShowsAndCloses와 mainWindowStartsWithKoreanUi 통과(초기화·정리 포함 4 passed). 표시 전 readiness, 준비 시간 중복 방지, 대상 창 삭제, 로고 클릭 회귀 검사도 통과했다. 로고 PNG를 확인했다.

tools/package_portable.ps1 -ReuseMmRuntime으로 패키징하고 tools/install_portable.ps1로 C드라이브 별도 설치본을 갱신했다. 설치본 manifest 1,243개 파일 SHA-256, 빌드·배포·설치·ZIP 실행 파일 일치, ZIP CRC 검사를 통과했다. 바탕화면 Youngcraft 바로가기는 별도 설치본을 가리킨다. 기존 실행 중인 원본 프로세스는 유지했으며 변경은 종료 후 바로가기로 다시 실행할 때 적용된다.
