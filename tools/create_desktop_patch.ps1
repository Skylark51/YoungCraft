[CmdletBinding()]
param(
    [string]$PackageZip = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Youngcraft-portable.zip')
)
$ErrorActionPreference = 'Stop'
$PackageZip = (Resolve-Path -LiteralPath $PackageZip).Path
$desktop = [Environment]::GetFolderPath('Desktop')
if (-not $desktop) { throw 'Desktop folder was not found.' }
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
