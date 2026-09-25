#!/usr/bin/env bash
# Print the flow5 version (e.g. "7.6"), read from MAJOR_VERSION/MINOR_VERSION in
# flow5-lib/api/fl5core.h, which is the single place where the version is defined.
set -euo pipefail

HDR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/flow5-lib/api/fl5core.h"
MAJOR="$(sed -nE 's/^#define[[:space:]]+MAJOR_VERSION[[:space:]]+([0-9]+).*/\1/p' "$HDR")"
MINOR="$(sed -nE 's/^#define[[:space:]]+MINOR_VERSION[[:space:]]+([0-9]+).*/\1/p' "$HDR")"
[[ -n "$MAJOR" && -n "$MINOR" ]] || { echo "version.sh: cannot parse version from $HDR" >&2; exit 1; }
echo "$MAJOR.$MINOR"
