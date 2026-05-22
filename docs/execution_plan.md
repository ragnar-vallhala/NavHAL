# NavHAL Standardization — Execution Plan

**Status:** Draft v1
**Date:** 2026-05-18
**Companion to:** `docs/api_standardization.md`
**Goal:** Standardize the Cortex-M4 / STM32F401RE port against the API spec,
then add an AVR / ATmega328p port as the validation of the multi-ISA design.

---

## 0. How to read this plan

Work is grouped into **milestones M0–M6**. Each milestone is independently
shippable and lists: *objective*, *work items (WI)*, *acceptance criteria*, and
*AVR-lens notes* — design choices made now specifically so the ATmega328p port
in M6 does not force rework.

The single most important strategy: **the AVR port is not a separate project
bolted on later — it is the acceptance test for the abstraction.** Every design
decision in M0–M5 is reviewed against "does this still hold for an 8-bit MCU
with no DMA, no NVIC, no FPU, 2 KB RAM, and a Harvard memory model?"

---

## 1. Sequencing overview

```
M0   Baseline & bug fixes          ── safe, no API change
M1   Foundations (status, types)   ── new primitives, old code untouched
M2   Naming & signature unify      ── per-driver, deprecation shims keep samples green
M2+  Comprehensive test suite      ── full-API regression net before the refactors
M3   Directory restructure         ── arch / vendor / board split (file moves)
M4   Build-time dispatch           ── kill #ifdef ARCH, Kconfig-generated target header
M5   Conformance + samples + cleanup ── remove shims, HAL_API_VERSION = 1
M6   AVR / ATmega328p port          ── second ISA proves the design
```

M0→M5 are the **Cortex-M4 standardization**. M6 is the **AVR expansion**.
M1–M2 may proceed driver-by-driver in parallel once the primitives land.
M2+ slots between M2 and M3: M2 froze the API surface, so building the
test suite *before* the M3/M4 refactors turns those large mechanical
changes into verifiable, behavior-preserving ones.

---

## 2. The AVR lens — constraints surfaced now

ATmega328p differs from Cortex-M4 in ways that *must* shape the standard before
M5, or the M6 port will require breaking changes:

| Cortex-M4 assumption | ATmega328p reality | Required design decision (in M1–M2) |
|----------------------|--------------------|--------------------------------------|
| `SysTick` core timer | No SysTick; a general timer drives the tick | **Rename `systick` → `hal_timebase`.** SysTick is a Cortex peripheral name, not a HAL concept. |
| `DWT` cycle counter | No equivalent | **Reframe `hal_dwt` as `hal_cycle_counter`, gated by `NAVHAL_HAS_CYCLE_COUNTER`.** Cortex-only is fine; the *name* must not leak. |
| NVIC interrupt controller | Flat AVR vector table, no priorities | `hal_interrupt` API must not expose NVIC concepts. Priority is an optional arg ignored where unsupported. |
| DMA available | No DMA | DMA functions gated by `NAVHAL_HAS_DMA`; **compiled out** on AVR, not stubbed. App guards with `#if NAVHAL_HAS_DMA`. |
| Hardware FPU | None (software float) | `hal_fpu` gated by `NAVHAL_HAS_FPU`; no public API depends on it. |
| SDIO peripheral | None | `hal_sdio` gated by `NAVHAL_HAS_SDIO`. |
| `uint64_t` tick is cheap | 64-bit math is very expensive on 8-bit | **Tick counters are `uint32_t`.** Revisit `hal_get_tick` returning `uint64_t`. |
| Ports A–E, 16 pins each | Ports B, C, D only | GPIO uses a **two-layer** model: a per-MCU core `GPIO_Pxnn` enum (target-defined constant set) + per-board macro aliases (`D5`, `LED_BUILTIN`). No port-count assumptions in portable code. |
| Flat unified memory | Harvard: code in flash, data in RAM | String/const APIs must allow a flash-resident variant later (`_P` / `PROGMEM`). Reserve naming; do not block on it. |
| `arm-none-eabi-` toolchain, `.ld` linker script | `avr-gcc`, avr-libc startup | `arch/` layer owns startup + linker; `CONFIG_TOOLCHAIN_PREFIX` already parameterized. |
| 96 KB RAM | 2 KB RAM | Config structs stay small; no large static buffers in the HAL; favor caller-supplied buffers. |

