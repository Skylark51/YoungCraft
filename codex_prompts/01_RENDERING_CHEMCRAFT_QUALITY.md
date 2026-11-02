# 01 — Chemcraft급 기본 렌더링 개선

## 작업 위치
`E:\03\_프로젝트\_개발\01\_진행중\_개발프로젝트\Youngcraft`

## 최우선 원칙
- 먼저 `docs/MODULE_MAP.md`만 읽고 아래 허용 파일만 확인한다.
- 저장소 전수조사 금지. 관련 없는 파일 검색/리팩터링 금지.
- 분자 조작, 진동 애니메이션, 파서, TD-DFT, Scan/IRC 기능은 수정하지 않는다.
- 기존 기능을 삭제하지 않는다.

## 수정 허용 범위
우선 아래 파일만 수정한다.
- `src/render/moleculeview/MoleculeShaders.inc`
- `src/render/moleculeview/MoleculeView_AtomsBonds.cpp`
- `src/render/moleculeview/MoleculeView_Scene.cpp`
- `src/settings/StyleManager.h`
- `src/settings/StyleManager.cpp`
- `src/ui/styleeditor/StyleEditor_Lighting.cpp`
- `src/ui/styleeditor/StyleEditor_Atoms.cpp`

컴파일 오류가 실제로 발생할 때만 관련 헤더 1개를 추가 수정한다. 그 외 파일은 건드리지 않는다.

## 목표
현재 조명 조절 기능은 있으나 원자 표면과 색감이 플라스틱처럼 보이고 Chemcraft보다 저급하다. **사용자가 세부 조명을 만지지 않아도 기본 상태에서 Chemcraft와 비슷하게 차분하고 선명한 ball-and-stick 결과가 나오도록 기본 렌더링을 개선한다.**

## 구현 요구
1. `Chemcraft` 렌더링 프리셋을 만들고 기본 프리셋으로 사용한다.
2. 원소별 기본색은 과포화되지 않도록 조정하되 CPK 계열 식별성은 유지한다.
3. 원자 표면은 강한 작은 하이라이트 대신 넓고 부드러운 하이라이트가 보이게 한다.
4. 암부가 검게 뭉개지지 않도록 ambient/fill을 조정한다.
5. 원자 구 내부를 결합이 비쳐 보이는 것처럼 표현하지 않는다. 원자는 불투명 solid sphere로 유지한다.
6. bond는 atom보다 약간 matte하게 보이게 하여 결합이 원자 표면과 경쟁하지 않게 한다.
7. 금속 원자는 너무 번쩍이는 금속광이 아니라 약간 더 단단한 재질감 정도만 준다.
8. 흰 배경에서 H/C가 사라지지 않고, 어두운 배경에서도 색이 과포화되지 않도록 한다.
9. UI에는 일반 사용자가 이해할 수 있는 항목을 우선 노출한다: `광택`, `표면 부드러움`, `입체감`, `색 농도`. 기존 raw lighting 파라미터는 고급 설정으로 유지해도 된다.
10. 기존 사용자 지정 색상, 그림자, 두 번째 광원, background 기능은 그대로 보존한다.

## 금지
- 렌더 엔진 전면 교체 금지.
- 새로운 외부 렌더링 라이브러리 추가 금지.
- 기존 OpenGL 파이프라인을 이유 없이 재작성하지 말 것.
- 파일명/클래스명 대규모 변경 금지.

## 검증
- 기존 샘플 구조에서 원자/결합 표시가 정상인지 확인한다.
- 그림자 ON/OFF, perspective ON/OFF 모두 깨지지 않는지 확인한다.
- Debug/Release 빌드 중 가능한 환경 하나를 빌드하고 기존 테스트를 실행한다.
- 마지막 보고는 `수정 파일 / 핵심 변경 / 검증 결과`만 간단히 작성한다.
