#!/usr/bin/env bash
# =============================================================================
# tools/run_codeql.sh — run the CodeQL analysis locally, exactly as CI runs it.
#
#   tools/run_codeql.sh [m4|m7|avr|x86|all]      default: all
#
# Why this exists: alerts only appear in GitHub's Security tab after a push,
# which is a slow loop for triaging a backlog or for checking that a fix really
# cleared a finding. This builds the same databases .github/workflows/codeql.yml
# builds and runs the same query suite, so a local result and a CI result should
# agree.
#
# The targets are NOT interchangeable. CodeQL for C/C++ only sees translation
# units the build actually compiles, and no single NavHAL build compiles the
# whole tree -- the arch and vendor trees are mutually exclusive by design:
#
#   m4   -DTEST=ON, default (F401RE) toolchain: src/common, src/utils,
#        src/arch/armv7e-m, include/port/cortex-m4, the F4 vendor drivers, and
#        tests/arch/cortex-m4. Widest single database of the four.
#   m7   the F767 toolchain. Overlaps m4 heavily -- the two differ by the _f7
#        variants (clock_f7.c, i2c_f7.c, spi_f7.c, uart_f7.c), the M7 port
#        headers and tests/arch/cortex-m7. Four driver files is a whole database
#        for little, until one of them is the only place a finding lives: the
#        dead `pll_p == 0` branch in clock_f7.c is m7-only, and m4 cannot see it.
#   avr  src/arch/avr, src/vendor/microchip, include/port/avr. 16-bit int and
#        PROGMEM, so it is also where a size assumption that holds on 32-bit
#        surfaces.
#   x86  src/arch/x86_64, src/vendor/pc. Built as a sample, not as the test ELF:
#        the on-target test build is not wired for x86 yet
#        (cmake/arch/x86_64.cmake), so this one compiles no tests/.
#
# eth_f7.c and sdio.c are in the m4 database as well as the m7 one -- the vendor
# source list carries them for both families and the driver bodies are guarded
# inside. Nothing to fix; just do not read an m7-only alert as "an ETH alert".
#
# The host build (cmake -S tests/host) is deliberately NOT a target: it compiles
# a hand-picked handful of pure-logic files, every one of which the m4 build
# compiles too, so it would add a database and no coverage.
#
# Environment:
#   CODEQL=<path>    the codeql executable; default $HOME/codeql/codeql/codeql,
#                    then whatever is on PATH.
#   CODEQL_OUT=<dir> databases + SARIF; default build-codeql-local/ (gitignored
#                    by the build-*/ rule).
#
# Two things the local run does not know about, both worth remembering before
# reading anything into a diff against the Security tab:
#
#   1. Dismissals are server-side. An alert dismissed on GitHub still appears
#      here. Local output is the raw finding set, not the triaged one.
#   2. src/utils/fatfs/ is NOT analysed, by accident rather than by design: every
#      defconfig these four targets use leaves DRV_FS off, so no build compiles
#      it and the vendored third-party code produces no findings either way. A
#      target that enables it would start reporting them, and they could not be
#      filtered out (see the paths-ignore caveat in codeql.yml) -- they would be
#      dismissed in the Security tab instead. The summary tags them
#      [third-party] so they are at least obvious.
#
# Exit: 0 if the analysis ran, 1 if a build or the analysis itself failed, and
# 77 (vtest's SKIP) when the CLI is absent -- the bundle is a ~1.4 GB optional
# install, so "not run here" must not read as "passed". It does NOT fail on
# findings: this is a triage tool, and the gate is CI.
# =============================================================================
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR" || exit 1

OUT="${CODEQL_OUT:-$ROOT_DIR/build-codeql-local}"
WHICH="${1:-all}"

# The bundle ships the CLI with its query packs precompiled; a bare CLI would
# have to resolve codeql/cpp-queries over the network on first use.
CODEQL="${CODEQL:-$HOME/codeql/codeql/codeql}"
[ -x "$CODEQL" ] || CODEQL="$(command -v codeql 2>/dev/null)"
if [ -z "${CODEQL:-}" ] || [ ! -x "$CODEQL" ]; then
  echo "SKIP: 'codeql' CLI not installed (analysis not run)"
  cat <<'EOF'
  Install the bundle (CLI + precompiled query packs) once, ~1.4 GB extracted:

    gh release download codeql-bundle-v2.27.1 --repo github/codeql-action \
       --pattern codeql-bundle-linux64.tar.gz --dir "$HOME"
    mkdir -p "$HOME/codeql" && tar -xzf "$HOME/codeql-bundle-linux64.tar.gz" -C "$HOME/codeql"

  or set CODEQL=<path to the codeql executable>.
EOF
  exit 77
fi

SUITE="codeql/cpp-queries:codeql-suites/cpp-security-and-quality.qls"
mkdir -p "$OUT"
rc=0