**Net effect on M1–M2:** rename `systick`→`timebase`, generalize `dwt`, make tick
types `uint32_t`, lock the GPIO pin encoder, and confirm every optional
peripheral has a `NAVHAL_HAS_*` gate. Doing this now costs little; doing it in M6
is a breaking change.

---

## 3. M0 — Baseline & bug fixes

**Objective:** Remove outright defects; land both design docs. No API change.

Work items:
- WI0.1 — Rename `include/common/hal_12c.h` → `hal_i2c.h`; update the include in
  `navhal.h` and any references.
- WI0.2 — Fix Doxygen `@file` tags that name the wrong file (`hal_status.h` says
  `@file status.h`, etc.).
- WI0.3 — Fix `hal_status.h`: `FAILURE` macro is `1` but documented `-1`.
  (Will be replaced entirely in M1 — just make it self-consistent now.)
- WI0.4 — Add `.gitignore` entries for `.cache/`, `build/`, `.aider.*`,
  `.config.old`; remove the empty `.gitmodules` or populate it.
- WI0.5 — Commit `docs/api_standardization.md` and `docs/execution_plan.md`.

**Acceptance:** Project builds the existing `hal_blink` sample unchanged; no
`hal_12c.h` references remain; docs are in-tree.

---

## 4. M1 — Foundations

**Objective:** Introduce the standard primitives. Existing drivers untouched.

Work items:
- WI1.1 — New `hal_status.h`: the `hal_status_t` enum (Section 6 of the spec) +
  `HAL_OK_OR_RETURN`. Keep old `SUCCESS`/`FAILURE` as deprecated aliases.
- WI1.2 — Clean `hal_types.h`: keep `__I/__O/__IO`, add `NAVHAL_INLINE`,
  `NAVHAL_UNUSED`, `NAVHAL_DEPRECATED`; drop the `byte` alias.
- WI1.3 — `include/navhal/internal/navhal_compiler.h` — compiler/attribute shims
  that work on both `arm-none-eabi-gcc` and `avr-gcc`.
- WI1.4 — Lock the two-layer GPIO pin model: keep the per-MCU core
  `hal_gpio_pin_t` enum (`GPIO_Pxnn`, as today), and define the board-layer
  macro-alias contract (`D5`, `LED_BUILTIN`, …) supplied by each `board.h`.
- WI1.5 — Capability-macro contract: document every `NAVHAL_HAS_*` flag and its
  owning `CONFIG_DRV_*`.
- WI1.6 — Naming-convention header / lint note for contributors.

**Acceptance:** New headers compile under both toolchains (compile-only check);
`hal_blink` still builds via the deprecated aliases.

**AVR lens:** WI1.3 and WI1.4 are the load-bearing ones — verify the compiler
shims and the pin encoder against `avr-gcc` now, before any driver depends on
them.

---

## 5. M2 — Naming & signature unification

**Objective:** Migrate each driver to the standard API shape (Section 8 of the
spec) with deprecation shims so the 27 samples keep building.

Per-driver loop (repeat for each driver):
1. Add standardized prototypes to `include/navhal/hal_<p>.h`.
2. Implement them in the existing core source (still under `core/cortex-m4/` —
   restructure is M3).
3. Add `compat/<p>_compat.h` mapping old names → new (`__attribute__((deprecated))`).
4. Update the driver's unit test in `tests/` to the new API.

**Driver order** (low-risk / high-leverage first):

