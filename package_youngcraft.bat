@echo off
setlocal
pushd "%~dp0"

echo [Youngcraft] Building and packaging the portable release...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\package_portable.ps1" -ProjectRoot "%~dp0."
set "PACKAGE_EXIT_CODE=%ERRORLEVEL%"

echo.
if "%PACKAGE_EXIT_CODE%"=="0" (
    echo [Youngcraft] Packaging completed successfully.
    echo EXE: %~dp0dist\Youngcraft.exe
    echo ZIP: %~dp0Youngcraft-portable.zip
) else (
    echo [Youngcraft] Packaging failed with exit code %PACKAGE_EXIT_CODE%.
)

popd
pause
exit /b %PACKAGE_EXIT_CODE%
