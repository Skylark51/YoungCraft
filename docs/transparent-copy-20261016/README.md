# 투명 배경 이미지 복사 — 2026-10-16

- 이미지 복사 메뉴·도구 모음·Ctrl+I가 `MoleculeView::image(true)`로 투명 배경을 캡처한다.
- 클립보드에는 기존 Qt 이미지와 `image/png`를 함께 제공해 PNG를 선택하는 붙여넣기 대상에도 알파 채널을 전달한다.
- 복사 완료 메시지에 투명 배경을 명시했다. 화면 배경과 그림 저장 옵션은 그대로다.
- Windows OpenGL에서 흰색·어두운색·검은색 배경으로 메뉴·단축키·도구 모음을 각각 검사했다. 네이티브 이미지와 PNG의 배경 alpha=0, 분자 불투명 픽셀, 복사 후 화면 배경 복원을 확인했다. 테스트는 기존 클립보드 데이터를 보관하고 종료 시 복원한다.
- 관련 렌더링·preset·파일 로딩 검사 포함 QtTest 결과: 8 passed, 0 failed, 0 skipped. `tests.txt` 참조.
- MSVC 2022 / Qt 6.8.3 Release 빌드 성공. 기본 `Youngcraft.exe`와 `dist/Youngcraft.exe`를 갱신했다. 기존 Qt DLL과 빌드의 DLL 일치도 확인했다.
- 이전 실행 파일은 `.build/before-transparent-copy-20261016/`에 보관했다. 기존 portable ZIP은 재생성하지 않았다.
