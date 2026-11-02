[CmdletBinding()]
param(
    [string]$ProjectRoot = '',
    [string]$Python = 'python',
    [string]$OutputDir = ''
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) { $ProjectRoot = Split-Path -Parent $PSScriptRoot }
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $ProjectRoot 'build-mm-runtime'
}
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
$OutputDir = [IO.Path]::GetFullPath($OutputDir)
$projectPrefix = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\') + '\'
if (-not $OutputDir.StartsWith($projectPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "MM runtime 출력 경로는 프로젝트 내부여야 합니다: $OutputDir"
}
& $Python -c 'import rdkit, PyInstaller'
if ($LASTEXITCODE -ne 0) { throw 'MM runtime 빌드에는 Python, RDKit, PyInstaller가 필요합니다.' }
& $Python -m PyInstaller --noconfirm --clean --onedir --console `
    --name YoungcraftMM `
    --distpath $OutputDir `
    --workpath (Join-Path $OutputDir 'work') `
    --specpath $OutputDir `
    (Join-Path $ProjectRoot 'tools/mm_optimize.py')
if ($LASTEXITCODE -ne 0) { throw "PyInstaller MM runtime 빌드 실패: $LASTEXITCODE" }
$worker = Join-Path $OutputDir 'YoungcraftMM/YoungcraftMM.exe'
if (-not (Test-Path -LiteralPath $worker)) { throw "MM worker가 없습니다: $worker" }
Write-Output $worker
