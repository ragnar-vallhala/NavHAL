@page roadmap_m8 M8 — CI tiering + portable test framework

# M8 — CI tiering + portable test framework

> Status: **done**
> Scope: scale CI matrix by changed-files; finish removing Cortex-M
> assumptions from the test framework so the same suite runs on every arch.
> Predecessor: M6 (AVR portability of the test suite landed Tier 1).
> Unlocks: 10+ arches without CI-runner exhaustion; HAL-only tests
> actually exercising every supported port.

## What landed

* §8.1 — Changed-files CI dispatch (commit `d40d18d`). Per-arch jobs
  in `ci.yml` and `renode.yml` gate on `dispatch.outputs.run-cortex-m`
  / `run-avr`. New `CI required` + `PIL required` aggregators handle
  branch-protection-friendly skipped-vs-failed semantics.
* §8.2 — Runner tiering doc lives at
  [docs/testing/model.md §"CI runner tiers"](../testing/model.md).
  Free GitHub-hosted runners cover every per-PR + release-gate job;
  self-hosted is reserved for HIL and any future >10-min PIL backend.
* §8.3 — Test reorg (commit `0439255`). Tests now live under
  `tests/portable/` (HAL-only, every arch), `tests/cap/<X>/`
  (capability-gated), `tests/arch/<arch>/` (white-box). No more
  source-level `#if defined(__arm__)` gates — directory membership
  drives selection via the per-tier CMake glob.
* §8.4 — Conformance suite (commit `47f62fb`, gating dropped in
  `30bf346`). `tests/portable/conformance/test_conformance.c`
  asserts `hal_status_t` semantics, the null-pointer contract on
  every init function, and that every `NAVHAL_HAS_*` macro is
  numerically defined. Runs on both Cortex-M4 (32 tests in the
  ELF) and AVR (32 tests in simavr, 0 failures), thanks to the
  navtest PROGMEM-string support that landed alongside.

## Branch-protection contexts to update on `stable`

The aggregators are now the right thing to require — drop the
individual per-arch contexts (they go "skipped" on arch-only PRs
and would block forever otherwise):

  remove: `Build on-target ELF`, `Capability contract`,
          `Build all samples`, `Build all AVR samples`,
          `PIL nucleo_f401re`, `PIL atmega328p`
  add:    `CI required`, `PIL required`

`Host tests`, `Commit message lint`, and `Release gate complete`
stay as they were.

## Goal

After M8:

1. A PR that touches `src/vendor/microchip/` doesn't rebuild Cortex
   anything. A PR that touches `include/common/hal_gpio.h` rebuilds
   everywhere.
2. The same `tests/test_hal_*.c` test runs on every supported arch
   without per-arch gating — gating only for true white-box tests
   that legitimately depend on a specific MCU's registers.

## Why now

**CI budget**: today's setup is 28 Cortex samples × 1 arch +
12 portable samples × AVR + 2 PIL emulators + release gate.
GitHub free runners handle that fine. Add Tier-1 RP2040, then ESP32,
then nRF52 — you're at 40+ build matrix cells per PR. Free runners
hit timing limits; self-hosted is the next step but you don't want
every PR using self-hosted bandwidth either.

**Test coverage gap**: Tier 1 of M6 only got AVR to *compile* the test
ELF. The actual portable-test count on AVR is small (21 today vs ~128
on Cortex) because most tests are white-box on STM32 registers. The
HAL contract — `hal_gpio_init` does what its docstring promises — is
the same on every port; the tests for that should be portable.

## What changes

### 8.1 — Changed-files CI dispatch

Add a `dispatch` job that runs first, computes which arches need
rebuilding from the diff, and emits a matrix for the downstream jobs.

