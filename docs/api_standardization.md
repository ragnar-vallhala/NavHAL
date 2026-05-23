# NavHAL API Standardization & Long-Term Structure

**Status:** Draft v1
**Date:** 2026-05-18
**Scope:** Defines the standardized public API, naming rules, error model, type
system, directory layout, and target-dispatch mechanism for NavHAL — designed so
the project can scale from a single board (Nucleo-F401RE) to multiple ISAs,
silicon vendors, and boards without breaking application code.

---

## 1. Goals

1. **One stable contract.** Application code includes `navhal.h` and is written
   against a single, architecture-independent API. Porting to a new MCU must not
   require changing application source.
2. **Clean layer separation.** ISA / CPU-core peripherals, vendor IP-block
   drivers, and board pin maps must live in distinct layers with no leakage.
3. **Predictable surface.** Every peripheral exposes the same lifecycle, naming,
   error, and instance conventions — learn one driver, know them all.
4. **Build-time dispatch.** Target selection happens in the build system
   (CMake + Kconfig), not via `#ifdef ARCH` chains scattered through headers.
5. **Incremental migration.** The existing F401RE port keeps working through the
   transition via deprecation shims.

---

## 2. Current-state assessment

Problems the standardization must fix (observed in the current tree):

| Area | Current state | Problem |
|------|---------------|---------|
| Naming | `hal_gpio_setmode`, `hal_clock_init`, `hal_i2c_init` *vs* `uart_init`, `timer_init`, `delay_ms`, `uart_write_char` | No consistent prefix; verbs unspaced (`setmode`) vs spaced (`set_mode`) |
| Error model | GPIO/UART/timer return `void`; I2C returns `hal_i2c_status_t`; `hal_status.h` has `SUCCESS`/`FAILURE` macros, barely used | No unified status type; `FAILURE` defined `1` but documented `-1` |
| Instances | UART: `hal_uart_t` enum **and** `uart1_*`/`uart2_*` macro families; I2C: `hal_i2c_bus_t` in `init`, bare `uint8_t bus` in `read`/`write` | Two parallel APIs; type-unsafe parameters |
| Dispatch | Every `common/hal_*.h` does `#ifdef CORTEX_M4 → core/cortex-m4/*.h` | Single ISA hard-wired; does not scale to N targets |
| Layering | `core/cortex-m4/` mixes ISA peripherals (SysTick, NVIC, DWT, FPU), vendor IP (USART, GPIO, RCC, I2C), and board values | Cannot reuse STM32 drivers on a different Cortex-M, or Cortex-M4 core code under a different vendor |
| Leakage | `TIM2_IRQHandler`, `SysTick_Handler`, raw register macros exposed in headers | Vendor/silicon internals are part of the apparent public surface |
| Convenience | 4× duplicated `_Generic` write macros in `uart.h` | Maintenance burden, bloats the contract |
| Feature flags | `_DMA_ENABLED`, `_UART_BACKEND_DMA` | Ad-hoc; disconnected from Kconfig `CONFIG_*` |
| Hygiene | `hal_12c.h` (should be `hal_i2c.h`); `@file status.h` in `hal_status.h` | Typos propagated into includes and Doxygen |

---

## 3. Design principles

- **The public header is the contract.** `include/navhal/hal_<p>.h` contains
  *declarations only* — prototypes, standardized enums/structs, doc comments. It
  has **zero** `#ifdef ARCH`/`#ifdef VENDOR`.
- **Ports implement the contract.** Each target provides a `.c` implementing the
  prototypes, plus an optional `*_port.h` for inline/register-backed definitions.
- **Dispatch is a build-system job.** Kconfig chooses ISA + vendor + board; CMake
  compiles the matching port `.c` files and injects the matching include path.
  Headers never select a target.
- **Stable IDs, no dynamic allocation.** Peripheral instances are referenced by
  enum IDs; no `malloc`, no opaque handle pools. Suits bare-metal constraints.
- **Capabilities are explicit.** Optional features (DMA, FPU, IT mode) are gated
  by generated `NAVHAL_HAS_*` macros derived from Kconfig.

---

## 4. Target model & layered architecture

A target is decomposed into four orthogonal axes:

```
ISA / CPU core   →  armv7e-m (Cortex-M4)      core peripherals: NVIC, SysTick, SCB, DWT, FPU, MPU
Vendor           →  stm32                     silicon-vendor IP blocks: GPIO, USART, RCC, I2C, SPI
Family           →  stm32f4                   register base addresses, IP revisions, quirks
Board            →  nucleo_f401re             pin map, oscillator values, default clock tree
```

