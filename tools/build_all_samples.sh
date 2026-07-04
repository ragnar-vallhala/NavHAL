#!/usr/bin/env bash
# Builds every Cortex-M4-buildable sample (the default toolchain). Catches
# missing `select` clauses on SAMPLE_* entries (a sample that uses DRV_SDIO but
# doesn't select it will fail to link). Used by CI and the .githooks/pre-push
# hook.
#
# Samples gated to another arch (e.g. Ethernet, `depends on ARCH_CORTEX_M7`) are
# skipped here and covered by build_all_f767_samples.sh — see
# tools/samples_for_arch.sh for the gate-aware selection.
#
# Each sample is a fresh configure + build; about a second each.
#
# Usage: tools/build_all_samples.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# The default toolchain targets the Cortex-M4; build only the samples whose
# Kconfig arch gate admits it.
SAMPLES=$("$REPO_ROOT/tools/samples_for_arch.sh" ARCH_CORTEX_M4)

[ -n "$SAMPLES" ] || { echo "error: no ARCH_CORTEX_M4 samples parsed from Kconfig" >&2; exit 2; }

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
