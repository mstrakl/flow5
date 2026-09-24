#!/usr/bin/env bash
# CI helper: run a build command, and if it fails, repeat the first compiler errors
# and the end of the log as GitHub annotations, so they show on the run's summary
# page (and are readable through the public API, unlike the raw logs).
#
#   packaging/ci-run.sh <command> [args...]
set -uo pipefail

log="$(mktemp)"
"$@" 2>&1 | tee "$log"
rc=${PIPESTATUS[0]}

if [[ $rc != 0 ]]; then
    enc() { sed -e 's/%/%25/g' -e 's/\r//g' | awk '{printf "%s%%0A", $0}'; }
    errors="$(grep -E ':[0-9]+(:[0-9]+)?: (fatal )?error|error:|undefined reference|No such file|cannot find|\*\*\*' "$log" | head -30)"
    [[ -n "$errors" ]] && echo "::error title=First errors::$(printf '%s\n' "$errors" | cut -c1-400 | enc)"
    echo "::error title=End of log::$(tail -40 "$log" | cut -c1-400 | enc)"
fi
rm -f "$log"
exit "$rc"
