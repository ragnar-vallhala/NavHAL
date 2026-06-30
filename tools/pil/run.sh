#!/usr/bin/env bash
# Build the on-target test ELF for <board> and run it under that board's
# PIL emulator. The per-board config files under tools/pil/boards/
# describe the toolchain, build dir, runner script, and apt deps; this
# script is the single entry point CI (and humans) call.
#
# Usage:
#   tools/pil/run.sh <board>                  # build + run
#   tools/pil/run.sh <board> --install-deps   # install this board's deps, then exit
#   tools/pil/run.sh --list                   # list known boards
#
# Adding a new MCU is one new file under tools/pil/boards/<name>.conf
# plus (optionally) a new runner script under tools/<emulator>/.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO_ROOT"

BOARDS_DIR="tools/pil/boards"

if [ $# -lt 1 ] || [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
  echo "Usage: $0 <board> [--install-deps]" >&2
  echo "       $0 --list" >&2
  exit 2
fi

if [ "$1" = "--list" ]; then
  echo "Known boards:"
  for f in "$BOARDS_DIR"/*.conf; do
    [ -f "$f" ] || continue
    name=$(basename "$f" .conf)
    arch=$(grep '^ARCH=' "$f" | head -1 | cut -d= -f2)
    printf '  %-20s  arch=%s\n' "$name" "$arch"
  done
  exit 0
fi

BOARD=$1
shift || true
INSTALL_DEPS=0
for arg in "$@"; do
  case "$arg" in
    --install-deps) INSTALL_DEPS=1 ;;
    *) echo "error: unknown flag '$arg'" >&2; exit 2 ;;
  esac
done

CONF="$BOARDS_DIR/$BOARD.conf"
if [ ! -f "$CONF" ]; then
  echo "error: no board config at $CONF" >&2
  echo "available boards:" >&2
  ls "$BOARDS_DIR" 2>/dev/null | sed 's/\.conf$//' | sed 's/^/  /' >&2
  exit 2
fi

# shellcheck source=/dev/null
. "$CONF"

# Required vars.
for v in ARCH TOOLCHAIN_FILE BUILD_DIR RUNNER; do
  if [ -z "${!v:-}" ]; then
    echo "error: $CONF is missing required variable $v" >&2
    exit 2
  fi
done

# Optional dep installation pass (CI uses this; humans typically don't).
# Two stages: apt for distro-packaged deps, then an optional SETUP_SCRIPT
# for things that aren't apt-packaged (e.g. Renode).
#
# --install-deps is install-ONLY and exits when done: the CI workflow runs
# it as a separate "Install deps" stage before the "Build + run" stage, so
# falling through to build+run here would run the suite twice. The second,
# back-to-back Renode boot races the dotnet child the first run leaves
# behind and intermittently writes an empty UART log — a spurious CI red.
# Humans wanting one shot run `--install-deps` once, then the plain form.
if [ "$INSTALL_DEPS" = "1" ]; then
  if [ -n "${APT_DEPS:-}" ]; then
    echo ">> apt install ($BOARD): $APT_DEPS"
    sudo apt-get update
    # shellcheck disable=SC2086
    sudo apt-get install -y --no-install-recommends $APT_DEPS
  fi
  if [ -n "${SETUP_SCRIPT:-}" ]; then
    if [ ! -x "$SETUP_SCRIPT" ]; then
      echo "error: SETUP_SCRIPT '$SETUP_SCRIPT' not executable" >&2
      exit 2
    fi
    echo ">> running $SETUP_SCRIPT"
    "$SETUP_SCRIPT"
  fi
  echo ">> deps installed for $BOARD"
  exit 0
fi

echo ">> board=$BOARD arch=$ARCH toolchain=$TOOLCHAIN_FILE"
echo ">> build: $BUILD_DIR"

# Each PIL run starts from a clean .config so the toolchain file's
# NAVHAL_DEFCONFIG seeds the right target. Stash and restore the user's
# .config so an interactive run doesn't destroy local state.
SAVED=""
if [ -f .config ]; then
  SAVED=$(mktemp)
  mv .config "$SAVED"
fi
restore() {
  if [ -n "$SAVED" ] && [ -f "$SAVED" ]; then
    mv -f "$SAVED" .config
  else
    rm -f .config
  fi
}
trap restore EXIT

rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" -DTEST=ON -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" >/dev/null
cmake --build "$BUILD_DIR" --target tests -j >/dev/null

echo ">> running $RUNNER $BUILD_DIR/tests"
# Forward the board's optional RESC (Renode script) override to the runner.
env ${RESC:+RESC="$RESC"} "$RUNNER" "$BUILD_DIR/tests"
