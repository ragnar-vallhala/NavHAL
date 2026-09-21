@page roadmap_m9_plan M9 — Execution plan (HAL-wide vtable shift)

# M9 — Execution plan: the complete vendor/arch vtable shift

> Status: **in progress** — GPIO landed as the reference; the remaining
> subsystems + cross-cutting infra are tracked here.
> Parent: @ref roadmap_m9 (the design). Cost mechanics: @ref roadmap_abstraction.
> Scope: roll the driver-vtable abstraction across **every** HAL subsystem
> (vendor- and arch-layer) in one cohesive change, plus the LTO release
> config, the vtable-conformance gate, and a cycle-counted perf regression.

## Context

The current model is **vendor = copy and re-implement the public API**: each
vendor's driver redefines the public `hal_*` functions and re-validates the
same arguments (NULL checks, bounds), so the public contract is enforced N
times with N chances to silently diverge. M9 replaces this with one
architecture-wide pattern — **a shared public layer that validates and
dispatches through a per-backend operations table (vtable)**, with backends
doing register work only.

GPIO already landed as the reference implementation (`src/common/hal_gpio.c`,
`include/internal/hal_gpio_ops.h`, vendor `gpio.c` → static impls +
`_hal_gpio_ops`). This plan is the **complete shift**: M9 is architecture-wide,
not one subsystem, so it covers the whole HAL plus the infrastructure that
makes the abstraction zero-cost and self-enforcing.

**Delivery decisions:**

* **One cohesive change** on branch `feat/m9-driver-vtable` (keeps the GPIO
  commit as step 1), with one self-contained, individually-building commit per
  subsystem for bisectability.
* **All subsystems** — vendor-layer *and* arch-layer (interrupt, timebase,
  fpu, dwt).
* **All three infra pieces** — LTO release config, vtable-completeness
  conformance tests, cycle-counted perf regression.

**Outcome:** adding a new vendor/port = filling in ops tables, never
re-declaring or re-validating the public API; the vtable shape *is* the
conformance contract; and `-O2 -flto` collapses every dispatch back to a
direct call (zero runtime cost).

## The uniform pattern

For each subsystem `<sub>` with a public `hal_<sub>` API:

1. **`include/internal/hal_<sub>_ops.h`** (new, non-public) — declares
   `typedef struct { … } hal_<sub>_ops_t;` (one function pointer per public
   *slow-path* call) and `extern const hal_<sub>_ops_t _hal_<sub>_ops;`. The
   table is **embedded directly** (a `const` object, not pointer-to-table) —
   drops one indirection and devirtualises under `-flto`.
