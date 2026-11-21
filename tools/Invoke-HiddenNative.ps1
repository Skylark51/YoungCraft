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
