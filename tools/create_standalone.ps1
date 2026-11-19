# Build one offline EXE from an already complete portable ZIP. Does not rebuild
# Youngcraft or modify the source portable package. Windows .NET Framework 4.5+
# is the only launcher prerequisite; all application runtimes stay in the ZIP.
[CmdletBinding()]
param(
    [string]$PackageZip = '',
    [string]$OutputPath = ''
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($PackageZip)) { $PackageZip = Join-Path (Split-Path -Parent $PSScriptRoot) 'Youngcraft-portable.zip' }
if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'Youngcraft-Standalone.exe' }
$PackageZip = (Resolve-Path -LiteralPath $PackageZip).Path
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
# The desktop is the only place where the standalone uses the native filename.
if ([IO.Path]::GetFileName($OutputPath).Equals('Youngcraft.exe', [StringComparison]::OrdinalIgnoreCase)) {
    $desktop = [Environment]::GetFolderPath('Desktop')
    if (-not $desktop -or -not $OutputPath.Equals((Join-Path $desktop 'YoungCraft.exe'), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Use Youngcraft-Standalone.exe outside the Windows Desktop to protect the native executable.'
    }
    if (Test-Path -LiteralPath $OutputPath -PathType Leaf) {
        $existing = Get-Item -LiteralPath $OutputPath -Force
        if ($existing.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Invalid Desktop launcher: $OutputPath" }
        $assembly = try { [Reflection.AssemblyName]::GetAssemblyName($OutputPath).Name } catch [BadImageFormatException] { $null }
        if ($assembly -ne 'Youngcraft-Standalone') { throw "The existing Desktop executable is not the Youngcraft single-file launcher: $OutputPath" }
    }
}
if ([IO.Path]::GetExtension($OutputPath) -ine '.exe') { throw 'OutputPath must have an .exe extension.' }
$compiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    $compiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework/v4.0.30319/csc.exe'
}
if (-not (Test-Path -LiteralPath $compiler)) { throw 'Windows .NET Framework C# compiler was not found.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$stage = Join-Path ([IO.Path]::GetTempPath()) ('YoungcraftStandaloneBuild-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
$publish = $null
try {
    # Snapshot with read sharing only: packaging cannot rewrite our input midway.
    $snapshot = Join-Path $stage 'payload.zip'
    $source = [IO.File]::Open($PackageZip, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    try {
        $destination = [IO.File]::Create($snapshot)
        try { $source.CopyTo($destination) } finally { $destination.Dispose() }
    } finally { $source.Dispose() }
    $archive = [IO.Compression.ZipFile]::OpenRead($snapshot)
    try {
        $entries = @($archive.Entries | Where-Object { $_.FullName -ieq 'package-manifest.json' })
        if ($entries.Count -ne 1) { throw 'Portable ZIP must contain exactly one root package-manifest.json.' }
        if ($entries[0].Length -gt 32MB) { throw 'Package manifest is too large.' }
        $manifest = Join-Path $stage 'package-manifest.json'
        $input = $entries[0].Open()
        try {
            $output = [IO.File]::Create($manifest)
            try { $input.CopyTo($output) } finally { $output.Dispose() }
        } finally { $input.Dispose() }
        $info = Get-Content -LiteralPath $manifest -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($info.schema -ne 1 -or -not ($info.files | Where-Object { $_.path -ieq 'Youngcraft.exe' })) {
            throw 'Portable manifest schema 1 with a root Youngcraft.exe is required.'
        }
    } finally { $archive.Dispose() }
    $hashPath = Join-Path $stage 'payload.sha256'
    [IO.File]::WriteAllText($hashPath, (Get-FileHash -LiteralPath $snapshot -Algorithm SHA256).Hash.ToLowerInvariant(), [Text.Encoding]::ASCII)
    $temporaryExe = Join-Path $stage 'Youngcraft-Standalone.exe'
    $arguments = @(
        '/nologo', '/target:winexe', '/platform:anycpu', '/optimize+',
        "/out:$temporaryExe", '/reference:System.Windows.Forms.dll',
        '/reference:System.IO.Compression.dll', '/reference:System.IO.Compression.FileSystem.dll',
        '/reference:System.Web.Extensions.dll',
        "/resource:$snapshot,YoungcraftStandalone.Payload",
        "/resource:$manifest,YoungcraftStandalone.Manifest",
        "/resource:$hashPath,YoungcraftStandalone.PayloadHash"
    )
    $icon = Join-Path (Split-Path -Parent $PSScriptRoot) 'resources/youngcraft.ico'
    if (Test-Path -LiteralPath $icon) { $arguments += "/win32icon:$icon" }
    $arguments += (Join-Path $PSScriptRoot 'YoungcraftStandalone.cs')
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw "Standalone compilation failed: $LASTEXITCODE" }
    $parent = [IO.Path]::GetDirectoryName($OutputPath)
    # Prevent output junctions/symlinks from redirecting publishing elsewhere.
    $ancestor = $OutputPath
    while ($ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Standalone output cannot traverse reparse points: $ancestor"
        }
        $ancestor = [IO.Path]::GetDirectoryName($ancestor)
    }
    [IO.Directory]::CreateDirectory($parent) | Out-Null
    $publish = Join-Path $parent ('.YoungcraftStandalone-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    [IO.File]::Copy($temporaryExe, $publish, $false)
    if ([IO.File]::Exists($OutputPath)) { [IO.File]::Replace($publish, $OutputPath, [NullString]::Value) }
    else { [IO.File]::Move($publish, $OutputPath) }
    $publish = $null
    Write-Output "Standalone: $OutputPath"
    Write-Output "Payload SHA256: $([IO.File]::ReadAllText($hashPath))"
} finally {
    if ($publish -and [IO.File]::Exists($publish)) { [IO.File]::Delete($publish) }
    # Only this invocation's unique temporary build directory is removable.
    $resolvedStage = [IO.Path]::GetFullPath($stage)
    $temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if ($resolvedStage.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase) -and
        [IO.Path]::GetFileName($resolvedStage).StartsWith('YoungcraftStandaloneBuild-')) {
        Remove-Item -LiteralPath $resolvedStage -Recurse -Force
    }
}
