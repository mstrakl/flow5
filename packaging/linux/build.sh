#!/usr/bin/env bash
# Incremental build of flow5. Runs inside the flow5-build container (see docker/dev.sh).
#
#   packaging/linux/build.sh [release|debug]
#
# Output: build/<config>/flow5-app/flow5 (+ the three shared libs in their subdirs).
# Object files persist in build/<config>, so only changed sources are recompiled;
# ccache (compiler cache) and mold (fast linker) make the rest cheap.
set -euo pipefail

CONFIG="${1:-release}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/$CONFIG}"

case "$CONFIG" in
    release) QCONFIG=(CONFIG+=release CONFIG-=debug) ;;
    debug)   QCONFIG=(CONFIG+=debug   CONFIG-=release) ;;
    *) echo "usage: $0 [release|debug]" >&2; exit 2 ;;
esac

mkdir -p "$BUILD"
cd "$BUILD"

# qmake computes header dependencies only when it runs, so rerun it every time to
# pick up new files and new #includes. It rewrites the Makefiles only; nothing is
# recompiled because of it.
qmake -r "$ROOT/flow5.pro" "${QCONFIG[@]}" "QMAKE_LFLAGS+=-fuse-ld=mold" >/dev/null

make -j"$(nproc)"
