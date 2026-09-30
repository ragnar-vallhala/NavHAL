@page test_vtest vtest integration

# Running NavHAL's suites with vtest

[vtest](https://github.com/ragnar-vallhala/vtest) is the test orchestrator the
rest of the stack (vaios, Vayu, NavLink) runs its suites with: one TUI and one
`--run` batch report across ctest suites and whole-repo checks. **In NavHAL it
replaces `tools/ntest`.** vtest is the front end (TUI, catalog, batch report);
the per-tier shell scripts do the work, and CI calls those same scripts
directly.

```sh
tools/vtest.sh              # interactive TUI
tools/vtest.sh --run        # everything, batch report
tools/vtest.sh --list       # the catalog
```

## Install vtest (once per machine)

vtest is a system tool shared by every repo on the machine — NavHAL does not
build it, and pins it as neither a submodule nor a clone. vaios pulls NavHAL in
as `extern/NavHAL`, and a nested vtest pin would ride along into every vaios
checkout.

```sh
git clone https://github.com/ragnar-vallhala/vtest.git ~/src/vtest
cd ~/src/vtest
cmake -S . -B build && cmake --build build && cmake --install build --prefix ~/.local
vtest --version          # e.g. vtest v1.3.0-9-ge33986f
```

That puts `vtest` and `vtest-loc` (vtest's `loc.sh`) in `~/.local/bin`, which
must be on `PATH`. Re-run the same commands after pulling vtest. `VTEST=path`
forces a specific binary.

## The catalog

`vtest.conf` at the repo root declares the suites; it is the only NavHAL file
vtest reads. Two adapters cover everything here:

- **`ctest`** — the host suites, which `tests/host/CMakeLists.txt` registers
  with `add_test`. Each binary shows as its own case and builds alone when
  selected. `[host]` is the plain build; `[host-asan]` is the same cases in a
  build dir configured with ASan + UBSan.
- **`check`** — one command, one verdict, its exit status the result: the sample
  matrices, the capability contract, coverage, the line count, CodeQL, and one
  suite per PIL and HIL board. **Exit 77 is SKIP**, which is how a HIL board that
  is not plugged in reports — a bench carrying two of the five boards is the
  normal case, not a report with three failures in it — and also how `[codeql]`
  reports on a machine without the CLI bundle installed.

A new board is a new `.conf` under `tools/{pil,hil}/boards/` plus one block in
`vtest.conf`.

### HIL and the probe bus

`vtest --run` runs every HIL suite, and a board it cannot find has to be
identified before it can be skipped. For a `CONSOLE=swd` board, pin its probe by
USB location and the runner skips enumeration altogether:

```sh
export NAVHAL_HIL_LOCATION_NAVIXSMF401RE=3-2      # per lsusb -t
```

Without a pin the runner falls back to `st-info --probe`, which opens every
ST-Link on the bus to read its target's chip-id — and so resets whatever is
running behind the other probes. On a bench with more than one probe, run the
HIL suites one board at a time with that board pinned, not `--run`.

## CI

CI does not go through vtest: each job already has its own toolchain and runs
one tier, so it calls that tier's script — the same one the suite above runs.

| job | command |
|-----|---------|
| ci.yml `host-tests` | `cmake -S tests/host -B build-host && cmake --build build-host -j && ctest --test-dir build-host --output-on-failure` |
| ci.yml `cap-contract` | `tools/test_cap_contract.sh` |
| ci.yml `sample-matrix*` | `tools/samples.sh m4\|avr\|m7` |
| renode.yml | `tools/pil/run.sh <board>` |
| release-gate.yml `coverage` | `tools/coverage.sh` |
| release-gate.yml `sanitizer-host` | its own cmake flags (the `[host-asan]` suite locally) |
| ci.yml `x86-smoke` | `tools/qemu/smoke.sh <sample> <expected>` |
| codeql.yml | the CodeQL action, one job per arch (see below) |

`codeql.yml` is the one job that does **not** call the local script: the
analysis has to run inside `github/codeql-action`, which does its own build
tracing, so the workflow repeats the four build commands rather than invoking
`tools/run_codeql.sh`. The script exists to run the same four locally — alerts
otherwise only appear in the Security tab after a push, which is a slow loop for
triage. Keep the two in step when a build command changes.

`x86-smoke` is deliberately **not** in `vtest.conf`: `smoke.sh` rewrites the
tree's `.config` and deletes it on the way in, which is fine in a throwaway
checkout and not fine under a TUI someone runs mid-edit. Run it by hand.

## What replaced ntest

| ntest | now |
|-------|-----|
| `ntest host` | `ctest --test-dir build-host`, via the ctest adapter |
| `ntest pil <board>` | `tools/pil/run.sh <board>` |
| `ntest hil <board>` | `tools/hil/run.sh <board>` — gained ntest's SWD capture, the USB-location pin, and exit 77 |
| `ntest samples <arch>` | `tools/samples.sh <arch>`; the three `build_all_*samples.sh` shims call it |
| `ntest cap-contract` | `tools/test_cap_contract.sh` |
| `ntest coverage [--gate]` | `tools/coverage.sh [--min-lines N]` — the gate is gcovr's `--fail-under-line` |
| `ntest tui`, `list`, `all` | vtest |

`tools/ntest`, `docs/testing/ntest.md` and `tools/run_host_tests.sh` are gone.
The sample shims stay, because the hooks and muscle memory call them.

## Which vtest am I running?

```sh
command -v vtest && vtest --version
```

Exit 77 as SKIP needs vtest newer than v1.3.0. An older one reports a missing
HIL board as FAIL.
