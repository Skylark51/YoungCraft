# 오비탈·분자 편집 개선 검증 (2026-11-14)

범위는 표면 생성의 불필요한 계산 제거와 선택 원자의 수소 편집으로 제한했다. 외부 라이브러리를 새로 의존성에 추가하거나 기존 렌더러·MM 엔진을 교체하지 않았다.

## 참고한 설계

- [VTK Flying Edges](https://vtk.org/doc/nightly/html/classvtkFlyingEdges3D.html)의 computational trimming과 메모리 할당 최소화: 기존 marching tetrahedra에서 8개 꼭짓점이 모두 같은 쪽인 voxel은 좌표·법선 계산 전에 건너뛰며, 교차점은 최대 4개의 고정 배열로 계산한다. 표면이 없는 경우 큰 출력 버퍼도 할당하지 않는다. Flying Edges 자체를 이식한 것은 아니다.
- [RDKit AddHs의 onlyOnAtoms](https://www.rdkit.org/docs/source/rdkit.Chem.rdmolops.html#rdkit.Chem.rdmolops.AddHs): 기존 원자가 규칙을 재사용하면서 수소 처리를 선택한 heavy atom으로 제한한다.
- [Avogadro Hydrogens](https://github.com/OpenChemistry/avogadrolibs/blob/master/avogadro/qtplugins/hydrogens/hydrogens.cpp): 수소 추가와 보정을 별도 명령으로 제공하고 한 번의 실행 취소로 되돌린다.

## 동작

구조 만들기에 `선택 원자 수소 추가`, `선택 원자 수소 보정`을 추가했다. 추가는 기존 수소 위치를 보존하고 부족분만 채운다. 보정은 선택 원자에 단일 일반 결합으로 붙은 수소만 재배치한다. 선택하지 않은 원자, 고정 수소, 고정·잠긴 heavy atom, 금속 결합 및 가교 수소를 보존한다. 수소 제거에 따라 원자 인덱스가 바뀌어도 처리 범위를 유지한다. 기존 전체 수소 명령도 계속 제공한다.

## 표면 생성 비교

Windows MSVC Release `/O2`, 동일 입력과 isovalue 0.02, 양·음 위상 합산, 각 4회. 파일 파싱·네트워크·GPU 업로드를 제외한 CPU 표면 생성 시간이다. 새 캐시 전환의 시간이 아니며 다른 분자에서도 같은 배속을 보장하지 않는다.

| 입력 | 기존 중앙값 | 개선 중앙값 | 좌표·법선 SHA256 |
|---|---:|---:|---|
| 실제 FCHK에서 생성한 MO cube A (70×98×74) | 385.6 ms | 14.6 ms | 전후 일치 |
| 실제 FCHK에서 생성한 MO cube B (70×98×74) | 383.8 ms | 16.4 ms | 전후 일치 |
| 합성 p 오비탈 (129³) | 1882.6 ms | 47.5 ms | 전후 일치 |

실제 cube A: `d1942b1d680b375ecdb11a6ee715ed0a0583b369acc6e5e720f753d167fc35fd` (106929 vertices).
실제 cube B: `6f5949afba3997ce05c08b5c8e569ff8c5a409eafa992240f0cc47ae9b023a7e` (144879 vertices).

## 자동 검증

새 검증은 선택 인덱스의 이동, 다른 원자의 수소 보존, 반복 추가의 멱등성, 고정·잠금·금속·가교 보호, 양전하 질소, 잘못된 선택의 무동작, 메뉴 연결과 Undo/Redo를 확인한다. 표면 검증은 빈 출력의 미할당, 양·음 위상 평면의 좌표·법선, 삼각형 제한을 확인한다. 기존 편집·오비탈·캐시·별도 창 회귀 검증도 함께 실행한다.

## 최종 실행 결과

- MSVC Release 빌드 성공. Editor 19 passed, Model 20 passed, Orbitals 13 passed / 2 skipped (명시적 서버 통합 테스트 및 별도 입력 테스트). 실제 FCHK를 지정한 추가 테스트는 66 atoms, 2968 spin orbitals, 250 occupied로 통과했다. 이번에는 이미 저장된 실제 계산 결과를 사용했으며 원격 Gaussian 계산을 새로 실행하지 않았다.
- Windows OpenGL GUI 관련 7개 시나리오 모두 통과 (init/cleanup 포함 9 passed): 선택 수소 메뉴·Undo/Redo, 기존 수소 제거, 자동 수소 그리기/삭제·Undo, 오비탈 캐시·별도 창, 위상/표현 렌더링, 오비탈/MEP 컨트롤, 화면 크기별 배치. 기존 도구막대 아이콘 검사에서 QLabel 구분선까지 아이콘을 요구하던 조건도 실제 버튼에만 적용하도록 바로잡았다.
- C드라이브에 설치한 portable에서 실제 FCHK를 열고 HOMO/LUMO를 각각 한 번 표시한 뒤 6회 교차 선택했다. 6회 모두 cube-memory-hit와 mesh-cache-hit, 추가 cube-generation-start 및 mesh-build-start 없음. display 처리 평균 3.14 ms, 최대 4.27 ms. 이 수치는 이벤트 처리 및 표면 전환 시간이며 모니터의 표시 지연 측정은 아니다.
- 별도 창 초기 frame 680×764, 위치 (1351,40), 작업 영역 2048×816 내 포함. 제목 막대로 이동 후 모든 주요 컨트롤 접근을 확인했다. 아래쪽 표현 설정은 세로 스크롤로 접근한다.
- 설치된 portable에서 탄소 원자를 새로 그리고 `선택 원자 수소 추가`로 CH4 생성, Ctrl+Z로 탄소만 복원, Ctrl+Shift+Z로 CH4 복원까지 직접 확인했다. 검증용 탭을 닫고 원래 FCHK로 돌아왔다.
- tools/package_portable.ps1 -ReuseMmRuntime 및 tools/install_portable.ps1 실행 완료. 설치된 manifest 1250개 파일 전부 SHA256 일치, 빌드·dist·설치 실행 파일 일치. 바탕화면 바로가기가 C드라이브의 별도 설치 Youngcraft.exe를 가리킨다.

Portable 실행 파일 SHA256: `1a4312713dfbbd5559c744189a09a68b124c0198f8d5c10bb7fb7ea0067f549f`.
