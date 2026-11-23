[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
# Use the same verified software driver for every remote Windows GUI review.
$archive = Join-Path $env:RUNNER_TEMP 'youngcraft-mesa.7z'
$directory = Join-Path $env:RUNNER_TEMP 'youngcraft-mesa'
Invoke-WebRequest -Uri 'https://github.com/pal1000/mesa-dist-win/releases/download/26.2.6/mesa3d-26.2.6-release-mingw.7z' -OutFile $archive
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne 'da0f71d9181b672c1918f3c342f11118c687f6059a46b16502845160733cc388') {
    throw 'Software OpenGL checksum mismatch'
}
& 7z x $archive "-o$directory" -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Software OpenGL extraction failed' }
$library = Get-ChildItem -LiteralPath $directory -Recurse -File -Filter opengl32.dll |
    Where-Object { $_.FullName -match '[\\/]x64[\\/]' } | Select-Object -First 1
if (-not $library) { throw '64-bit software OpenGL library missing' }
@(
    "PATH=$($library.DirectoryName);$env:PATH"
    'GALLIUM_DRIVER=llvmpipe'
    'QT_OPENGL=software'
    "QT_OPENGL_DLL=$($library.FullName)"
    'QT_LOGGING_RULES=qt.qpa.gl=true'
) | Out-File -LiteralPath $env:GITHUB_ENV -Encoding utf8 -Append
