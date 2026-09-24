# Building flow5 in a container

Everything needed to build flow5 on Linux (Qt, OpenCascade, gmsh, OpenBLAS, GCC 14,
ccache, mold, linuxdeploy) lives in one image. The same image is used for day-to-day
development and for producing the AppImage in CI.

Requirements: Docker (or Podman) on a Linux host.

```bash
./docker/dev.sh image          # once, and after changes to docker/Dockerfile (~30 min first time)
./docker/dev.sh build          # incremental release build  -> build/release/
./docker/dev.sh run            # start the GUI from the container
./docker/dev.sh build debug    # separate debug tree        -> build/debug/
./docker/dev.sh run debug
./docker/dev.sh appimage       # -> build/dist/flow5-<version>-x86_64.AppImage
./docker/dev.sh shell          # shell inside the container
```

## Rebuild speed

- Object files persist in `build/<config>/`, so after editing a few files only those
  (and whatever includes a changed header) are recompiled.
- ccache (in `~/.cache/flow5-ccache`) makes rebuilds after `clean`, branch switches or
  release/debug toggles close to free.
- mold is used as the linker; linking against Qt + OpenCascade is otherwise the slow part.
- `build` reruns qmake each time, so new source files and new `#include`s are
  picked up automatically.

## GUI from the container

`run` passes through the X11/XWayland socket and `/dev/dri` (Mesa GPU acceleration on
Intel/AMD). Qt uses the `xcb` platform by default; `QT_QPA_PLATFORM=wayland
./docker/dev.sh run` uses native Wayland instead. Settings written by flow5 during
these runs go to `build/home/`.

NVIDIA's proprietary driver is not passed through; the app then falls back to
software OpenGL (llvmpipe), which works but is slower.

## Versions

Pinned in `docker/Dockerfile` (`ARG ...`): Qt, OpenCascade, gmsh, linuxdeploy.
The image is based on Ubuntu 24.04, so the AppImage needs glibc >= 2.39
(Ubuntu 24.04+, Debian 13+, Fedora 40+).
