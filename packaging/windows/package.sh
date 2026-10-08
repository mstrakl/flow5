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

VERSION="${VERSION:-$("$ROOT/packaging/version.sh")}"

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
# Recent MSYS2 nsis packages no longer tell makensis where their plugins are (the log shows an
# empty "Plugin directories:" and "Plugin not found, cannot call nsDialogs::Create"), so locate
# the Unicode plugins in the MSYS2 prefix and register them explicitly.
NSIS_PLUGIN="$(find "$(cygpath -u "$MINGW_PREFIX")" -type f -iname 'nsDialogs.dll' -ipath '*x86-unicode*' 2>/dev/null | head -1)"
if [[ -z "$NSIS_PLUGIN" ]]; then
    echo "package.sh: NSIS plugin nsDialogs.dll (x86-unicode) not found under $MINGW_PREFIX; NSIS files present:" >&2
    find "$(cygpath -u "$MINGW_PREFIX")" -maxdepth 5 -ipath '*nsis*' 2>/dev/null | head -60 >&2
    exit 1
fi
NSIS_PLUGINDIR="$(cygpath -w "$(dirname "$NSIS_PLUGIN")")"
echo "NSIS plugins: $NSIS_PLUGINDIR"

# MSYS2 must not rewrite the "/x86-unicode" switch inside the -X argument as a path
MSYS2_ARG_CONV_EXCL='-X' \
makensis -V2 "-X!addplugindir /x86-unicode \"$NSIS_PLUGINDIR\"" \
    -DVERSION="$VERSION" -DSRCDIR="$(cygpath -w "$DEPLOY")" \
    -DOUTFILE="$(cygpath -w "$DIST/flow5-$VERSION-win64-setup.exe")" \
    -DICON="$(cygpath -w "$ROOT/meta/win64/flow5.ico")" \
    "$(cygpath -w "$ROOT/packaging/windows/flow5.nsi")"

ls -la "$DIST"
