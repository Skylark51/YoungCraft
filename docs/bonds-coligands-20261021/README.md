# Ctrl+B 실제 결합과 산소 착물 보조 리간드

- 일반 보기에서 원자를 두 개 순서대로 클릭하거나 Shift/Ctrl로 두 원자를 선택하고 Ctrl+B를 누르면 실제 결합이 추가/삭제됩니다. 두 원자 선택은 유지되므로 같은 단축키로 반복 토글할 수 있습니다. 금속이 포함되면 배위결합으로 연결합니다. 원자/좌표/명시적 수소는 유지하며 Ctrl+Z/Ctrl+Y가 적용됩니다.
- 두 원자가 선택되지 않으면 기존 결합 편집 모드를 엽니다. 계산 파일을 편집하면 원본을 보존하고 새 편집 탭을 만듭니다.
- 구조 만들기 → 금속–리간드–산소 조합에서 peroxo/hydroperoxo/superoxo의 end-on·side-on 조각과 보조 리간드 없음/MeCN/H2O를 고를 수 있습니다. 기존 여섯 산소 조각을 유지하고 MeCN/H2O 템플릿을 추가했습니다. 리간드/작용기 브라우저에서도 Lab Oxygen, Lab Solvents 분류로 찾을 수 있습니다.
- 보조 리간드는 기존 금속 주개 원자에서 열린 방향을 탐색하고 충돌을 피해서 실제 배위결합으로 연결합니다. 생성 좌표는 편집 가능한 시작 구조입니다.
- MMFF/UFF에는 기존 구조의 실제 결합 목록을 전달합니다. GFN-FF에는 모든 원자의 양방향 이웃 목록을 $ffnb로 전달합니다. 따라서 가까운 두 원자 사이에서 삭제한 결합이 MM 입력에서 자동 복구되지 않습니다. GFN-FF의 결합 차수는 계산기가 결정합니다.
- 구형 Windows GFN-FF 실행기가 $ffnb를 무시하는 것을 확인하여, 명시적 topology를 지원하는 별도 backend를 xtb_runtime/gfnff에 함께 배포합니다. 기존 GFN1/2-xTB 실행기는 유지합니다. 출처, 재빌드 방법, 소스, 라이선스는 해당 폴더의 RUNTIME.md와 sources/licenses에 있습니다.
- 골격 유지 옵션은 end-on과 side-on 양쪽의 배위결합 및 O–O 거리에 적용됩니다.

## 검증

Release 빌드 완료. 구조/파서/모델/리간드 편집/xTB CTest 6개 묶음 통과.
추가 Windows GUI 검사 및 관련 기존 입력·MM 검사: 10 passed, 0 failed, 0 skipped.
리간드 5종 × 산소 fragment 6종 × 보조 리간드 2종 조합 검사 통과.
실제 GFN-FF에서 동일 좌표의 결합 삭제/복원에 따른 내부 이웃 목록과 에너지 변화 확인.
Fe–12MTP–end-on hydroperoxo에 MeCN/H2O를 각각 연결한 실제 GFN-FF 최적화 실행(10 cycles) 및 O–O restraint 전달 확인. 이 검사는 최적화 실행 경로를 확인하며 완전 수렴을 요구하지 않습니다.

전체 offscreen GUI 묶음은 기존 styleEditorPreviewsImmediately 색상 기대값 불일치로 1건 실패했습니다(현재 검은 배경과 이전 남색 기대값). 이번 작업에서 스타일 코드는 변경하지 않았습니다. 전체 GUI 결과는 36 passed, 1 failed, 9 skipped이며 실제 OpenGL 입력 검사는 별도 Windows 실행으로 통과했습니다.

GFN-FF 수동 topology 입력 설명: https://xtb-docs.readthedocs.io/en/latest/gfnff.html#manual-neighbor-list-setup