Application ── `navhal.h` ──▶ **Common API** (contract)
                                   │  resolved at build time
                                   ▼
              **HAL glue** ─▶ **Vendor driver** ─▶ **Family reg map** ─▶ uses **ISA/core**
                                                                            **Board** supplies constants

### Proposed directory layout

```
include/
  navhal/                       PUBLIC API — the only thing apps include
    navhal.h                    umbrella header
    hal_status.h                hal_status_t
    hal_types.h                 fixed-width + qualifier macros
    hal_gpio.h  hal_uart.h  hal_i2c.h  hal_spi.h  hal_timer.h
    hal_clock.h hal_pwm.h   hal_dma.h  hal_crc.h  hal_flash.h
    hal_interrupt.h hal_dwt.h hal_fpu.h hal_sdio.h ...
  navhal/internal/              shared internal contracts (not for apps)
    navhal_assert.h  navhal_compiler.h

src/
  arch/                         ISA / CPU-core layer
    armv7e-m/                   nvic.c systick.c scb.c dwt.c fpu.c startup.s
      include/arch/             nvic.h ...
    armv6-m/   riscv32/   ...    (future)
  vendor/                       silicon-vendor IP-block drivers
    stm32/
      gpio/  usart/  i2c/  spi/  rcc/  tim/  dma/  flash/  crc/  sdio/
      family/
        stm32f4/                reg maps + base addresses + quirks
          include/family/       *_reg.h
        stm32f7/  ...            (future)
  board/                        board-support packages
    nucleo_f401re/
      board.h                   pin aliases, LED/button names, osc values
      board.c                   default clock-tree init
    <other boards>/
  hal/                          glue: binds Common API → selected vendor driver
    hal_gpio.c  hal_uart.c  ...  (thin; often just re-exports vendor symbols)
  common/                       portable C — no hardware
    conversion.c  util.c  v_fs.c  fatfs/

  navhal_target.h               GENERATED by Kconfig — target constants + NAVHAL_HAS_*
```

**Rule of dependency direction:** `board` → `vendor` → `family` → `arch`.
Lower layers never include higher ones. `hal/` and the public headers depend on
none of them at compile time of the *contract* (only the chosen port `.c` does).

---

## 5. Naming conventions

| Symbol kind | Rule | Example |
|-------------|------|---------|
| Public function | `hal_<peripheral>_<verb>[_<qualifier>]`, lower snake_case | `hal_uart_write`, `hal_gpio_set_mode` |
| Async qualifier | `_it` (interrupt-driven), `_dma` (DMA-backed) suffix | `hal_uart_write_dma`, `hal_i2c_read_it` |
| Public type | `hal_<peripheral>_<noun>_t` | `hal_uart_config_t`, `hal_gpio_pin_t` |
| Enum constant | `HAL_<PERIPH>_<NAME>` (UPPER) | `HAL_UART_1`, `HAL_GPIO_MODE_OUTPUT` |
| Public macro / constant | `HAL_<PERIPH>_<NAME>` | `HAL_UART_MAX_BAUD` |
| Generated config macro | `NAVHAL_CONFIG_*`, `NAVHAL_HAS_*` | `NAVHAL_HAS_DMA` |
| Internal symbol | `navhal_` prefix, or `static`, in `src/` only | `navhal_uart_isr_dispatch` |
| Vendor/family/arch symbol | `<layer>_` prefix, never `hal_` | `stm32_usart_*`, `armv7em_nvic_*` |

**Banned from the public API:** bare `uart_*`, `timer_*`, `delay_ms`,
per-instance macro families (`uart1_write_*`), and any `IRQHandler`/`*_Handler`
ISR names. Verbs are always `snake_case` (`set_mode`, not `setmode`).

---

## 6. Error & return-value model

Replace the `SUCCESS`/`FAILURE` macros with a single enum in `hal_status.h`:

```c
typedef enum {
    HAL_OK = 0,                 /* success */
    HAL_ERR = 1,                /* unspecified failure */
    HAL_ERR_INVALID_ARG,        /* bad parameter / out-of-range id */
    HAL_ERR_TIMEOUT,            /* operation timed out */
    HAL_ERR_BUSY,               /* peripheral busy / arbitration lost */
    HAL_ERR_NOT_INITIALIZED,    /* used before hal_<p>_init */
    HAL_ERR_NOT_SUPPORTED,      /* not implemented on this target */
    HAL_ERR_IO,                 /* bus / framing / NAK error */
    HAL_ERR_NO_MEM,             /* buffer too small / no resource */
} hal_status_t;

#define HAL_OK_OR_RETURN(expr)  do { hal_status_t _s=(expr); \
                                     if (_s != HAL_OK) return _s; } while (0)
```

