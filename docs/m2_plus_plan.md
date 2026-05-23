# M2+ — Comprehensive test suite

**Status:** Approved 2026-05-21
**Companion to:** `docs/api_standardization.md`, `docs/execution_plan.md` §5+

## Context

M2 froze the standardized `hal_{p}_*` API surface across 13 drivers (~68 `hal_status_t` functions plus non-fallible getters). The current `tests/` directory has ~3–6 tests per driver that still call the **deprecated** legacy API (`uart2_init`, `hal_gpio_setmode`, etc.), exercises only happy paths, and has no CI. M3 (directory restructure) and M4 (Kconfig-driven dispatch) are large mechanical refactors that can silently regress behavior, and M6 (AVR port) needs a conformance bar.

M2+ grows the suite into full-surface coverage of the standardized API, fixes the build separation bug that blocks a clean `-DTEST=ON` configure, and stands up CI with both a fast host-runnable subset and a Renode-emulated on-target run of the full ELF. The sequencing is **infra-first, then driver waves**; the CI strategy is **Renode + host subset both**.

Acceptance (from `docs/execution_plan.md` §5+, §12):

- Every standardized `hal_*` function has at least one success-path test; every `hal_status_t`-returning function has at least one error-path test.
- `tests` binary builds from one clean `cmake -DTEST=ON`, with the sample collision resolved.
- Host subset runs natively in CI; Renode runs the on-target ELF in CI.
- `docs/testing.md` documents on-target vs host build/run and how to add a test for a new driver.

## PR breakdown (9 PRs)

### PR1 — Build separation + harness refactor *(WI2+.1, WI2+.6)*

**Problem.** Root `CMakeLists.txt` line 127 unconditionally adds `samples/` when `SAMPLE` is set, even with `-DTEST=ON`. Every sample declares `add_custom_target(flash …)` and the test build declares `flash_tests` — but if any sample is in the configure, the `flash` target also lands in the test build. Also, the 27 manual `RUN_TEST(...)` calls in `tests/main.c` are duplicated across each driver's group function.

**Changes.**

- `CMakeLists.txt`: make `TEST` exclusive — if `TEST=ON`, skip the `samples/` subdir entirely. Document the rule in a comment near the `if(SAMPLE …)` block.
- `tests/main.c` → split: each `tests/test_{driver}.c` exports a `const navtest_case_t test_{driver}_cases[]` table (function ptr + name). `tests/main.c` becomes ~40 lines: iterates a master `navtest_suite_t` array and prints the rollup.
- `include/navtest/navtest.h`: add `navtest_case_t`/`navtest_suite_t` and a `NAVTEST_RUN_SUITE()` macro that walks the case table — replaces the manual `RUN_TEST` boilerplate. Keep `RUN_TEST()` for any one-off tests.
- Verify the existing on-target build still passes (no API changes in this PR; only the harness shape).

Files: `CMakeLists.txt`, `tests/main.c`, every `tests/test_*.c` (boilerplate-only edit), `include/navtest/navtest.h`.

### PR2 — Host-runnable subset *(WI2+.5)*

**Goal.** Add a build mode `cmake -DTEST_HOST=ON` that compiles the spec-listed pure-logic tests with the system `gcc` (no toolchain prefix) and runs as a native binary.

**Changes.**

- New top-level `cmake/host.cmake` (or guarded block in root `CMakeLists.txt`) — when `TEST_HOST=ON`, do **not** set `CMAKE_C_COMPILER` to `arm-none-eabi-gcc`, skip the linker script, drop `-mcpu`/`-mthumb`/FPU flags.
- New `tests/host/` directory with:
  - `tests/host/main.c` — host entry point; calls into the host-test suites and prints to `stdout`.
  - `tests/host/test_conversion.c` — covers `include/utils/conversion.h` helpers.
  - `tests/host/test_hal_status.c` — `HAL_OK_OR_RETURN` short-circuits, enum stability (`HAL_OK == 0`, `HAL_ERR == 1`).
  - `tests/host/test_crc_sw.c` — software CRC reference vector path (the parts of `crc.c` that are not register-bound).
  - `tests/host/test_gpio_encoding.c` — `GPIO_Pxnn` enum encoding/decoding round-trips from `include/utils/gpio_types.h`.