| Wave | Drivers | Notes |
|------|---------|-------|
| A | `gpio`, `clock`, `interrupt` | Foundation; everything depends on them |
| B | `timebase` (was `systick`), `timer` | Rename `systick`→`timebase`; tick types → `uint32_t` |
| C | `uart`, `i2c`, `spi` | Collapse `uartN_*` macro families into one `_Generic` helper; type the I2C `bus` param |
| D | `pwm`, `crc`, `flash` | |
| E | `dma`, `dwt`→`cycle_counter`, `fpu`, `sdio` | All capability-gated; AVR will disable these |

**Acceptance per driver:** conforms to the Section 14 checklist of the spec;
old + new APIs both build; test passes.

**AVR lens:** Wave E drivers must end up fully behind `NAVHAL_HAS_*` so an AVR
build simply omits them. Wave B locks the `uint32_t` tick decision.

---

## 5+. M2+ — Comprehensive test suite

**Objective:** Grow the unit-test suite from its current partial state
(~3-6 tests per driver) into comprehensive coverage of the **whole
standardized `hal_*` API** — every function, success *and* error paths — so
the M3/M4 refactors are provably behavior-preserving and M6 (AVR) has a real
conformance bar.

**Why here:** M2 froze the API surface. M3 (file moves) and M4 (dispatch
rework) are large, mechanical changes that can silently regress behavior. A
full-surface suite turns them into verifiable refactors. The suite is also the
conformance harness the AVR port is validated against — so it must exist
before M6, and is most cheaply built now while the Cortex-M4 behavior is the
known-good reference.

Work items:
- WI2+.1 — **Make the test build self-contained.** `cmake -DTEST=ON` currently
  collides with a sample's `flash` target and does not cleanly separate a test
  build from a sample build (the duplicate-target error seen during M2). Make
  `TEST` exclude the sample targets so `tests` builds from one clean configure.
- WI2+.2 — **Per-driver API coverage.** For each of the 14 drivers, extend
  `tests/test_<driver>.c` to exercise every standardized function — lifecycle
  (`init`/`deinit`), every operation, every getter — not just a sampled subset.
- WI2+.3 — **Error-path tests.** Assert the `hal_status_t` contract: NULL
  config → `HAL_ERR_INVALID_ARG`, out-of-range instance id, etc. The
  standardized error model only has value if it is tested.
- WI2+.4 — **Capability-gated drivers.** Exercise `dma`, `sdio`, `fpu`,
  `cycle_counter` under their enabling `CONFIG_*`; confirm they are cleanly
  absent (not stubbed) when disabled.
- WI2+.5 — **Host-runnable subset.** Split out the pure-logic tests (conversion
  utils, `hal_status` helpers / `HAL_OK_OR_RETURN`, the software CRC path, GPIO
  pin encoding) so they compile and run on the build host with no hardware —
  fast CI feedback. Register-touching tests stay on-target.
- WI2+.6 — **Harness improvements.** A central per-driver test registry with a
  pass/fail summary; reduce the manual `RUN_TEST` boilerplate in `tests/main.c`.
- WI2+.7 — **CI.** A GitHub Actions workflow that compile-builds the on-target
  `tests` binary and runs the host subset. (The `ci.yml` the README badge
  points at does not yet exist — create it.)
- WI2+.8 — **`docs/testing.md`** — how to build/run the suite (on-target vs
  host) and how to add a test for a new driver.

**Acceptance:** every standardized `hal_*` function has at least one
success-path test, and an error-path test where it returns `hal_status_t`;
the `tests` binary builds clean from a single `cmake -DTEST=ON`; the host
subset runs in CI; `docs/testing.md` exists.

**AVR lens:** keep the per-driver test structure and the host-runnable subset
target-agnostic. In M6 the portable subset becomes the ATmega328p conformance
harness — the same tests proving the second ISA behaves to the contract.

---

## 6. M3 — Directory restructure

**Objective:** Split `core/cortex-m4/` into the layered tree (Section 4 of the
spec). Pure file moves — no API change.

