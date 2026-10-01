#!/usr/bin/env bash
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# Build every sample an arch admits, one clean configure per sample.
#
# Usage:
#   tools/samples.sh <m4|m7|avr>   # one arch
#   tools/samples.sh --all         # all three
#   tools/samples.sh --list <arch> # the slugs, without building
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

build_arch() {  # $1 = ARCH_* symbol; echoes a line per sample, returns #failures
  local arch="$1" tc slug nfail=0 npass=0 fails=""
  tc=$(toolchain_for "$arch")

  local slugs
  slugs=$(tools/samples_for_arch.sh "$arch")
  if [ -z "$slugs" ]; then
    echo "!! no samples admit $arch" >&2
    return 1
  fi

  echo "=================================================================="
  echo ">> samples arch=$arch toolchain=$tc"
  for slug in $slugs; do
    rm -rf "$BUILD_DIR"
    rm -f .config
    if cmake -B "$BUILD_DIR" -DSAMPLE="$slug" \
         -DCMAKE_TOOLCHAIN_FILE="$tc" >/dev/null 2>&1 &&
       cmake --build "$BUILD_DIR" -j >/dev/null 2>&1; then
      npass=$((npass + 1))
      echo "  OK   $slug"
    else
      nfail=$((nfail + 1))
      fails="$fails $slug"
      echo "  FAIL $slug"
    fi
  done
  echo ">> $arch: $npass/$((npass + nfail)) built${fails:+, failed:$fails}"
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
  --all)
    overall=0
    for a in $ARCHES; do build_arch "$a" || overall=$((overall + $?)); done
    echo "=================================================================="
    echo ">> samples --all aggregate failure count: $overall"
    exit "$overall"
    ;;
esac

build_arch "$(normalize "$1")"
