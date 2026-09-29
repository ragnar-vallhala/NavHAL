#!/usr/bin/env bash
# Consumer check — NavHAL built the way a downstream project builds it.
#
# Every other tier builds NavHAL from inside this repo, where the compile and
# link flags come from CMAKE_C_FLAGS and CMAKE_EXE_LINKER_FLAGS. Those are
# directory-scoped variables: a parent project that does add_subdirectory() here
# does not inherit them. So anything NavHAL needs and does not put on the `hal`
# target is invisible to a consumer, and invisible to CI as well.
#
# That gap shipped twice. 0.3.3 moved each board's section layout into a shared
# arch script the board INCLUDEs by name, without carrying the -L that resolves
# it, so a consumer's link died on "cannot open linker script file cortex-m4.ld".
# 0.3.5 fixed that and left the twin in place: the public headers are written
# against NAVHAL_CONFIG_* symbols from a force-included navhal_target.h, so
# `#include "navhal.h"` did not compile outside the repo either. Neither was
# caught here, because nothing here ever built NavHAL from outside.
#
# So this builds a throwaway project against the working tree and asserts it
# compiles and links. The project supplies only what is genuinely its own
# choice -- which board's memory map to link against, and the bare-metal link
# mode. Everything NavHAL knows about its own build has to arrive with the
# target, which is exactly the property under test.
#
#   tools/check_consumer.sh            # every covered arch
#   tools/check_consumer.sh m4         # one arch
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

check_arch() {
  local arch="$1" proj
  arch_config "$arch"

  if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "  SKIP $arch — arm-none-eabi-gcc not installed"
    return 0
  fi

  proj="$WORK/$arch"
  mkdir -p "$proj/src"

  # The whole point: no -I, no -L, no -include, no knowledge of NavHAL's
  # internal layout. Only the board script and the bare-metal link mode, which
  # are the consumer's own choices.
  cat > "$proj/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.20)
project(consumer C ASM)
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
    echo "  FAIL $arch — configure failed"
    grep -iE 'error' "$proj/cfg.log" | head -5 | sed 's/^/       /'
    return 1
  fi

  if ! cmake --build "$proj/build" -j >"$proj/build.log" 2>&1; then
    echo "  FAIL $arch — a consumer cannot build against this tree"
    grep -iE 'error|cannot open|undefined reference' "$proj/build.log" |
      head -5 | sed 's/^/       /'
    return 1
  fi

  if [[ ! -f "$proj/build/consumer" ]]; then
    echo "  FAIL $arch — build reported success but produced no executable"
    return 1
  fi

  echo "  OK   $arch — consumer compiles and links ($(
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
  done
  [[ $rc -eq 0 ]] && echo "==== NavHAL is consumable ====" \
                  || echo "==== a consumer cannot build NavHAL ===="
  return $rc
}

main "$@"
