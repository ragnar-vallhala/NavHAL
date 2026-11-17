#!/usr/bin/env bash
# Fail when a change refers to an internal tracker item by number.
#
# Todo and room-message numbers mean nothing to a reader of the repository:
# they are renumbered when the tracker is archived, and the repository outlives
# any one tracker. Say why in words instead of pointing at a number.
#
# Checked, for the range given:
#   - every line a diff ADDS (file headers and hunk headers are skipped), and
#   - every commit message in the range.
#
#   tools/check_tracker_refs.sh origin/main..HEAD
#
# Exits non-zero and prints each offending line if there is a match.
set -euo pipefail

RANGE="${1:?usage: check_tracker_refs.sh <base>..<head>}"

# Split so this file does not match itself.
PAT='(todo|room|stack|ashu) #'"[0-9]+"

rc=0

# Added lines only. After a "diff --git" line, everything up to the first hunk
# is header (---/+++ lines, mode changes); skipping it stops a "+++ b/path"
# line being read as added text.
added=$(git diff --no-color --unified=0 "$RANGE" |
  awk '/^diff --git /{hdr=1;next} /^@@/{hdr=0;next} hdr{next} /^\+/{print}' |
  grep -niE "$PAT" || true)
if [[ -n "$added" ]]; then
  echo "tracker references in added lines (write the reason, not the number):"
  sed 's/^/  /' <<<"$added"
  rc=1
fi

msgs=$(git log --no-color --format='commit %h%n%B' "$RANGE" | grep -niE "$PAT" || true)
if [[ -n "$msgs" ]]; then
  echo "tracker references in commit messages (write the reason, not the number):"
  sed 's/^/  /' <<<"$msgs"
  rc=1
fi

[[ $rc -eq 0 ]] && echo "no tracker references in $RANGE"
exit $rc