2. **`src/common/hal_<sub>.c`** (new) — the one public implementation. Hoists
   the genuinely duplicated validation (NULL `cfg`/buffer checks) out of the
   vendors, then dispatches via `_hal_<sub>_ops.<fn>(…)`. Include `<stddef.h>`
   for `NULL` (AVR headers don't pull it in transitively).
3. **Backend** defines `const hal_<sub>_ops_t _hal_<sub>_ops = { … }` over
   `static` register-poking impls (validation already done upstream):
   vendor subsystems in `src/vendor/<vendor>/<sub>/<sub>.c`; arch subsystems in
   `src/arch/<isa>/<sub>/<sub>.c`.
4. **Hot-path inlines stay in the port header** and never enter the table —
   GPIO `write`/`read`/`toggle`, interrupt `global_enable`/`global_disable`.
5. **CMake:** one gated line in `src/CMakeLists.txt` —
   `if(CONFIG_DRV_<SUB>) list(APPEND COMMON_SOURCES .../common/hal_<sub>.c)`.
   The `common` OBJECT library already carries the port/family/board include
   dirs (added for GPIO), so each new public layer needs no further CMake work.

Reference to copy: `include/internal/hal_gpio_ops.h`, `src/common/hal_gpio.c`,
`src/vendor/stm32/gpio/gpio.c`.

### Boundary principle: primitives vs 1:1 dispatch

A 1:1 ops table (one entry per public function) only dedups *validation* — a
small win. The larger win comes from **inverting the boundary**: where the
public API is built from portable logic on top of a few hardware operations,
the vtable should expose just those **primitives** and the common layer should
implement the rich API **once**. Apply this where there is genuinely shared
logic to lift; keep the plain 1:1 dispatch where vendors legitimately diverge
(the 1:1 layer still earns the single-validation-point + conformance seam).

| Subsystem | Treatment | Rationale |
|---|---|---|
| uart  | **primitives** (done) | init/enable_irq/write_char/read_char/available; the 6 formatters + read_until live once in the common layer |
| crc   | **primitives**        | software CRC-32/MPEG-2 is identical across AVR + STM32-software → one shared impl; vtable = HW-accel hook |
| spi   | **primitives**        | transmit/receive/transmit_receive collapse to shared loops over one `xfer_byte` |
| i2c   | partial               | write loop lifts; read keeps STM32's N=1/2/>2 framing in the backend |
| gpio, clock, pwm, timer, flash | **1:1 dispatch** | register pokes or divergent strategies (e.g. flash 2-sector compaction vs EEPROM append) — nothing portable to lift |

## Scope

Sixteen ops tables landed, not the ten this section originally listed. The
differences are worth recording, because each was a wrong call rather than a
change of mind.

### Migrated — one table per subsystem

| Subsystem | Backends | Shape |
|---|---|---|
| gpio | stm32, microchip, acme | 1:1 + inline hot path in the vendor |
| clock | stm32 (F4/F7), microchip, pc | `{init, get_sysclk, get_bus_count, get_bus_clock}` |
| crc | stm32, microchip | primitives; one shared software CRC |
| flash | stm32, microchip | 1:1 |
| timer | stm32, microchip | inverted, 19 entries to 16 |
| timebase | armv7e-m, microchip, pc | 1:1; `tick_us != 0` hoisted |
| interrupt | armv7e-m, avr, pc | 1:1; NULL-callback hoisted |
| pwm | stm32, microchip | 1:1 |
| uart | stm32 (F4/F7), microchip, pc | primitives; 6 formatters shared |
| i2c | stm32 (F4/F7), microchip | 1:1 + `deinit` |
| spi | stm32 (F4/F7), microchip | primitives, `{init, xfer_byte}` |
| adc | stm32, microchip | 1:1 |
| reset | stm32, microchip | 1:1 |
| watchdog | stm32, microchip | 1:1 |
| wwdg | stm32 | sibling table, `DRV_WWDG` |
| uart-dma, i2c-dma | stm32 (F4/F7) | sibling tables; binding op |

### Deliberately not migrated

`rtc`, `eth`, `sdio`, `usb_cdc` are STM32-only; `mpu`, `cache`, `dwt`, `fpu`,
`tcm` are ARMv7E-M arch features; `dma` has one backend. A table over a single
implementation is indirection for a choice with no alternatives, so this
section's call to give `dma`, `sdio`, `fpu` and `dwt` "thin tables for contract
uniformity" was not taken. Revisit when a second implementation appears.

### Corrections to this section as first written

* **`interrupt` and `timebase` were listed as arch-layer work and nearly
  skipped.** Both have three or four backends and genuinely earned tables. A
  survey that looked only under `src/vendor/` missed them.
* **The instruction to move the AVR timebase to `src/arch/avr/` was wrong.**
  SysTick is a core peripheral, so the Cortex timebase belongs to arch; the
  ATmega328P timebase is Timer0 with `ISR(TIMER0_COMPA_vect)`, a vendor
  peripheral. It is filed correctly where it is. Moving it would have repeated
  the layering error that put GPIO's hot path, the NVIC and the vector table
  in the wrong tree.
* **`DRV_TIMER` gated two different drivers.** The general-purpose
  `hal_timer_*` and `hal_timebase_*` now have separate symbols, because a port
  can have a timebase without a timer peripheral — x86 does.

## Work breakdown

**Phase A — vendor subsystems.** Apply the pattern to the 10 vendor subsystems,
one at a time, building + running the relevant host/PIL check after each so a
regression is caught at its own commit. Order by ascending surface: crc →
clock → flash → pwm → uart → i2c → spi → dma → sdio/diskio → timer (largest
last).

**Phase B — arch subsystems.** interrupt, then fpu and dwt (thin tables), then
timebase — which also moves the AVR implementation to `src/arch/avr/timebase/`
and repoints both vendor `CMakeLists.txt` from
`${CMAKE_CURRENT_SOURCE_DIR}/timebase` to `${SRC_ARCH}/timebase`. Add
`CONFIG_DRV_TIMEBASE` (selected by `DRV_TIMER` and `DRV_UART`) so the
common-layer gate is clean rather than an awkward `DRV_TIMER OR DRV_UART`.

**Phase C — LTO / release build config.**

* `Kconfig`: add a "Build type" choice — `BUILD_DEBUG` (`-O0`/`-Os` + `-g`,
  default) vs `BUILD_RELEASE` (`-O2 -flto`).
* `cmake/arch/armv7e-m.cmake` / `avr.cmake`: stop hard-coding `-O0`/`-Os`; emit
  base flags and let an `OPT_FLAGS` var (from the Kconfig choice, resolved in
  the root `CMakeLists.txt`) append the optimisation + `-flto`; add `-flto` to
  the link flags on release.
* **LTO safety (top risk):** verify ISRs / the vector table / weak aliases
  survive section GC — they must be `__attribute__((used))` and/or `KEEP()` in
  the linker script. Verify by disassembly + a green release-build PIL run on
  both arches.

**Phase D — conformance + perf harness.**

* **Vtable completeness:** new `tests/portable/conformance/test_vtable.{c,h}`
  including the `internal/hal_<sub>_ops.h` headers and asserting every
  `_hal_<sub>_ops.<fn> != NULL`, each block gated by the subsystem's
  `NAVHAL_HAS_*`. The TEST build already has `include/` on its path and globs
  the vendor `.c` that defines the symbol, so this links. Register the suite in
  `tests/main.c`. This makes "did the port really implement the HAL" a build-
  and link-time gate (design §9.4).
* **Perf regression:** extend `tests/cap/cycle_counter/` (gated
  `NAVHAL_HAS_CYCLE_COUNTER`, Cortex-M4) — use `hal_cycle_counter_*` (DWT) to
  assert (a) the inline hot-path cost is unchanged and (b) under a release/LTO
  build the dispatched call devirtualises to within the "≤ 2 cycles" budget.
  AVR side is best-effort/skipped (simavr exposes no host-readable guest
  cycles).

**Phase E — docs + final verification.** Update @ref roadmap_m9: flip every
subsystem row to done, document the arch-ops variant and the timebase
unification, and mark the cost-of-abstraction section validated. Then run the
full matrix below.

## Critical files

* New per subsystem: `include/internal/hal_<sub>_ops.h`, `src/common/hal_<sub>.c`.
* Rewritten backends: `src/vendor/{stm32,microchip}/<sub>/<sub>.c` and
  `src/arch/{armv7e-m,avr}/<sub>/<sub>.c` → static impls + ops table.
* Moved: `src/vendor/microchip/timebase/timebase.c` → `src/arch/avr/timebase/`.
* Build: `src/CMakeLists.txt` (one gated line per public layer),
  `src/vendor/*/CMakeLists.txt` (timebase repoint), `cmake/arch/*.cmake` + root
  `CMakeLists.txt` + `Kconfig` (LTO / build-type).
* Tests: `tests/portable/conformance/test_vtable.{c,h}`,
  `tests/cap/cycle_counter/*`, `tests/main.c` (suite registration).
* Docs: `docs/roadmap/M9-driver-vtable.md`.

## Risks & mitigations

* **LTO breaks ISRs / vector table / weak symbols (highest risk).** `-flto` +
  section GC can drop interrupt handlers or the vector table. Mitigate with
  `used` / `KEEP`; verify by disassembly and a green release-build PIL run on
  both arches before relying on it.
* **AVR SRAM pressure.** ~14 `const` ops tables land in RAM (~150–250 B on a
  2 KB part). Check `.data` after migration; if tight, move AVR tables to
  PROGMEM (costs an `LPM` per dispatch). Flagged, not done pre-emptively.
* **Interrupt bounds can't fully hoist** (arch-specific IRQ max) — that check
  stays in the backend; only NULL-callback validation moves up.
* **Single large change is hard to review/bisect** (accepted) — mitigated by
  one self-contained, individually-building commit per subsystem.

## Verification (end-to-end)

Run after each subsystem commit, and every job before opening the PR. The
counts below are what the tree produces today, not what it produced when this
plan was written.

* **Host (SIL):** `tools/ntest host` — 24 pure-logic + 65 driver tests.
* **PIL, both boards:** `bash tools/pil/run.sh nucleo_f401re` (201) and
  `bash tools/pil/run.sh nucleo_f767zi` (208).
* **Capability contract:** `tools/ntest cap-contract` — 20 scenarios. This is
  the only job that builds without pinning a vendor, and the only one that
  caught the ACME port becoming the default vendor on Cortex-M4.
* **Samples:** `tools/ntest samples m4` (34), `m7` (25), `avr` (13).
* **x86:** the three `tools/qemu/smoke.sh` assertions.
* **HIL, when a board is attached:** `bash tools/hil/run.sh nucleo_f401re`
  (184 on silicon). Needs `tools/hil/99-navhal-stlink.rules` installed, or a
  board's console stays root-owned and the runner reports it as absent.
* **Disassembly spot-check:** `-Os -flto` leaves zero indirect dispatches in
  `hal_blink` on both arches.
* **Lint:** `tools/lint_commits.sh origin/main..HEAD`.

What each tier can and cannot catch is worth stating, because relying on the
wrong one cost time during this migration:

* PIL does not model real baud or bus timing, so a clock-tree error passes
  there and garbles the console on hardware.
* The sample matrices only check that things build. An AVR image doubling in
  size from `-O0` passed 13/13.
* Neither PIL nor the samples pin nothing, so both missed a Kconfig default
  changing under them. Only cap-contract did.
* A stale build directory reports the previous run's result. Reconfigure from
  scratch when a build-system change is in play.
