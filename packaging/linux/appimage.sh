#!/usr/bin/env bash
# Build a release and package it as an AppImage. Runs inside the flow5-build container.
#
#   packaging/linux/appimage.sh            -> build/dist/flow5-<version>-x86_64.AppImage
#   VERSION=7.57-rc1 packaging/linux/appimage.sh
#
# linuxdeploy copies every non-system library flow5 needs (Qt, OpenCascade, gmsh,
# OpenBLAS, libgfortran, the flow5 libs) into the AppDir and fixes their rpaths;
# the Qt plugin adds the Qt platform/image plugins.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build/release"
APPDIR="$ROOT/build/AppDir"
DIST="$ROOT/build/dist"

VERSION="${VERSION:-$(sed -nE 's/^VERSION *= *([0-9.]+).*/\1/p' "$ROOT/flow5-app/flow5-app.pro")}"

"$ROOT/packaging/linux/build.sh" release

rm -rf "$APPDIR"
mkdir -p "$DIST"
install -Dm755 "$BUILD/flow5-app/flow5"          "$APPDIR/usr/bin/flow5"
install -Dm644 "$ROOT/meta/res/flow5.png"        "$APPDIR/usr/share/icons/hicolor/128x128/apps/flow5.png"
install -Dm644 "$ROOT/packaging/linux/flow5.desktop" "$APPDIR/usr/share/applications/flow5.desktop"

export LD_LIBRARY_PATH="$BUILD/XFoil-lib:$BUILD/flow5-lib:$BUILD/flow5-io-lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QMAKE="$QTDIR/bin/qmake"
# xcb works everywhere (X11 and XWayland); add native Wayland too
export EXTRA_PLATFORM_PLUGINS="libqwayland.so"
export EXTRA_QT_MODULES="svg;waylandclient"
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"

cd "$DIST"
rm -f flow5-*.AppImage
linuxdeploy --appdir "$APPDIR" \
    --desktop-file "$APPDIR/usr/share/applications/flow5.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/128x128/apps/flow5.png" \
    --plugin qt

# The Qt plugin deploys libqwayland.so but not the Wayland shell/decoration plugins
# it needs; without them Qt falls back to XWayland. Their rpath ($ORIGIN/../../lib)
# already points at the bundled Qt libs.
for d in wayland-shell-integration wayland-decoration-client wayland-graphics-integration-client; do
    cp -r "$QTDIR/plugins/$d" "$APPDIR/usr/plugins/"
done

linuxdeploy --appdir "$APPDIR" --output appimage

echo "==> $DIST/flow5-$VERSION-x86_64.AppImage"