- `include/navtest/navtest.h`: gate the `uart2_write` calls behind a `NAVTEST_BACKEND_*` macro; add a host backend that writes to `stdout`.

Files: `CMakeLists.txt`, `cmake/host.cmake`, `tests/host/*`, `include/navtest/navtest.h`.

### PR3 — Renode emulator + GitHub Actions CI *(WI2+.7)*

**Goal.** Replace the current Pages-only workflow with a real CI matrix.

**Changes.**

- New `.github/workflows/ci.yml` with three jobs:
  1. **`host-tests`** — Ubuntu runner, `cmake -DTEST_HOST=ON && ctest`. Fast (<30 s).
  2. **`build-on-target`** — install `arm-none-eabi-gcc`, `cmake -DTEST=ON && make tests`, upload `tests.elf` as a workflow artifact.
  3. **`renode-on-target`** — depends on `build-on-target`, downloads the ELF, installs Renode, runs `tools/renode/navhal_f401re.resc` which boots the F401RE machine model, loads the ELF, redirects USART2 to a file, runs for a bounded number of ticks, exits, then a small script greps the captured UART log for `"FAIL"` and the final `Total failures: 0` line.
  4. (optional) **`build-no-cap`** — `cmake -DTEST=ON -DCONFIG_DRV_DMA=n -DCONFIG_DRV_FPU=n -DCONFIG_DRV_SDIO=n -DCONFIG_DRV_DWT=n` builds; proves capability-gated drivers are cleanly absent.
- New `tools/renode/navhal_f401re.resc` and a thin `tools/renode/run_tests.sh` wrapper.
- Update `README.md` CI badge URL to point at the new workflow.

Files: `.github/workflows/ci.yml`, `tools/renode/*`, `README.md`.

### PR4 — Wave A: gpio, clock, interrupt *(WI2+.2 + WI2+.3, first slice)*

For each driver, rewrite `tests/test_{driver}.c` to call the standardized `hal_{p}_*` API only (drop all `uart2_*`, `hal_gpio_setmode`, etc.), and cover:

- **Lifecycle:** `hal_{p}_init` happy path + NULL-config → `HAL_ERR_INVALID_ARG`; `hal_{p}_deinit` round-trip.
- **Every public function** in `include/core/cortex-m4/{driver}.h` gets at least one success-path test.
- **Every `hal_status_t`-returning function** gets at least one error-path test (NULL pointer, bad id, out-of-range arg, pre-init call → `HAL_ERR_NOT_INITIALIZED`).
- Where two-layer GPIO is exercised (board macros), include a `LED_BUILTIN` / `GPIO_PA05` round-trip to lock the encoding contract for M6.

Files: `tests/test_gpio.{c,h}`, `tests/test_clock.{c,h}`, `tests/test_interrupt.{c,h}` (new), `tests/main.c` (suite table only — boilerplate already trimmed in PR1).

### PR5 — Wave B: timebase (was systick), timer

Same template as PR4. Special items for this wave:

- Lock the `uint32_t` tick decision (no `uint64_t` in any public API used by the tests).
- Reframe `test_systick_*` → `test_timebase_*` to match the M2 rename. Drop the legacy file once the new one is in.

Files: `tests/test_timer.{c,h}`, `tests/test_timebase.{c,h}` (replaces the partial `test_systick` calls in `test_timer.c`).

### PR6 — Wave C: uart, i2c, spi

Same template. Bus-specific items:

- I2C: confirm the `hal_i2c_id_t` typed param replaced the bare `uint8_t bus` everywhere in the tests.
- UART: keep one `hal_uart_print(id, val)` `_Generic` test, since the spec preserves the single optional helper.
- SPI: cover all transfer modes the standardized header exposes.

Files: `tests/test_uart.{c,h}` (renamed/expanded from `test_uart_protocol`), `tests/test_i2c.{c,h}`, `tests/test_spi.{c,h}`.

### PR7 — Wave D: pwm, crc, flash

Same template.

- CRC: the software path moves to the host subset in PR2; this PR adds the hardware-CRC path tests.
- Flash: extend beyond `test_flash_storage_integration` to cover every public function with success + error paths.

Files: `tests/test_pwm.{c,h}`, `tests/test_crc.{c,h}`, `tests/test_flash.{c,h}` (replaces `test_flash_raw.{c,h}`).

