@page roadmap_m9 M9 — Driver vtable + vendor-backend abstraction

# M9 — Driver vtable + vendor-backend abstraction

> Status: **planned**
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

Once the vtable is the contract, a port that doesn't fill in an
entry can be caught at build time (`-Wmissing-field-initializers`)
or at link time (the symbol stays NULL). The conformance test from
M8.4 walks every required vtable entry and asserts non-NULL plus
documented-behaviour. This becomes the "did the port really
implement the HAL" gate that's missing today.

## Cost estimate

* Subsystem 1 (GPIO): **~1 week**. Most of the design effort lives
  here — what does an ops table look like, how does validation split
  between public layer and vendor layer, how do the inline hot-paths
  survive.
* Subsystems 2–8: **~3 days each**, mostly mechanical once GPIO sets
  the pattern. Call it **~5 weeks** total for the migration.
* Conformance harness: ~1 week.
* Total: **~6–7 weeks** of focused work, end to end.

## Exit criteria

* Adding a hypothetical "RP2040 GPIO" port = filling in
  `rp2040_gpio_ops`, no other code changes. Concrete proof: a
  one-PR addition of a new vendor's GPIO that's ≤ 300 lines.
* Conformance harness covers every public `hal_*` function and
  documents the expected behaviour. Passes on both Cortex-M4 and AVR.
* No vendor `gpio.c` (or other migrated subsystem) re-implements
  validation logic that's already in the public layer.
* Performance: AVR `hal_gpio_set` is ≤ 2 cycles slower than today's
  inlined version; release builds with `-flto` show no regression.

## Open questions

* ~~Should the vtable be a `const struct` at fixed address (Zephyr's
  pattern, devirtualises under LTO) or function-pointer-via-getter
  (more flexible, slower)?~~ **Resolved (GPIO migration):** a `const`
  table embedded directly — `extern const hal_gpio_ops_t _hal_gpio_ops;`,
  not a pointer-to-table. This drops one indirection and devirtualises
  under `-flto`. The hot paths (write/read/toggle) stay `static inline`
  in the port header and never enter the table.
* What about subsystems where vendor implementations are *wildly*
  different — e.g., a chip with hardware multi-master I²C arbitration
  that needs APIs the rest don't have? Two options: extend the ops
  table with optional entries (caller checks for non-NULL), or
  punt to a vendor-specific extension namespace
  (`hal_stm32_i2c_*`). M9 needs an answer before the I²C migration.
* When a port wants to substitute an arch-shared implementation
  (e.g., a software-emulated SPI on a chip without hardware SPI),
  how does the vtable accommodate? Probably: software-fallback
  vendor `software_spi_ops` lives in `src/vendor/_software/`, ports
  point at it when their hardware is missing.
