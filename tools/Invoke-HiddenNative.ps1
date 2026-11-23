# Native programs must work without a console, including Windows PowerShell 5.
function Invoke-YoungcraftNative {
    param([Parameter(Mandatory=$true)][string]$FilePath, [string[]]$Arguments = @(), [ref]$OutputText)
    $resolved = (Get-Command -Name $FilePath -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    # ProcessStartInfo on .NET Framework uses CommandLineToArgvW quoting.
    $quoted = foreach ($argument in $Arguments) {
        '"' + [regex]::Replace([regex]::Replace($argument, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
    }
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $resolved
    $info.Arguments = $quoted -join ' '
    $info.WorkingDirectory = (Get-Location).ProviderPath
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.RedirectStandardInput = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    try {
        if (-not $process.Start()) { throw "Could not start: $resolved" }
        $process.StandardInput.Close()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        if ($PSBoundParameters.ContainsKey('OutputText')) { $OutputText.Value = $stdout.Result.TrimEnd() }
        elseif ($stdout.Result) { Write-Host $stdout.Result.TrimEnd() }
        if ($stderr.Result) { [Console]::Error.WriteLine($stderr.Result.TrimEnd()) }
        return $process.ExitCode
    } finally { $process.Dispose() }
}

# .NET Framework starts its stdin writer with the console encoding, including
# a possible UTF-8 BOM. Newer .NET supports a process-specific encoding.
function Start-YoungcraftUtf8Process {
    param([Parameter(Mandatory=$true)][Diagnostics.Process]$Process)
    $utf8 = New-Object Text.UTF8Encoding($false)
    if ($Process.StartInfo.PSObject.Properties['StandardInputEncoding']) {
        $Process.StartInfo.StandardInputEncoding = $utf8
        return $Process.Start()
    }
    $previous = [Console]::InputEncoding
    try {
        [Console]::InputEncoding = $utf8
        return $Process.Start()
    } finally { [Console]::InputEncoding = $previous }
}

# Send precisely one UTF-8 line, including requests read from text files.
function Write-YoungcraftProcessLine {
    param([Parameter(Mandatory=$true)][Diagnostics.Process]$Process,
          [Parameter(Mandatory=$true)][string]$Text)
    $bytes = [Text.Encoding]::UTF8.GetBytes($Text.TrimEnd([char[]]"`r`n") + "`n")
    $stream = $Process.StandardInput.BaseStream
    $stream.Write($bytes, 0, $bytes.Length)
    $stream.Flush()
}
