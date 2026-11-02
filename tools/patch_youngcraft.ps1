[CmdletBinding()]
param([string]$PatchExecutable='', [string]$PackageZip='', [string]$TargetDir='', [switch]$CleanupOnly)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Log([string]$s) { Write-Host "[YoungCraft Patch] $s" }
function Abs([string]$p) { [IO.Path]::GetFullPath($p) }
function Rel([string]$p) {
    if (-not $p -or $p -match '(^/|^[A-Za-z]:|(^|/)\.\.?(/|$)|\\|:|//)') { throw "Unsafe package path: $p" }
    $p
}
function SafePath([string]$base,[string]$p) {
    $result=Abs (Join-Path $base ($p.Replace('/','\')))
    if (-not $result.StartsWith(((Abs $base).TrimEnd('\')+'\'),[StringComparison]::OrdinalIgnoreCase)) {
        throw "Path escapes installation: $p"
    }
    $result
}
function UserFile([string]$p) {
    $p -eq 'youngcraft-server.json' -or $p.StartsWith('lab_fragments/',[StringComparison]::OrdinalIgnoreCase)
}
$homeDir=if($PatchExecutable){Split-Path -Parent (Abs $PatchExecutable)}else{$PSScriptRoot}
$places=New-Object 'System.Collections.Generic.Dictionary[string,int]' ([StringComparer]::OrdinalIgnoreCase)
function Candidate([string]$dir,[int]$score) {
    if(-not $dir){return}
    try {
        $dir=Abs $dir
        if(Test-Path -LiteralPath (Join-Path $dir 'Youngcraft.exe') -PathType Leaf){
            if(-not $places.ContainsKey($dir) -or $places[$dir] -lt $score){$places[$dir]=$score}
        }
    }catch{}
}
if($TargetDir){
    Candidate $TargetDir 1000
    if(-not $places.Count){throw "Youngcraft.exe missing in $TargetDir"}
}else{
    Candidate $homeDir 900
    Candidate (Join-Path $homeDir 'dist') 800
    Candidate (Split-Path -Parent $homeDir) 500
    if($env:LOCALAPPDATA){
        $marker=Join-Path $env:LOCALAPPDATA 'Youngcraft\install-location.txt'
        if(Test-Path -LiteralPath $marker){Candidate (Get-Content -LiteralPath $marker -TotalCount 1) 850}
        Candidate (Join-Path $env:LOCALAPPDATA 'Programs\Youngcraft') 400
    }
    foreach($proc in @(Get-Process Youngcraft -ErrorAction SilentlyContinue)){
        try{Candidate (Split-Path -Parent $proc.Path) 950}catch{}
    }
    $bases=@($env:ProgramFiles,[Environment]::GetEnvironmentVariable('ProgramFiles(x86)'),$env:USERPROFILE,
        [Environment]::GetFolderPath('Desktop'),[Environment]::GetFolderPath('MyDocuments'))
    foreach($base in $bases){if($base){Candidate (Join-Path $base 'Youngcraft') 300}}
    if(-not $places.Count){
        $q=New-Object 'System.Collections.Generic.Queue[object]'
        $drives=@([IO.DriveInfo]::GetDrives() | Sort-Object -Property @{Expression={$_.Name.TrimEnd('\') -eq $env:SystemDrive};Descending=$false})
        foreach($d in $drives){if($d.DriveType -eq 'Fixed' -and $d.IsReady){$q.Enqueue(@($d.RootDirectory.FullName,0))}}
        $skip=@('$Recycle.Bin','System Volume Information','Windows','ProgramData','AppData','.git','node_modules','Temp')
        $seen=0
        while($q.Count -gt 0 -and $seen -lt 15000 -and -not $places.Count){
            $it=$q.Dequeue();$dir=[string]$it[0];$level=[int]$it[1];$seen++
            Candidate $dir 100
            if($level -ge 5){continue}
            try{
                foreach($child in [IO.Directory]::EnumerateDirectories($dir)){
                    if($skip -contains [IO.Path]::GetFileName($child)){continue}
                    if(([IO.File]::GetAttributes($child) -band [IO.FileAttributes]::ReparsePoint) -ne 0){continue}
                    $q.Enqueue(@($child,($level+1)))
                }
            }catch{}
        }
    }
}
if(-not $places.Count){throw 'Youngcraft.exe not found. Run with -TargetDir.'}
$matches=@($places.GetEnumerator() | Sort-Object -Property @{Expression='Value';Descending=$true},@{Expression='Key';Descending=$false})
if($matches.Count -gt 1 -and $matches[0].Value -eq $matches[1].Value){throw 'Multiple installations found; specify -TargetDir.'}
$target=$matches[0].Key
Log "Target: $target"
$busy=@(Get-Process Youngcraft -ErrorAction SilentlyContinue | Where-Object {
    try{(Abs $_.Path).StartsWith(((Abs $target).TrimEnd('\')+'\'),[StringComparison]::OrdinalIgnoreCase)}catch{$false}
})
if($busy.Count){throw 'Close Youngcraft.exe before patching.'}
$download=''
$archive=''
if(-not $CleanupOnly){
    $archive=if($PackageZip){Abs $PackageZip}else{Join-Path $homeDir 'Youngcraft-portable.zip'}
    if(-not (Test-Path -LiteralPath $archive -PathType Leaf)){
        $download=Join-Path ([IO.Path]::GetTempPath()) ('Youngcraft-'+[Guid]::NewGuid().ToString('N')+'.zip')
        $archive=$download
        try{
            Log 'Downloading latest release...'
            Invoke-WebRequest -UseBasicParsing -TimeoutSec 120 -Uri 'https://github.com/Skylark51/YoungCraft/releases/latest/download/Youngcraft-portable.zip' -OutFile $archive
        }catch{
            Remove-Item -LiteralPath $download -Force -ErrorAction SilentlyContinue
            $download=''
            $archive=''
            throw 'YoungCraft 최신 배포 ZIP을 다운로드하지 못했습니다. 프로그램 업데이트가 적용되지 않았습니다. GitHub Releases 상태와 인터넷 연결을 확인하세요.'
        }
    }
}
$parent=Split-Path -Parent $target
$stage=Join-Path $parent ('.Youngcraft-stage-'+[Guid]::NewGuid().ToString('N'))
$backup=Join-Path $parent ('.Youngcraft-backup-'+[Guid]::NewGuid().ToString('N'))
$record=Join-Path $target '.youngcraft-installed-files.json'
$hadRecord=Test-Path -LiteralPath $record
$changedRecord=$false
$touched=New-Object 'System.Collections.Generic.List[string]'
$removed=New-Object 'System.Collections.Generic.List[string]'
$success=$false
try{
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    $files=@()
    $wanted=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    if(-not $CleanupOnly){
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip=[IO.Compression.ZipFile]::OpenRead($archive)
        try{
            $names=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
            foreach($item in $zip.Entries){
                $p=$item.FullName.Replace('\','/')
                if($p.EndsWith('/')){continue}
                $p=Rel $p
                if(-not $names.Add($p)){throw "Duplicate ZIP entry: $p"}
            }
        }finally{$zip.Dispose()}
        New-Item -ItemType Directory -Path $stage -Force | Out-Null
        [IO.Compression.ZipFile]::ExtractToDirectory($archive,$stage)
        $manifestFile=Join-Path $stage 'package-manifest.json'
        if(-not (Test-Path -LiteralPath $manifestFile)){throw 'Package manifest missing. Rebuild optimized ZIP.'}
        $manifest=Get-Content -LiteralPath $manifestFile -Raw -Encoding UTF8 | ConvertFrom-Json
        if($manifest.schema -ne 1){throw 'Unknown manifest schema.'}
        foreach($item in @($manifest.files)){
            $p=Rel ([string]$item.path)
            if(-not $wanted.Add($p)){throw "Duplicate manifest path: $p"}
            $src=SafePath $stage $p
            if(-not (Test-Path -LiteralPath $src -PathType Leaf)){throw "Missing: $p"}
            if((Get-FileHash -LiteralPath $src -Algorithm SHA256).Hash -ne [string]$item.sha256){throw "Hash mismatch: $p"}
            $files+=$p
        }
        foreach($p in @('Youngcraft.exe','Qt6Core.dll','mm_runtime/YoungcraftMM.exe','xtb_runtime/xtb.exe')){
            if(-not $wanted.Contains($p)){throw "Runtime missing: $p"}
        }
        $actual=@(Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object {$_.FullName -ne $manifestFile})
        if($actual.Count -ne $files.Count){throw 'Unlisted files in package.'}
        Log "Verified $($files.Count) files."
    }
    if($hadRecord){Copy-Item -LiteralPath $record -Destination (Join-Path $backup 'previous.json')}
    foreach($p in $files){
        $dest=SafePath $target $p
        if((UserFile $p) -and (Test-Path -LiteralPath $dest)){continue}
        $src=SafePath $stage $p
        if(Test-Path -LiteralPath $dest){
            $old=SafePath $backup ('replaced/'+$p)
            New-Item -ItemType Directory -Path (Split-Path -Parent $old) -Force | Out-Null
            Copy-Item -LiteralPath $dest -Destination $old -Force
        }
        New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
        $touched.Add($p)
        Copy-Item -LiteralPath $src -Destination $dest -Force
    }
    if($hadRecord -and -not $CleanupOnly){
        $previous=Get-Content -LiteralPath $record -Raw | ConvertFrom-Json
        if($previous.schema -eq 1){
            foreach($name in @($previous.files)){
                $p=Rel ([string]$name)
                if($wanted.Contains($p) -or (UserFile $p)){continue}
                $old=SafePath $target $p
                if(Test-Path -LiteralPath $old -PathType Leaf){
                    $save=SafePath $backup ('obsolete/'+$p)
                    New-Item -ItemType Directory -Path (Split-Path -Parent $save) -Force | Out-Null
                    Move-Item -LiteralPath $old -Destination $save
                    $removed.Add('obsolete:'+$p)
                }
            }
        }
    }
    foreach($p in @('xtb_runtime/sources','resources/fragments')){
        $old=SafePath $target $p
        if(Test-Path -LiteralPath $old){
            $save=SafePath $backup ('legacy/'+$p)
            New-Item -ItemType Directory -Path (Split-Path -Parent $save) -Force | Out-Null
            Move-Item -LiteralPath $old -Destination $save
            $removed.Add('legacy:'+$p)
        }
    }
    foreach($pattern in @('Youngcraft-before-*.exe','Youngcraft-running-before-*.exe','xtb_runtime\xtb-source-*.tar.gz')){
        foreach($file in @(Get-ChildItem -Path (Join-Path $target $pattern) -File -ErrorAction SilentlyContinue)){
            $p=$file.FullName.Substring($target.Length+1).Replace('\','/')
            $save=SafePath $backup ('legacy/'+$p)
            New-Item -ItemType Directory -Path (Split-Path -Parent $save) -Force | Out-Null
            Move-Item -LiteralPath $file.FullName -Destination $save
            $removed.Add('legacy:'+$p)
        }
    }
    foreach($p in $files){
        $dest=SafePath $target $p
        if((UserFile $p) -and (Test-Path -LiteralPath $dest)){continue}
        if((Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath (SafePath $stage $p) -Algorithm SHA256).Hash){
            throw "Installed hash mismatch: $p"
        }
    }
    if(-not $CleanupOnly){
        $changedRecord=$true
        @{schema=1;files=@($files)} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $record -Encoding UTF8
    }
    $success=$true
    if($env:LOCALAPPDATA){
        try{
            $markerDir=Join-Path $env:LOCALAPPDATA 'Youngcraft'
            New-Item -ItemType Directory -Path $markerDir -Force | Out-Null
            Set-Content -LiteralPath (Join-Path $markerDir 'install-location.txt') -Value $target -Encoding UTF8
        }catch{Log 'Could not save location marker.'}
    }
    Log "Complete: $($touched.Count) files applied; $($removed.Count) legacy entries removed."
}catch{
    Log "Patch failed; restoring prior installation. $_"
    $undo=$touched.ToArray();[array]::Reverse($undo)
    foreach($p in $undo){
        $dest=SafePath $target $p
        $old=SafePath $backup ('replaced/'+$p)
        Remove-Item -LiteralPath $dest -Force -ErrorAction SilentlyContinue
        if(Test-Path -LiteralPath $old){Copy-Item -LiteralPath $old -Destination $dest -Force}
    }
    $undo=$removed.ToArray();[array]::Reverse($undo)
    foreach($r in $undo){
        $isLegacy=$r.StartsWith('legacy:')
        $p=$r.Substring($(if($isLegacy){7}else{9}))
        $folder=if($isLegacy){'legacy/'}else{'obsolete/'}
        $old=SafePath $backup ($folder+$p)
        $dest=SafePath $target $p
        if(Test-Path -LiteralPath $old){
            New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
            Move-Item -LiteralPath $old -Destination $dest -Force
        }
    }
    if($changedRecord){
        if($hadRecord){Copy-Item -LiteralPath (Join-Path $backup 'previous.json') -Destination $record -Force}
        elseif(Test-Path -LiteralPath $record){Remove-Item -LiteralPath $record -Force}
    }
    throw
}finally{
    if(Test-Path -LiteralPath $stage){Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue}
    if($success -and (Test-Path -LiteralPath $backup)){Remove-Item -LiteralPath $backup -Recurse -Force -ErrorAction SilentlyContinue}
    if($download -and (Test-Path -LiteralPath $download)){Remove-Item -LiteralPath $download -Force -ErrorAction SilentlyContinue}
}
