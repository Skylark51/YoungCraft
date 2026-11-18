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

# The worker only constructs molecules and minimizes force fields. RDKit's
# notebook detection and optional database support otherwise pull in GUI/image
# libraries and SQLite. Optional NumPy frontends can bring an unused BLAS DLL;
# leave NumPy's core available for RDKit builds that need its array bindings.
$excludedModules = @(
    'rdkit.Chem.AllChem', 'rdkit.Chem.Draw', 'rdkit.Dbase',
    'rdkit.Chem.rdCoordGen', 'rdkit.Chem.rdMolInterchange',
    'sqlite3', 'pysqlite2', 'pyPgSQL',
    'IPython', 'matplotlib', 'PIL', 'tkinter', '_tkinter',
    'PyQt5', 'PyQt6', 'PySide2', 'PySide6',
    'numpy.f2py', 'numpy.testing', 'numpy.tests',
    'numpy.linalg', 'numpy.random', 'numpy.fft',
    'scipy', 'pandas'
)
$exclusionArgs = @()
foreach ($module in $excludedModules) { $exclusionArgs += @('--exclude-module', $module) }

# Keep the previous runtime available for rollback and size comparisons.
$runtimeDir = [IO.Path]::GetFullPath((Join-Path $OutputDir 'YoungcraftMM'))
if (-not $runtimeDir.StartsWith($projectPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "MM runtime 경로는 프로젝트 내부여야 합니다: $runtimeDir"
}
if (Test-Path -LiteralPath $runtimeDir) {
    $backupDir = $runtimeDir + '.backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
    Move-Item -LiteralPath $runtimeDir -Destination $backupDir
    Write-Host "Previous MM runtime preserved: $backupDir"
}
# rdForceFieldHelpers imports these Python bindings from native code, so static
# import analysis cannot discover them after removing AllChem.
& $Python -m PyInstaller --noconfirm --clean --onedir --console `
    --name YoungcraftMM `
    --hidden-import rdkit.ForceField `
    --distpath $OutputDir `
    --workpath (Join-Path $OutputDir 'work') `
    --specpath $OutputDir `
    @exclusionArgs `
    (Join-Path $ProjectRoot 'tools/mm_optimize.py')
if ($LASTEXITCODE -ne 0) { throw "PyInstaller MM runtime 빌드 실패: $LASTEXITCODE" }
$worker = Join-Path $OutputDir 'YoungcraftMM/YoungcraftMM.exe'
if (-not (Test-Path -LiteralPath $worker)) { throw "MM worker가 없습니다: $worker" }
Write-Output $worker
