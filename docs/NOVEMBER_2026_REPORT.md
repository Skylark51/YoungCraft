# 2026년 11월 개발 결과 보고

Windows Release 빌드, 공개 계산 출력·CIF 검증, 원격 GUI 회귀와 로컬 portable 설치를 완료했다. 로컬 GUI 실행 없이 백그라운드에서 검증·배포했다. GitHub 반영 기록은 [PR #21](https://github.com/Skylark51/YoungCraft/pull/21)에서 확인할 수 있다.

## 1. 작업 전 구현 수준

- Gaussian: Opt/Freq/Scan/IRC/TD-DFT 분석 화면, 계산 프레임과 기본 그래프 선택 기능이 있었다. 일부 IRC 문구와 실제 누적 좌표를 인식하지 못했고, 프레임과 별도 에너지 배열을 뒤에서 맞추는 경로가 있었다.
- ORCA: 좌표·에너지·기본 진동수 읽기는 있었으나 진동 변위, IR/Raman 표, TD 상태·전이, relaxed scan 및 IRC 연결이 빠져 있었다.
- CIF: 기존 Gemmi를 사용해 단위격자·대칭·비대칭 단위·단위격자/2×2×2 packing 표시를 지원했다. CIF 저장은 없었고, 편집 복사본이 결정 메타데이터에서 분리될 수 있었다.
- 다중 탭, 프로젝트 라이브러리, 구조 편집·측정, xTB/MM은 이미 구현돼 있어 기존 구현을 유지했다.

## 2. 실제 변경

- Opt 프레임 이동과 Scan/IRC 선택점을 동기화하고, 에너지 없는 프레임에 다른 에너지 점이 선택되지 않도록 수정했다. ORCA 수렴 표의 gradient/displacement 값을 프레임에 연결했다.
- ORCA normal-mode 변위, IR/Raman 표, TD-DFT/CIS singlet/triplet·부호 있는 전이 계수·전기 쌍극자 세기를 읽는다. IR 형식별 열/단위를 구분한다.
- TD 그래프의 확장 곡선 샘플과 실제 전이 선택점을 분리했다. 그래프·상태 표·해당 계산 구조를 연결하고 스펙트럼 설정 변경 시 선택을 유지한다.
- Gaussian의 실제 IRC 경로 문구·누적 좌표를 읽고 평가된 구조/에너지에 연결했다. ORCA relaxed scan의 계산점별 구조 이력과 최종 구조, ORCA IRC 요약과 전체 XYZ trajectory 연결을 추가했다.
- CIF 비대칭 단위의 좌표 편집을 원문 문서에 유지하고, 표시 모드 전환 시 대칭·packing을 재생성한다. 원문 CIF의 다른 데이터 블록과 알 수 없는 태그를 보존하며 원자 좌표 열만 수정해 원자적으로 저장한다. 분율/Cartesian 열이 함께 있으면 둘 다 갱신한다.
- 지원하지 않는 원자 정체성·개수·전하·명시적 결합 변경은 저장 오류로 알린다. 인식할 수 없는 공간군을 명시적 연산 없이 임의의 P1으로 바꾸지 않는다.
- 주기적 접촉 목록에 원자쌍·격자 이동·거리를 표시한다. 비직교 격자와 두 격자 이상 떨어진 이웃도 포함하며 역방향 중복을 제거한다.
- GUI 테스트 컨텍스트를 실제 앱의 OpenGL 3.3 Core와 맞췄고, GL 함수 초기화 실패를 검사한다. CI에서 검증된 소프트웨어 OpenGL과 검사별 시간 제한을 사용한다.

## 3. 검증 근거

| 요구사항 | 현재 근거 | 상태 |
|---|---|---|
| 실제 Gaussian·ORCA 출력 최소 20개 | 공개 cclib-data 소스 커밋·SHA256 고정, 실제 출력 24개 | 콘솔 통과 |
| 실제 CIF 최소 10개 편집·재저장 | COD 1000041–1000050, 각 구조 첫 좌표 변경·재저장·재읽기 | 콘솔 통과 |
| CIF 격자·대칭·알 수 없는 정보 보존 | 모든 데이터 블록/항목/loop/save frame과 변경하지 않은 좌표 토큰 비교 | 콘솔 통과 |
| 실제 경로의 구조·에너지 대응 | Gaussian IRC 103개 지점, ORCA Scan 12개 계산점/134개 프레임, 원문 수치와 프레임 인덱스 검사 | 콘솔 통과 |
| ORCA IRC 연결 | 공식 출력 형식의 합성 fixture, trajectory 부재·일치·원소 불일치 검사 | 파서 통과; 실제 IRC 원문 확대 검증 한계 |
| 콘솔 CTest | parser_gaussian/parser_orca/structure_files/november_public_corpus 4/4 | 로컬 통과 |
| 기존 계산·편집 회귀 | 최종 원격 실행 43773421414에서 콘솔·계산 CTest 10/10, 실제 xTB 포함 | 통과 |
| 그래프·3D·그리기·탭·프로젝트·CIF GUI 흐름 | 같은 실행에서 개별 GUI 테스트 15/15, 실제 CIF 저장 대화상자·재읽기 포함 | 통과 |
| Windows Release | 앱·파서·GUI 테스트 바이너리 로컬 및 원격 컴파일 | 통과 |
| Portable 설치·바로가기 | 지정 스크립트 패키징·설치, 설치본 1243개 manifest 해시 일치, C: 바로가기 경로 확인 | 통과 |
| GitHub 반영 | 최종 검증 커밋 1cd7a38695a7d6f8ee40b785ee14aabee62ee9ab, PR #21 | 두 Windows 검사 성공; 병합 상태는 PR 기록 참조 |

재현: `python tools/fetch_november_corpus.py .build/november-corpus` 후 `YOUNGCRAFT_NOVEMBER_CORPUS`를 해당 경로로 설정하고 `november_public_corpus` CTest를 실행한다. GUI 검사는 사용자 요청에 따라 로컬에서 실행하지 않고 GitHub Windows 실행기에서만 실행한다. 상세 이력은 `november-2026-development.md`에 기록했다.

## 4. 남은 기능적 한계

- CIF 저장은 원자 정체성/개수를 유지한 좌표 편집 중심이다. 원자 추가·삭제, 원소·전하·명시적 결합 변경은 지원하지 않는다고 알린다. 격자/공간군 자체의 편집은 이번 저장 경로의 대상이 아니다.
- 저장한 CIF의 기존 거리·각도·anisotropic displacement 등 파생 데이터는 보존하지만 재계산하지 않는다. 좌표를 바꾼 뒤 해당 항목을 연구에 사용하려면 별도 재정련/검증이 필요하다.
- 접촉 목록은 결합을 포함한 기하학적 원자 접촉이다. 자동 수소결합 분류, 상호작용 에너지, Hirshfeld surface, fingerprint plot, energy framework는 구현하지 않았다.
- ORCA IRC 구조 연결은 같은 경로 순서의 전체 trajectory가 필요하다. 가로축은 질량가중 좌표가 아닌 경로 단계이다. 실제 IRC 출력의 형식·버전 다양성 검증은 합성 테스트보다 제한적이다.
- 모든 Gaussian/ORCA 버전·모든 계산 방법을 검증한 것은 아니다. 검증한 파일과 버전의 범위를 벗어난 형식은 추가 확인이 필요하다.

## 5. 12월 권장 순서

1. 분자/공유결합 망과 주기 경계를 구분하는 접촉 분류, disorder·특수 위치 검증을 강화한다.
2. 기준 결정구조에서 수소결합·접촉 거리를 Mercury 등과 대조하고 재현 가능한 reference 결과를 확보한다.
3. 전자밀도 모델·라이선스와 수치 정확성을 먼저 결정한 뒤 Hirshfeld surface를 구현·비교 검증한다.
4. 검증된 surface에서 fingerprint plot과 면적 적분·원소별 분해를 연결한다.
5. 별도로 검증한 상호작용 에너지 모델이 준비된 뒤 energy framework를 구현한다.

## 6. 커밋·GitHub·배포 상태

작업 브랜치: `codex/november-analysis-crystal-20261116`. [PR #21](https://github.com/Skylark51/YoungCraft/pull/21)에 커밋·기본 브랜치 병합 기록을 남긴다. 최종 코드 검증 커밋은 `1cd7a38695a7d6f8ee40b785ee14aabee62ee9ab`이며, 이후 최종 보고 정리는 문서만 변경한다. 마지막 앱 소스 변경 `4e037f8` 이후 검증 커밋까지 `src` 차이가 없다.

- [전체 Windows 회귀 실행 43773421414](https://github.com/Skylark51/YoungCraft/actions/runs/43773421414): Release 빌드, CTest 10/10, GUI 15/15 성공.
- [ORCA 파서 실행 43773421399](https://github.com/Skylark51/YoungCraft/actions/runs/43773421399): 성공.
- `tools/package_portable.ps1 -BuildDir build-current -ReuseMmRuntime` 및 `tools/install_portable.ps1` 실행 완료. MM 코드는 변경하지 않았으며 검증된 기존 runtime을 재사용했다.
- 설치 위치: `C:\Users\UNIST\AppData\Local\Programs\Youngcraft Portable`. 바탕화면 `Youngcraft.lnk`는 해당 폴더의 `Youngcraft.exe`를 가리킨다. 설치 후 앱을 실행하지 않았다.
- 설치본 파일 1,243개를 manifest와 대조해 불일치 0개. 빌드·패키지·설치 실행 파일 SHA256은 `44C620A1DE64D095F70DAF79F8F7147438F53A9CC7BD19EAC54FCBEACBBB2B00`으로 일치한다.
- 기존 개인 파일과 관련 없는 미커밋 변경은 보존했다.
