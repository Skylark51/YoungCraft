# 가로·세로 2배 슈퍼샘플링

2026-10-16 / MSVC 2022 / Qt 6.8.3 / Windows OpenGL Release

## 구현

antiAliasing이 켜져 있으면 실제 장치 픽셀의 가로/세로 2배인 RGBA8 + depth/stencil FBO에 3D 장면을 그린다. 단일 샘플 FBO의 2×2 픽셀을 bilinear sampling으로 평균하고, 전체 화면 삼각형 셰이더로 Qt 기본 프레임버퍼에 출력한다. Qt 기본 프레임버퍼는 MSAA일 수 있으므로 크기가 다른 framebuffer blit 대신 셰이더를 사용한다. 빈 구조는 이 경로를 생략한다.

기존 그림자 pass 이후에 고해상도 타깃을 바인딩한다. 3D 축까지 처리한 뒤 축소하고, QPainter 기반 격자선/진동 벡터/원자 번호/측정값은 기존 논리 좌표에 표시한다. 투영과 picking 좌표는 변경하지 않는다. 타깃은 재사용하며 크기가 바뀌면 다시 만들고 OpenGL 컨텍스트가 current인 상태에서 해제한다.

장치 최대 texture/viewport 크기와 16,777,216 고해상도 픽셀 한도를 검사한다. 한도를 넘거나 타깃 생성이 실패하면 경고를 한 번 기록하고 기존 MSAA로 돌아간다. 안티앨리어싱을 끄면 타깃을 해제한다. 3D 장면의 색상/깊이 픽셀이 4배가 되므로 GPU 부하가 늘어난다.

투명 캡처는 clear RGBA를 모두 0으로 설정하고 캡처 직전에 동기 렌더링한다. 이전 불투명 프레임을 읽어 배경색을 지우던 방식 대신 실제 알파를 유지하며, premultiplied RGB를 straight alpha로 변환하여 PNG/클립보드 가장자리의 어두운 테두리를 방지한다. 주석과 측정 표시의 캡처도 기존 옵션을 따른다.

## 검증

- Windows OpenGL 전체 GUI 회귀: **41 passed, 0 failed, 1 skipped** (`full-gui-tests.txt`). 기존 realScanGuiInputs는 별도 실제 파일 경로가 없어 생략했다.
- 얇은 대각선 파란 결합을 조명 없이 렌더링해 실루엣의 중간색 픽셀을 측정했다. AA 해제 시 0개, 2배 슈퍼샘플링 시 429개였다. `before.png`는 AA 해제, `after.png`는 슈퍼샘플링이며 두 이미지를 직접 열어 비교했다. 기존 MSAA와의 정량 비교는 아니다.
- 실제 FBO 가로/세로 2배, 캡처 크기 유지, 투영행렬 불변, 원자 picking과 resize 후 타깃 재생성 및 OpenGL 오류 없음 확인.
- 투명 배경의 중앙 결합에서 부분 알파 288픽셀과 원래 파란 색 보존을 확인했다. 검은 배경/흰 배경에서 투명 캡처 결과가 동일했다. `transparent.png`.
- 원자 번호/측정/편집 미리보기, 그림자와 결합 연속성, 일반/Scan 레이아웃, 카메라/스크롤 유지, 투명 클립보드 등 기존 GUI 회귀 통과.
- 관련 헤더 변경의 영향을 받는 소스를 모두 다시 컴파일한 뒤 최종 전체 회귀를 수행했다.

## 배포와 복원

루트 Youngcraft.exe와 dist/Youngcraft.exe를 갱신하고 빌드 파일과 SHA-256 일치를 확인했다. 기존 portable ZIP은 갱신하지 않았다. 수정 전 src, GUI 테스트, README 및 루트/dist 실행 파일은 `.build/before-supersampling-20261016/`에 보관한다. 소스/테스트 변경 비교는 `source.patch`다.
