#!/usr/bin/env bash
# Incremental macOS (Apple Silicon) build of flow5 against Homebrew's Qt, OpenCascade
# and gmsh; BLAS/LAPACK come from Apple's Accelerate. Runs on a Mac with Homebrew
# and the packages listed in .github/workflows/build.yml.
#
#   packaging/macos/build.sh      -> build/macos-release/flow5-app/flow5.app
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/macos-release}"
BREW="$(brew --prefix)"

# Homebrew's libraries are arm64-only and built for macOS 14+, so the upstream
# universal (x86_64 + arm64) / macOS 13.3 settings and /usr/local paths are
# overridden here. -after applies these once the .pro files have been read.
MIN_MACOS="${MIN_MACOS:-14.0}"

mkdir -p "$BUILD"
cd "$BUILD"

# see packaging/linux/build.sh for why qmake is rerun every time
"$BREW/bin/qmake" -r "$ROOT/flow5.pro" CONFIG+=release CONFIG-=debug -after \
    "QMAKE_APPLE_DEVICE_ARCHS=arm64" \
    "QMAKE_MACOSX_DEPLOYMENT_TARGET=$MIN_MACOS" \
    "INCLUDEPATH+=$BREW/include/opencascade $BREW/include" \
    "LIBS+=-L$BREW/lib" >/dev/null

make -j"$(sysctl -n hw.ncpu)"
