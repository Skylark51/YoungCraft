[CmdletBinding()]
param(
    [string]$Source = (Join-Path (Split-Path -Parent $PSScriptRoot) 'dist'),
    [string]$Destination = (Join-Path $env:LOCALAPPDATA 'Programs/Youngcraft Portable')
)
$ErrorActionPreference = 'Stop'
$Source = (Resolve-Path -LiteralPath $Source).Path
$Destination = [IO.Path]::GetFullPath($Destination).TrimEnd('\')
if ([IO.Path]::GetPathRoot($Destination) -ne 'C:\') { throw 'Portable installation must be on C:.' }
if ($Destination.StartsWith($Source + '\', [StringComparison]::OrdinalIgnoreCase) -or
    $Source.StartsWith($Destination + '\', [StringComparison]::OrdinalIgnoreCase) -or
    $Source.Equals($Destination, [StringComparison]::OrdinalIgnoreCase)) { throw 'Source and installation must be separate.' }
$manifest = Get-Content -LiteralPath (Join-Path $Source 'package-manifest.json') -Raw | ConvertFrom-Json
foreach ($entry in $manifest.files) {
    $path = [IO.Path]::GetFullPath((Join-Path $Source $entry.path))
    if (-not $path.StartsWith($Source + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid manifest path.' }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.sha256) { throw "Package checksum mismatch: $($entry.path)" }
}
$executable = Join-Path $Destination 'Youngcraft.exe'
if (Get-Process Youngcraft -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $executable }) {
    throw 'Close the installed Youngcraft before replacing its files.'
}
$stage = $Destination + '.staging-' + [Guid]::NewGuid().ToString('N')
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Get-ChildItem -LiteralPath $Source -Force | Copy-Item -Destination $stage -Recurse -Force
if (-not (Test-Path -LiteralPath (Join-Path $stage 'Youngcraft.exe'))) { throw 'Package has no Youngcraft.exe.' }
$backup = $Destination + '.backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
if (Test-Path -LiteralPath $Destination) { Move-Item -LiteralPath $Destination -Destination $backup }
try { Move-Item -LiteralPath $stage -Destination $Destination }
catch {
    if (Test-Path -LiteralPath $backup) { Move-Item -LiteralPath $backup -Destination $Destination }
    throw
}
$desktop = [Environment]::GetFolderPath('Desktop')
$shell = New-Object -ComObject WScript.Shell
$shortcuts = @(Get-ChildItem -LiteralPath $desktop -Filter '*.lnk' | Where-Object { $_.BaseName -match '^Youngcraft($|\s|-)' })
if ($shortcuts.Count -eq 0) { $shortcutPaths = @(Join-Path $desktop 'Youngcraft.lnk') }
else { $shortcutPaths = @($shortcuts.FullName) }
foreach ($path in $shortcutPaths) {
    $shortcut = $shell.CreateShortcut($path)
    $shortcut.TargetPath = $executable
    $shortcut.WorkingDirectory = $Destination
    $shortcut.IconLocation = "$executable,0"
    $shortcut.Save()
}
if ((Get-FileHash -LiteralPath $executable).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Source 'Youngcraft.exe')).Hash) { throw 'Installed executable mismatch.' }
Write-Output "Installed: $executable"
Write-Output "Desktop shortcuts: $($shortcutPaths -join ', ')"
