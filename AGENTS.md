# Youngcraft 작업 규칙

- 사용자가 별도로 지시하지 않으면 코드 수정 후 관련 검증을 완료하고, 변경을 커밋·푸시한 뒤 GitHub 기본 브랜치에 병합한다.
- 배포에는 수정 사항을 반영한 Windows portable 패키지를 사용한다. C드라이브의 `%LOCALAPPDATA%\Programs\Youngcraft Portable`에 소스와 별도로 설치하고, 바탕화면 Youngcraft 바로가기를 해당 폴더의 `Youngcraft.exe`에 연결한다.
- `tools/package_portable.ps1`로 패키징하고 `tools/install_portable.ps1`로 설치한다. MM 관련 코드가 바뀌지 않았다면 검증된 기존 MM runtime을 재사용할 수 있다.
- 개인 예제·임시 파일·관련 없는 변경을 임의로 삭제하지 않는다. 기존 미커밋 코드가 배포에 필요하면 별도 커밋으로 구분한다.
