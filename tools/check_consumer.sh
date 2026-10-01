#!/usr/bin/env bash
# Consumer check — NavHAL built the way a downstream project builds it.
#
# Every other tier builds NavHAL from inside this repo, where the compile and
# link flags come from CMAKE_C_FLAGS and CMAKE_EXE_LINKER_FLAGS. Those are
# directory-scoped variables: a parent project that does add_subdirectory() here
# does not inherit them. So anything NavHAL needs and does not put on the `hal`
# target is invisible to a consumer, and invisible to CI as well.
#
# That gap shipped three times. 0.3.3 moved each board's section layout into a shared
# arch script the board INCLUDEs by name, without carrying the -L that resolves
# it, so a consumer's link died on "cannot open linker script file cortex-m4.ld".
# 0.3.5 fixed that and left the twin in place: the public headers are written
# against NAVHAL_CONFIG_* symbols from a force-included navhal_target.h, so
# `#include "navhal.h"` did not compile outside the repo either. Neither was
# caught here, because nothing here ever built NavHAL from outside.
#
# The third was a consumer *flag* rather than a missing one. nav passes
# -DSUBMODULE when a dependency owns the CPU vectors, and the arch tree wrapped
# its nine weak system-exception handlers in `#ifndef SUBMODULE` while the
# startup vector table went on naming all nine -- so that build died on
# undefined references to its own vectors. The same guard also made
# Default_Handler a strong empty stub, shadowing the startup file's tail-branch
# into hal_irq_default_dispatch: that half links cleanly and silently stops every
# IRQ from reaching its callback, which is why the submodule rows below assert
# the branch survives rather than only that the link succeeded.
#
# So this builds a throwaway project against the working tree and asserts it
# compiles and links. The project supplies only what is genuinely its own
# choice -- which board's memory map to link against, and the bare-metal link
# mode. Everything NavHAL knows about its own build has to arrive with the
# target, which is exactly the property under test.
#
#   tools/check_consumer.sh            # every covered arch, both link modes
#   tools/check_consumer.sh m4         # one arch, both link modes
#
# Cortex only. AVR and x86-64 consumers are a different link shape (no -T of a
# board script), so covering them means a second fixture rather than another
# row in the table; the compile half of the contract is identical, so they are
# worth adding when someone consumes NavHAL on those targets.
#
# Exits non-zero if a consumer cannot build.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

WORK=""
SAVED_CONFIG=""

# Leave the developer's .config exactly as it was: seeding a board here would
# otherwise silently retarget their next in-tree build.
cleanup() {
  [[ -n "$WORK" && -d "$WORK" ]] && rm -rf "$WORK"
  if [[ -n "$SAVED_CONFIG" && -f "$SAVED_CONFIG" ]]; then
    mv -f "$SAVED_CONFIG" "$REPO_ROOT/.config"
  else
    rm -f "$REPO_ROOT/.config"
  fi
}
trap cleanup EXIT

arch_config() {
  case "$1" in
    m4)  TC=cmake/toolchains/arm-none-eabi-toolchain.cmake
         DEFCONFIG=cmake/defconfigs/cortex-m4_stm32f4_nucleo_f401re.defconfig
         BOARD=nucleo_f401re ;;
    m7)  TC=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake
         DEFCONFIG=cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig
         BOARD=nucleo_f767zi ;;
    *)   echo "unknown arch '$1' (m4|m7)" >&2; return 1 ;;
  esac
}

