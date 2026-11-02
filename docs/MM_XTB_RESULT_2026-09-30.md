# Youngcraft MM / xTB 작업 결과 — 2026-09-30

1. **48 iteration 원인**
   제보 구조가 없어 제보 상황의 정확한 원인은 미확인이다. 확보한 원래 portable worker로 2-carbon 테스트 구조를 실행하면 `ready 0 → step 8, converged=false → done 48, converged=true`가 나온다. 해당 종료는 RDKit `Minimize()` 반환값 0에 따른 정상 수렴 종료다. 48은 기존 worker가 요청한 iteration budget인 8+40이며, 수렴한 경우 실제 수행 횟수의 상한이다. worker는 `iteration_exact=false`로 이를 알린다. 기존 GUI에도 정상 가짜 worker를 연결해 `0,8,48,56,...120`을 보냈을 때 120까지 진행했다. 따라서 기존 소스에 48 고정 제한 또는 `Minimize()==1`을 실패로 처리하는 오류가 있었다고 단정하지 않는다. 초기 테스트 환경변수 주입 오류로 실제 worker를 fake로 잘못 판단한 기록은 최종 판단에서 제외했다.
   증거: `docs/mm-xtb-baseline/ethane-original-worker.json`, `docs/mm-xtb-baseline/verified-fake.txt`.

2. **MM control flow 변경**
   `Idle, Starting, Running, Paused, Stopping, Finished, Failed`를 명시적으로 구분한다. `ready`에서 Running으로 이동하고, 각 step에서 실제 좌표/model, energy/gradient 표시 및 iteration을 갱신한다. Running이고 미수렴이며 limit 미만이면 응답 처리에서 다음 chunk를 요청한다. 요청당 한 개의 in-flight 명령만 허용한다. 애니메이션 타이머/프레임 queue에서 계산 scheduling을 분리했다. 실제 좌표를 즉시 표시하고 기본 chunk는 8, 다음 요청 전 UI 간격은 20ms다. 수렴/limit/Stop/실제 오류에서만 종료한다. UI thread의 `waitForFinished()`를 stop 경로에서 제거했다. 최종 구조는 실제 worker 결과다. 48에 대한 예외 처리나 계산 없는 보간은 없다.

3. **MM regression tests**
   GUI와 실제 QProcess에 native mock worker를 연결하여 `0,8,48,56,64,72,80,88,96,104,112,120` 전체 응답/graph sequence와 최종 좌표를 검증했다. 50회 이상 진행, 정확한 120 limit, pause 중 새 요청 없음, resume, Stop, 수렴 시 64회 종료가 통과했다. 실제 bundled worker를 외부 Python 없는 환경에서 실행한 결과도 `0,8,16,...48,56,...120`이고 마지막 `converged=false`다.
   증거: `tests/mm/mock_mm_worker.cpp`, `tests/smoke/test_smoke.cpp`, `docs/mm-xtb-gui-tests.txt`, `docs/portable-xtb-tests.txt`.

4. **xTB runtime version**
   공식 Windows `6.7.1pre (5071a88)`, 2024-07-23 build. 원본 ZIP SHA256은 `043e578da4a7e114a4d584972959a875e3ffb9f2767a86723b95aa6719d28d9c`로 확인했다.
   공식 배포: https://github.com/grimme-lab/xtb/releases/tag/v6.7.1
   `xtb.exe`, `libiomp5md.dll`, parameter files, LGPL/GPL 및 Intel license, upstream dependency notices와 source archives를 `xtb_runtime`에 포함했다. 정확한 dependency build revision이 공개되지 않은 upstream HEAD dependency는 sources/manifest.json에 선택 근거와 한계를 명시했다. 별도 libxtb linking은 사용하지 않는다.

5. **구현한 xTB 기능**
   도구 메뉴의 “빠른 양자화학 계산 (xTB)...”, nonmodal 계산 화면, GFN2/GFN1 Single Point (+gradient) 및 Geometry Optimization, integer charge, multiplicity→UHF=M−1, Gas/ALPB, water/acetonitrile/benzene/toluene/thf/ch2cl2/chcl3/dmf/dmso/methanol, loose/normal/tight, cycles 10~5000 (기본 500), CPU 1/2/4/8/Auto (기본 2, Auto 최대 4), absolute Eh/첫 frame 기준 relative kcal/mol graph, energy/gradient/gap 결과, 출력 보기, 현재 구조 적용/새 구조/XYZ 저장, 미수렴 마지막 구조 확인, 비동기 terminate→1500ms 후 필요 시 kill. 계산별 UUID temp directory를 사용한다. 적용 후 기존 Gaussian input 생성에 동일한 current geometry가 전달된다. Gaussian 코드는 변경하지 않았다.

