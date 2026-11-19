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
    $desktopLauncher = Join-Path $desktop 'YoungCraft.exe'
    & (Join-Path $PSScriptRoot 'create_standalone.ps1') -PackageZip $PackageZip -OutputPath $desktopLauncher

    $legacyStandalone = Join-Path $desktop 'Youngcraft-Standalone.exe'
    if (Test-Path -LiteralPath $legacyStandalone -PathType Leaf) {
        $legacyFile = Get-Item -LiteralPath $legacyStandalone -Force
        if (-not ($legacyFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            $legacyAssembly = try { [Reflection.AssemblyName]::GetAssemblyName($legacyStandalone).Name }
                catch [BadImageFormatException] { $null }
            if ($legacyAssembly -eq 'Youngcraft-Standalone') { Remove-Item -LiteralPath $legacyStandalone -Force }
        }
    }
    $oldShortcut = Join-Path $desktop 'Youngcraft.lnk'
    if (Test-Path -LiteralPath $oldShortcut -PathType Leaf) {
        $shell = New-Object -ComObject WScript.Shell
        $target = $shell.CreateShortcut($oldShortcut).TargetPath
        $installed = Join-Path $env:LOCALAPPDATA 'Programs/Youngcraft Portable/Youngcraft.exe'
        if ($target -in @($installed, $legacyStandalone, $desktopLauncher)) {
            Remove-Item -LiteralPath $oldShortcut -Force
        }
    }
    Write-Output "Desktop patch: $(Join-Path $desktop 'Youngcraft-Patch.exe')"
    Write-Output "Desktop package: $desktopZip"
} finally {
    if (Test-Path -LiteralPath $temporaryExe) { Remove-Item -LiteralPath $temporaryExe -Force }
}