Rules:

- **Every fallible function returns `hal_status_t`.** Output data is returned via
  a caller-supplied pointer parameter (last parameter).
- **Hot-path trivial getters may return the value directly** when failure is not
  meaningful — e.g. `hal_gpio_read()` returns `hal_gpio_state_t`,
  `hal_get_millis()` returns `uint32_t`. These are the documented exceptions.
- `HAL_ERR_NOT_SUPPORTED` is the standard return when a port lacks a capability,
  so application code can degrade gracefully instead of failing to link.
- No `errno`-style globals; status is always the return value.

---

## 7. Instance / handle model

Fixed on-chip peripherals are addressed by **enum IDs**, never opaque handles or
pointers — no dynamic allocation.

```c
/* Declared in the public header; values supplied by the target port header. */
typedef enum hal_uart_id   hal_uart_id_t;   /* HAL_UART_1, HAL_UART_2, ... */
```

The target port header (`navhal_port_uart.h`, resolved by include path) defines
exactly which IDs exist for that MCU:

```c
/* src/vendor/stm32/family/stm32f4/include/family/navhal_port_uart.h */
enum hal_uart_id { HAL_UART_1 = 1, HAL_UART_2 = 2, HAL_UART_6 = 6 };
```

Passing an ID the target does not implement → `HAL_ERR_INVALID_ARG` at runtime.

### GPIO pins — two-layer model

GPIO pins use a **two-layer** identification model so that drivers stay generic
while application code keeps board-friendly names.

**Core layer (per-MCU).** Each MCU port supplies a `hal_gpio_pin_t` enum whose
constants name the physical pin — `GPIO_PA05`, `GPIO_PB04`, … — exactly as in
the current implementation (`utils/gpio_types.h`). The constant set is
target-defined: STM32F401RE enumerates ports A–E; ATmega328p enumerates only
ports B/C/D (`GPIO_PB05`, …). Every driver and the HAL contract operate on this
layer; no port-count assumptions are baked into portable code.

**Board layer (per-board).** Each board's `board.h` adds macro aliases that map
human-friendly silkscreen / connector names onto core pins:

```c
/* board/nucleo_f401re/board.h */
#define LED_BUILTIN   GPIO_PA05      /* LD2 user LED */
#define D5            GPIO_PB04      /* Arduino-style header pin */

/* board/atmega328p/board.h */
#define LED_BUILTIN   GPIO_PB05
#define D5            GPIO_PD05
```

Because the board names are plain macros over core enum constants, application
code may use either freely — `hal_gpio_write(D5, HAL_GPIO_HIGH)` and
`hal_gpio_write(GPIO_PB04, HAL_GPIO_HIGH)` are identical. Portable application
code uses the board layer; driver and HAL code uses the core layer. The same
two-layer split applies to other board aliases (`A0…`, button names, etc.).

---

## 8. Standard peripheral API shape

Every driver follows the same lifecycle and shape. Init takes a `const` config
struct; config structs are zero-initializable to sane defaults.

```c
/* ---- lifecycle ---- */
hal_status_t hal_<p>_init   (hal_<p>_id_t id, const hal_<p>_config_t *cfg);
hal_status_t hal_<p>_deinit (hal_<p>_id_t id);

/* ---- blocking I/O ---- */
hal_status_t hal_<p>_write  (hal_<p>_id_t id, const uint8_t *buf, size_t len,
                             uint32_t timeout_ms);
hal_status_t hal_<p>_read   (hal_<p>_id_t id, uint8_t *buf, size_t len,
                             uint32_t timeout_ms);

/* ---- interrupt mode (gated by NAVHAL_HAS_IT) ---- */
hal_status_t hal_<p>_write_it (hal_<p>_id_t id, const uint8_t *buf, size_t len);
hal_status_t hal_<p>_read_it  (hal_<p>_id_t id, uint8_t *buf, size_t len);

/* ---- DMA mode (gated by NAVHAL_HAS_DMA) ---- */
hal_status_t hal_<p>_write_dma (hal_<p>_id_t id, const uint8_t *buf, size_t len);
hal_status_t hal_<p>_read_dma  (hal_<p>_id_t id, uint8_t *buf, size_t len);

/* ---- event callbacks (replaces exposed ISR names) ---- */
typedef void (*hal_<p>_callback_t)(hal_<p>_id_t id, uint32_t event, void *ctx);
hal_status_t hal_<p>_attach_callback (hal_<p>_id_t id, hal_<p>_event_t evt,
                                      hal_<p>_callback_t cb, void *ctx);
```

