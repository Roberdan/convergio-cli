#!/usr/bin/env bash
set -euo pipefail

root="$(git rev-parse --show-toplevel)"

bad_files="$(cd "$root" && git ls-files -- "V7Plan*.md" | grep -v '^docs/plans/V7Plan/' || true)"

if [[ -n "${bad_files}" ]]; then
  echo "ERROR: V7 plan files found outside docs/plans/V7Plan/:" >&2
  echo "${bad_files}" >&2
  exit 1
fi

echo "OK: All V7 plan files are under docs/plans/V7Plan/."