# check_arch <arch> [submodule]
#   submodule: build NavHAL the way nav builds it when a dependency owns the CPU
#   vectors (add_compile_definitions(SUBMODULE) before add_subdirectory). The
#   consumer still brings no handlers of its own -- NavHAL's weak ones have to
#   cover its vector table either way, since an RTOS that wants a vector takes it
#   by defining that one strongly.
check_arch() {
  local arch="$1" mode="${2:-plain}" proj label submodule_line=""
  arch_config "$arch"
  label="$arch"
  if [[ "$mode" == submodule ]]; then
    label="$arch+submodule"
    submodule_line="add_compile_definitions(SUBMODULE)"
  fi

  if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "  SKIP $label — arm-none-eabi-gcc not installed"
    return 0
  fi

  proj="$WORK/$arch-$mode"
  mkdir -p "$proj/src"

  # The whole point: no -I, no -L, no -include, no knowledge of NavHAL's
  # internal layout. Only the board script and the bare-metal link mode, which
  # are the consumer's own choices.
  cat > "$proj/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.20)
project(consumer C ASM)
$submodule_line
add_subdirectory($REPO_ROOT navhal)
add_executable(consumer src/main.c \${STARTUP_FILE})
# The script comes from NavHAL by name -- a consumer that spells out
# src/board/<board>/linker.ld is depending on NavHAL's directory layout, which
# is the thing this check exists to keep working.
if(NOT NAVHAL_LINKER_SCRIPT)
  message(FATAL_ERROR "NavHAL did not export NAVHAL_LINKER_SCRIPT")
endif()
target_link_options(consumer PRIVATE
  "-T" "\${NAVHAL_LINKER_SCRIPT}" "-nostdlib" "--specs=nosys.specs")
target_link_libraries(consumer PRIVATE hal)
EOF

  # Touches a board alias and a HAL call, so the include paths and the
  # force-included config header both have to have arrived.
  cat > "$proj/src/main.c" <<'EOF'
#include "board.h"
#include "navhal.h"

int main(void) {
  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  for (;;) {
    hal_gpio_toggle(LED_BUILTIN);
  }
}
EOF

  cp "$DEFCONFIG" "$REPO_ROOT/.config"

  # srctree is how NavHAL's Kconfig resolves its source globs; a consumer build
  # is not rooted here, so it has to be told. nav does the same thing.
  if ! srctree="$REPO_ROOT" cmake -S "$proj" -B "$proj/build" \
        -DCMAKE_TOOLCHAIN_FILE="$REPO_ROOT/$TC" >"$proj/cfg.log" 2>&1; then
    echo "  FAIL $label — configure failed"
    grep -iE 'error' "$proj/cfg.log" | head -5 | sed 's/^/       /'
    return 1
  fi

  if ! cmake --build "$proj/build" -j >"$proj/build.log" 2>&1; then
    echo "  FAIL $label — a consumer cannot build against this tree"
    grep -iE 'error|cannot open|undefined reference' "$proj/build.log" |
      head -5 | sed 's/^/       /'
    return 1
  fi

  if [[ ! -f "$proj/build/consumer" ]]; then
    echo "  FAIL $label — build reported success but produced no executable"
    return 1
  fi

  # The generic IRQ fallback has to survive the link: Default_Handler must still
  # be the startup file's tail-branch into hal_irq_default_dispatch. A stub that
  # shadowed it links perfectly and leaves every attached IRQ unable to fire, so
  # the link status alone cannot see this.
  # By address, not by label: a weak symbol with no extent gets folded into the
  # function before it, so grepping the disassembly for "<Default_Handler>:" can
  # miss a handler that is perfectly present.
  local dh_addr
  dh_addr=$(arm-none-eabi-nm "$proj/build/consumer" |
            awk '$3 == "Default_Handler" {print $1; exit}')
  if [[ -z "$dh_addr" ]]; then
    echo "  FAIL $label — no Default_Handler in the link at all"
    return 1
  fi
  if ! arm-none-eabi-objdump -d --start-address="0x$dh_addr" \
        --stop-address="$((0x$dh_addr + 8))" "$proj/build/consumer" |
        grep -q 'hal_irq_default_dispatch'; then
    echo "  FAIL $label — Default_Handler no longer reaches hal_irq_default_dispatch"
    echo "       (an IRQ with an attached callback would never be dispatched)"
    return 1
  fi

  echo "  OK   $label — consumer compiles and links ($(
    arm-none-eabi-size "$proj/build/consumer" | awk 'NR==2 {print $1" text, "$4" total"}'))"
  return 0
}

main() {
  local arches=("m4" "m7") rc=0
  [[ $# -ge 1 ]] && arches=("$1")

  WORK="$(mktemp -d)"
  if [[ -f "$REPO_ROOT/.config" ]]; then
    SAVED_CONFIG="$(mktemp)"
    cp "$REPO_ROOT/.config" "$SAVED_CONFIG"
  fi

  echo "==== consumer build (out-of-tree, add_subdirectory) ===="
  for a in "${arches[@]}"; do
    check_arch "$a" || rc=1
    check_arch "$a" submodule || rc=1
  done
  [[ $rc -eq 0 ]] && echo "==== NavHAL is consumable ====" \
                  || echo "==== a consumer cannot build NavHAL ===="
  return $rc
}

main "$@"