### PR8 — Wave E: dma, cycle_counter (was dwt), fpu, sdio *(WI2+.4)*

All four are capability-gated. For each:

- The driver's `tests/test_{driver}.c` is wrapped in `#if NAVHAL_HAS_{X}` so the suite still links when the capability is off.
- The Wave E suite is registered into `tests/main.c`'s suite table only when its `NAVHAL_HAS_*` is on.
- PR3's `build-no-cap` job proves the absence by building with these capabilities disabled and checking no symbol from the disabled driver landed in the ELF.
- `dwt` → `cycle_counter` rename completes the M2 work; remove the legacy `test_dwt.{c,h}` once `test_cycle_counter.{c,h}` lands.

Files: `tests/test_dma.{c,h}`, `tests/test_cycle_counter.{c,h}` (replaces `test_dwt`), `tests/test_fpu.{c,h}` (renamed from `test_fpu_accel`), `tests/test_sdio.{c,h}`.

### PR9 — `docs/testing.md` *(WI2+.8)*

Concise (one page) document covering:

1. **On-target run** — `cmake -B build -DTEST=ON && make -C build tests && make -C build flash_tests`. Connect a USB-TTL to UART2 and watch the navtest output.
2. **Host subset run** — `cmake -B build-host -DTEST_HOST=ON && cmake --build build-host && build-host/tests_host`.
3. **Renode run** — `tools/renode/run_tests.sh build/tests` → loads the ELF in the F401RE machine model and prints the captured UART log.
4. **Adding a test for a new driver** — pattern: `tests/test_{x}.{c,h}` with a `navtest_case_t test_{x}_cases[]` table; register in `tests/main.c`'s suite array; if the driver is capability-gated, wrap in `#if NAVHAL_HAS_{X}`.
5. **CI matrix** — what each job covers, when each one runs.

Files: `docs/testing.md`, `README.md` (link in).

---

## Critical files & where things live

| Purpose | Path |
|---|---|
| Standardized API surface to test | `include/core/cortex-m4/*.h` (68 `hal_status_t` funcs) |
| Public umbrella header | `include/common/hal_*.h` (still pre-M3 — wraps core) |
| Existing test harness | `include/navtest/navtest.h`, `tests/navtest_state.c` |
| Test entry point (refactored in PR1) | `tests/main.c` |
| CMake test integration (refactored in PR1) | `CMakeLists.txt` lines 181–225 |
| Sample collision source | `samples/*/CMakeLists.txt` — each has `add_custom_target(flash …)` |
| Kconfig (used in PR3 capability builds) | `Kconfig`, `tools/kconfig.py` |
| GPIO pin encoder under test | `include/utils/gpio_types.h` |
| Conversion utils for host subset | `include/utils/conversion.h` |
| Existing CI (will be replaced) | `.github/workflows/static.yml` (Pages-only) |

## Verification

- **PR1**: `cmake -B build -DTEST=ON && make -C build tests` configures and builds with no duplicate-target warnings; existing tests still pass on-target (no behavior change).
- **PR2**: `cmake -B build-host -DTEST_HOST=ON && cmake --build build-host && ./build-host/tests_host` exits 0; all four host suites print PASS.
- **PR3**: open a PR with a deliberately broken test; all three CI jobs go red; revert and they go green. Verify the Renode log contains the navtest summary line.
- **PR4–PR8**: per-driver — `make tests && make flash_tests` on the F401RE, watch UART for `0 failures`; `make ci-renode` locally produces the same. Each PR also bumps coverage: at the end of PR8, every header function in `include/core/cortex-m4/*.h` is referenced by at least one test, verified by a `grep`-based audit script committed in PR8.
- **PR9**: a teammate follows `docs/testing.md` from a clean clone and reaches a green host run + a Renode run without external help.

## Acceptance gate for M2+

When all 9 PRs are merged:

- Every standardized `hal_*` function has ≥1 success-path test; every `hal_status_t`-returning function has ≥1 error-path test.
- `cmake -DTEST=ON` configures clean; `cmake -DTEST_HOST=ON` builds and runs natively.
- CI runs host subset + on-target build + Renode on-target run + no-capability build on every PR.
- `docs/testing.md` exists; README CI badge points at the new workflow.
- This unblocks M3 (file moves) and M4 (dispatch rework) — both can land with the suite as a behavior-preservation net.
