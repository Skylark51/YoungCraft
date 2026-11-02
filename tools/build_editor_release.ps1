[CmdletBinding()]
param([string]$QtRoot = '', [string]$BuildDir = 'build-editor-final')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
if (-not $QtRoot) {
    $cache = Join-Path $project "$BuildDir/CMakeCache.txt"
    if (-not (Test-Path -LiteralPath $cache)) { $cache = Join-Path $project 'build-current/CMakeCache.txt' }
    $entry = Get-Content -LiteralPath $cache -Encoding UTF8 | Where-Object { $_ -match '^Qt6_DIR:[^=]+=' } | Select-Object -First 1
    if (-not $entry) { throw 'Supply -QtRoot with a Qt6 MSVC SDK directory.' }
    $QtRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent ($entry -split '=', 2)[1]))
}
$QtRoot = (Resolve-Path -LiteralPath $QtRoot).Path
if (-not (Test-Path -LiteralPath (Join-Path $QtRoot 'lib/cmake/Qt6/Qt6Config.cmake'))) { throw 'Qt6 SDK not found.' }
$buildPath = [IO.Path]::GetFullPath((Join-Path $project $BuildDir))
if (-not $buildPath.StartsWith($project.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'BuildDir must be inside the project.' }
# Shorten long SDK paths for Ninja's Windows path limit. This is a build-only junction.
if ($QtRoot.Length -gt 100) {
    $short = Join-Path $project '.build/qt-sdk'
    New-Item -ItemType Directory -Path (Split-Path -Parent $short) -Force | Out-Null
    if (-not (Test-Path -LiteralPath $short)) { New-Item -ItemType Junction -Path $short -Target $QtRoot | Out-Null }
    $QtRoot = $short
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'MSVC C++ Build Tools not found.' }
$vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$ninja = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
$script = Join-Path $env:TEMP ('YoungcraftRelease-' + [Guid]::NewGuid().ToString('N') + '.cmd')
$commands = @"
@echo off
chcp 65001 >nul
call "$vcvars" >nul
cmake -S . -B "$BuildDir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="$ninja" -DQt6_DIR:PATH="$QtRoot/lib/cmake/Qt6"
if errorlevel 1 exit /b %errorlevel%
cmake --build "$BuildDir" --parallel 6
"@
[IO.File]::WriteAllText($script, $commands, [Text.UTF8Encoding]::new($false))
Push-Location -LiteralPath $project
try { & cmd.exe /c $script; if ($LASTEXITCODE -ne 0) { throw "Release build failed: $LASTEXITCODE" } }
finally { Pop-Location; Remove-Item -LiteralPath $script -Force }
