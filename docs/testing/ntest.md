# ntest — the NavHAL test orchestrator

`tools/ntest` is one entry point over every test tier, so you don't juggle a
dozen scripts. It owns the single `cmake -DTEST=ON` build recipe, the one
`Total failures:` parser, and a merged board registry; it shells to the
emulator backends and `st-flash` underneath.

```
ntest host                 # SIL — host unit + real-driver-on-host suites
ntest pil   <board|--all>  # PIL — build + run in the emulator (Renode/simavr)
ntest hil   <board|--all>  # HIL — build + flash + capture a real board
ntest samples <arch|--all> # build every sample the arch admits (m4|m7|avr)
ntest cap-contract         # link-time capability symbol contract
ntest coverage [--gate]    # host gcov/gcovr line coverage (+ optional floor)
ntest all [--hil]          # everything runnable here
ntest list                 # the catalog (boards, host suites, arches)
ntest                      # interactive TUI (a TTY) / `all` (piped)
```

Exit code is `0` if everything passed, `1` otherwise — so it drops straight
into CI. A summary table prints at the end:

```
== summary ==
  tests_host          PASS  24/24
  tests_host_drivers  PASS  62/62
  pil:nucleo_f767zi   PASS  182/182
  hil:nucleo_f401re   PASS  164/164
```

## What each tier is

| Tier | Command | What runs |
|---|---|---|
| **SIL** | `ntest host` | `tests_host` (pure logic) + `tests_host_drivers` (real F7 drivers on host MMIO), native gcc |
| **PIL** | `ntest pil <board>` | build the test ELF, run it in Renode (Cortex-M) or simavr (AVR) |
| **HIL** | `ntest hil <board>` | build → `st-flash` (chip-id matched) → capture the board's UART |
| **samples** | `ntest samples <arch>` | build every sample whose Kconfig `depends on ARCH_*` admits the arch |
| **contract** | `ntest cap-contract` | disabling a cap actually drops its driver symbols from the ELF |
| **coverage** | `ntest coverage` | host build with `--coverage`, `gcovr` summary over `src/` |

`hil` with no attached board **skips cleanly** (green) so CI/laptops stay
honest. Boards are matched to hardware by ST-Link chip-id, never a hard-coded
serial, so a checkout works on any bench.

## Board registry

`ntest` reads the existing `tools/hil/boards/*.conf` and `tools/pil/boards/*.conf`
and merges them by board name — one board can have both an HIL and a PIL
profile. Add a board by dropping a `.conf`; no ntest change needed.

## Relationship to the old scripts

`ntest` subsumes the per-tier runners. These are now thin deprecation shims
that `exec` the ntest equivalent (kept so hooks / muscle-memory keep working):

- `tools/run_host_tests.sh` → `ntest host`
- `tools/build_all_samples.sh` → `ntest samples m4`
- `tools/build_all_f767_samples.sh` → `ntest samples m7`
- `tools/build_all_avr_samples.sh` → `ntest samples avr`

The low-level backends stay and are called by ntest: `tools/renode/run_tests.sh`,
`tools/simavr/run_tests.sh`, `tools/test_cap_contract.sh`, `tools/samples_for_arch.sh`.

## CI

- `ci.yml`: `host-tests` → `ntest host`; `cap-contract` → `ntest cap-contract`;
  the three sample matrices → `ntest samples {m4,avr,m7}`.
- `renode.yml`: the PIL matrix run step → `ntest pil <board>`.
- `release-gate.yml`: adds a `coverage` job → `ntest coverage --gate`.

## Coverage floors

`ntest coverage --gate` enforces per-area line-coverage floors defined in the
`COVERAGE_FLOORS` dict at the top of `tools/ntest` (empty by default → report
only). Set e.g. `{"src/vendor/stm32": 60.0}` to ratchet coverage up.

## Requirements

Zero third-party deps for `host`/`samples`/`cap-contract`. `hil` needs
`pyserial` + `st-flash` (only when a board is attached); `pil` needs Renode /
simavr; `coverage` needs `gcovr`.
