[CmdletBinding()]
param(
    [string]$PackageZip = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Youngcraft-portable.zip')
)
$ErrorActionPreference = 'Stop'
$PackageZip = (Resolve-Path -LiteralPath $PackageZip).Path
$desktop = [Environment]::GetFolderPath('Desktop')
if (-not $desktop) { throw 'Desktop folder was not found.' }
$compiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) { throw "C# compiler missing: $compiler" }
$temporaryExe = Join-Path $env:TEMP ('Youngcraft-Patch-' + [Guid]::NewGuid().ToString('N') + '.exe')
try {
    & $compiler /nologo /target:exe /optimize+ "/out:$temporaryExe" "/resource:$PSScriptRoot/patch_youngcraft.ps1,YoungcraftPatchScript" (Join-Path $PSScriptRoot 'YoungcraftPatcher.cs')
    if ($LASTEXITCODE -ne 0) { throw 'Patcher compilation failed.' }
    $desktopZip = Join-Path $desktop 'Youngcraft-portable.zip'
    if (-not $PackageZip.Equals($desktopZip, [StringComparison]::OrdinalIgnoreCase)) {
        Copy-Item -LiteralPath $PackageZip -Destination $desktopZip -Force
    }
    Copy-Item -LiteralPath $temporaryExe -Destination (Join-Path $desktop 'Youngcraft-Patch.exe') -Force
    $desktopStandalone = Join-Path $desktop 'Youngcraft-Standalone.exe'
    & (Join-Path $PSScriptRoot 'create_standalone.ps1') -PackageZip $PackageZip -OutputPath $desktopStandalone

    # Refresh existing copies of the single-file launcher published as Youngcraft.exe.
    $desktopAlias = Join-Path $desktop 'Youngcraft.exe'
    if (Test-Path -LiteralPath $desktopAlias -PathType Leaf) {
        $aliasFile = Get-Item -LiteralPath $desktopAlias -Force
        if (-not ($aliasFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            $aliasAssembly = try { [Reflection.AssemblyName]::GetAssemblyName($desktopAlias).Name }
                catch [BadImageFormatException] { $null }
            if ($aliasAssembly -eq 'Youngcraft-Standalone') {
                $stagedAlias = Join-Path $desktop ('.Youngcraft-' + [Guid]::NewGuid().ToString('N') + '.tmp')
                try {
                    [IO.File]::Copy($desktopStandalone, $stagedAlias)
                    [IO.File]::Replace($stagedAlias, $desktopAlias, [NullString]::Value)
                    Write-Output "Desktop single-file launcher: $desktopAlias"
                } finally {
                    if (Test-Path -LiteralPath $stagedAlias) { Remove-Item -LiteralPath $stagedAlias -Force }
                }
            }
        }
    }
    Write-Output "Desktop patch: $(Join-Path $desktop 'Youngcraft-Patch.exe')"
    Write-Output "Desktop package: $desktopZip"
} finally {
    if (Test-Path -LiteralPath $temporaryExe) { Remove-Item -LiteralPath $temporaryExe -Force }
}
