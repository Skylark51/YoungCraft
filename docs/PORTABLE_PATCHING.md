# YoungCraft portable release and updater

## Build the optimized ZIP

On Windows with Qt 6, MSVC, Python, RDKit and PyInstaller installed, run:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/package_portable.ps1

Packaging also builds `Youngcraft-Standalone.exe`, a complete offline application with the portable ZIP embedded. Copy this single EXE to another Windows x64 PC and run it; no adjacent ZIP, DLLs, Python installation, or runtime download is needed. The first launch expands and verifies the payload under `%LOCALAPPDATA%\Youngcraft\Standalone`; later launches reuse that version's cache. Embedded compression reduces transfer size, while running still requires space for the extracted files. The launcher forwards file arguments and preserves the caller's working directory.

After packaging, run `tools/create_desktop_patch.ps1` to update the single-file app `YoungCraft.exe` on the Windows Desktop. This is the desktop deliverable. Share it to distribute the complete application. The portable ZIP used to build it stays in the project directory.

The output is Youngcraft-portable.zip. It contains the executable, Qt and VC runtime dependencies, the independent RDKit MM worker, xTB executables (including gfnff), runtime data, license notices, and lab fragments. It omits xTB source archives and duplicate copies of fragments already compiled into Qt resources.

The ZIP includes package-manifest.json with the relative path, size and SHA-256 hash of each file.

The MM worker imports the narrow RDKit force-field APIs and excludes unused drawing, notebook, GUI and optional numerical libraries. The existing MM test suite and clean-environment parity checks cover the frozen worker. Both existing xTB engines are retained: consolidating them changes gradients and some optimized conformations. Source archives under the newer engine's `sources` directory are kept in the source tree and excluded from the runtime package.

Rebuild the newer explicit-topology backend in an MSYS2 UCRT64 shell with `bash tools/build_xtb_runtime.sh`. The helper verifies pinned source archive hashes, collects recursive DLL dependencies, preserves the reviewed license notices, and keeps source/build files outside the runtime. The portable release workflow builds this backend as well as downloading the verified legacy engine.

Run `python tests/packaging/test_standalone.py` on Windows to verify isolated extraction, cached launches, simultaneous launches, checksums, path validation, argument forwarding, user-data migration and repeated PowerShell 5 builds. To inspect a real EXE without opening a GUI, use `Youngcraft-Standalone.exe --youngcraft-extract-only` or `--youngcraft-verify-only`. `--youngcraft-cache-root <directory>` selects a separate cache. A damaged cache reports an error and is preserved; selecting a fresh cache directory extracts a new copy. User-created fragments and Qt settings retain their normal AppData/registry locations; modified adjacent server configuration and CJSON fragments also migrate between standalone cache versions.

## Running the patcher

Download release/Youngcraft-Patch.exe. Place Youngcraft-portable.zip next to it, or publish that ZIP under the same name as an asset of the repository's latest GitHub release. Close the Youngcraft application, then run the patch EXE.

The patcher searches for Youngcraft.exe beside itself, in recorded prior install locations, in running-process paths and common install folders, then uses a bounded search of fixed drives. If it finds multiple equally likely locations, it stops rather than guessing. It validates all ZIP entries and file hashes before altering the installation.

It replaces the program files, verifies the installed hashes, and removes obsolete entries tracked in a prior installed-files manifest. It also cleans specific historical leftovers: xtb_runtime/sources, xtb_runtime/xtb-source-*.tar.gz, resources/fragments, Youngcraft-before-*.exe and Youngcraft-running-before-*.exe. Existing youngcraft-server.json and user lab_fragments/ files are preserved. Unexpected errors trigger restoration from a temporary rollback copy.

For diagnostics or a known path use the PowerShell source directly:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/patch_youngcraft.ps1 -TargetDir 'D:\path\to\Youngcraft' -PackageZip 'D:\path\to\Youngcraft-portable.zip'

A cleanup-only run is available with -CleanupOnly, which removes the explicit known legacy paths without requiring a ZIP. The patch EXE always attempts an update. The application launcher `Youngcraft-Standalone.exe` is separate and starts the complete embedded application.

## Patch EXE compilation and release verification

The Windows Actions workflow parses the PowerShell code, performs a disposable-install integration test, compiles tools/YoungcraftPatcher.cs with tools/patch_youngcraft.ps1 embedded as a resource, then commits release/Youngcraft-Patch.exe on its branch. The resulting EXE requires no separate PS1 download; Windows PowerShell 5.1 is part of supported Windows installations.

The Actions integration test uses placeholder executables. Before releasing a package, run tools/verify_portable_xtb.ps1 and GUI smoke tests on a Windows system without external Python/RDKit/xTB on PATH. Compare compressed ZIP size against the prior version.

The automated Windows portable-release workflow builds Youngcraft-portable.zip and publishes it as the latest GitHub Release asset after compilation and CTest succeed. With that release available, download only **release/Youngcraft-Patch.exe**, close Youngcraft, and double-click the EXE. It discovers the installation and downloads the ZIP without a separate manual download. If the ZIP is unavailable, the EXE reports failure without changing the installed application. For manual cleanup, run the PowerShell source with -CleanupOnly.
