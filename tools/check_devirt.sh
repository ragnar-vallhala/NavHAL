#!/usr/bin/env bash
# Devirtualisation check — the vtable's cost-of-abstraction claim, enforced.
#
# M9 dispatches every driver call through a `const` ops table. The claim that
# this costs nothing rests on one thing: under -flto the table is resolved at
# link time and the indirect call becomes a direct one. That is measurable, so
# it should not be asserted in a document and left there.
#
# This builds a sample under ReleaseLTO and gates on one thing: no `_hal_*_ops`
# symbol survives in the binary. A table every caller resolved has no remaining
# reference, so the linker drops it; a table still present is one something
# still reads at run time, which is the regression worth catching.
#
# It deliberately does not gate on counting indirect branches. Every port has
# some that are indirect by design -- a callback the application registered at
# run time, dispatched from an ISR -- and which instruction that becomes is an
# accident of the target: `bx r3` from armv7em_interrupt_dispatch, SysTick_Handler
# and hal_irq_default_dispatch on ARM, `icall` from __vector_14 on AVR. A count
# would fail on a sample that registers one more callback, which says nothing
# about devirtualisation. They are listed with the function they sit in, so a
# new one can be recognised for what it is.
#
# Byte size is not a gate either: it moves with -Os and with whatever the
# sample does, so it proves nothing on its own.
#
#   tools/check_devirt.sh              # every arch, hal_blink
#   tools/check_devirt.sh m4           # one arch
#   tools/check_devirt.sh m4 hal_timer      # names as tools/samples_for_arch.sh lists them
#
# Exits non-zero if any indirect dispatch survives.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SAMPLE="${2:-hal_blink}"
BUILD_DIR="build-devirt"

# arch -> toolchain, tool prefix, and the two instruction classes.
#
# ARM: `blx <reg>` calls through a register, `bx <reg>` tail-branches through
# one. `bx lr` is how every function returns, so matching on `r[0-9]+` leaves
# it out. AVR: icall/eicall call through Z, ijmp/eijmp tail-branch through it.
arch_config() {
  case "$1" in
    m4)  TC=cmake/toolchains/arm-none-eabi-toolchain.cmake
         TOOLPREFIX=arm-none-eabi
         CALL_PATTERN='blx[[:space:]]+r[0-9]+'
         TAIL_PATTERN='bx[[:space:]]+r[0-9]+' ;;
    m7)  TC=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake
         TOOLPREFIX=arm-none-eabi
         CALL_PATTERN='blx[[:space:]]+r[0-9]+'
         TAIL_PATTERN='bx[[:space:]]+r[0-9]+' ;;
    avr) TC=cmake/toolchains/avr-toolchain.cmake
         TOOLPREFIX=avr
         CALL_PATTERN='e?icall'
         TAIL_PATTERN='e?ijmp' ;;
    *)   echo "unknown arch '$1' (m4|m7|avr)" >&2; return 1 ;;
  esac
}

check_arch() {
  local arch="$1" elf
  arch_config "$arch"

  if ! command -v "${TOOLPREFIX}-objdump" >/dev/null 2>&1; then
    echo "  SKIP $arch — ${TOOLPREFIX}-objdump not installed"
    return 0
  fi

  rm -rf "$BUILD_DIR" .config
  cmake -B "$BUILD_DIR" -DSAMPLE="$SAMPLE" \
        -DCMAKE_TOOLCHAIN_FILE="$TC" \
        -DCMAKE_BUILD_TYPE=ReleaseLTO >/dev/null 2>&1
  cmake --build "$BUILD_DIR" -j >/dev/null 2>&1

  # CMake drops the binary at samples/<group>/<nn_name>/<name>; the sample is
  # the only executable under there.
  elf="$(find "$BUILD_DIR/samples" -type f -perm -u+x \
         -not -name '*.*' 2>/dev/null | head -1)"
  if [[ -z "$elf" ]]; then
    echo "  FAIL $arch — no executable built for $SAMPLE"
    return 1
  fi

  local objdump="${TOOLPREFIX}-objdump" nm="${TOOLPREFIX}-nm"
  local tables sites nsites

  tables="$("$nm" "$elf" | grep -E '_hal_[a-z0-9_]+_ops$' || true)"

  # Each remaining indirect branch, labelled with the function containing it.
  sites="$("$objdump" -d "$elf" |
           awk -v pat="($CALL_PATTERN|$TAIL_PATTERN)" '
             /^[0-9a-f]+ </ { fn = $2 }
             $0 ~ pat      { print "       " fn " " $NF }' || true)"
  nsites="$(printf '%s' "$sites" | grep -c . || true)"

  if [[ -n "$tables" ]]; then
    echo "  FAIL $arch — ops table(s) still referenced after LTO:"
    echo "$tables" | sed 's/^/       /'
    return 1
  fi

  echo "  OK   $arch — every ops table resolved away ($nsites callback site(s))"
  [[ -n "$sites" ]] && echo "$sites"
  return 0
}

main() {
  local arches=("m4" "m7" "avr") rc=0
  [[ $# -ge 1 ]] && arches=("$1")

  echo "==== devirtualisation (ReleaseLTO, $SAMPLE) ===="
  for a in "${arches[@]}"; do
    check_arch "$a" || rc=1
  done
  rm -rf "$BUILD_DIR" .config
  [[ $rc -eq 0 ]] && echo "==== all dispatches devirtualised ====" \
                  || echo "==== indirect dispatch survived ===="
  return $rc
}

main "$@"
