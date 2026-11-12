# 모듈별 작업 범위

각 모듈의 소스 목록은 해당 폴더의 `CMakeLists.txt`에서 관리한다. 루트 CMake는 빌드 순서, 실행 파일과 배포 설정을 관리한다.

| 경로 | CMake 타깃 | 기능과 수정 기준 | 검증 |
| --- | --- | --- | --- |
| `src/model` | `youngcraft_model` | 원자·결합, 좌표 편집, 프래그먼트, NBO 데이터. 공용 데이터 변경은 파서·계산·렌더링 작업자와 합의한다. | `structure_tools`, `model_atoms_bonds`, `molecular_editor` |
| `src/io` | `youngcraft_parsers`, `youngcraft_io` | 형식별 파싱과 파일 형식 분기. 파서는 모델을 사용하고, 파일 분기에서 체크포인트 변환을 호출한다. 새 형식은 해당 파서와 입력 예제를 함께 다룬다. | `parser_gaussian`, `parser_orca`, `structure_files`, `orbital_study` |
| `src/compute` | `youngcraft_compute` | Gaussian 로컬·원격 작업, xTB 실행과 제약. 명령 구성, 작업 취소, 결과 처리를 이 경계에서 수정한다. | `xtb_runner`, `orbital_study` |
| `src/analysis` | `youngcraft_analysis` | 분자 데이터에서 분석값과 그래프용 수치를 계산한다. 계산식과 단위를 여기서 관리한다. | `gui_smoke` |
| `src/animation` | `youngcraft_animation` | 애니메이션 상태와 반응 경로 타임라인. 프레임 순서와 반응 좌표를 여기서 관리한다. | `mechanism_animation`, `model_atoms_bonds` |
| `src/settings` | `youngcraft_settings` | 스타일 저장과 공용 배경 패턴. 설정 키를 바꿀 때 기존 저장값의 호환성을 확인한다. | `model_atoms_bonds`, `gui_smoke` |
| `src/render` | `youngcraft_isosurface`, `youngcraft_viewport` | 등가면 메시 생성과 OpenGL 뷰포트. 메시 계산, 카메라, 셰이더 변경은 해당 구현 파일에서 처리한다. | `orbital_study`, `gui_smoke`, `render_quality_preview` 빌드 |
| `src/export` | `youngcraft_export` | 이미지·애니메이션 내보내기와 반응 경로 캡처. 출력 크기, 포맷, 클립보드 동작을 여기서 관리한다. | `mechanism_animation`, `model_atoms_bonds`, `gui_smoke` |
| `src/ui` | `youngcraft_ui` | 그래프, 사이드바, 설정 및 계산 대화상자. 입력·표시는 이 모듈에서 처리하고 계산은 담당 모듈에 위임한다. | `gui_smoke` |
| `src/app` | `youngcraft_app`, `Youngcraft` | 앱 진입점, 창과 탭, 기능 연결, 공용 리소스. MM·xTB 테스트 훅이 있는 연결 코드는 앱과 스모크 테스트에서 각각 컴파일한다. | `gui_smoke`, `single_instance` |

## 병렬 작업 규칙

1. 작업마다 브랜치와 빌드 폴더를 분리한다. 모듈 내부 변경은 해당 폴더와 관련 테스트를 한 PR에 담는다.
2. 공용 헤더나 타깃 의존성을 바꿀 때는 영향받는 모듈과 먼저 인터페이스를 합의한다. 의존성은 각 모듈의 `target_link_libraries`에 선언하고 순환 참조를 피한다.
3. 병합 전 해당 모듈의 테스트를 실행한다. 공용 인터페이스나 모듈 간 연결 변경은 전체 CTest도 실행한다. MM 작업은 `src/app/mainwindow/MainWindow_MM.cpp`와 `tools/mm_optimize.py`를 함께 검증한다.

빌드 설정을 완료한 뒤 xTB만 검증하는 예시다. Visual Studio 생성기를 사용하면 빌드에 `--config Release`, CTest에 `-C Release`를 추가한다.

```powershell
cmake --build build --target test_xtb --parallel
ctest --test-dir build --output-on-failure -R '^xtb_runner$'
```
