[CmdletBinding()]
param([string]$ProjectRoot = '', [string]$BuildDir = '')
$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) { $ProjectRoot = Split-Path -Parent $PSScriptRoot }
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
if (-not $BuildDir) { $BuildDir = Join-Path $ProjectRoot 'build-current/Release' }
$verifyRoot = Join-Path $env:TEMP ('Youngcraft portable 한글 공백 ' + [Guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath (Join-Path $ProjectRoot 'Youngcraft-portable.zip') -DestinationPath $verifyRoot
# The verification harness is added only to this temporary extraction, not the ZIP.
Copy-Item -LiteralPath (Join-Path $BuildDir 'test_xtb.exe') -Destination $verifyRoot
Copy-Item -LiteralPath (Join-Path $BuildDir 'Qt6Test.dll') -Destination $verifyRoot
New-Item -ItemType Directory -Path (Join-Path $verifyRoot 'tests/xtb') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'tests/xtb/dodecane.xyz') -Destination (Join-Path $verifyRoot 'tests/xtb')
$savedPath = $env:PATH
$savedXtbPath = $env:XTBPATH
$savedXtbHome = $env:XTBHOME
$savedRuntime = $env:YOUNGCRAFT_XTB_RUNTIME
$report = Join-Path $ProjectRoot 'docs/portable-xtb-tests.txt'
Push-Location -LiteralPath $verifyRoot
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    Remove-Item Env:XTBPATH,Env:XTBHOME -ErrorAction SilentlyContinue
    $env:YOUNGCRAFT_XTB_RUNTIME = Join-Path $verifyRoot 'xtb_runtime'
    if (Get-Command python,conda,xtb -ErrorAction SilentlyContinue) { throw 'External runtime found on test PATH' }
    foreach ($dll in @('msvcp140.dll','msvcp140_1.dll','vcruntime140.dll','vcruntime140_1.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $verifyRoot $dll))) { throw "App-local CRT missing: $dll" }
    }
    & (Join-Path $verifyRoot 'test_xtb.exe') neutralSP chargedSP openShellSP gfn1ALPB allBundledSolvents realOptimization realLiveNonconvergence -o "$report,txt"
    if ($LASTEXITCODE -ne 0) { throw "Portable xTB tests failed: $LASTEXITCODE" }
    $workerInfo = New-Object System.Diagnostics.ProcessStartInfo
    $workerInfo.FileName = Join-Path $verifyRoot 'mm_runtime/YoungcraftMM.exe'
    $workerInfo.WorkingDirectory = $verifyRoot
    $workerInfo.UseShellExecute = $false
    $workerInfo.CreateNoWindow = $true
    $workerInfo.RedirectStandardInput = $true
    $workerInfo.RedirectStandardOutput = $true
    $workerInfo.RedirectStandardError = $true
    $worker = New-Object System.Diagnostics.Process
    $worker.StartInfo = $workerInfo
    $steps = @()
    try {
        $worker.Start() | Out-Null
        $worker.StandardInput.WriteLine((Get-Content -LiteralPath (Join-Path $ProjectRoot 'tests/mm/portable-input.json') -Raw))
        while ($true) {
            $lineTask = $worker.StandardOutput.ReadLineAsync()
            if (-not $lineTask.Wait(10000)) { throw 'Portable MM worker response timed out.' }
            if (-not $lineTask.Result) { throw 'Portable MM worker closed stdout unexpectedly.' }
            $response = $lineTask.Result | ConvertFrom-Json
            if ($response.type -eq 'error') { throw "Portable MM worker failed: $($response.message)" }
            $steps += $response.iteration
            if ($response.type -eq 'done') { break }
            $worker.StandardInput.WriteLine('chunk')
        }
        if ($response.iteration -ne 120 -or $response.converged) { throw 'Portable MM limit test did not reach 120 unconverged iterations.' }
        Add-Content -LiteralPath $report -Encoding UTF8 -Value "Portable MM worker: PASS; iterations: $($steps -join ','); external Python unavailable."
    } finally {
        if (-not $worker.HasExited) { $worker.Kill(); $worker.WaitForExit(3000) | Out-Null }
        $worker.Dispose()
    }
    $application = Start-Process -FilePath (Join-Path $verifyRoot 'Youngcraft.exe') -ArgumentList @('-platform','windows') -WorkingDirectory $verifyRoot -WindowStyle Hidden -PassThru
    try {
        Start-Sleep -Milliseconds 1200
        if ($application.HasExited) { throw "Portable GUI exited during startup: $($application.ExitCode)" }
        Add-Content -LiteralPath $report -Encoding UTF8 -Value "Portable GUI startup: PASS (Windows platform); no external runtime PATH."
    } finally {
        if (-not $application.HasExited) { $application.Kill(); $application.WaitForExit(3000) | Out-Null }
        $application.Dispose()
    }
    Add-Content -LiteralPath $report -Encoding UTF8 -Value "Extraction: $verifyRoot"
    Add-Content -LiteralPath $report -Encoding UTF8 -Value "PATH: $env:PATH"
    Write-Output "Portable tests passed: $verifyRoot"
} finally {
    Pop-Location
    $env:PATH = $savedPath
    $env:XTBPATH = $savedXtbPath
    $env:XTBHOME = $savedXtbHome
    $env:YOUNGCRAFT_XTB_RUNTIME = $savedRuntime
}
