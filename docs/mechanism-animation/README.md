# Deformylation 미리보기

사용자가 지정한 5개 최적화·진동 결과 파일을 연결한 예제입니다. 원본 계산 파일은 수정하지 않았습니다.

- 프로젝트: `../../examples/Deformylation-mechanism.ycmechanism`
- 영상: `deformylation.gif` — 640×360, 25 fps, 301프레임, 약 12초
- `deformylation-window.png`: 편집 화면
- `deformylation-0.png`, `-1.png`, `-2.png`: 처음·중간·끝 구조

Youngcraft 상단 오른쪽 **메커니즘 영상 > 프로젝트 열기…**에서 예제 프로젝트를 여세요. Co·O 및 첫 구조에서 이 원자와 직접 결합된 원자는 선명하게, 나머지는 불투명도 20%로 표시했습니다. **전체 복원**으로 모두 선명하게 되돌리거나, 원자를 다시 선택해 강조 대상을 바꿀 수 있습니다. 구조는 65개 원자를 모두 유지합니다.

순서는 Reactant → C–C cleavage TS → After TS1 → O–O cleavage TS → After TS2입니다. 첫 입력의 `irHoCRev` 표기는 실제 폴더의 `ircrev-opt-freq.out` 파일로 대응했습니다. 이 다섯 파일에서 각 최종 구조를 사용했으므로 구조 사이 구간은 보간입니다. 입력되지 않은 IRC 궤적이나 분자동역학을 새로 계산하지 않았습니다. After TS2를 전체 Deformylation의 최종 생성물이라고 단정하지 않습니다.

연속 속도, 원자 대응, 원자 수 변화, 이탈 조각·수동 이동, 결합 페이드, 원본 구조 보존, 프로젝트 왕복, 비동기 파일 추가, GIF/PNG 렌더링과 취소 시 기존 파일 보존을 자동 검증했습니다. 사용자 지정 경로의 원자 추적 검증도 통과했습니다. MP4 인코딩은 ffmpeg가 없어 이 환경에서 실행하지 않았습니다.
