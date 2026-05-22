# M5 milestone gate — AVR readiness review

The M5→M6 gate (`docs/execution_plan.md` §8): before starting the AVR /
ATmega328P port, review the frozen v1 API against the AVR-constraints table
(`execution_plan.md` §2, "The AVR lens"). Anything that still fails is fixed
**now, under v1** — fixing it after a second port exists would be a breaking
change.

Reviewed at `HAL_API_VERSION 1` (commit `dea4bd9`).

## Verdict per constraint

| # | AVR-lens constraint | Verdict |
|---|---------------------|---------|
| 1 | `systick` → `hal_timebase` (SysTick is a Cortex name) | **PASS** |
| 2 | `dwt` → `hal_cycle_counter`, gated `NAVHAL_HAS_CYCLE_COUNTER` | **PASS** |
| 3 | `hal_interrupt` exposes no NVIC concepts; priority optional | **RESOLVED (F3)** |
| 4 | DMA gated by `NAVHAL_HAS_DMA`, compiled out (not stubbed) | **PASS** (minor, F4) |
| 5 | `hal_fpu` gated; no public API depends on a hardware FPU | **PASS** |
| 6 | `hal_sdio` gated by `NAVHAL_HAS_SDIO` | **RESOLVED (F1)** |
| 7 | Tick counters are `uint32_t` (no `uint64_t`) | **PASS** |
| 8 | GPIO two-layer: per-MCU core pin enum + per-board aliases | **FINDING — structural (F2)** |
| 9 | String/const APIs allow a later flash-resident `_P` variant | **PASS** |
| 10 | `arch/` owns startup + linker; `CONFIG_TOOLCHAIN_PREFIX` param | **PASS** (minor, F5) |
| 11 | Small config structs; caller-supplied buffers; no static bufs | **PASS** |

7 clean passes. The M1–M2 design work holds up: the `timebase` rename,
`hal_cycle_counter`, `uint32_t` ticks, the capability-macro scheme, and the
two-layer GPIO *concept* are all in place. Four findings follow.

## Findings

### F1 — SDIO is not capability-gated *(RESOLVED)*

> **Resolved.** `hal_sdio.h` and `navhal_port_sdio.h` now gate their full body
> on `_SDIO_ENABLED`, mirroring `hal_dma.h` / `navhal_port_dma.h`. On a target
> without SDIO the headers collapse to nothing and `family/sdio_reg.h` is not
> pulled. Verified: STM32 build unchanged, 142/142 on-target green.


Constraint 6 requires `hal_sdio` to be gated by `NAVHAL_HAS_SDIO`. It is not.

`hal_dma.h` is the correct model: its entire body sits inside
`#ifdef _DMA_ENABLED`, and `navhal_port_dma.h` guards its
`#include "family/dma_reg.h"` the same way — so on a target without DMA the
headers evaporate to nothing.

`hal_sdio.h` has no such guard, and `navhal_port_sdio.h` includes
`family/sdio_reg.h` **unconditionally**. `navhal.h` pulls in `hal_sdio.h`, so
any target whose port does not ship a `family/sdio_reg.h` cannot include the
umbrella header. The AVR port could paper over this with an empty
`navhal_port_sdio.h` stub, but that (a) violates the stated contract and
(b) leaves SDIO asymmetric with DMA.

**Recommendation:** gate `hal_sdio.h` / `navhal_port_sdio.h` exactly like
`hal_dma.h` / `navhal_port_dma.h`. Non-breaking for SDIO-enabled targets;
removes a class of M6 build failure. Small, self-contained change — do it
under v1.

### F2 — Target-shaped enums in shared/common headers *(structural — extended during M6)*

> **Scope correction (M6).** As first written this finding covered only the
> `utils/*_types.h` headers below. Implementing M6 surfaced that the same
> defect applies to **three peripheral *instance* enums baked directly into
> the common headers** — `hal_uart_t` (`common/hal_uart.h`), `hal_i2c_bus_t`
> (`common/hal_i2c.h`), `hal_spi_instance_t` (`common/hal_spi.h`), all
> hardwired to STM32 instances (USART1/2/6, I2C1/2/3, SPI1/2). ATmega328P has
> USART0 / one TWI / one SPI, and an enum cannot be extended port-side. This
> original review under-scoped F2 by missing them; M6 resolves it by
> port-resolving those three enums into `utils/{uart,i2c,spi}_types.h`
> alongside the headers below — same mechanism, no STM32 value change.

The two-layer GPIO model (constraint 8) is correctly *designed* — a per-MCU
core pin enum plus per-board aliases (`board.h`, landed in M3 WI3.4/3.5). But
the core pin enum is not yet *located* for multi-target use:

- `include/utils/gpio_types.h` — `hal_gpio_pin_t` hardwired to STM32 ports
  A–E (`GPIO_PA00`..`GPIO_PE15`). AVR has ports B/C/D.
