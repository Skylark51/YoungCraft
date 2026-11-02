# Youngcraft explicit-topology GFN-FF runtime

This isolated backend is selected only for GFN-FF (method 0). Existing GFN1/2-xTB calculations use ../xtb.exe.

Upstream xTB: https://github.com/grimme-lab/xtb/tree/68fe777a02418471b9ddfc7a9ea24b4021d7bca2
Source snapshot: sources/xtb-68fe777a.zip (unmodified upstream).
mctc-lib v0.6.1: sources/mctc-lib-0.6.1.zip.
Compiler: MSYS2 UCRT64 GCC/GFortran 16.2.0; OpenBLAS 0.3.34.

Build from an ASCII-only directory with GCC, GFortran, OpenBLAS, CMake and Ninja in PATH:

```powershell
cmake -S <xtb-source> -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Release -DWITH_TBLITE=OFF -DWITH_CPCMX=OFF -DWITH_TESTS=OFF -DBLAS_LIBRARIES=<ucrt64>/lib/libopenblas.dll.a -DLAPACK_LIBRARIES=<ucrt64>/lib/libopenblas.dll.a
cmake --build <build> --parallel 6
```

mctc-lib can be supplied using FETCHCONTENT_SOURCE_DIR_MCTC-LIB after extracting its archive.
Copy xtb.exe and its UCRT64 runtime DLL dependencies beside it. License notices are in licenses/.
OpenBLAS source: https://github.com/OpenMathLib/OpenBLAS/releases/tag/v0.3.34
GCC runtime sources: https://gcc.gnu.org/releases.html (16.2.0; runtime linking exception in licenses/).

Youngcraft writes all current bond neighbors symmetrically using $ffnb, including isolated atoms (nb = i:0).
GFN-FF computes its own bond orders; the specified adjacency controls which atoms are connected.
Documentation: https://xtb-docs.readthedocs.io/en/latest/gfnff.html#manual-neighbor-list-setup
The old Windows pre-release binary ignores $ffnb, so this backend is required for edited topology.
A completed calculation must confirm that the neighbor list was adjusted; otherwise Youngcraft reports failure.
