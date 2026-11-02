@echo off
chcp 65001 >nul
set VSLANG=1033
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cmake -S . -B build-editor-final -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -DQt6_DIR="C:/Users/UNIST/Documents/ChatGPT/Youngcraft/qt-sdk/lib/cmake/Qt6"
if errorlevel 1 exit /b %errorlevel%
cmake --build build-editor-final --parallel 6
