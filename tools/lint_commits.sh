#!/usr/bin/env bash
# Validate the commit-message format of every commit in a range against the
# same rules as .githooks/commit-msg. Used by CI to catch --no-verify bypasses.
#
# Usage:
#   tools/lint_commits.sh                # checks origin/main..HEAD
#   tools/lint_commits.sh <base>..<head> # arbitrary range
#   tools/lint_commits.sh <sha>          # single commit

set -eu

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

HOOK="$REPO_ROOT/.githooks/commit-msg"
[ -x "$HOOK" ] || { echo "error: $HOOK not executable" >&2; exit 2; }

RANGE=${1:-}
if [ -z "$RANGE" ]; then
  if git rev-parse --verify origin/main >/dev/null 2>&1; then
    RANGE="origin/main..HEAD"
  else
    echo "error: no range given and origin/main not found" >&2
    exit 2
  fi
fi

# Resolve the range to a list of SHAs. If the arg is a single SHA, use it.
if git cat-file -e "${RANGE}^{commit}" 2>/dev/null && ! [[ "$RANGE" == *..* ]]; then
  SHAS="$RANGE"
else
  SHAS=$(git rev-list --reverse "$RANGE")
fi

[ -n "$SHAS" ] || { echo "no commits in range $RANGE"; exit 0; }

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

PASS=0
FAIL=0
while read -r sha; do
  git log -1 --format="%B" "$sha" > "$TMP"
  if "$HOOK" "$TMP" 2>/dev/null; then
    printf '  \033[32mOK\033[0m   %s  %s\n' "${sha:0:8}" "$(head -n1 "$TMP")"
    PASS=$((PASS+1))
  else
    printf '  \033[31mFAIL\033[0m %s  %s\n' "${sha:0:8}" "$(head -n1 "$TMP")"
    FAIL=$((FAIL+1))
    # Re-run to show the hook's error message
    "$HOOK" "$TMP" >&2 || true
  fi
done <<<"$SHAS"

echo
printf 'pass=%d fail=%d\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