Work items:
- WI3.1 — Create `src/arch/armv7e-m/` — move NVIC, SysTick HW, SCB, DWT HW, FPU
  HW, `startup.s`.
- WI3.2 — Create `src/vendor/stm32/<ip>/` — move GPIO, USART, RCC, I2C, SPI, TIM,
  DMA, FLASH, CRC, SDIO drivers.
- WI3.3 — Create `src/vendor/stm32/family/stm32f4/include/family/` — move all
  `*_reg.h` here; mark family-private.
- WI3.4 — Create `src/board/nucleo_f401re/` — `board.h` (pin aliases, LED/button,
  osc values), `board.c`, linker script.
- WI3.5 — Update CMake to glob the new directories; verify `hal_blink` builds.

**Acceptance:** Build output byte-identical (or functionally identical) to
pre-restructure; no `*_reg.h` reachable from `include/navhal/`.

**AVR lens:** This is where the tree shape is proven. After M3, adding AVR in M6
should mean adding `src/arch/avr8/`, `src/vendor/microchip/`,
`src/board/atmega328p/` — three siblings, zero edits to existing drivers.

---

## 7. M4 — Build-time dispatch

**Objective:** Eliminate `#ifdef CORTEX_M4` header dispatch.

Work items:
- WI4.1 — Convert every `common/hal_*.h` (now `navhal/hal_*.h`) to a pure
  prototype header with no target conditionals.
- WI4.2 — Introduce include-path-resolved `navhal_port_<p>.h` for inline /
  register-backed definitions and per-target enum IDs.
- WI4.3 — Kconfig → `navhal_target.h` generator (emits `NAVHAL_CONFIG_*`,
  `NAVHAL_HAS_*`, target identity). Extend `tools/kconfig.py`.
- WI4.4 — Add the `FAMILY` axis and per-driver capability sub-options to Kconfig.
- WI4.5 — CMake selects sources + include paths from the resolved config.

**Acceptance:** No `#ifdef CORTEX_M4` remains in any header; `hal_blink` builds
from a clean Kconfig run.

**AVR lens:** WI4.3/WI4.4 must already model `ARCH_AVR8` / `VENDOR_MICROCHIP` /
`BOARD_ATMEGA328P` as *selectable but not-yet-implemented* options, so M6 only
fills in directories — it does not touch the build system's structure.

---

## 8. M5 — Conformance, samples, cleanup

**Objective:** Finish the Cortex-M4 standardization.

Work items:
- WI5.1 — Per-driver conformance audit against the Section 14 checklist.
- WI5.2 — Migrate all 28 samples to the new API; rename for clarity if needed.
- WI5.3 — **Keep the `compat/` shims and pre-standardization aliases as a
  permanent backward-compatibility surface.** The M2 deprecations
  (`SUCCESS`/`FAILURE`, the `dma_*` / `hal_get_millis` / `UART1` /
  `FlashRecord_t` / `HAL_I2C_OK` / etc. names) stay in tree behind
  `NAVHAL_DEPRECATED` so existing application code continues to compile
  with a warning, not an error. Update the per-symbol doc strings that
  currently say "removed in M5" to reflect the new policy.
- WI5.4 — Set `HAL_API_VERSION = 1`; update `README.md` (correct the
  multi-arch claims to reflect actual support) and Doxygen `mainpage.md`.
- WI5.5 — Regenerate Doxygen + the LaTeX design doc.
- WI5.6 — **C++ compatibility.** `extern "C"` guards on every public header
  so the C-compiled HAL archive links clean from a C++ TU; `CMAKE_CXX_COMPILER`
  wired at the toolchain block so `arm-none-eabi-g++` is picked up project-wide;
  a C++ sample (`27_hal_blink_cpp`) that drives the standardized `hal_*`
  surface through `navhal.h` under bare-metal flags (`-fno-exceptions
  -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics`) — proves the guards
  hold end-to-end on hardware. (Landed early, before the rest of M5, via
  commits `885889a` and `89893de`.)