Conventions:
- Buffer parameters are `(pointer, length)` pairs; `size_t` for lengths.
- Timeouts in **milliseconds**; `HAL_TIMEOUT_NONE` (0) = poll-once,
  `HAL_TIMEOUT_FOREVER` = block.
- ISRs are an implementation detail of the port; applications register
  `*_callback_t` functions and never name `IRQHandler` symbols.
- The optional `hal_uart_print(id, val)` `_Generic` convenience macro may exist,
  but as **one** macro — not a per-instance family.

### Capability gating

Optional code stays in the same header, guarded by generated macros:

```c
#if NAVHAL_HAS_DMA
hal_status_t hal_uart_write_dma(hal_uart_id_t id, const uint8_t *buf, size_t len);
#endif
```

`NAVHAL_HAS_DMA`, `NAVHAL_HAS_FPU`, etc. are emitted into `navhal_target.h` by
the Kconfig→header generator from the corresponding `CONFIG_DRV_*` symbols.

---

## 9. Type system

- `hal_types.h` keeps `<stdint.h>`/`<stdbool.h>`, the `__I/__O/__IO` qualifiers,
  and `NAVHAL_UNUSED`/`NAVHAL_INLINE` compiler shims. Drop the `byte` alias
  (use `uint8_t`).
- Per-peripheral public types move **into the peripheral's public header**
  (`hal_gpio.h` owns `hal_gpio_mode_t`, etc.) — retire the scattered
  `utils/*_types.h` split, or keep them but include them only from the matching
  `hal_<p>.h`.
- `*_reg.h` register-map headers are **family-private** — moved under
  `src/vendor/stm32/family/.../include/family/` and never reachable from
  `include/navhal/`.

---

## 10. Header-dispatch mechanism

Replace `#ifdef CORTEX_M4 #include "core/cortex-m4/gpio.h"` entirely.

1. **Pure-prototype headers.** `include/navhal/hal_gpio.h` declares the API. No
   target conditionals.
2. **Port headers by include path.** When a header genuinely needs target
   compile-time content (inline register-backed `hal_gpio_write`, the
   `hal_uart_id` enumerators), it does:
   ```c
   #include "navhal_port_gpio.h"   /* fixed name; resolved via -I path */
   ```
   CMake adds the include directory of the *selected* family, so the same
   `#include` resolves to the correct target with no `#ifdef`.
3. **One generated header.** `navhal_target.h` (Kconfig output) carries
   `NAVHAL_CONFIG_*` / `NAVHAL_HAS_*` and target identity macros.

`navhal.h` then becomes a flat list of `#include "navhal/hal_<p>.h"` with no
conditionals.

---

## 11. Build-system integration

- Kconfig already models `ARCH`, `VENDOR`, `BOARD`, `DRV_*`. Extend it with a
  `FAMILY` axis and per-driver capability sub-options (`DRV_UART_DMA`, etc.).
- A generator step turns `.config` into `navhal_target.h` (replacing manual
  `compile_commands`/macro plumbing).
- CMake selects sources by glob over the chosen `arch/<isa>`,
  `vendor/<vendor>/family/<family>`, and `board/<board>` directories, and adds
  their `include/` dirs to the include path. Adding a new MCU = adding three
  directories + Kconfig entries; **no CMakeLists edits in drivers**.

---

## 12. Versioning

- Keep `VERSION_*` in `navhal.h` for the release version (SemVer).
- Add a separate **API contract version** `HAL_API_VERSION` (integer, bumped on
  any breaking change to a public header). Ports declare the contract version
  they implement; a mismatch is a compile-time error.
- Public API removals require a full deprecation cycle (Section 13, Phase 5).

---

## 13. Migration plan (phased, non-breaking)

Each phase is independently shippable; application code keeps building until
Phase 5.

**Phase 0 — Baseline.**
Land this document. Fix outright bugs: rename `hal_12c.h` → `hal_i2c.h`, correct
`@file` doc tags, fix `FAILURE` macro/doc mismatch. No API change.

**Phase 1 — Foundations.**
Introduce `hal_status_t`, the cleaned `hal_types.h`, and the naming-convention
header. New code uses them; old code untouched.

**Phase 2 — Naming unification.**
Rename peripheral functions to `hal_<p>_*` and standardize signatures
(status returns, `(buf,len)`, timeouts). Ship a deprecated compatibility shim
per driver so existing samples build:
```c
/* compat/uart_compat.h — deprecated */
#define uart_init(b,u)  hal_uart_init((u), &(hal_uart_config_t){.baud=(b)})
```
Mark shims with `__attribute__((deprecated))` where possible.

