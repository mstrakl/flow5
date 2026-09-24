#!/usr/bin/env bash
# Build flow5 and package it for Windows. Runs in an MSYS2 UCRT64 shell.
#
#   packaging/windows/package.sh
#     -> build/dist/flow5-<version>-win64-setup.exe   (NSIS installer)
#     -> build/dist/flow5-<version>-win64.zip         (portable)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build/windows-release"
DEPLOY="$ROOT/build/windows-deploy/flow5"
DIST="$ROOT/build/dist"

VERSION="${VERSION:-$(sed -nE 's/^VERSION *= *([0-9.]+).*/\1/p' "$ROOT/flow5-app/flow5-app.pro")}"

"$ROOT/packaging/windows/build.sh"

rm -rf "$DEPLOY"
mkdir -p "$DEPLOY" "$DIST"
cp "$BUILD/flow5-app/flow5.exe" \
   "$BUILD/XFoil-lib/"XFoil*.dll \
   "$BUILD/flow5-lib/"flow5-lib*.dll \
   "$BUILD/flow5-io-lib/"flow5-io-lib*.dll \
   "$DEPLOY/"

# Qt libraries and plugins
windeployqt6 --release --no-translations --compiler-runtime "$DEPLOY/flow5.exe"

# Everything else (OpenCascade, gmsh, OpenBLAS, their own deps, MinGW runtime):
# copy every DLL from the MSYS2 prefix that anything in $DEPLOY links to, until
# nothing new turns up.
PREFIX_BIN="$(cygpath -u "$MINGW_PREFIX")/bin"
while :; do
    new=0
    while read -r dll; do
        [[ -f "$DEPLOY/$(basename "$dll")" ]] && continue
        cp "$dll" "$DEPLOY/"
        new=1
    done < <(find "$DEPLOY" -name '*.exe' -o -name '*.dll' | xargs ldd 2>/dev/null \
                 | awk '{print $3}' | grep -i "^$PREFIX_BIN/" | sort -u)
    [[ $new == 0 ]] && break
done

cp "$ROOT/LICENSE" "$DEPLOY/LICENSE.txt"

# portable zip
rm -f "$DIST"/flow5-*-win64*
(cd "$(dirname "$DEPLOY")" && zip -qr "$DIST/flow5-$VERSION-win64.zip" flow5)

# installer
makensis -V2 -DVERSION="$VERSION" -DSRCDIR="$(cygpath -w "$DEPLOY")" \
    -DOUTFILE="$(cygpath -w "$DIST/flow5-$VERSION-win64-setup.exe")" \
    -DICON="$(cygpath -w "$ROOT/meta/win64/flow5.ico")" \
    "$(cygpath -w "$ROOT/packaging/windows/flow5.nsi")"

ls -la "$DIST"