# Every target seeds .config from its own toolchain's NAVHAL_DEFCONFIG, and the
# root CMakeLists only seeds when .config is ABSENT -- so a leftover .config
# would silently build the previous target's board under this target's compiler.
# Stash the user's file once, delete it before each target, restore on the way
# out, the same dance tools/pil/run.sh does.
SAVED=""
if [ -f .config ]; then
  SAVED="$(mktemp)"
  mv .config "$SAVED"
fi
restore() {
  if [ -n "$SAVED" ] && [ -f "$SAVED" ]; then mv -f "$SAVED" .config; else rm -f .config; fi
}
trap restore EXIT

# analyse <name> <build-dir> <build-command...>
analyse() {
  local name="$1" build_dir="$2"; shift 2
  local db="$OUT/db-$name" sarif="$OUT/$name.sarif"

  echo "=== $name: building + tracing ==="
  # A traced build must be a FULL build: CodeQL only records what it watches
  # compile, so an incremental build yields a database with almost nothing in it
  # and an analysis that cheerfully reports no problems.
  rm -rf "$db" "$build_dir" .config
  # The build goes through a script file rather than an inline --command:
  # CodeQL does not run --command through a shell, so `&&`, `export` and
  # redirection inside one are not shell syntax to it. A script keeps the
  # multi-step builds honest instead of silently tracing only their first word.
  local runner="$OUT/$name.build.sh"
  { echo '#!/usr/bin/env bash'; echo 'set -euo pipefail'; echo "cd $(printf '%q' "$ROOT_DIR")"; echo "$*"; } >"$runner"
  chmod +x "$runner"
  if ! "$CODEQL" database create "$db" --language=c-cpp --overwrite \
        --command="$runner" >"$OUT/$name.build.log" 2>&1; then
    echo "  FAIL: database create (see $OUT/$name.build.log)" >&2
    tail -15 "$OUT/$name.build.log" >&2
    rc=1; return
  fi

  echo "=== $name: analysing (security-and-quality) ==="
  if ! "$CODEQL" database analyze "$db" "$SUITE" \
        --format=sarif-latest --output="$sarif" --threads=0 \
        >"$OUT/$name.analyze.log" 2>&1; then
    echo "  FAIL: analyze (see $OUT/$name.analyze.log)" >&2
    tail -15 "$OUT/$name.analyze.log" >&2
    rc=1; return
  fi
  summarise "$name" "$sarif"
}

summarise() {
  python3 - "$1" "$2" <<'PY'
import json, sys, collections
name, path = sys.argv[1], sys.argv[2]
run = json.load(open(path))["runs"][0]
# SARIF puts the human-readable rule text in the driver's rules table, not on
# each result, so build the id -> description map once.
rules = {r["id"]: r.get("shortDescription", {}).get("text", "") for r in
         run.get("tool", {}).get("driver", {}).get("rules", [])}
rows = []
for res in run.get("results", []):
    rid = res.get("ruleId", "?")
    for loc in res.get("locations", [])[:1]:
        pl = loc.get("physicalLocation", {})
        f = pl.get("artifactLocation", {}).get("uri", "?")
        line = pl.get("region", {}).get("startLine", 0)
        rows.append((rid, f, line))
print(f"\n---- {name}: {len(rows)} finding(s) ----")
for rid, n in collections.Counter(r[0] for r in rows).most_common():
    print(f"  {n:3d}  {rid}    {rules.get(rid,'')}")
if rows:
    print()
    for rid, f, line in sorted(rows, key=lambda r: (r[0], r[1], r[2])):
        tag = " [third-party]" if f.startswith("src/utils/fatfs/") else ""
        print(f"  {rid}\t{f}:{line}{tag}")
PY
}

# want <target>; also skips a target whose compiler this machine lacks.
want() { [ "$WHICH" = "$1" ] || [ "$WHICH" = all ]; }
have() {
  command -v "$1" >/dev/null 2>&1 && return 0
  echo "SKIP $2: $1 not installed"; return 1
}

if want m4 && have arm-none-eabi-gcc m4; then
  analyse m4 "$ROOT_DIR/build-codeql-m4" \
    "cmake -B build-codeql-m4 -DTEST=ON && cmake --build build-codeql-m4 --target tests -j"
fi

if want m7 && have arm-none-eabi-gcc m7; then
  analyse m7 "$ROOT_DIR/build-codeql-m7" \
    "cmake -B build-codeql-m7 -DTEST=ON \
       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake && \
     cmake --build build-codeql-m7 --target tests -j"
fi

if want avr && have avr-gcc avr; then
  analyse avr "$ROOT_DIR/build-codeql-avr" \
    "cmake -B build-codeql-avr -DTEST=ON \
       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-toolchain.cmake && \
     cmake --build build-codeql-avr --target tests -j"
fi

if want x86 && have gcc x86; then
  analyse x86 "$ROOT_DIR/build-codeql-x86" \
    "cmake -B build-codeql-x86 \
       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-qemu-toolchain.cmake \
       -DSAMPLE=hal_x86_hello && \
     cmake --build build-codeql-x86"
fi

echo
echo "SARIF + databases under $OUT"
exit "$rc"