**Phase 2+ — Comprehensive test suite.**
With the API surface frozen by Phase 2, grow `tests/` to cover the whole
standardized `hal_*` API — every function, success and `hal_status_t` error
paths — plus a host-runnable subset and CI. Built before Phases 3–4 so those
mechanical refactors are provably behavior-preserving, and it doubles as the
conformance harness for the M6 AVR port. See M2+ in `docs/execution_plan.md`.

**Phase 3 — Directory restructure.**
Split `core/cortex-m4/` into `arch/armv7e-m/`, `vendor/stm32/`,
`board/nucleo_f401re/`. Move `*_reg.h` to family-private. Update CMake globs.
Pure file moves — API unchanged.

**Phase 4 — Dispatch replacement.**
Remove `#ifdef CORTEX_M4` from `common/hal_*.h`; switch to prototype headers +
include-path port headers + generated `navhal_target.h`.

**Phase 5 — Conformance & cleanup.**
Per-driver pass against the Section 14 checklist; migrate the 27 samples to the
new API; delete the Phase-2 shims; bump `HAL_API_VERSION` to 1.

### Example function mapping

| Current | Standardized |
|---------|--------------|
| `uart_init(baud, UART2)` | `hal_uart_init(HAL_UART_2, &cfg)` |
| `uart_write_char(c, UART2)` / `uart2_write_char(c)` | `hal_uart_write(HAL_UART_2, &c, 1, HAL_TIMEOUT_FOREVER)` |
| `uart_write_string(s, u)` | `hal_uart_write(u, (const uint8_t*)s, strlen(s), tmo)` |
| `uart_read_char(u)` | `hal_uart_read(u, &c, 1, tmo)` |
| `uart_write_dma(d,l,u)` | `hal_uart_write_dma(u, d, l)` |
| `hal_gpio_setmode(pin,mode,pupd)` | `hal_gpio_set_mode(pin, mode, pull)` — or `hal_gpio_init(pin, &cfg)` for full config |
| `hal_gpio_digitalwrite(pin,st)` | `hal_gpio_write(pin, st)` |
| `hal_gpio_digitalread(pin)` | `hal_gpio_read(pin)` |
| `systick_init(us)` / `delay_ms(ms)` | `hal_systick_init(us)` / `hal_delay_ms(ms)` |
| `timer_init(t,psc,arr)` | `hal_timer_init(t, &hal_timer_config_t{...})` |
| `hal_i2c_write(uint8_t bus,...)` | `hal_i2c_write(hal_i2c_id_t id, ...)` (typed id) |
| `TIM2_IRQHandler` (exposed) | internal; app uses `hal_timer_attach_callback(...)` |

---

## 14. Per-driver conformance checklist

A driver is "standardized" when all of the following hold:

- [ ] Public header is `include/navhal/hal_<p>.h`, declarations only, no `#ifdef`.
- [ ] All public symbols use the `hal_<p>_` prefix and `snake_case` verbs.
- [ ] `hal_<p>_init`/`hal_<p>_deinit` exist; init takes `const hal_<p>_config_t*`.
- [ ] Every fallible function returns `hal_status_t`; data out via pointer.
- [ ] Instances addressed by a typed `hal_<p>_id_t` enum (no bare `uint8_t`).
- [ ] Buffers passed as `(ptr, size_t len)`; timeouts in ms.
- [ ] IT/DMA variants use `_it`/`_dma` suffixes and are gated by `NAVHAL_HAS_*`.
- [ ] No `IRQHandler`/`*_Handler` or `*_reg.h` symbols in the public header.
- [ ] Events delivered via a registered `hal_<p>_callback_t`.
- [ ] Doxygen `@file`/`@ingroup` correct; one example in the header.
- [ ] Unit test in `tests/` updated to the new signatures.

Drivers to migrate: `gpio, uart, clock, timer, pwm, i2c, spi, dma, crc, flash,
dwt, fpu, sdio, interrupt`.

---

## 15. Open questions

> **Resolved:** GPIO pin identification — adopted the two-layer model in
> Section 7 (per-MCU `GPIO_Pxnn` core enum + per-board macro aliases).

1. **FatFs / v_fs** — treated as `common/` portable code or as an optional
   middleware layer above the HAL?
2. **C++ wrapper** — the README promises one; the namespacing here (stable C
   ABI, enum IDs) is wrapper-friendly, but the wrapper layout is out of scope of
   this draft.
3. **RTOS hooks** — should `hal_<p>_*_it`/`_dma` completion integrate with an
   RTOS notification abstraction now, or after Phase 5?
