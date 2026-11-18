#!/usr/bin/env bash
# Run in MSYS2 UCRT64 after installing gcc-fortran, cmake, ninja, openblas and curl.
# Sources/build stay outside the runtime. No stripping or legacy-runtime cleanup.
# Override XTB_BUILD_ROOT (ASCII path), XTB_RUNTIME_DIR, XTB_JOBS, CMAKE or NINJA.
set -euo pipefail

die() { printf '%s\n' "$*" >&2; exit 1; }
[[ ${MSYSTEM:-} == UCRT64 ]] || die 'Run this script in an MSYS2 UCRT64 shell.'
project_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
prefix=${MINGW_PREFIX:-/ucrt64}
cmake_bin=${CMAKE:-cmake}
ninja_bin=${NINJA:-ninja}
for command in curl sha256sum objdump gcc gfortran cygpath "$cmake_bin" "$ninja_bin"; do
    command -v "$command" >/dev/null || die "Missing build tool: $command"
done
[[ $(gcc -dumpmachine) == x86_64-w64-mingw32 ]] || die 'Expected an x86_64 MinGW compiler.'
[[ -f "$prefix/lib/libopenblas.dll.a" ]] || die 'Missing UCRT64 OpenBLAS import library.'

xtb_commit=68fe777a02418471b9ddfc7a9ea24b4021d7bca2
xtb_sha=ed92501f770a02bea4d00782f61d0bdcb61981353ee9093578d49a0b0a942bcd
mctc_sha=549477402f38de3a2ffc3d16558b12e177920d7e0c3e3af5067c83cfc2968ba7
build_root=$(cygpath -au "${XTB_BUILD_ROOT:-${RUNNER_TEMP:-${TMPDIR:-/tmp}}/youngcraft-xtb-68fe777a}")
runtime_dir=$(cygpath -au "${XTB_RUNTIME_DIR:-$project_root/xtb_runtime/gfnff}")
[[ $(cygpath -am "$build_root") != *[![:ascii:]]* ]] || die 'XTB_BUILD_ROOT must be ASCII-only (Fortran build limitation).'
mkdir -p "$build_root"
build_root=$(cd -- "$build_root" && pwd)
cd -- "$build_root"

fetch() {
    local url=$1 archive=$2 checksum=$3
    if [[ ! -f $archive ]]; then
        curl --fail --location --retry 3 --output "$archive.download" "$url"
        printf '%s  %s\n' "$checksum" "$archive.download" | sha256sum --check -
        mv -- "$archive.download" "$archive"
    fi
    printf '%s  %s\n' "$checksum" "$archive" | sha256sum --check -
}
fetch "https://codeload.github.com/grimme-lab/xtb/zip/$xtb_commit" xtb.zip "$xtb_sha"
fetch 'https://codeload.github.com/grimme-lab/mctc-lib/zip/refs/tags/v0.6.1' mctc.zip "$mctc_sha"
# Refresh upstream files from the verified archives on every invocation.
"$cmake_bin" -E tar xf xtb.zip
"$cmake_bin" -E tar xf mctc.zip
xtb_source="$build_root/xtb-$xtb_commit"
mctc_source="$build_root/mctc-lib-0.6.1"
"$cmake_bin" -S "$(cygpath -am "$xtb_source")" -B "$(cygpath -am "$build_root/build")" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    "-DCMAKE_C_COMPILER=$(cygpath -am "$prefix/bin/gcc.exe")" \
    "-DCMAKE_Fortran_COMPILER=$(cygpath -am "$prefix/bin/gfortran.exe")" \
    "-DCMAKE_MAKE_PROGRAM=$(cygpath -am "$(command -v "$ninja_bin")")" \
    -DWITH_TBLITE=OFF -DWITH_CPCMX=OFF -DWITH_TESTS=OFF \
    -DMCTCLIB_FIND_METHOD=fetch \
    "-DFETCHCONTENT_SOURCE_DIR_MCTC-LIB=$(cygpath -am "$mctc_source")" \
    "-DBLAS_LIBRARIES=$(cygpath -am "$prefix/lib/libopenblas.dll.a")" \
    "-DLAPACK_LIBRARIES=$(cygpath -am "$prefix/lib/libopenblas.dll.a")"
"$cmake_bin" --build "$(cygpath -am "$build_root/build")" --parallel "${XTB_JOBS:-4}"

# Stage only the executable and its recursive non-system DLL import closure.
# Existing source-tree runtime files (including licenses) are never deleted.
stage=$(mktemp -d "$build_root/runtime-stage-XXXXXX")
cp -- "$build_root/build/xtb.exe" "$stage/xtb.exe"
declare -A copied=()
queue=(xtb.exe)
system_dir=$(cygpath -au "${SYSTEMROOT:-C:/Windows}/System32")
for ((index=0; index<${#queue[@]}; index++)); do
    imports=$(objdump -p "$stage/${queue[index]}")
    while read -r dll; do
        [[ -n $dll && ! ${copied[${dll,,}]:-} ]] || continue
        copied[${dll,,}]=1
        if [[ -f "$prefix/bin/$dll" ]]; then
            cp -- "$prefix/bin/$dll" "$stage/$dll"
            queue+=("$dll")
        elif [[ ${dll,,} == api-ms-win-* || ${dll,,} == ext-ms-win-* || -f "$system_dir/$dll" ]]; then
            : # Windows supplies these DLLs; never redistribute system files.
        else
            die "Unresolved runtime dependency: $dll"
        fi
    done < <(printf '%s\n' "$imports" | awk '/DLL Name:/ {print $3}')
done

# Retain the reviewed repository notices, never substitute package-manager text.
notices="$project_root/xtb_runtime/gfnff/licenses"
for notice in COPYING COPYING.LESSER OpenBLAS/LICENSE OpenBLAS/LICENSE-lapack \
    gcc-libgfortran/COPYING.RUNTIME gcc-libs/COPYING3 gcc-libs/COPYING.RUNTIME winpthreads/COPYING; do
    [[ -f "$notices/$notice" ]] || die "Missing reviewed runtime notice: $notice"
done
mkdir -p "$stage/licenses"
cp -R -- "$notices/." "$stage/licenses/"
# These additions come only from the checksum-verified source archives above.
cp -- "$mctc_source/LICENSE" "$stage/licenses/mctc-lib-LICENSE"
cp -- "$xtb_source/src/solv/ddcosmo/LICENSE" "$stage/licenses/ddcosmo-LICENSE"
cp -- "$project_root/xtb_runtime/gfnff/RUNTIME.md" "$stage/RUNTIME.md"
{
    printf 'xtb commit: %s\nxtb.zip SHA256: %s\nmctc.zip SHA256: %s\n' "$xtb_commit" "$xtb_sha" "$mctc_sha"
    gcc --version
    gfortran --version
    "$cmake_bin" --version
    "$ninja_bin" --version
    pacman -Q mingw-w64-ucrt-x86_64-gcc-fortran mingw-w64-ucrt-x86_64-openblas
    (cd -- "$stage" && sha256sum xtb.exe ./*.dll)
} > "$stage/BUILDINFO.txt"
mkdir -p "$runtime_dir"
cp -R -- "$stage/." "$runtime_dir/"
printf 'Built runtime: %s\nSource archives (source-only): %s\n' "$runtime_dir" "$build_root"
