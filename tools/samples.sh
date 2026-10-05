#!/usr/bin/env bash
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# Build every sample an arch admits, one clean configure per sample.
#
# Usage:
#   tools/samples.sh <m4|m7|avr>              # one arch, its primary board
#   tools/samples.sh <arch> --board <name>    # one arch, one named board
#   tools/samples.sh <arch> --all-boards      # one arch, every board it has
#   tools/samples.sh --all                    # all three arches, primary boards
#   tools/samples.sh --list <arch>            # the sample slugs
#   tools/samples.sh --list-boards <arch>     # the boards found for an arch
#
# An arch is not a board. Two boards of the same arch differ in their generated
# board.h, their pin maps and which drivers their Kconfig admits, so a sample that
# builds for a Nucleo can still fail for a custom board -- and until this had a
# board axis, every board but one was only ever built by its HIL run, which does
# not happen in CI and does not happen at all for a board nobody has plugged in.
#
# Boards are discovered from cmake/defconfigs/*.defconfig rather than listed here,
# so adding a board adds it to the matrix. Two kinds are skipped: fragments named
# boot_*, which configure a bootloader image rather than a board, and the acme
# vendor, which is a scaffolding fixture with no drivers behind it.
#
# Which samples an arch admits comes from tools/samples_for_arch.sh, which
# reads each sample's Kconfig `depends on ARCH_*` gate. The toolchain file is
# pinned per arch rather than left to the default: with no toolchain file there
# is no NAVHAL_DEFCONFIG, so the board falls out of the Kconfig choice default,
# and that moves whenever a board is added to the family.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

BUILD_DIR=build-sample
ARCHES="ARCH_CORTEX_M4 ARCH_CORTEX_M7 ARCH_AVR8"

# echoes "BOARD<TAB>defconfig" per board of this arch, primary first.
# No pipeline: a `for` loop feeding one would run in a subshell, and the primary
# picked there would be invisible to the other side -- which silently dropped
# every arch's main board from the list.
boards_for() {
  local arch="$1" want="${1#ARCH_}" f a b primary="" rest=""
  for f in cmake/defconfigs/*.defconfig; do
    case "$(basename "$f")" in boot_*) continue ;; esac
    # The acme vendor is a fixture: it exists to prove a new vendor port can be
    # added and carries no drivers, so 31 of its 35 samples cannot build and the
    # number says nothing about any board. The cap-contract tier is what covers it.
    grep -q '^CONFIG_VENDOR_ACME=y' "$f" && continue
    a=$(grep -oE 'CONFIG_ARCH_[A-Z0-9_]+=y' "$f" | head -1 | sed 's/CONFIG_ARCH_//; s/=y//')
    [ "$a" = "$want" ] || continue
    b=$(grep -oE 'CONFIG_BOARD_[A-Z0-9_]+=y' "$f" | head -1 | sed 's/CONFIG_BOARD_//; s/=y//')
    [ -n "$b" ] || continue
    # The toolchain's own fragment is the primary board; it goes first so the
    # default run is the one that was always run.
    if [ "$f" = "$(toolchain_defconfig "$arch")" ]; then
      primary="$b	$f"
    else
      rest="${rest}${b}	${f}
"
    fi
  done
  [ -n "$primary" ] && printf '%b\n' "$primary"
  [ -n "$rest" ] && printf '%b' "$rest"
  return 0
}

toolchain_defconfig() {
  case "$1" in
    ARCH_CORTEX_M4) echo cmake/defconfigs/cortex-m4_stm32f4_nucleo_f401re.defconfig ;;
    ARCH_CORTEX_M7) echo cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig ;;
    ARCH_AVR8)      echo cmake/defconfigs/avr_atmega328p.defconfig ;;
    *) return 1 ;;
  esac
}

toolchain_for() {
  case "$1" in
    ARCH_CORTEX_M4) echo cmake/toolchains/arm-none-eabi-toolchain.cmake ;;
    ARCH_CORTEX_M7) echo cmake/toolchains/arm-none-eabi-f767-toolchain.cmake ;;
    ARCH_AVR8)      echo cmake/toolchains/avr-toolchain.cmake ;;
    *) return 1 ;;
  esac
}

normalize() {  # alias -> ARCH_* symbol
  case "$(echo "$1" | tr '[:upper:]' '[:lower:]')" in
    m4|f401|cortex-m4)   echo ARCH_CORTEX_M4 ;;
    m7|f767|cortex-m7)   echo ARCH_CORTEX_M7 ;;
    avr|avr8|atmega)     echo ARCH_AVR8 ;;
    arch_cortex_m4)      echo ARCH_CORTEX_M4 ;;
    arch_cortex_m7)      echo ARCH_CORTEX_M7 ;;
    arch_avr8)           echo ARCH_AVR8 ;;
    *) echo "error: unknown arch '$1' (m4|m7|avr)" >&2; return 2 ;;
  esac
}

# Each sample configures from scratch, so .config has to be absent for the
# toolchain's NAVHAL_DEFCONFIG to seed the target. Stash the user's and put it
# back however we exit -- an interactive run must not lose local state.
SAVED=""
if [ -f .config ]; then SAVED=$(mktemp); mv .config "$SAVED"; fi
restore() {
  rm -rf "$BUILD_DIR"
  rm -f .config
  if [ -n "$SAVED" ] && [ -f "$SAVED" ]; then mv -f "$SAVED" .config; fi
}
trap restore EXIT

build_arch() {  # $1 = ARCH_*, $2 = board name, $3 = defconfig; returns #failures
  local arch="$1" board="${2:-}" dc="${3:-}" tc slug nfail=0 npass=0 fails=""
  tc=$(toolchain_for "$arch")

  local slugs
  slugs=$(tools/samples_for_arch.sh "$arch")
  if [ -z "$slugs" ]; then
    echo "!! no samples admit $arch" >&2
    return 1
  fi

  echo "=================================================================="
  echo ">> samples arch=$arch board=${board:-primary} toolchain=$tc"
  local dcarg=() out nskip=0
  [ -n "$dc" ] && dcarg=(-DNAVHAL_DEFCONFIG="$dc")
  for slug in $slugs; do
    rm -rf "$BUILD_DIR"
    rm -f .config
    if out=$( { cmake -B "$BUILD_DIR" -DSAMPLE="$slug" "${dcarg[@]}" \
                  -DCMAKE_TOOLCHAIN_FILE="$tc" &&
                cmake --build "$BUILD_DIR" -j ; } 2>&1 ); then
      npass=$((npass + 1))
      echo "  OK   $slug"
    elif printf '%s' "$out" | grep -q '#error'; then
      # The sample refused this board itself. A sample that drives two UARTs on a
      # board with one spare is not a broken build, and counting it as one would
      # make a board axis useless the day it was added.
      nskip=$((nskip + 1))
      echo "  N/A  $slug -- $(printf '%s' "$out" | grep -m1 '#error' | sed 's/.*#error *//; s/^"//; s/"$//')"
    else
      nfail=$((nfail + 1))
      fails="$fails $slug"
      echo "  FAIL $slug"
    fi
  done
  echo ">> $arch${board:+/$board}: $npass/$((npass + nfail)) built${nskip:+, $nskip n/a}${fails:+, failed:$fails}"
  return "$nfail"
}

