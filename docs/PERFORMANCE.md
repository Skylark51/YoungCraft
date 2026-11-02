# 성능 기록

## 측정 환경

2026-09-16 기준 Windows x64, GCC 16.2.0 UCRT64, Qt 6.11.2, CMake 4.4.0, Ninja 1.13.2 Release 빌드에서 측정했다. 배포물은 `tools/package_portable.ps1`로 생성하며, 시스템 `PATH`에 MinGW/Qt 경로가 없어도 실행되는지 별도로 확인한다.

| 항목 | Release 측정값 | 측정 방법 |
|---|---:|---|
| CTest 전체 | 0.75 s | `ctest --test-dir build-release --output-on-failure` |
| cold start | 461 ms 중앙값 | portable EXE 5회, 인자 없이 첫 창 표시까지, 시스템 PATH 제거 |
| Gaussian open | 632 ms 중앙값 | `tests/data/mvp.out` 5회, 제목에 파일명 반영까지 |
| portable 배포 폴더 | 78.56 MiB | Qt/MinGW/ICU transitive DLL 포함, Debug·샘플·translation 제외 |
| portable ZIP | 31.76 MiB | `Compress-Archive -CompressionLevel Optimal` |
| 회전 FPS (100–300 atom) | 미측정 | 네이티브 화면 프레임 계측 후 기록 |
| 회전 FPS (1000 atom) | 미측정 | 동일 |
| frequency animation | 미측정 | 30초 재생 중 frame drop 계측 후 기록 |

portable 폴더가 큰 이유는 현재 Qt MinGW 빌드가 `libicudt78.dll`(약 33 MB)을 직접 요구하기 때문이다. 이 DLL을 빼면 시스템 PATH가 없는 PC에서 실행되지 않으므로, 1차 목표를 초과한 수치를 숨기지 않고 기록한다.

최종 ZIP을 임시 폴더에 풀어 시스템 PATH 없이 `mvp.out`을 인자로 실행하는 설치 시뮬레이션도 통과했다(창/파일명 표시 약 3초 이내).

## 설계상 성능 기준

- Gaussian parser는 `QFile::readLine()` 기반 streaming이며, 구조/section offset을 함께 저장합니다.
- 파싱은 `QThread` worker에서 실행합니다.
- sphere/cylinder 메시를 한 번만 만들고 frame 변경 시 위치 데이터만 갱신합니다.
- 렌더링 중 parser, 파일 I/O, isosurface 생성 작업을 호출하지 않습니다.

## 측정 명령

```powershell
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
powershell -ExecutionPolicy Bypass -File tools/package_portable.ps1
```
