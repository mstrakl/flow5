#!/usr/bin/env bash
# Builds flow5 and the automated tests, then runs the tests headless (Qt offscreen platform).
# Runs inside the flow5-build container (see docker/dev.sh).
#
#   packaging/linux/test.sh [release|debug]
#
# Output: build/<config>/tests/; the exit status is non-zero if a test fails.
set -euo pipefail

CONFIG="${1:-release}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/$CONFIG}"

"$ROOT/packaging/linux/build.sh" "$CONFIG"

case "$CONFIG" in
    release) QCONFIG=(CONFIG+=release CONFIG-=debug) ;;
    debug)   QCONFIG=(CONFIG+=debug   CONFIG-=release) ;;
esac

mkdir -p "$BUILD/tests"
cd "$BUILD/tests"
# rerun qmake every time: the GUI tests link the app's object files, listed when qmake runs
qmake -r "$ROOT/tests/tests.pro" "${QCONFIG[@]}" "QMAKE_LFLAGS+=-fuse-ld=mold" >/dev/null
make -j"$(nproc)"

export LD_LIBRARY_PATH="$BUILD/XFoil-lib:$BUILD/flow5-lib:$BUILD/flow5-io-lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_QPA_PLATFORM=offscreen
# keep the tests' settings and files out of the user's home
export HOME="$BUILD/tests/home" XDG_CONFIG_HOME="$BUILD/tests/home/.config"
mkdir -p "$XDG_CONFIG_HOME"

status=0
for t in lib/tst_flow5lib gui/tst_gui; do
    echo "=== $t"
    "./$t" || status=1
done
exit $status