[ $# -ge 1 ] || { echo "usage: $0 <m4|m7|avr> | --all | --list <arch>" >&2; exit 2; }

case "$1" in
  -h|--help) sed -n '6,12p' "$0"; exit 0 ;;
  --list)
    [ $# -ge 2 ] || { echo "usage: $0 --list <arch>" >&2; exit 2; }
    tools/samples_for_arch.sh "$(normalize "$2")"
    exit 0
    ;;
  --list-boards)
    [ $# -ge 2 ] || { echo "usage: $0 --list-boards <arch>" >&2; exit 2; }
    boards_for "$(normalize "$2")" | cut -f1
    exit 0
    ;;
  --all)
    overall=0
    for a in $ARCHES; do build_arch "$a" || overall=$((overall + $?)); done
    echo "=================================================================="
    echo ">> samples --all aggregate failure count: $overall"
    exit "$overall"
    ;;
esac

ARCH=$(normalize "$1")
shift

case "${1:-}" in
  --all-boards)
    overall=0
    while IFS=$'\t' read -r board dc; do
      [ -n "$board" ] || continue
      build_arch "$ARCH" "$board" "$dc" || overall=$((overall + $?))
    done < <(boards_for "$ARCH")
    echo "=================================================================="
    echo ">> $ARCH all boards: aggregate failure count: $overall"
    exit "$overall"
    ;;
  --board)
    [ $# -ge 2 ] || { echo "usage: $0 <arch> --board <name>" >&2; exit 2; }
    want=$(echo "$2" | tr '[:lower:]-' '[:upper:]_')
    found=""
    while IFS=$'\t' read -r board dc; do
      [ "$board" = "$want" ] && found="$dc"
    done < <(boards_for "$ARCH")
    if [ -z "$found" ]; then
      echo "error: no board '$2' for $ARCH. Known:" >&2
      boards_for "$ARCH" | cut -f1 | sed 's/^/  /' >&2
      exit 2
    fi
    build_arch "$ARCH" "$want" "$found"
    exit $?
    ;;
  "") build_arch "$ARCH" ;;
  *) echo "error: unexpected argument '$1'" >&2; exit 2 ;;
esac
