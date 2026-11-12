[CmdletBinding()]
param(
    [string]$ProjectRoot = '',
    [string]$BuildDir = '',
    [string]$DistDir = '',
    [string]$ZipPath = '',
    [string]$Python = 'python',
    [string]$ApplicationPath = '',
    [switch]$ReuseMmRuntime
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) { $ProjectRoot = Split-Path -Parent $PSScriptRoot }
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $ProjectRoot 'build-current' }
if ([string]::IsNullOrWhiteSpace($DistDir)) { $DistDir = Join-Path $ProjectRoot 'dist' }
if ([string]::IsNullOrWhiteSpace($ZipPath)) { $ZipPath = Join-Path $ProjectRoot 'Youngcraft-portable.zip' }
$expectedDist = [IO.Path]::GetFullPath((Join-Path $ProjectRoot 'dist'))
$actualDist = [IO.Path]::GetFullPath($DistDir)
if (-not $actualDist.Equals($expectedDist, [StringComparison]::OrdinalIgnoreCase)) {
    throw "배포 대상은 프로젝트의 dist 폴더여야 합니다: $actualDist"
}

$cache = Join-Path $BuildDir 'CMakeCache.txt'
if (-not (Test-Path -LiteralPath $cache)) { throw "CMake build cache가 없습니다: $cache" }
$qtLine = Get-Content -LiteralPath $cache -Encoding UTF8 | Where-Object { $_ -match '^Qt6_DIR:PATH=' } | Select-Object -First 1
if (-not $qtLine) { throw 'Qt6_DIR을 CMakeCache에서 찾지 못했습니다.' }
$qtCmakeDir = $qtLine.Substring('Qt6_DIR:PATH='.Length)
$qtRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $qtCmakeDir))
$windeployqt = Join-Path $qtRoot 'bin/windeployqt.exe'
if (-not (Test-Path -LiteralPath $windeployqt)) { throw "windeployqt가 없습니다: $windeployqt" }

& cmake --build $BuildDir --config Release --target Youngcraft
if ($LASTEXITCODE -ne 0) { throw "Release 빌드 실패: $LASTEXITCODE" }
$application = if ($ApplicationPath) { [IO.Path]::GetFullPath($ApplicationPath) } else { Join-Path $ProjectRoot 'Youngcraft.exe' }
if (-not (Test-Path -LiteralPath $application)) { throw "실행 파일이 없습니다: $application" }