**Acceptance:** all samples + tests build and pass on the F401RE using
v1-only API; the deprecated aliases still compile (with deprecation warnings,
not errors); `HAL_API_VERSION` frozen at 1; at least one C++ sample builds
and runs on-target. Removing the shim layer is explicitly deferred — it would
be a v2 API break, not part of v1's stabilisation.

**Milestone gate — "AVR readiness review":** before starting M6, review the
frozen API against the Section 2 table. Any item that still fails is fixed here,
under v1, not after the AVR port exists.

---

## 9. M6 — AVR / ATmega328p port

**Objective:** Implement a second ISA against the *unchanged* v1 API. Success =
the same application source (`hal_blink`, `hal_uart_tx`) compiles and runs on
both F401RE and ATmega328p with only a Kconfig change.

Work items:
- WI6.1 — `src/arch/avr8/` — startup, vector table, `hal_interrupt` backend
  (global enable/disable, no priorities), avr-libc integration.
- WI6.2 — `src/vendor/microchip/` — ATmega328p IP drivers:
  - `gpio` — core `GPIO_Pxnn` enum for ports B/C/D.
  - `timebase` — Timer0 (or Timer2) drives the millisecond tick.
  - `timer` — 8-bit Timer0/2 + 16-bit Timer1.
  - `uart` — USART0 only.
  - `i2c` — TWI peripheral.
  - `spi` — SPI peripheral.
  - `pwm` — timer-compare output.
  - `clock` — fuse/prescaler-based; mostly static.
  - `crc` — software (no hardware CRC); or `HAL_ERR_NOT_SUPPORTED`.
- WI6.3 — `src/board/atmega328p/` (or `arduino_uno`) — pin aliases, 16 MHz
  oscillator, board init.
- WI6.4 — `navhal_port_*.h` for AVR — core `GPIO_Pxnn` enum (B/C/D),
  `hal_uart_id` = `{HAL_UART_0}`; `board/atmega328p/board.h` supplies the
  board-layer aliases (`D5`, `LED_BUILTIN`, …).
- WI6.5 — Kconfig: `ARCH_AVR8`, `VENDOR_MICROCHIP`, `BOARD_ATMEGA328P`, toolchain
  `avr-`; disable `DRV_DMA`/`DRV_FPU`/`DRV_DWT`/`DRV_SDIO` for this target.
- WI6.6 — `tools/flash.sh` — add `avrdude` flashing path.
- WI6.7 — Port the unit-test harness; run the portable subset on AVR.
- WI6.8 — Build a curated set of samples for AVR (blink, uart_tx, i2c, pwm,
  timer); samples needing DMA/SDIO/FPU are excluded by Kconfig.
- WI6.9 — **Renode AVR simulation.** Stand up a Renode machine for the
  ATmega328p, mirroring `tools/renode/navhal_f401re.resc` for the
  Cortex-M4 port. There is no physical ATmega328p on the dev/CI rig, so a
  simulator is the *only* PIL path for the AVR port — it gives the AVR
  test build (WI6.7) the same emulated on-target run the STM32 port gets,
  and unblocks the CI matrix in Section 10. Deliverables: a
  `tools/renode/navhal_atmega328p.resc` script (load the ELF, wire
  USART0 to a log file, RunFor a bounded window, exit on the navtest
  summary line) and a thin `run_tests.sh` wrapper. Note: Renode's AVR
  support is less mature than its Cortex-M support — if the ATmega328p
  model proves inadequate, `simavr` is the fallback simulator; the
  `.resc`-equivalent and the harness contract stay the same.
