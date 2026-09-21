@page roadmap_m9 M9 — Driver vtable + vendor-backend abstraction

# M9 — Driver vtable + vendor-backend abstraction

> Status: **done** — every exit criterion below is met and measured.
> Detail in the execution plan: @ref roadmap_m9_plan.
> Scope: introduce a HAL-internal interface between the public
> `hal_*` API and per-vendor implementations, so adding a new vendor
> means filling in a vtable, not re-writing every driver from scratch.
> Predecessor: M7 (modular build) — vtable layout cleanly per arch
> requires per-arch CMake fragments to install it.
> Unlocks: ~80 % less per-vendor boilerplate; mechanical conformance
> enforcement (the vtable shape *is* the contract).

## Goal

After M9, a new vendor adds:

* One file per driver: `src/vendor/<vendor>/<peripheral>/<peripheral>.c`,
  filling in the vendor-specific operations.
* No new public headers. No new types. No copies of validation logic.
* A single declaration like `NAVHAL_REGISTER_VENDOR(nrf52, &nrf52_ops);`
  in a port-init file.

The public `hal_gpio_init(pin, cfg)` call goes through the same code
path on every vendor — it validates `cfg`, then dispatches to the
vendor's `gpio_init_impl(pin, cfg)` via the vtable.

## Why now

The current model is **vendor = copy and re-implement**. Today's `gpio.c`
files:

```
src/vendor/stm32/gpio/gpio.c        ~400 lines
src/vendor/microchip/gpio/gpio.c    ~200 lines
```

Both implement the same `hal_status_t hal_gpio_init(...)` API. Both
validate the same `cfg` field combinations (`HAL_GPIO_MODE_AF` requires
a non-zero `alternate`, etc.). Both translate `HAL_GPIO_PULL_UP` to
vendor-specific registers.

Add five more vendors and you've got seven copies of the same
validation, plus seven chances to silently disagree on edge cases.
The cap-contract checks "does the symbol exist", not "does it implement
the same semantics" — and the more vendors, the more likely a port
ships with a subtly different behaviour from the reference one.

## What changes

### 9.1 — Vtable shape per driver

A vendor-facing interface per HAL subsystem. Example for GPIO:

```c
/* include/internal/hal_gpio_ops.h — NOT public */
typedef struct {
    hal_status_t (*init)        (hal_gpio_pin_t pin, const hal_gpio_config_t *cfg);
    hal_status_t (*set_mode)    (hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                                 hal_gpio_pull_t pull);
    hal_gpio_mode_t (*get_mode) (hal_gpio_pin_t pin);
    /* …one entry per public hal_gpio_* function… */
} hal_gpio_ops_t;

/* Embedded directly (a const object, NOT a pointer-to-table) — drops one
 * indirection and lets -flto devirtualise the dispatch. Defined by the
 * active vendor. */
extern const hal_gpio_ops_t _hal_gpio_ops;
```

`src/common/hal_gpio.c` (new!) provides the public implementation,
which validates and dispatches:

```c
hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
    if (!cfg) return HAL_ERR_INVALID_ARG;
    return _hal_gpio_ops.init(pin, cfg);
}
```

> The illustrative `cfg->alternate == HAL_GPIO_AF_NONE` rule from earlier
> drafts is deferred: the current port API has no `HAL_GPIO_AF_NONE`
> sentinel, so the first GPIO migration hoists only the genuinely
> duplicated check (the NULL `cfg`) to stay behaviour-preserving. Richer
> shared validation can follow once the sentinel exists.

Vendor `src/vendor/stm32/gpio/gpio.c` becomes:

```c
static hal_status_t stm32_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
    /* register-poking only — validation already happened upstream */
    ...
}

const hal_gpio_ops_t _hal_gpio_ops = {
    .init     = stm32_gpio_init,
    .set_mode = stm32_gpio_set_mode,
    /* … */
};
```

### 9.2 — Per-subsystem migration

Roll this out one subsystem at a time so each step is reviewable:

| Order | Subsystem      | Why this order |
|---|---|---|
| 1 | GPIO ✅ done    | Smallest stable surface, most heavily duplicated, lowest risk |
| 2 | UART           | Second-smallest surface; sets the pattern for "options struct" |
| 3 | INTERRUPT      | Architecture-touching but the pattern is similar |
| 4 | TIMER + PWM    | Closely related, do them together |
| 5 | I2C + SPI      | Both are bus drivers with similar shape |
| 6 | DMA            | Capability-gated — only ports with `NAVHAL_HAS_DMA == 1` need the ops table |
| 7 | FLASH          | Touches hardware-write semantics; do last |
| 8 | CLOCK          | The chicken-and-egg one — drivers may depend on it; resolve after |

