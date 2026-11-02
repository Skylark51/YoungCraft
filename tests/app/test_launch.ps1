param([Parameter(Mandatory=$true)][string]$Application)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class YoungcraftLaunchTest {
  public delegate bool EnumCallback(IntPtr window, IntPtr data);
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumCallback callback, IntPtr data);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr w, IntPtr l);
  public static Dictionary<IntPtr,string> Windows(uint process) {
    var result = new Dictionary<IntPtr,string>();
    EnumWindows((window,data) => {
      uint id; GetWindowThreadProcessId(window, out id);
      if (id == process && IsWindowVisible(window)) { var text = new StringBuilder(1024); GetWindowText(window,text,text.Capacity); if(text.ToString().StartsWith("Youngcraft")) result[window]=text.ToString(); }
      return true;
    }, IntPtr.Zero);
    return result;
  }
}
"@
function Start-TestApplication([string[]]$Arguments) {
    $info = [Diagnostics.ProcessStartInfo]::new($Application)
    $info.UseShellExecute = $false
    # This test exercises visible GUI windows and closes only its own process.
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    return [Diagnostics.Process]::Start($info)
}
function Wait-TestCondition([scriptblock]$Condition, [string]$Message) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while ($timer.ElapsedMilliseconds -lt 15000) {
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 100
    }
    throw $Message
}
$directory = Join-Path ([IO.Path]::GetTempPath()) ('youngcraft-launch-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $directory | Out-Null
$paths = 1..4 | ForEach-Object {
    $file = Join-Path $directory "한글 분자 $_.xyz"
    [IO.File]::WriteAllText($file, "1`nMolecule $_`nC 0 0 0`n", [Text.UTF8Encoding]::new($false))
    $file
}
$primary = $null
try {
    $primary = Start-TestApplication @($paths[0],$paths[1])
    Wait-TestCondition { $primary.Refresh(); !$primary.HasExited -and [YoungcraftLaunchTest]::Windows($primary.Id).Values -contains ('Youngcraft — ' + [IO.Path]::GetFileName($paths[1])) } 'Initial Unicode multi-file launch failed'
    'PASS: One launch accepts multiple Unicode paths'
    $secondary = Start-TestApplication @($paths[2])
    if (!$secondary.WaitForExit(15000) -or $secondary.ExitCode -ne 0) { throw 'File forwarding failed' }
    Wait-TestCondition { [YoungcraftLaunchTest]::Windows($primary.Id).Values -contains ('Youngcraft — ' + [IO.Path]::GetFileName($paths[2])) } 'Forwarded file did not reach existing window'
    if ([YoungcraftLaunchTest]::Windows($primary.Id).Count -ne 1) { throw 'Repeated launch created another window' }
    'PASS: Repeated execution adds a tab to the same window and exits'
    $separate = Start-TestApplication @('--new-window',$paths[3])
    if (!$separate.WaitForExit(15000) -or $separate.ExitCode -ne 0) { throw 'Separate window forwarding failed' }
    Wait-TestCondition { [YoungcraftLaunchTest]::Windows($primary.Id).Count -eq 2 } 'Explicit separate window did not open in primary process'
    'PASS: --new-window creates a second window in the same process'
} finally {
    if ($primary -and !$primary.HasExited) {
        foreach ($window in [YoungcraftLaunchTest]::Windows($primary.Id).Keys) { [void][YoungcraftLaunchTest]::PostMessage($window,0x10,[IntPtr]::Zero,[IntPtr]::Zero) }
        if (!$primary.WaitForExit(10000)) { $primary.Kill() }
    }
    $resolved = [IO.Path]::GetFullPath($directory)
    $expected = [IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetFileName($directory))))
    if ($resolved -eq $expected -and [IO.Path]::GetFileName($resolved).StartsWith('youngcraft-launch-')) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}

