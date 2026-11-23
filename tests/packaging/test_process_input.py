"""Exercise actual native stdin bytes on Windows PowerShell 5 and PowerShell 7."""
import base64
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(os.name == 'nt', 'Windows process input encodings')
class ProcessInput(unittest.TestCase):
    def test_frozen_mm_iterations_without_external_runtime(self):
        root = Path(__file__).resolve().parents[2]
        worker = root / 'dist/mm_runtime/YoungcraftMM.exe'
        if not worker.is_file():
            self.skipTest('Build the portable distribution for the frozen MM integration test')
        with tempfile.TemporaryDirectory(prefix='Youngcraft MM protocol ') as temporary:
            script = Path(temporary) / 'mm.ps1'
            script.write_text('''param([string]$Helper,[string]$Worker,[string]$Fixture)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object Text.UTF8Encoding($false)
[Console]::InputEncoding = New-Object Text.UTF8Encoding($true)
. $Helper
$env:PATH = "$env:SystemRoot\\System32;$env:SystemRoot"
$info = New-Object Diagnostics.ProcessStartInfo
$info.FileName = $Worker
$info.WorkingDirectory = Split-Path -Parent $Worker
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardInput = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$child = New-Object Diagnostics.Process
$child.StartInfo = $info
try {
    Start-YoungcraftUtf8Process -Process $child | Out-Null
    Write-YoungcraftProcessLine -Process $child -Text (Get-Content -LiteralPath $Fixture -Raw -Encoding UTF8)
    $count = 0
    do {
        $line = $child.StandardOutput.ReadLineAsync()
        if (-not $line.Wait(10000) -or -not $line.Result) { throw 'Worker response timeout or EOF' }
        $response = $line.Result | ConvertFrom-Json
        if ($response.type -eq 'error') { throw $response.message }
        $count++
        if ($count -gt 20) { throw 'Unexpected number of chunks' }
        if ($response.type -ne 'done') { Write-YoungcraftProcessLine -Process $child -Text 'chunk' }
    } until ($response.type -eq 'done')
    @{iteration=$response.iteration;converged=$response.converged;chunks=$count} | ConvertTo-Json -Compress
} finally {
    if (-not $child.HasExited) { $child.Kill(); $child.WaitForExit(3000) | Out-Null }
    $child.Dispose()
}
''', encoding='utf-8-sig')
            for shell in ('powershell.exe', 'pwsh.exe'):
                with self.subTest(shell=shell):
                    environment = {key: value for key, value in os.environ.items()
                                   if key.lower() != 'psmodulepath'}
                    result = subprocess.run(
                        [shutil.which(shell), '-NoProfile', '-NonInteractive', '-File', str(script),
                         '-Helper', str(root / 'tools/Invoke-HiddenNative.ps1'), '-Worker', str(worker),
                         '-Fixture', str(root / 'tests/mm/portable-input.json')],
                        env=environment, capture_output=True, text=True, encoding='utf-8',
                        errors='replace', timeout=60, creationflags=subprocess.CREATE_NO_WINDOW)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    response = json.loads(result.stdout.lstrip('\ufeff'))
                    self.assertEqual(response['iteration'], 120)
                    self.assertFalse(response['converged'])
                    self.assertGreaterEqual(response['chunks'], 15)

    def test_json_and_commands_are_utf8_lines_without_bom(self):
        helper = Path(__file__).resolve().parents[2] / 'tools/Invoke-HiddenNative.ps1'
        with tempfile.TemporaryDirectory(prefix='Youngcraft protocol ') as temporary:
            probe = Path(temporary) / 'read.py'
            probe.write_text('import base64,sys\n'
                             'print(base64.b64encode(sys.stdin.buffer.readline()+'
                             'sys.stdin.buffer.readline()).decode("ascii"))\n', encoding='utf-8')
            script = Path(temporary) / 'send.ps1'
            script.write_text('''param([string]$Helper,[string]$Python,[string]$Probe)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object Text.UTF8Encoding($false)
[Console]::InputEncoding = New-Object Text.UTF8Encoding($true)
. $Helper
$info = New-Object Diagnostics.ProcessStartInfo
$info.FileName = $Python
$info.Arguments = '"' + $Probe + '"'
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardInput = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$child = New-Object Diagnostics.Process
$child.StartInfo = $info
try {
    Start-YoungcraftUtf8Process -Process $child | Out-Null
    if ([Console]::InputEncoding.GetPreamble().Length -eq 0) { throw 'Host encoding changed' }
    Write-YoungcraftProcessLine -Process $child -Text ('{"label":"한글"}' + "`r`n")
    Write-YoungcraftProcessLine -Process $child -Text 'chunk'
    if (-not $child.WaitForExit(10000)) { $child.Kill(); throw 'Probe timeout' }
    if ($child.ExitCode -ne 0) { throw $child.StandardError.ReadToEnd() }
    $child.StandardOutput.ReadToEnd()
} finally { $child.Dispose() }
''', encoding='utf-8-sig')
            for shell in ('powershell.exe', 'pwsh.exe'):
                with self.subTest(shell=shell):
                    executable = shutil.which(shell)
                    self.assertIsNotNone(executable)
                    environment = {key: value for key, value in os.environ.items()
                                   if key.lower() != 'psmodulepath'}
                    result = subprocess.run(
                        [executable, '-NoProfile', '-NonInteractive', '-File', str(script),
                         '-Helper', str(helper), '-Python', sys.executable, '-Probe', str(probe)],
                        env=environment, capture_output=True, text=True, encoding='utf-8',
                        errors='replace', timeout=20, creationflags=subprocess.CREATE_NO_WINDOW)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(base64.b64decode(result.stdout.strip().lstrip('\ufeff')),
                                     '{"label":"한글"}\nchunk\n'.encode('utf-8'))


if __name__ == '__main__':
    unittest.main()
