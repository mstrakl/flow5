#!/usr/bin/env bash
# Incremental Windows build of flow5 with MinGW-w64. Runs in an MSYS2 UCRT64 shell
# (GitHub Actions, or a Windows PC with MSYS2 and the packages listed in
# .github/workflows/build.yml).
#
#   packaging/windows/build.sh     -> build/windows-release/flow5-app/flow5.exe
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/windows-release}"

mkdir -p "$BUILD"
cd "$BUILD"

# see packaging/linux/build.sh for why qmake is rerun every time
qmake6 -r "$ROOT/flow5.pro" CONFIG+=release CONFIG-=debug "QMAKE_LFLAGS+=-fuse-ld=lld" >/dev/null

mingw32-make -j"$(nproc)"
