#!/usr/bin/env bash
# Print the sample slugs buildable on a given arch, honouring each sample's
# Kconfig `depends on ARCH_*` gate. A sample with no arch dependency (the
# portable tier) builds everywhere; one gated `depends on ARCH_CORTEX_M7` is
# emitted only for that arch, etc. Used by the build_all_*_samples.sh matrices
# so each arch builds exactly the samples that can run on it — no false reds
# from, say, an Ethernet (M7-only) sample attempted under the M4 toolchain.
#
# Usage: tools/samples_for_arch.sh <ARCH_SYMBOL>
#        e.g. ARCH_CORTEX_M4 | ARCH_CORTEX_M7 | ARCH_AVR8

set -euo pipefail

ARCH="${1:-}"
[ -n "$ARCH" ] || { echo "usage: $0 <ARCH_CORTEX_M4|ARCH_CORTEX_M7|ARCH_AVR8>" >&2; exit 2; }

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Two-part parse of samples/Kconfig: the `config SAMPLE_<N>_<NAME>` blocks carry
# an optional `depends on` arch gate; the `config SAMPLE` string block maps each
# `default "<slug>" if SAMPLE_<N>_<NAME>` to its symbol. Emit the slug when the
# symbol's gate admits ARCH (or has no arch gate).
awk -v ARCH="$ARCH" '
  /^[ \t]*#/                 { next }   # skip comments (header shows a "<slug>" example)
  /^config SAMPLE_/          { sym = $2; dep[sym] = ""; next }
  /^endchoice/              { sym = "" }
  /^config /                { sym = "" }
  sym != "" && $1 == "depends" && $2 == "on" {
    line = $0; sub(/^[ \t]*depends on[ \t]*/, "", line); dep[sym] = line; next
  }
  /^[ \t]*default "/ && /if SAMPLE_/ {
    match($0, /"[^"]+"/);            slug = substr($0, RSTART + 1, RLENGTH - 2)
    match($0, /SAMPLE_[A-Za-z0-9_]+/); s  = substr($0, RSTART, RLENGTH)
    d = (s in dep) ? dep[s] : ""
    if (slug != "" && (d == "" || index(d, ARCH) > 0)) print slug
  }
' "$REPO_ROOT/samples/Kconfig" | sort -u
