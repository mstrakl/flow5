#!/usr/bin/env bash
# Host-side helper for the flow5 build container.
#
#   ./docker/dev.sh image                 build (or update) the container image
#   ./docker/dev.sh build [release|debug] incremental build   -> build/<config>/
#   ./docker/dev.sh run   [release|debug] [-- flow5 args]  start the GUI from the container
#   ./docker/dev.sh appimage              release build + AppImage -> build/dist/
#   ./docker/dev.sh shell                 interactive shell in the container
#   ./docker/dev.sh clean [release|debug] delete a build tree
#
# The repo is mounted at the same path as on the host (so compiler errors are
# clickable in the IDE) and everything runs as your user, so build files are yours.
#
# Environment:
#   ENGINE=docker|podman   container engine (default: docker, else podman)
#   FLOW5_IMAGE=...        image name (default: flow5-build:latest)
#   QT_QPA_PLATFORM=...    Qt platform for `run` (default: xcb, via XWayland on Wayland)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${FLOW5_IMAGE:-flow5-build:latest}"
ENGINE="${ENGINE:-$(command -v docker >/dev/null && echo docker || echo podman)}"
CCACHE_HOST="${XDG_CACHE_HOME:-$HOME/.cache}/flow5-ccache"

run_in() {
    mkdir -p "$CCACHE_HOST"
    local args=(--rm -v "$ROOT:$ROOT" -w "$ROOT" -v "$CCACHE_HOST:/ccache")
    if [[ "$ENGINE" == podman ]]; then
        args+=(--userns=keep-id --group-add keep-groups --security-opt label=disable)
    else
        args+=(--user "$(id -u):$(id -g)" -e HOME=/tmp)
    fi
    [[ -t 0 && -t 1 ]] && args+=(-it)
    "$ENGINE" run "${args[@]}" "${EXTRA_ARGS[@]}" "$IMAGE" "$@"
}

gui_args() {
    EXTRA_ARGS+=(--ipc=host -e QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}")
    # X11 / XWayland
    if [[ -n "${DISPLAY:-}" ]]; then
        EXTRA_ARGS+=(-e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix)
        if [[ -n "${XAUTHORITY:-}" && -f "$XAUTHORITY" ]]; then
            EXTRA_ARGS+=(-v "$XAUTHORITY:/tmp/.Xauthority:ro" -e XAUTHORITY=/tmp/.Xauthority)
        fi
    fi
    # native Wayland (used with QT_QPA_PLATFORM=wayland)
    if [[ -n "${WAYLAND_DISPLAY:-}" && -S "${XDG_RUNTIME_DIR:-}/$WAYLAND_DISPLAY" ]]; then
        EXTRA_ARGS+=(-e WAYLAND_DISPLAY -e XDG_RUNTIME_DIR=/tmp/xdg
                     -v "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY:/tmp/xdg/$WAYLAND_DISPLAY")
    fi
    # GPU (Mesa: Intel/AMD). The render group is needed to open /dev/dri/renderD*.
    if [[ -d /dev/dri ]]; then
        EXTRA_ARGS+=(--device /dev/dri)
        if [[ "$ENGINE" != podman ]]; then
            local gid
            for gid in $(stat -c %g /dev/dri/* 2>/dev/null | sort -u); do
                EXTRA_ARGS+=(--group-add "$gid")
            done
        fi
    fi
    # keep flow5 settings between runs
    mkdir -p "$ROOT/build/home"
    EXTRA_ARGS+=(-e HOME="$ROOT/build/home")
}

config_arg() {
    case "${1:-release}" in
        release|debug) echo "${1:-release}" ;;
        *) echo "unknown config '$1' (use release or debug)" >&2; exit 2 ;;
    esac
}

EXTRA_ARGS=()
cmd="${1:-help}"; shift || true

case "$cmd" in
    image)
        "$ENGINE" build -t "$IMAGE" "$@" "$ROOT/docker"
        ;;
    build)
        run_in packaging/linux/build.sh "$(config_arg "${1:-}")"
        ;;
    run)
        cfg=release
        if [[ $# -gt 0 && "$1" != -- ]]; then cfg="$(config_arg "$1")"; shift; fi
        [[ "${1:-}" == -- ]] && shift
        bin="$ROOT/build/$cfg"
        [[ -x "$bin/flow5-app/flow5" ]] || { echo "no $cfg build yet: ./docker/dev.sh build $cfg" >&2; exit 1; }
        gui_args
        EXTRA_ARGS+=(-e LD_LIBRARY_PATH="$bin/XFoil-lib:$bin/flow5-lib:$bin/flow5-io-lib")
        run_in "$bin/flow5-app/flow5" "$@"
        ;;
    appimage)
        run_in env ${VERSION:+VERSION="$VERSION"} packaging/linux/appimage.sh
        ;;
    shell)
        gui_args
        run_in bash
        ;;
    clean)
        rm -rf "$ROOT/build/$(config_arg "${1:-}")"
        ;;
    help|-h|--help)
        sed -n '2,/^set -euo/{/^set -euo/d;s/^# \{0,1\}//;p}' "$0"
        ;;
    *)
        echo "unknown command '$cmd' (see ./docker/dev.sh help)" >&2; exit 2
        ;;
esac