Each subsystem migration is one PR. Cap-contract test gates the
behaviour-preservation property.

### 9.3 — Cost-of-abstraction defence

> Deep dive (current model, this proposal, five other HALs, measured
> per-arch costs, and the compiler mechanics that make it free):
> @ref roadmap_abstraction.

The vtable adds one indirect call per HAL invocation. Measured on a
faithful model with the project's real flags: ~3–6 cycles on Cortex-M4
at `-O0`, ~+8 cycles on AVR at `-Os` (~0.5 µs @ 16 MHz), and **zero**
under `-O2 -flto` (the indirect call devirtualises to a direct call and
inlines — verified by disassembly on both arches). Mitigation:

* For known hot paths (GPIO `set` / `clear` / `toggle`), keep the
  inline accessors in the port header (today's pattern). The vtable
  covers the slow-path init/config calls. AVR's "inline GPIO" perf
  commit (ecae042) stays correct.
* `-flto` on release builds devirtualises the indirect call when the
  ops table is `const` and known at link time. Common pattern in
  Zephyr's device-driver model.

### 9.4 — Conformance enforcement

Once the vtable is the contract, a port that doesn't fill in an entry
should be caught automatically. This section first said the compiler
would do it, via `-Wmissing-field-initializers`. **It does not.** GCC
does not warn about missing fields in a *designated* initializer —
which is how every ops table in the tree is written — under `-Wall`,
under `-Wextra`, or with the flag named explicitly. A port that omits
an entry gets a NULL there and a clean build; the first symptom is a
jump to address zero on hardware. The flag was added and removed again
during M9 after it was verified to catch nothing.

The gate is therefore a test, `tests/portable/conformance/test_vtable.c`:
one case per ops table the build links, asserting the table contains no
null entry. It walks the table as an array of function pointers rather
than naming each field, so a table that grows an entry is covered the
day it grows. That works only because optional capabilities are
*sibling tables* (`DRV_WWDG`, `DRV_UART_DMA`, `DRV_I2C_DMA`) rather
than nullable entries in their parent — if a nullable entry is ever
added, this suite is what breaks, and that design decision is what
should be reconsidered.

Behavioural conformance is the separate, larger suite next to it
(`test_conformance.c`), which covers 128 of the 164 public entry
points. Together they are the "did the port really implement the HAL"
gate that was missing.

## Cost estimate

* Subsystem 1 (GPIO): **~1 week**. Most of the design effort lives
  here — what does an ops table look like, how does validation split
  between public layer and vendor layer, how do the inline hot-paths
  survive.
* Subsystems 2–8: **~3 days each**, mostly mechanical once GPIO sets
  the pattern. Call it **~5 weeks** total for the migration.
* Conformance harness: ~1 week.
* Total: **~6–7 weeks** of focused work, end to end.

The migration came in broader than this estimate. It covers 16 ops
tables rather than 8, because `adc`, `reset`, `watchdog`, `wwdg`,
`interrupt`, `timebase` and the DMA siblings were all multi-backend
too, and because two of those — `interrupt` and `timebase` — live under
`src/arch/` and were missed by a scope survey that only looked at
`src/vendor/`.

## Exit criteria

* **Met.** Adding a vendor's GPIO = filling in an ops table, no other
  code changes. Proved by the ACME reference port
  (`src/vendor/acme/`): 365 lines across 13 new files, of which the
  driver is 118 and the rest is scaffolding every port needs. Kconfig
  and CMake discover it by glob and identity string, so neither needed
  editing.

  What the criterion did not anticipate: the inlined hot path was not
  covered by it. `hal_gpio_write/read/toggle` lived in the *arch*
  header, written against STM32's `BSRR`/`IDR`/`ODR`, so a second
  vendor on the same arch had to emulate another vendor's register
  layout. They now live with the vendor, beside the register map. A
  vendor's GPIO contribution is two things it owns: an ops table for
  configuration, and inline accessors for the hot path.

* **Met.** Conformance covers 128 of the 164 public functions —
  argument rejection, instance-id rejection, getter sanity and the
  round-trips a port can be wrong about. Plus the vtable-completeness
  suite above, which is what makes "filling in a table" checkable at
  all.

  What is deliberately not covered is what a bare board cannot prove:
  nothing arms a watchdog (the IWDG cannot be stopped again), resets
  the part, erases flash, or enters an eth/sdio/usb\_cdc path that
  waits on hardware that is not attached. Timing cases check the tick
  is running first, because `hal_delay_ms` spins on it and never
  returns on a target whose application never started a timebase.

  Writing it found four real defects: two STM32 timer entry points
  that accepted any instance id and answered `HAL_OK`, `hal_sdio_read_block`
  reaching a half-second card poll before looking at its buffer, and —
  the one no Cortex tier could see — the AVR test image overflowing a
  32 KB part, because avr-gcc never merges two identical `PSTR`s and
  every assertion carried its own copy of an absolute `__FILE__` path.

* **Met.** No migrated backend re-implements validation the shared
  layer performs. Checked by inspection across all migrated drivers;
  what remains in backends is register-base and instance-range
  checking, which only a port can do.

* **Met, and measured rather than asserted.** `hal_gpio_write` is not
  ≤2 cycles slower, it is identical: a constant pin compiles to one
  `sbi` on AVR, the same instruction hand-written code emits.

  Dispatch resolves completely under LTO. The figure that shows it is
  not a byte count but whether the ops tables are still *there*: a
  table every caller resolved has no remaining reference, so the linker
  drops it. Measured on `hal_blink`:

  | Profile | AVR text / tables left | Cortex-M4 text / tables left |
  |---|---|---|
  | Debug (`-Og`) | 8240 B / 6 | 12640 B / 7 |
  | Release (`-Os`) | 7280 B / 6 | 11016 B / 7 |
  | ReleaseLTO | 1330 B / **0** | 4288 B / **0** |

  The byte counts owe as much to `-Os` as to LTO and are context; the
  table count is the claim. `tools/check_devirt.sh` runs this check on
  all three arches and fails if any table survives.

  What does remain indirect under LTO is callbacks the application
  registers at run time, dispatched from an ISR — three sites on ARM
  (`armv7em_interrupt_dispatch`, `SysTick_Handler`,
  `hal_irq_default_dispatch`) and two on AVR (`avr_interrupt_dispatch`,
  `__vector_14`). Those are indirect by design and no amount of LTO
  can resolve them, which is why the check does not count branches:
  the count moves when a sample registers one more callback, and that
  says nothing about dispatch. Earlier revisions of this page reported
  "zero indirect calls", counting only `blx <reg>` and so missing
  ARM's indirect tail branches and AVR's `icall` from the timer ISR.

  Reaching any of this needed three build fixes first: the tree could
  not produce an optimised binary at all, because `-O0` was hardcoded
  in the arch flags, the root CMakeLists discarded caller-supplied
  flags, and armv7e-m never passed `-ffreestanding`, so GCC compiled
  `hal_strlen` into a call to `strlen`.

## Open questions

* ~~Should the vtable be a `const struct` at fixed address (Zephyr's
  pattern, devirtualises under LTO) or function-pointer-via-getter
  (more flexible, slower)?~~ **Resolved (GPIO migration):** a `const`
  table embedded directly — `extern const hal_gpio_ops_t _hal_gpio_ops;`,
  not a pointer-to-table. This drops one indirection and devirtualises
  under `-flto`. The hot paths (write/read/toggle) stay `static inline`
  in the port header and never enter the table.
* ~~What about subsystems where vendor implementations are *wildly*
  different — e.g., a chip with hardware multi-master I²C arbitration
  that needs APIs the rest don't have?~~ **Resolved (WWDG, then both
  DMA pairs):** a **sibling table** behind its own `DRV_*` symbol —
  `_hal_wwdg_ops`, `_hal_uart_dma_ops`, `_hal_i2c_dma_ops` — not
  optional entries in the parent table. A port either has the
  capability and fills in its table, or does not have it and the table
  is not linked. Nullable entries were rejected because they make
  completeness uncheckable: with them, no tool can distinguish "this
  port does not have WWDG" from "this port forgot WWDG". The
  vendor-extension namespace (`hal_stm32_i2c_*`) remains available for
  something genuinely un-portable, and nothing has needed it yet.
* When a port wants to substitute an arch-shared implementation
  (e.g., a software-emulated SPI on a chip without hardware SPI),
  how does the vtable accommodate? Probably: software-fallback
  vendor `software_spi_ops` lives in `src/vendor/_software/`, ports
  point at it when their hardware is missing.
