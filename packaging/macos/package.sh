#!/usr/bin/env bash
# Build flow5 and package it as a .dmg for Apple Silicon Macs.
#
#   packaging/macos/package.sh     -> build/dist/flow5-<version>-macos-arm64.dmg
#
# The app is ad-hoc signed only (no Apple Developer ID), so macOS quarantines it
# when downloaded. Users open it once with:
#   xattr -dr com.apple.quarantine /Applications/flow5.app
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build/macos-release"
STAGE="$ROOT/build/macos-dmg"
DIST="$ROOT/build/dist"
BREW="$(brew --prefix)"

VERSION="${VERSION:-$("$ROOT/packaging/version.sh")}"
DMG="$DIST/flow5-$VERSION-macos-arm64.dmg"

"$ROOT/packaging/macos/build.sh"

rm -rf "$STAGE"
mkdir -p "$STAGE" "$DIST"
cp -R "$BUILD/flow5-app/flow5.app" "$STAGE/"
APP="$STAGE/flow5.app"

# Qt frameworks + plugins, and every other non-system dylib (OpenCascade, gmsh and
# their deps) into Contents/Frameworks, with load paths rewritten.
"$BREW/bin/macdeployqt" "$APP" -libpath="$BREW/lib" -verbose=1

# Apple Silicon refuses to run unsigned code; an ad-hoc signature is enough.
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

ln -s /Applications "$STAGE/Applications"
rm -f "$DMG"
hdiutil create -volname "flow5 $VERSION" -srcfolder "$STAGE" -fs HFS+ -format UDZO -ov "$DMG"

ls -la "$DIST"
