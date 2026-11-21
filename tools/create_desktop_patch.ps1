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
# Remove only recognized Youngcraft shortcuts after the single EXE is published.
$shell = New-Object -ComObject WScript.Shell
$installed = Join-Path $env:LOCALAPPDATA 'Programs/Youngcraft Portable/Youngcraft.exe'
foreach ($shortcut in Get-ChildItem -LiteralPath $desktop -Filter '*.lnk' -File) {
    if ($shortcut.BaseName -notmatch '^Youngcraft($|\s|-)') { continue }
    $target = $shell.CreateShortcut($shortcut.FullName).TargetPath
    if ($target -in @($installed, $legacyStandalone, $desktopLauncher)) {
        Remove-Item -LiteralPath $shortcut.FullName -Force
    }
}
