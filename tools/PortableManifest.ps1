# Get-ChildItem expands Windows 8.3 aliases. Derive the same canonical root
# from a file it enumerates rather than slicing with the caller's TEMP path.
function Get-YoungcraftPortableFileRecords {
    param([Parameter(Mandatory=$true)][string]$Root)
    $application = Get-ChildItem -LiteralPath $Root -File -Filter 'Youngcraft.exe' | Select-Object -First 1
    if (-not $application) { throw 'Portable root has no Youngcraft.exe.' }
    $prefix = $application.DirectoryName.TrimEnd('\') + '\'
    Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        if (-not $_.FullName.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Portable file is outside its root: $($_.FullName)"
        }
        [ordered]@{
            path = $_.FullName.Substring($prefix.Length).Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            size = $_.Length
        }
    }
}
