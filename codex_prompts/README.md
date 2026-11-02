# Codex prompt order

각 작업은 독립적으로 실행 가능하다. 한 번에 하나만 Codex에 준다.

1. `01_RENDERING_CHEMCRAFT_QUALITY.md`
2. `02_LEFT_CONTEXT_SIDEBAR.md`
3. `03_TDDFT_WORKSPACE_SWITCH.md`
4. `04_EXTREME_CONVENIENCE_QUICK_ACTIONS.md`

각 프롬프트의 핵심은 수정 허용 파일을 제한하는 것이다. Codex가 임의로 전수조사/대규모 리팩터링하지 않도록 `docs/MODULE_MAP.md`와 함께 사용한다.
