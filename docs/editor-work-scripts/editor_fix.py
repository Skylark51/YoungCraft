from pathlib import Path
p=Path('src/model/FragmentLibrary.cpp');s=p.read_text(encoding='utf-8').replace('a.autoHydrogen=false;map[i]', 'a.autoHydrogen=false;a.autoHydrogenLocked=true;map[i]');p.write_text(s,encoding='utf-8')
p=Path('src/model/MoleculeEditController.cpp');s=p.read_text(encoding='utf-8').replace('a.autoHydrogen=false;a.colorOverride={};','a.autoHydrogen=false;a.autoHydrogenLocked=false;a.colorOverride={};');p.write_text(s,encoding='utf-8')
# Fresh Ninja configuration with the installed MSVC Qt SDK.
cache=Path('build-current/CMakeCache.txt').read_text(encoding='utf-8')
qt=next(x.split('=',1)[1] for x in cache.splitlines() if x.startswith('Qt6_DIR:PATH='))
script='@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
script+='cmake -S . -B build-editor-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -DQt6_DIR="'+qt+'"\nif errorlevel 1 exit /b %errorlevel%\ncmake --build build-editor-release --parallel 6\n'
Path('tools/editor_release_build.cmd').write_text(script,encoding='utf-8')