- `include/utils/timer_types.h` — `hal_timer_t` = `TIM0`..`TIM12`.
- `include/utils/clock_types.h` — `hal_clock_source_t` = `HSI`/`HSE`/`PLL`
  (STM32 clock-tree terms).
- `include/utils/bus_types.h` — `hal_bus_t` = `APB1`/`APB2`/`AHB1`/`AHB2`
  (STM32 bus names; AVR has no APB/AHB).

These sit in a shared `utils/` directory but are target-specific. On AVR they
are simply wrong. This is **not a v1 API break** — pin/timer/bus identifiers
are per-target by design (an AVR app uses `GPIO_PB05`, an STM32 app uses
`GPIO_PA05`) — it is a file-location / include-resolution issue.

**Recommendation:** decide the mechanism now; implement in M6 (WI6.4). Cleanest
option: move these four headers under the port include tree (e.g.
`include/port/cortex-m4/utils/gpio_types.h`) so the existing
`#include "utils/gpio_types.h"` resolves per-port via the `-I` path that M4
WI4.5 already wired. The AVR port then ships its own set. No source edits to
the `hal_*` headers, no API change.

### F3 — `IRQn_Type` naming + register leakage in `hal_interrupt.h` *(RESOLVED)*

> **Resolved.** The IRQ-identifier type is renamed `IRQn_Type` → `hal_irq_t`
> across the interrupt API, drivers and tests; `IRQn_Type` is kept as a
> deprecated alias behind `NAVHAL_DEPRECATED`. An AVR port now declares its own
> `hal_irq_t`. (The `family/interrupt_reg.h` include in the public header is
> left as-is — `hal_irq_t` is genuinely a per-family enum, so a family header
> is its natural home; this is not register leakage in the harmful sense.)


`hal_interrupt`'s functions are clean — no `nvic` in any name, `priority` is an
explicit arg an AVR port can ignore. Two warts:

- The IRQ identifier type is `IRQn_Type` — CMSIS/Cortex jargon. Every MCU has
  IRQ numbers, so the *concept* is portable and an AVR port can define its own
  `IRQn_Type`; but the *name* leaks Cortex convention into the v1 contract.
- `common/hal_interrupt.h` (the public header) does
  `#include "family/interrupt_reg.h"`, pulling register definitions into the
  public surface.

Neither blocks the AVR port. But the readiness gate is the last cheap moment to
rename `IRQn_Type` → `hal_irq_t` (a small, mechanical v1 change) before a
second port makes it a true breaking change. **Recommendation:** decide
explicitly — rename now, or accept `IRQn_Type` as the v1 name and document it.

### F4 — DMA capability gate uses `_DMA_ENABLED`, not `NAVHAL_HAS_DMA` *(minor)*

Already recorded as D4 in `docs/m5_conformance_audit.md`. Internal `#ifdef`s
use the build flag `_DMA_ENABLED`; the app-facing `NAVHAL_HAS_DMA` is correct
and available. Functionally fine on AVR (DMA compiles out either way).
Cosmetic; align when convenient.

### F5 — Root CMake link flags are STM32-shaped *(minor — M6 build config)*

`CMakeLists.txt` hardcodes `-T ${SRC_BOARD}/linker.ld -nostdlib` for the link
step. AVR builds use avr-libc startup and typically no custom linker script.
Not a v1 API issue — M6 (WI6.1/WI6.5) will make the link/startup flags
arch-conditional. Noted so M6 expects it.

## Overall verdict

**v1 is AVR-ready. The two pre-M6 items, F1 and F3, are resolved under v1.**

- **F1 (SDIO gating)** — ✅ resolved. `hal_sdio.h` / `navhal_port_sdio.h` now
  self-gate on `_SDIO_ENABLED` like the DMA headers.
- **F3 (`IRQn_Type`)** — ✅ resolved. Renamed to `hal_irq_t`; `IRQn_Type`
  retained as a deprecated alias.
- **F2 (type-header location)** — open, for M6. Decision recorded: move the
  four target-shaped `utils/*_types.h` (`gpio_types.h`, `timer_types.h`,
  `clock_types.h`, `bus_types.h`) into the port include tree so each port
  ships its own; resolved via the M4 WI4.5 `-I` path. Not a v1 API break —
  implement as part of M6 WI6.4.
- **F4, F5** — minor; address during M6 (DMA gate-macro alignment; arch-
  conditional link/startup flags).

Nothing in the frozen `hal_*` API surface fails the AVR lens in a way that
forces a v1 contract break. The M1–M2 foresight (timebase, cycle counter,
`uint32_t` ticks, capability macros, two-layer GPIO) did its job; the two
gaps it left (F1, F3) are now closed under v1, before the second port locks
the contract.

**Gate verdict: M5 → M6 cleared.** Remaining work (F2/F4/F5) is M6-internal
and does not touch the v1 API.
