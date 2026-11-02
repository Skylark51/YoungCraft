using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

internal static class YoungcraftPatcher
{
    [DllImport("kernel32.dll")]
    private static extern uint GetConsoleProcessList([Out] uint[] processList, uint processCount);

    private static int Main()
    {
        string tempFile = Path.Combine(Path.GetTempPath(), "Youngcraft-Patch-" + Guid.NewGuid().ToString("N") + ".ps1");
        int exitCode = 1;
        try
        {
            using (Stream source = Assembly.GetExecutingAssembly().GetManifestResourceStream("YoungcraftPatchScript"))
            {
                if (source == null) throw new InvalidOperationException("Embedded patch script is missing.");
                using (FileStream destination = File.Create(tempFile)) source.CopyTo(destination);
            }
            string executable = Assembly.GetExecutingAssembly().Location;
            string arguments = "-NoProfile -ExecutionPolicy Bypass -File \"" + tempFile + "\" -PatchExecutable \"" + executable + "\"";
            ProcessStartInfo info = new ProcessStartInfo("powershell.exe", arguments);
            info.UseShellExecute = false;
            using (Process child = Process.Start(info))
            {
                if (child == null) throw new InvalidOperationException("Could not start PowerShell.");
                child.WaitForExit();
                exitCode = child.ExitCode;
            }
        }
        catch (Exception error)
        {
            Console.Error.WriteLine("[YoungCraft Patch] " + error.Message);
        }
        finally
        {
            try { File.Delete(tempFile); } catch { }
            // Keep the result visible after double-click from Windows Explorer.
            try
            {
                uint[] processes = new uint[16];
                if (GetConsoleProcessList(processes, (uint)processes.Length) <= 1)
                {
                    Console.WriteLine("Press Enter to close...");
                    Console.ReadLine();
                }
            }
            catch { }
        }
        return exitCode;
    }
}
