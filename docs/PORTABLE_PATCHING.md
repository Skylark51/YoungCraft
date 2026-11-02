# YoungCraft portable release and updater

## Build the optimized ZIP

On Windows with Qt 6, MSVC, Python, RDKit and PyInstaller installed, run:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/package_portable.ps1

The output is Youngcraft-portable.zip. It contains the executable, Qt and VC runtime dependencies, the independent RDKit MM worker, xTB executables (including gfnff), runtime data, license notices, and lab fragments. It omits xTB source archives and duplicate copies of fragments already compiled into Qt resources.

The ZIP includes package-manifest.json with the relative path, size and SHA-256 hash of each file.

## Running the patcher

Download release/Youngcraft-Patch.exe. Place Youngcraft-portable.zip next to it, or publish that ZIP under the same name as an asset of the repository's latest GitHub release. Close the Youngcraft application, then run the patch EXE.

The patcher searches for Youngcraft.exe beside itself, in recorded prior install locations, in running-process paths and common install folders, then uses a bounded search of fixed drives. If it finds multiple equally likely locations, it stops rather than guessing. It validates all ZIP entries and file hashes before altering the installation.

It replaces the program files, verifies the installed hashes, and removes obsolete entries tracked in a prior installed-files manifest. It also cleans specific historical leftovers: xtb_runtime/sources, xtb_runtime/xtb-source-*.tar.gz, resources/fragments, Youngcraft-before-*.exe and Youngcraft-running-before-*.exe. Existing youngcraft-server.json and user lab_fragments/ files are preserved. Unexpected errors trigger restoration from a temporary rollback copy.

For diagnostics or a known path use the PowerShell source directly:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/patch_youngcraft.ps1 -TargetDir 'D:\path\to\Youngcraft' -PackageZip 'D:\path\to\Youngcraft-portable.zip'

A cleanup-only run is available with -CleanupOnly, which removes the explicit known legacy paths without requiring a ZIP. The standalone EXE always attempts an update.

## Patch EXE compilation and release verification

The Windows Actions workflow parses the PowerShell code, performs a disposable-install integration test, compiles tools/YoungcraftPatcher.cs with tools/patch_youngcraft.ps1 embedded as a resource, then commits release/Youngcraft-Patch.exe on its branch. The resulting EXE requires no separate PS1 download; Windows PowerShell 5.1 is part of supported Windows installations.

The Actions integration test uses placeholder executables. Before releasing a package, run tools/verify_portable_xtb.ps1 and GUI smoke tests on a Windows system without external Python/RDKit/xTB on PATH. Compare compressed ZIP size against the prior version.

Until an optimized ZIP has been built or published as a GitHub release asset, running the patcher without an adjacent ZIP automatically performs cleanup only. It reports that the application binary has not been updated.