if (-not $ReuseMmRuntime) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'tools/build_mm_runtime.ps1') `
        -ProjectRoot $ProjectRoot -Python $Python
    if ($LASTEXITCODE -ne 0) { throw "MM runtime 빌드 실패: $LASTEXITCODE" }
}
$mmSource = Join-Path $ProjectRoot 'build-mm-runtime/YoungcraftMM'
if (-not (Test-Path -LiteralPath (Join-Path $mmSource 'YoungcraftMM.exe'))) {
    throw "MM worker가 없습니다: $mmSource"
}

# windeployqt is more reliable with an ASCII-only staging path than with this
# project's Unicode path. Copy the fully deployed result back into dist.
$stage = Join-Path $env:TEMP ('YoungcraftPortable-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage -Force | Out-Null
try {
    Copy-Item -LiteralPath $application -Destination (Join-Path $stage 'Youngcraft.exe') -Force
    & $windeployqt --release --compiler-runtime --no-translations --dir $stage (Join-Path $stage 'Youngcraft.exe')
    if ($LASTEXITCODE -ne 0) { throw "windeployqt 실패: $LASTEXITCODE" }
    # Some Qt SDKs do not discover MSVC's redistributables in windeployqt.
    # Use the permitted app-local CRT files so no VC installer is required.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsRoot) { throw 'MSVC redistributable installation was not found.' }
    $redistRoot = Join-Path $vsRoot 'VC/Redist/MSVC'
    $crtDirectories = Get-ChildItem -LiteralPath $redistRoot -Directory | ForEach-Object {
        $crtPath = Join-Path $_.FullName 'x64/Microsoft.VC143.CRT'
        if (Test-Path -LiteralPath $crtPath) { Get-Item -LiteralPath $crtPath }
    }
    $crtDirectory = $crtDirectories | Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $crtDirectory) { throw 'Microsoft.VC143.CRT redistributables were not found.' }
    Get-ChildItem -LiteralPath $crtDirectory.FullName -Filter '*.dll' | Copy-Item -Destination $stage -Force
    Copy-Item -LiteralPath $mmSource -Destination (Join-Path $stage 'mm_runtime') -Recurse -Force
    $xtbSource = Join-Path $ProjectRoot 'xtb_runtime'
    foreach ($required in @('xtb.exe', 'libiomp5md.dll', 'licenses/COPYING', 'licenses/COPYING.LESSER', 'licenses/Intel-Simplified-License.pdf', 'RUNTIME.txt')) {
        if (-not (Test-Path -LiteralPath (Join-Path $xtbSource $required))) { throw "xTB runtime 파일 누락: $required" }
    }
    # Only runtime files are required. Source tarballs under sources/ and
    # xtb-source-*.tar.gz are retained in the source tree for license compliance.
    $xtbDest = Join-Path $stage 'xtb_runtime'
    New-Item -ItemType Directory -Path $xtbDest -Force | Out-Null
    foreach ($item in @('xtb.exe', 'libiomp5md.dll', 'gfnff', 'share', 'licenses', 'RUNTIME.txt')) {
        $source = Join-Path $xtbSource $item
        if (-not (Test-Path -LiteralPath $source)) { throw "xTB runtime 구성요소 누락: $item" }
        Copy-Item -LiteralPath $source -Destination (Join-Path $xtbDest $item) -Recurse -Force
    }
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'lab_fragments') -Destination (Join-Path $stage 'lab_fragments') -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'README.md') -Destination (Join-Path $stage 'README.md') -Force
    New-Item -ItemType Directory -Path (Join-Path $stage 'resources') -Force | Out-Null
    # Built-in fragments are already embedded by resources/fragments.qrc.
    $licenses = Join-Path $stage 'licenses'
    New-Item -ItemType Directory -Path $licenses -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'licenses/Avogadro-fragments-BSD-3-Clause.txt') -Destination $licenses -Force
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'third_party/gemmi/LICENSE.txt') -Destination (Join-Path $licenses 'Gemmi-MPL-2.0.txt') -Force
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'third_party/gemmi/include/gemmi/third_party/tao/LICENSE') -Destination (Join-Path $licenses 'PEGTL-MIT.txt') -Force
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'third_party/gemmi/FAST_FLOAT_LICENSE_MIT.txt') -Destination (Join-Path $licenses 'fast_float-MIT.txt') -Force

    # The patcher verifies the payload before touching any installed files.
    # Paths in the manifest are normalized to '/' separators.
    $fileRecords = @(
        Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object {
            [ordered]@{
                path = $_.FullName.Substring($stage.Length + 1).Replace('\', '/')
                sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                size = $_.Length
            }
        }
    )
    $packageInfo = [ordered]@{ schema = 1; files = $fileRecords }
    $packageInfo | ConvertTo-Json -Depth 5 |
        Set-Content -LiteralPath (Join-Path $stage 'package-manifest.json') -Encoding UTF8

    if (Test-Path -LiteralPath $actualDist) {
        $resolved = (Resolve-Path -LiteralPath $actualDist).Path
        if (-not $resolved.Equals($expectedDist, [StringComparison]::OrdinalIgnoreCase)) {
            throw "dist 삭제 대상 경로가 예상과 다릅니다: $resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    New-Item -ItemType Directory -Path $actualDist -Force | Out-Null
    Get-ChildItem -LiteralPath $stage -Force | Copy-Item -Destination $actualDist -Recurse -Force
    Compress-Archive -Path (Join-Path $actualDist '*') -DestinationPath $ZipPath -CompressionLevel Optimal -Force
} finally {
    $resolvedStage = (Resolve-Path -LiteralPath $stage -ErrorAction SilentlyContinue).Path
    $tempRoot = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
    if ($resolvedStage -and $resolvedStage.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $resolvedStage -Recurse -Force
    }
}

$worker = Join-Path $actualDist 'mm_runtime/YoungcraftMM.exe'
if (-not (Test-Path -LiteralPath $worker)) { throw "배포본에 MM worker가 없습니다: $worker" }
Write-Host "portable 완료: $actualDist"
Write-Host "MM worker: $worker"
Write-Host "xTB runtime: $(Join-Path $actualDist 'xtb_runtime/xtb.exe')"
Write-Host "ZIP: $ZipPath"