- WI6.10 — **Samples reorganised by capability.** A flat `samples/` of
  ~28 STM32-first samples gave the build no way to know which sample can
  run where. Split into three tiers — `samples/portable/` (GPIO/UART/
  timer/clock only; same source on every port), `samples/cortex-m/`
  (needs DMA/FPU/DWT/SDIO), `samples/no_hal/` (register-level, per-MCU).
  Each Cortex-M and No-HAL `SAMPLE_*` Kconfig entry gains
  `depends on ARCH_CORTEX_M4`, so an AVR configure does not offer a
  sample that cannot run on it. `samples/README.md` is the registry.
  Portable samples migrate to board aliases incrementally (`hal_blink` /
  `hal_uart_tx` done; the rest follow the WI6.2 drivers).

**Acceptance:**
- `hal_blink` and `hal_uart_tx` build for ATmega328p from a Kconfig switch, with
  **no edits to `include/navhal/` and no edits to any STM32 driver**.
- DMA/FPU/DWT/SDIO drivers are cleanly absent (compiled out), not stubbed.
- If any contract change is needed → it is a **finding against the v1 API**, and
  the spec + Cortex-M4 port are updated together (this should be rare; if it is
  not rare, M5's readiness review failed).

---

## 10. Branching, PR & CI strategy

- One feature branch per milestone (`std/m1-foundations`, …); small PRs within.
- M2 driver waves are independent PRs — reviewable one driver at a time.
- CI must build `hal_blink` on every PR; from M4 onward, build a **matrix**:
  F401RE today, and from M6, F401RE + ATmega328p. The ATmega328p row has no
  physical board on the rig — its on-target run is the Renode AVR machine
  from WI6.9 (the F401RE row already runs under Renode).
- Add a CI compile-check that no header under `include/navhal/` contains
  `#ifdef CORTEX_M4` / `#ifdef ARCH_*` (enforces M4).
- Keep `main` releasable: each milestone merges only when its acceptance
  criteria pass.

## 11. Risk register

| Risk | Impact | Mitigation |
|------|--------|------------|
| API changes discovered late during M6 | Breaks v1 contract | The Section 2 AVR-lens table + the M5 readiness gate front-load the discovery |
| Sample/test churn from renames | Build breakage | M2 deprecation shims keep everything green until M5 |
| Restructure (M3) hides regressions | Silent behavior change | M3 is pure moves; diff must show no logic changes; binary/behavior compare |
| `_Generic` UART macros not portable | AVR/older compiler issues | Collapse to one optional helper in M2; core API is plain functions |
| Kconfig generator complexity (M4) | Build fragility | Land generator behind a flag; keep manual path until parity proven |
| AVR RAM pressure (2 KB) | HAL too heavy | No static HAL buffers; caller-supplied buffers; small config structs (set in M1) |

## 12. Definition of done (per milestone)

- M0: no known defects; docs in-tree; `hal_blink` builds.
- M1: standard primitives compile under both toolchains.
- M2: every driver exposes the standard API; old API deprecated; tests pass.
- M2+: every standardized `hal_*` function has a success-path test (and an
  error-path test where it returns `hal_status_t`); `tests` builds from one
  clean `cmake -DTEST=ON`; host subset runs in CI.
- M3: layered tree in place; no logic change; build verified.
- M4: zero `#ifdef ARCH` in headers; Kconfig-driven build.
- M5: `HAL_API_VERSION = 1`; samples + tests pass against the v1 API (incl.
  one C++ sample on-target); deprecated aliases retained behind
  `NAVHAL_DEPRECATED` for backward compatibility; AVR readiness review signed
  off.
- M6: ATmega328p runs `hal_blink` + `hal_uart_tx` from a Kconfig switch with no
  changes to the public API or the STM32 port.

---

## 13. Immediate next actions

1. Approve / amend the milestone sequencing and the M2 driver-wave order.
2. Decide the open questions in `api_standardization.md` §15 (GPIO encoder,
   FatFs placement, C++ wrapper, RTOS hooks) — at least the GPIO encoder, since
   M1 depends on it.
3. Start **M0** — it is safe, isolated, and unblocks everything.
