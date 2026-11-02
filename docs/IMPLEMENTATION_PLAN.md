# Youngcraft 구현 계획

## 목표

Windows x64에서 Gaussian 결과를 빠르게 탐색하고, 3D 구조를 부드럽게 회전·애니메이션·캡처하는 경량 Qt Widgets 애플리케이션을 만든다. 원본 프로그램은 분석 대상이 아니라 기능·UX 기준으로만 참고한다.

## 고정 기술과 경계

- C++20, Qt 6 Widgets, QOpenGLWidget, 현대 OpenGL, CMake + Ninja
- 외부 런타임 의존성 없음. JSON 대신 QSettings를 우선 사용한다.
- 파서는 순차 스트림을 사용하고, UI는 QThread 작업자에서 결과를 받는다.
- 렌더러는 재사용 메시와 카메라 상태를 유지하며 회전 중에는 파싱/I/O를 실행하지 않는다.

## 모듈 구조

`model`은 원자·결합·프레임·계산 결과의 순수 자료 구조, `io`는 파일 형식 dispatch와 Gaussian/XYZ/CUBE/FCHK 파서, `render`는 OpenGL viewport와 picking, `analysis`는 수치 요약·그래프 데이터, `animation`은 프레임/normal-mode 재생, `export`는 이미지·GIF·ffmpeg 연동, `settings`는 표시 스타일과 QSettings, `ui`는 한글 메뉴·dock·분석 패널을 담당한다.

## 단계

1. Phase 1: 프로젝트/테스트 골격, Gaussian geometry·optimization·frequency·charges parser, ball-and-stick OpenGL, camera/picking/labels, 한글 메뉴, frequency animation, PNG/clipboard.
2. Phase 2: scan/IRC/SCF/force/dipole/TD-DFT/원자 속성 및 그래프 패널.
3. Phase 3: CUBE/FCHK metadata, 등가면 mesh cache, trajectory/video/GIF, 구조 비교/중첩.
4. Phase 4: 편집·fragment·Z-matrix·결정학·스크립트·그래프 등 유틸리티.

각 단계는 incremental build와 smoke test를 우선하고, 큰 묶음이 끝날 때만 Release 빌드한다. 첨부 Chemcraft.zip이 실제 경로에 있으면 회귀 corpus로 연결하고, 배포물에는 복사하지 않는다.

## 현재 기준선

지정 경로에는 레퍼런스 PNG만 있고 `Chemcraft.zip`은 발견되지 않았다. 따라서 `tests/data/mvp.out`와 `tests/data/density.cube`를 회귀 fixture로 사용하며, 원본 프로그램이나 바이너리는 분석하지 않는다.

Phase 1과 Phase 2의 핵심 경로는 구현되었고, Phase 3은 CUBE/FCHK metadata·양/음 등가면·GIF까지 동작한다. Release CTest는 Gaussian parser, model/isosurface/export, GUI smoke의 3개 테스트를 통과한다. portable 배포는 `tools/package_portable.ps1`이 windeployqt 누락 DLL을 보완하도록 고정했다. Phase 4의 편집·fragment·Z-matrix·결정학·스크립트 유틸리티는 메뉴 골격과 후속 구현 대상으로 남아 있다.