```yaml
dispatch:
  outputs:
    archs: ${{ steps.compute.outputs.archs }}
  steps:
    - uses: dorny/paths-filter@v3
      id: filter
      with:
        filters: |
          common:
            - 'include/common/**'
            - 'tools/**'
            - 'cmake/**'
            - 'Kconfig'
            - 'CMakeLists.txt'
          cortex-m:
            - 'src/arch/armv7e-m/**'
            - 'src/vendor/stm32/**'
            - 'include/port/cortex-m4/**'
          avr:
            - 'src/arch/avr/**'
            - 'src/vendor/microchip/**'
            - 'include/port/avr/**'
    - id: compute
      run: |
        if [ "${{ steps.filter.outputs.common }}" = "true" ]; then
          echo "archs=[\"cortex-m4\",\"avr\"]" >> $GITHUB_OUTPUT
        else
          archs="[]"
          [ "${{ steps.filter.outputs.cortex-m }}" = "true" ] && archs="...cortex-m4..."
          [ "${{ steps.filter.outputs.avr }}" = "true" ] && archs="...avr..."
          echo "archs=$archs" >> $GITHUB_OUTPUT
        fi

sample-matrix:
  needs: dispatch
  strategy:
    matrix:
      arch: ${{ fromJson(needs.dispatch.outputs.archs) }}
  …
```

Cortex-only touches no longer fire the AVR matrix; common-header
touches fire both. The release gate (PRs to `stable`) still runs
everything regardless.

### 8.2 — Tiered runner pools

* **GitHub-hosted** for every per-PR job (free, fast enough).
* **Self-hosted** for any HIL job and for PIL runs longer than
  10 minutes (Renode on a large suite). Documented in
  `docs/testing/model.md`.
* Nightly schedule keeps running the full matrix on hosted runners
  as belt-and-suspenders.

### 8.3 — Portable test framework

Each test file gets classified into one of three buckets:

| Bucket | Where it lives | What it tests | Per-arch |
|---|---|---|---|
| **HAL-portable**  | `tests/portable/` | Black-box HAL contract behaviour | All arches |
| **Arch-specific** | `tests/arch/<arch>/` | White-box register / family-specific | One arch |
| **Cap-gated**     | `tests/cap/<cap>/`    | DMA / FPU / SDIO etc. behaviour     | Any arch with that cap |

Existing tests get audited and re-bucketed. The whole `#if defined(__arm__)`
gate inside `tests/test_*.c` files goes away because the file is in
`tests/arch/cortex-m4/` and CMake picks the right directory per arch.

`navtest` already has the abstraction (`NAVTEST_UART`, `navtest_in_pil`);
M8 finishes the job by moving the suite-table assembly into a
per-arch include so `tests/main.c` stops listing suites by hand.

### 8.4 — Conformance contract

Add a small "conformance" tier in `tests/portable/conformance/`:
mechanical assertions that the HAL contract documents (e.g.,
`hal_gpio_init` returns `HAL_OK` on first call and `HAL_ERR_*` on
second-call-with-conflicting-cfg). These tests are the source of
truth for "a port implements the HAL correctly" — and become the
gate ports must pass when M10 makes them packages.

## Cost estimate

* Changed-files dispatch: ~half a day, mostly YAML tuning.
* Runner-tier docs: half a day.
* Test reorg: **~1 week** — the bulk of the work. Audit every test
  file, decide the right bucket, move and de-gate. Risk: tests that
  thought they were HAL-portable but secretly depend on
  STM32 specifics surface as failures on AVR.
* Conformance suite: ~3 days (or less if existing tests are already
  HAL-portable and just need re-tagging).

## Exit criteria

* PR that only touches `samples/cortex-m/15_hal_uart_dma/main.c`
  runs only Cortex-M jobs in CI (≤ 5 min wall-clock).
* PR that touches `include/common/hal_uart.h` runs both arch matrices
  (current behaviour, preserved).
* `tests/test_*.c` files no longer carry `#if defined(__arm__)`
  blocks — arch-specific tests live in `tests/arch/<arch>/`.
* AVR test ELF runs at least 50 % of the suites Cortex-M runs (today: ~16 %).
* `tools/run_conformance.sh` exists and is required by the release gate.

## Open questions

* Which path-filter library? `dorny/paths-filter@v3` is most popular;
  alternatives are the native `paths:` filter on `pull_request`
  (less flexible — can't drive a matrix from it).
* For HIL (real boards): single shared runner or per-board?
  Per-board is honest but expensive; shared is fragile.
* The `tests/arch/<arch>/` move breaks all existing test paths
  referenced from docs and IDE bookmarks — coordinate with M7 so
  it lands in one big "tests reorg" PR.
