#!/usr/bin/env bash
# Installs the repo's git hooks for the current clone by pointing
# core.hooksPath at .githooks/. Run this once after cloning.
#
# Hooks installed:
#   pre-commit  — host tests + cmake configure (~1-2s)
#   pre-push    — capability contract + sample matrix (~30-60s)
#
# To uninstall: git config --unset core.hooksPath

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

if [ ! -d .githooks ]; then
  echo "error: .githooks/ not found at $REPO_ROOT" >&2
  exit 1
fi

git config core.hooksPath .githooks
chmod +x .githooks/*

CURRENT=$(git config --get core.hooksPath || echo "(unset)")
echo "core.hooksPath = $CURRENT"
ls -la .githooks
echo
echo "Installed. Bypass with --no-verify only when you've checked CI elsewhere."