6. **실행한 command 예**
   `xtb.exe input.xyz --gfn 2 --chrg 0 --uhf 0 --grad`
   `xtb.exe input.xyz --gfn 2 --chrg 0 --uhf 0 --opt normal --cycles 500`
   `xtb.exe input.xyz --gfn 2 --chrg 1 --uhf 0 --grad`
   `xtb.exe input.xyz --gfn 2 --chrg 0 --uhf 1 --grad`
   `xtb.exe input.xyz --gfn 1 --chrg 0 --uhf 0 --grad --alpb water`
   `xtb.exe input.xyz --gfn 2 --chrg 0 --uhf 0 --opt normal --cycles 10`
   shell 없이 QProcess program+QStringList를 전달한다. child environment에만 XTBPATH, OMP_NUM_THREADS, MKL_NUM_THREADS 등을 설정한다. 위 charged singlet은 hydronium, open-shell은 NO다.

7. **trajectory 실시간 표시**
   50ms QTimer가 xtbopt.log를 마지막 byte offset부터 읽고, 불완전한 line/frame을 parser buffer에 유지한다. 원자 수·원소 순서·finite coordinates를 검증한 완전한 frame만 emit한다. 새 frame마다 좌표, step, Eh energy, gnorm 및 graph를 갱신한다. 실제 dodecane 계산에서 process가 Running인 동안 새 frame이 반영되는 것을 자동 검증했다. 성공 시 xtbopt.xyz를 우선 사용한다. 실제 Windows build는 10-cycle 미수렴에서 NOT_CONVERGED 파일을 쓰지 않고 stdout에 FAILED TO CONVERGE를 출력했으며, 이를 미수렴으로 처리한다. NOT_CONVERGED marker 자체도 mock test로 검증했다.

8. **Release build**
   MSVC 2022 x64, Qt 6.8.3 Release 성공. CTest 5 suites 전부 통과. xTB native tests는 14 PASS, Windows GUI targeted tests는 7 PASS (init/cleanup 포함). 전체 CTest는 structure/parser/model/gui/xtb를 포함한다. 기존 CIF/render/TD-DFT/PES/frequency/SI/geometry editor/Gaussian parser 구현은 수정하지 않았다.
   로그: `docs/release-build.txt`, `docs/ctest-release.txt`, `docs/xtb-tests.txt`, `docs/mm-xtb-gui-tests.txt`.

9. **portable 검증**
   `Youngcraft-portable.zip` 재생성, 크기 98,045,112 bytes.
   SHA256: `1f9e49de46658a2717f06ed0f1c4de03fdbfcd056e0743a8bd4824208fce1cdb`.
   ZIP을 `Youngcraft portable 한글 공백 <UUID>`에 풀고 PATH를 Windows/System32 및 Windows만으로 제한했다. 외부 python/conda/xtb executable이 없음을 확인한 상태에서 neutral/charged/open-shell SP, GFN1 ALPB, 20 method-solvent 조합, GFN2 optimization, 실제 live update 및 미수렴, bundled MM 120회 진행과 portable GUI startup을 확인했다. xTB 테스트 9 PASS (init/cleanup 포함). Qt가 필요한 app-local MSVC redistributable DLL도 포함했다.
   검증 script: `tools/verify_portable_xtb.ps1`; 결과: `docs/portable-xtb-tests.txt`.

10. **남은 제한사항**
   원본 제보 구조/실행 상황을 확보하지 못했으므로 제보의 48회 정지 원인은 미확인이다. 검증된 48 종료는 수렴 응답이고, 미수렴 sequence는 120까지 진행한다. RDKit Python API가 실제 수렴 iteration count를 제공하지 않아 수렴 시 표시 iteration은 chunk budget 상한이다. 완전히 새 Windows VM에서의 검증은 수행하지 않았으며, 외부 runtime PATH를 차단한 현 Windows host에서 검증했다. GFN0/GFN-FF/spinpol/tblite/MD/frequency/CREST는 이번 UI에 추가하지 않았다. xTB는 고정밀 DFT 또는 정밀 spin-state energy 평가의 대체가 아니다. 일부 upstream dependency의 정확한 build revision은 공식 binary 배포물에서 확인되지 않는다.
