# 맞춤 캡처 업그레이드 — 2026-11-01

일반 구조에서는 도구 모음의 **맞춤 캡처**로 진입합니다. **편집 > 맞춤 캡처 — 열린 창·탭 다중 캡처**는 파일 종류와 관계없이 사용할 수 있습니다. 기존 IRC·TS 캡처에는 같은 이름의 전환 버튼을 추가했습니다.

## 다중 캡처

- 현재 실행 세션의 모든 문서 탭과 분리된 창을 수집합니다. 파일 연결이나 `--new-window`로 연 창도 같은 세션에 속합니다.
- 창 번호와 탭 이름으로 구분하고 체크한 구조만 표시·동기화·내보냅니다. 원자 수 제한은 없습니다.
- 어느 미리보기에서든 회전·확대·화면 이동을 조절하면 체크한 미리보기가 함께 따라갑니다.
- 열었던 시점의 구조 좌표, 원자 표시, 스타일과 카메라를 복사합니다. 원본 문서나 좌표를 변경하지 않으며, 원본 탭을 닫아도 캡처 사본은 유지됩니다. 새 탭이나 이후 원본 변경 사항은 캡처를 다시 열어 가져옵니다.
- 개별 투명 PNG를 새 `Youngcraft-Multi-…` 폴더에 순서대로 저장하거나 같은 크기의 그림을 한 줄로 복사합니다. Office용 HTML에 그림별 cm 크기를 넣습니다. 저장 PNG는 기본 600 dpi이며, 극단적인 가로세로 비율에서는 픽셀 수를 제한하고 지정한 cm 크기는 유지합니다.

## 회전

기존 맞춤 캡처와 다중 캡처가 같은 조작 막대를 사용합니다.

- 자유 회전은 화면의 X/Y 성분을 단일 회전축으로 결합해 대각선 드래그에서 yaw/pitch 적용 순서의 편향을 제거합니다.
- 화면 X/Y/Z축 고정을 제공합니다. X축은 세로 드래그, Y·Z축은 가로 드래그입니다. 매번 화면 축을 기준으로 회전하므로 이미 돌린 방향 때문에 조작 축이 바뀌지 않습니다.
- 드래그 각도는 뷰 크기에 비례해 계산하고 기본 감도를 0.6배로 낮췄습니다. 감도는 0.1–2.0배로 조절합니다.
- **− 회전 / + 회전**은 0.1–90°의 지정 각도만큼 회전합니다. 자유 회전에서는 화면 Y축을 사용합니다.
- **시작 뷰 복원**은 각 미리보기의 초기 카메라로 돌아갑니다.
- 우클릭 드래그: 회전, Ctrl+우클릭 또는 가운데 버튼: 이동, 휠: 확대/축소.

## 검증

Windows / Qt 6.11.2 / GCC 16.2.0 Release 빌드와 실제 Windows OpenGL 플랫폼에서 검증했습니다. offscreen의 렌더링 건너뛰기를 사용하지 않았습니다.

- `multiCaptureWorkflow`: 두 창에서 서로 다른 원자 수의 구조 수집, 자유·X/Y/Z축 회전, 정규화된 회전, 이동·휠 확대 동기화, 원본 카메라 유지, 체크 해제, 공통 PNG 크기·투명 배경, 클립보드 HTML, PNG 저장, 원본 탭 닫기, 뷰 복원, 선택 없음.
- `reactionCaptureWorkflow`, `reactionCaptureHydrogenControls`: 기존 IRC·TS GIF/PNG, SI 캡처와 수소 표시 회귀 검증.
- `matchingAtomViewsSynchronize` (직교·원근), `documentsAreIndependent`, `rectangularViewportPickingAndCapture`, `copiesMoleculeWithTransparentBackground`: 일반 뷰와 문서 동작 회귀 검증.

결과는 [workflow-test.txt](workflow-test.txt)와 [regression-test.txt](regression-test.txt)에 있습니다. 미리보기는 [다중 캡처](preview.png), [IRC·TS 캡처](reaction-preview.png)입니다.

빌드 환경의 한글 경로를 처리하지 못하는 MinGW/Qt 도구 때문에 `subst V:`를 프로젝트에 연결하고 `build-multicapture-ascii`에서 빌드했습니다. 소스 사본은 만들지 않았습니다. 빌드 명령을 실행할 때 `TEMP`와 `TMP`는 영문 경로 `E:/yc-tools/tmp`를 사용합니다.
