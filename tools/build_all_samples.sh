#!/usr/bin/env bash
# Builds every sample declared in Kconfig. Catches missing `select` clauses
# on SAMPLE_* entries (a sample that uses DRV_SDIO but doesn't select it
# will fail to link). Used by CI and the .githooks/pre-push hook.
#
# Each sample is a fresh configure + build; about a second each.
#
# Usage: tools/build_all_samples.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Sample names extracted from the SAMPLE Kconfig string defaults
# (config SAMPLE / default "<name>" if SAMPLE_<N>_*).
SAMPLES=$(awk '
  /^config SAMPLE$/        { in_sample = 1; next }
  in_sample && /^config /  { in_sample = 0 }
  in_sample && /default "/ {
    match($0, /"[^"]+"/);
    if (RSTART) {
      name = substr($0, RSTART + 1, RLENGTH - 2);
      if (name != "") print name;
    }
  }
' Kconfig samples/Kconfig 2>/dev/null | sort -u)

[ -n "$SAMPLES" ] || { echo "error: no SAMPLE names parsed from Kconfig" >&2; exit 2; }

PASS=0
FAIL=0
FAIL_LIST=""
BUILD=build-sample-matrix

# Build from a clean, Kconfig-default .config rather than whatever target a
# previous local build left in the repo root. A leftover AVR .config (e.g.
# from an atmega build) points every no-toolchain sample build at avr-gcc,
# which then chokes on the Cortex-M startup .s files — a false red that only
# reproduces on the polluted machine, never in clean CI. Stash and restore.
SAVED_CONFIG=""
if [ -f .config ]; then
  SAVED_CONFIG=$(mktemp)
  mv .config "$SAVED_CONFIG"
fi

cleanup() {
  rm -rf "$BUILD"
  if [ -n "$SAVED_CONFIG" ] && [ -f "$SAVED_CONFIG" ]; then
    mv -f "$SAVED_CONFIG" .config
  else
    rm -f .config
  fi
}
trap cleanup EXIT

for sample in $SAMPLES; do
  rm -rf "$BUILD"
  if cmake -B "$BUILD" -DSAMPLE="$sample" >/dev/null 2>&1 \
     && cmake --build "$BUILD" -j >/dev/null 2>&1; then
    printf '  \033[32mOK\033[0m   %s\n' "$sample"
    PASS=$((PASS+1))
  else
    printf '  \033[31mFAIL\033[0m %s\n' "$sample"
    FAIL=$((FAIL+1))
    FAIL_LIST="$FAIL_LIST $sample"
  fi
done

echo
echo "==== Summary ===="
printf 'pass=%d fail=%d\n' "$PASS" "$FAIL"
if [ "$FAIL" -gt 0 ]; then
  echo "Failed samples:$FAIL_LIST" >&2
  echo "Re-run a single failure for the full error:" >&2
  echo "  cmake -B build-X -DSAMPLE=<name> && cmake --build build-X -j" >&2
  exit 1
fi
