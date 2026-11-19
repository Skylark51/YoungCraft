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
# Carry forward user data without replacing unchanged bundled fragments with
# their older versions. Keep the entire previous installation as a backup too.
if (Test-Path -LiteralPath $Destination) {
    $previousHashes = @{}
    $previousManifest = Join-Path $Destination 'package-manifest.json'
    if (Test-Path -LiteralPath $previousManifest) {
        $previous = Get-Content -LiteralPath $previousManifest -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($entry in $previous.files) { $previousHashes[[string]$entry.path] = [string]$entry.sha256 }
    }
    $server = Join-Path $Destination 'youngcraft-server.json'
    if (Test-Path -LiteralPath $server) { Copy-Item -LiteralPath $server -Destination (Join-Path $stage 'youngcraft-server.json') -Force }
    $fragments = Join-Path $Destination 'lab_fragments'
    if (Test-Path -LiteralPath $fragments) {
        foreach ($file in Get-ChildItem -LiteralPath $fragments -Recurse -File) {
            $relative = $file.FullName.Substring($Destination.Length + 1).Replace('\', '/')
            if ($previousHashes.ContainsKey($relative) -and (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -eq $previousHashes[$relative]) { continue }
            $targetFile = Join-Path $stage $relative
            New-Item -ItemType Directory -Path (Split-Path -Parent $targetFile) -Force | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $targetFile -Force
        }
    }
}
$backup = $Destination + '.backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
if (Test-Path -LiteralPath $Destination) { Move-Item -LiteralPath $Destination -Destination $backup }
try { Move-Item -LiteralPath $stage -Destination $Destination }
catch {
    if (Test-Path -LiteralPath $backup) { Move-Item -LiteralPath $backup -Destination $Destination }
    throw
}
$desktop = [Environment]::GetFolderPath('Desktop')
$shell = New-Object -ComObject WScript.Shell
$shortcutPaths = @(Get-ChildItem -LiteralPath $desktop -Filter '*.lnk' |
    Where-Object { $_.BaseName -match '^Youngcraft(\s|-)' } |
    Select-Object -ExpandProperty FullName)
foreach ($path in $shortcutPaths) {
    $shortcut = $shell.CreateShortcut($path)
    $shortcut.TargetPath = $executable
    $shortcut.WorkingDirectory = $Destination
    $shortcut.IconLocation = "$executable,0"
    $shortcut.Save()
}
if ((Get-FileHash -LiteralPath $executable).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Source 'Youngcraft.exe')).Hash) { throw 'Installed executable mismatch.' }
# Repair an existing Open With registration that may still point at a source-tree build.
# Keep the user's default program for every file extension unchanged.
$openWithCommand = 'HKCU:\Software\Classes\Applications\Youngcraft.exe\shell\open\command'
if (Test-Path -LiteralPath $openWithCommand) {
    Set-Item -LiteralPath $openWithCommand -Value ('"' + $executable + '" "%1"')
}
Write-Output "Installed: $executable"
if ($shortcutPaths.Count) { Write-Output "Desktop shortcuts: $($shortcutPaths -join ', ')" }
