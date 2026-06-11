@page roadmap_abstraction HAL Abstraction — current model, the M9 proposal, and how compilers make it free

# HAL Abstraction: Current Model, the M9 Proposal, and the Compiler

> Companion reading for @ref roadmap_m9. This is a *reference* document,
> not a milestone: it explains, in depth, how NavHAL abstracts per-vendor
> hardware today, why that hurts as the vendor count grows, what the M9
> driver-vtable proposes, how four other HALs (Arduino, Zephyr, STM32Cube,
> ChibiOS, Mbed) solve the same problem, and — crucially — how C compilers
> turn the proposed indirection back into zero-cost direct calls.
>
> If you read only one thing: **the M9 vtable is a *source-organisation*
> device, not a runtime-polymorphism requirement.** NavHAL builds exactly
> one vendor per firmware, so the indirection it introduces is an artefact
> of separate compilation that `-flto` removes entirely. The runtime cost
> is therefore a build-configuration choice, not an inherent tax. The rest
> of this document is the evidence for that sentence.

---

## 1. How to read this

The four questions this document answers, in order:

1. **What does NavHAL do today?** (§2) Compile-time monomorphisation: one
   vendor's `.c` files are compiled per firmware; every `hal_*` call is a
   direct call; there is *no* runtime indirection anywhere.
2. **Why change it?** (§3) The same validation logic is copy-pasted into
   every vendor tree, the conformance gate checks *symbols* not *semantics*,
   and both problems scale with `vendors × drivers`.
3. **What is proposed (M9), and what does it cost?** (§4) A shared public
   layer that validates and dispatches through a per-vendor vtable. Measured
   cost: a few cycles per call in debug builds, **zero** under `-flto`.
4. **How does everyone else do it, and how do compilers erase the cost?**
   (§5 case studies, §6 compiler mechanics.)

---

## 2. The current model — compile-time monomorphisation

### 2.1 One vendor per firmware, selected by the build

NavHAL does not put two vendors in one binary. The Kconfig-resolved
`VENDOR` slug selects exactly one vendor source tree, and CMake compiles
*only* that tree (`CMakeLists.txt`):

```cmake
set(SRC_VENDOR ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/${VENDOR})
...
add_subdirectory(${SRC_VENDOR})          # only stm32/ OR microchip/, never both
```

Each vendor's `CMakeLists.txt` then adds one `.c` per enabled driver:

```cmake
if(CONFIG_DRV_GPIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c)
endif()
```

So the firmware contains exactly one definition of `hal_gpio_init`, linked
directly. **There is no vtable, no function-pointer dispatch, no `switch`
on a vendor tag.** A grep for ops-tables confirms it: the only function
pointers in the public headers are async *callbacks*
(`hal_timer_callback_t`, `hal_sdio_callback_t`), which are completion
notifications, not a dispatch mechanism.

This is the cheapest possible abstraction: the "abstraction" is the
*shared header* (`include/common/hal_gpio.h`), and binding to an
implementation happens at link time.

### 2.2 The public surface and the hot-path escape hatch

The portable contract lives in `include/common/hal_gpio.h`:

```c
typedef struct {
  hal_gpio_mode_t        mode;
  hal_gpio_pull_t        pull;
  hal_gpio_output_type_t output_type;
  hal_gpio_output_speed_t output_speed;
  hal_gpio_af_t          alternate;
} hal_gpio_config_t;

hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg);
hal_status_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                               hal_gpio_pull_t pull);
/* … */
```

The genuinely hot operations (`set` / `clear` / `toggle` / `read`) are
**not** regular functions at all — they are `static inline` accessors in
the per-port header (`include/port/<core>/navhal_port_gpio.h`) that fold
to a single register write. This matters for §4: the cost-sensitive paths
are *already* inlined and will stay that way under M9.

### 2.3 Capability gating

Optional drivers are gated at compile time by `NAVHAL_HAS_*` macros
(`include/common/hal_features.h`), always defined as `0` or `1` so source
can use `#if NAVHAL_HAS_DMA` without `#ifdef` gymnastics. These are
generated into `navhal_target.h` from Kconfig. Gating is compile-time;
there is no runtime capability query.

---

## 3. The problem

### 3.1 "Vendor = copy and re-implement"

Both vendors implement the *same* public API, and today that means each
re-implements the shared parts. Concretely, `hal_gpio_init` validation is
duplicated:

`src/vendor/stm32/gpio/gpio.c` (~109 lines):
```c
hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;          // <-- duplicated
  hal_gpio_set_mode(pin, cfg->mode, cfg->pull);
  if (cfg->mode == HAL_GPIO_MODE_OUTPUT || cfg->mode == HAL_GPIO_MODE_AF) {
    hal_gpio_set_output_type(pin, cfg->output_type);
    hal_gpio_set_output_speed(pin, cfg->output_speed);
  }
  if (cfg->mode == HAL_GPIO_MODE_AF)
    hal_gpio_set_alternate_function(pin, cfg->alternate);
  return HAL_OK;
}
```

`src/vendor/microchip/gpio/gpio.c` (~123 lines):
```c
hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;          // <-- duplicated, identical
  /* output_type/speed/alternate have no ATmega328P effect. */
  return hal_gpio_set_mode(pin, cfg->mode, cfg->pull);
}
```

With two vendors this is a `NULL` check in two places — annoying but
harmless. With seven vendors it is seven copies of every validation rule
(`MODE_AF requires a non-zero alternate`, error-code choices, idempotency
guarantees) and **seven chances to silently disagree** on an edge case.
The reference port's behaviour and a new port's behaviour drift apart, and
nothing catches it.

### 3.2 The conformance gate checks symbols, not semantics

The conformance suite
(`tests/portable/conformance/test_conformance.c`) is black-box and
*partial*: it checks that `hal_gpio_init(.., NULL)` returns
`HAL_ERR_INVALID_ARG`, that the call is idempotent, that status codes are
distinct and `uint8`-sized, and that capability macros are 0/1. It does
**not** — and structurally cannot — verify that a vendor's
register-poking implements the *same* semantics as the reference. The
current cap-contract answers "does the symbol exist and reject NULL", not
"does this port behave like the others".

### 3.3 It scales as `vendors × drivers`

Every new vendor re-pays the full validation surface for every driver.
The maintainer reviews and merges that boilerplate, and the drift risk
compounds. This is the wall M9 is meant to remove.

---

## 4. The proposed model — M9 driver vtable

### 4.1 Shape

Split each driver into a **shared public layer** (validate + dispatch) and
a **vendor backend** (register-poking only), connected by a per-subsystem
ops table. From @ref roadmap_m9:

```c
/* include/internal/hal_gpio_ops.h — NOT public */
typedef struct {
    hal_status_t (*init)     (hal_gpio_pin_t pin, const hal_gpio_config_t *cfg);
    hal_status_t (*set_mode) (hal_gpio_pin_t pin, hal_gpio_mode_t, hal_gpio_pull_t);
    /* …one entry per public hal_gpio_* function… */
} hal_gpio_ops_t;
extern const hal_gpio_ops_t *const _hal_gpio_ops;   /* the port supplies this */
```

```c
/* include/common/hal_gpio.c (new) — the ONE place validation lives */
hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
    if (!cfg) return HAL_ERR_INVALID_ARG;
    if (cfg->mode == HAL_GPIO_MODE_AF && cfg->alternate == HAL_GPIO_AF_NONE)
        return HAL_ERR_INVALID_ARG;
    return _hal_gpio_ops->init(pin, cfg);           /* dispatch */
}
```

```c
/* src/vendor/stm32/gpio/gpio.c — register-poking only */
static hal_status_t stm32_gpio_init_impl(...) { /* validation already done */ }
static const hal_gpio_ops_t stm32_gpio_ops = { .init = stm32_gpio_init_impl, … };
const hal_gpio_ops_t *const _hal_gpio_ops = &stm32_gpio_ops;
```

### 4.2 What it buys

* **Validation written once.** The `NULL` check and the `MODE_AF` rule live
  in the public layer; vendors cannot drift because they no longer own that
  code.
* **The vtable shape *is* the contract.** A port that forgets an entry is
  caught by `-Wmissing-field-initializers` (compile) or a NULL symbol
  (link), and the conformance harness can walk every required entry.
* **~80 % less per-vendor boilerplate** — a new vendor fills in
  `*_impl` functions, no new headers, no copied validation.

### 4.3 The critical nuance: this is *not* runtime polymorphism for NavHAL

Zephyr/Mbed use a vtable so **one binary** can drive **many chips chosen at
runtime**. NavHAL does not do that — it builds one vendor per firmware
(§2.1). So the M9 vtable is *not* there to switch vendors at runtime; it
exists purely to **deduplicate source** behind a clean contract. The
function pointer is, in NavHAL's single-target world, *always* pointing at
the one vendor that was compiled in.

That is exactly the situation a compiler can prove and erase. Which leads
to the cost analysis.

### 4.4 Measured cost (faithful model, your real toolchains)

A two-translation-unit model of the dispatch above (public layer + vendor
impl in separate `.c`, the `const *const` handle exactly as proposed),
compiled with the project's real flags:

| Target | Build | Dispatch overhead vs direct call | Cost |
|---|---|---|---|
| Cortex-M4 | `-O0` (project default) | `ldr;ldr;ldr` + `blx r3` | **~3–6 cyc** |
| Cortex-M4 | `-O2` (no LTO) | `ldr;ldr` + `bx r3` (tail) | ~2–4 cyc |
| Cortex-M4 | **`-O2 -flto`** | *none — inlined to `bl impl`* | **0** |
| AVR (328P) | `-Os` (project default) | 4× byte-load + `movw` + `ijmp` | **+8 cyc (~0.5 µs @16 MHz)** |
| AVR (328P) | **`-O2 -flto`** | *none — devirtualised* | **0** |

Two findings worth carrying into the design:

1. **The proposed handle is a pointer-*to*-table (`const hal_gpio_ops_t
   *const`), which is a *double* indirection** — `_hal_gpio_ops` (a pointer
   in memory) → the ops table → `->init`. On Cortex that is the third
   `ldr`; on AVR two of the four byte-loads. Declaring the table *directly*
   (`extern const hal_gpio_ops_t _hal_gpio_ops;`) drops one load on both
   arches. This is M9's own §9.1 open question — and the measurement
   answers it: **embed the table, don't point at it.**
2. **`-flto` erases the cost on both arches** (verified: zero `blx`/`ijmp`
   survivors; `main` calls the impl directly). But the project's *default*
   builds are `-O0`/`-Os` *without* LTO, so day-to-day dev/test binaries
   *will* carry the full overhead. The zero-cost result is a
   **release-build-configuration** property, not automatic — see §6.

### 4.5 The AVR secondary cost: SRAM

On AVR (Harvard architecture) a `static const` ops table accessed through
normal pointers lives in **RAM**, not flash (the `lds` loads prove it).
Every migrated driver therefore spends a few bytes of the **2 KB** SRAM
budget on its table + handle. Across ~13 drivers that is plausibly
150–250 bytes — on a part where the test suite already needed a 1.2 KB
PROGMEM rescue. Putting tables in `PROGMEM`/`__flash` avoids the RAM cost
but forces `LPM`-based dispatch (slower) — a per-driver RAM-vs-speed
choice the migration must make consciously.

### 4.6 Design recommendations that fall out of the measurement

1. **Embed the ops table, don't point to it** (drop one indirection level).
2. **Keep `set`/`clear`/`toggle`/`read` as `static inline`** — the vtable
   covers slow-path `init`/`config` only; hot paths stay zero-cost (this is
   what satisfies M9's "≤ 2 cycles slower" exit criterion).
3. **Add an LTO release configuration** so the abstraction is free where it
   ships; keep `-O0`/`-Og` for debug and accept the few cycles there.
4. **Decide AVR table placement** (RAM vs PROGMEM) per driver if SRAM tightens.
5. **Add a cycle-counted regression test** (the DWT cycle-counter driver +
   simavr already exist) so "zero-cost under LTO" is enforced, not asserted.

---

## 5. Case studies — how five other HALs abstract hardware

The spectrum runs from "no abstraction overhead, no runtime flexibility"
(CMSIS) to "full runtime polymorphism" (Zephyr). The recurring lesson:
**mature HALs bind hardware primitives at compile/link time and reserve
runtime vtables for genuinely heterogeneous, cold abstractions (byte
streams / files).**

### 5.1 Zephyr — the canonical const-vtable device model

Zephyr is the reference implementation of "vtable done right" for embedded.

* **`struct device`** carries a `const void *api` pointer to a per-driver
  **vtable** of function pointers, plus `const *config` (build-time, ROM)
  and `void *data` (per-instance RAM). The API struct is the vtable:

  ```c
  __subsystem struct gpio_driver_api {
      int (*pin_configure)(const struct device *port, gpio_pin_t pin, gpio_flags_t flags);
      int (*port_set_bits_raw)(const struct device *port, gpio_port_pins_t pins);
      int (*port_toggle_bits)(const struct device *port, gpio_port_pins_t pins);
      /* … */
  };
  ```

* **Dispatch** is a thin static-inline wrapper:
  ```c
  static inline int z_impl_gpio_pin_configure(const struct device *port, …) {
      const struct gpio_driver_api *api = (const struct gpio_driver_api *)port->api;
      return api->pin_configure(port, pin, flags);
  }
  ```

* **Instantiation is data-driven.** `DEVICE_DT_DEFINE` creates a
  `const struct device` at a **fixed link-time address** in an **iterable
  linker section**; `DT_INST_FOREACH_STATUS_OKAY` instantiates exactly one
  device per enabled devicetree node, with MMIO/IRQ values baked in at
  compile time via `DT_INST_*`. The binary contains exactly the instances
  the board declares — no runtime discovery, no heap.

* **One `const` API per driver, shared by reference** by every instance →
  the vtable costs ROM once, and being `const` at a fixed address it is a
  candidate for LTO devirtualisation.

* **The syscall layer (`__syscall` / `z_vrfy_` / `z_impl_`) is orthogonal**
  to dispatch: it adds user/kernel argument validation *around* the same
  call, with **zero** overhead in supervisor mode. A HAL with no privilege
  boundary can skip it entirely and keep just the vtable.

* **Tradeoffs Zephyr pays and mitigates:**
  - For years `struct device` wrongly lived in RAM because a mutable
    init-flag tainted the const struct; fixed by hoisting mutable state
    into a *separate* `struct device_state`. **Lesson: never mutate the api
    pointer or the device struct at runtime.**
  - A fat vtable that references *all* ops **defeats `--gc-sections`** (the
    linker must keep every function the reachable table points at). Zephyr
    gates optional ops behind Kconfig `#ifdef` so they compile out entirely
    rather than relying on the linker (see §6.5).

> Key takeaways for NavHAL: keep the vtable `const` and shared; never
> mutate it; gate optional ops behind config flags, not the linker; expect
> one pointer-load + one indirect call per call unless LTO devirtualises.

### 5.2 Arduino — two abstraction strategies in one stack

Arduino deliberately *mixes* mechanisms based on call frequency.

* **GPIO/timer: table+macro HAL, no vtable.** `digitalWrite(pin, val)` is a
  plain C function that, on every call, does three flash-table lookups
  (`digitalPinToPort/BitMask/Timer` via `pgm_read_*` from PROGMEM), a
  PWM-disable check, a validity guard, and an interrupt-safe
  read-modify-write:

  ```c
  void digitalWrite(uint8_t pin, uint8_t val) {
      uint8_t timer = digitalPinToTimer(pin);   // PROGMEM lookup
      uint8_t bit   = digitalPinToBitMask(pin);  // PROGMEM lookup
      uint8_t port  = digitalPinToPort(pin);     // PROGMEM lookup
      if (port == NOT_A_PIN) return;
      if (timer != NOT_ON_TIMER) turnOffPWM(timer);
      volatile uint8_t *out = portOutputRegister(port);
      uint8_t oldSREG = SREG; cli();
      if (val == LOW) *out &= ~bit; else *out |= bit;
      SREG = oldSREG;
  }
  ```
  This is famously ~40× slower than a raw `PORTB |= _BV(5)` (≈80 cycles /
  ~5 µs vs ~2 cycles at 16 MHz — exact numbers vary by source). The
  abstraction's cost is the lookups + safety, not any single op. Advanced
  users drop to direct register writes or compile-time-resolved
  `digitalWriteFast` macros. It is a runtime table lookup, not a
  function-pointer vtable.

* **Boards = compile-time "variants".** Each board ships a
  `pins_arduino.h` (the PROGMEM tables); `boards.txt` selects the variant
  directory at build time. No runtime board detection.

* **Byte streams: real C++ virtual functions.** `Print → Stream →
  HardwareSerial` use a *pure virtual* `write(uint8_t)`; `Serial` overrides
  it. Any `Stream&`-taking library works with `Serial`, `SoftwareSerial`,
  an LCD, a socket — genuine vtable dispatch, because here the polymorphism
  is the point and the per-call cost is trivial next to shifting a byte out
  a UART.

> Key takeaway for NavHAL: match the mechanism to the call — table/inline
> for hot per-pin ops (which NavHAL already does), indirection only where
> composability matters. And: a "convenience HAL" has a real per-call tax;
> always provide a documented fast path (NavHAL's `static inline`
> accessors are exactly that).

### 5.3 STM32Cube (CMSIS / LL / HAL) — portability with zero runtime indirection

ST's three layers show you can get broad portability with *no* vtable.

* **CMSIS-Core** = register-map structs + base-address pointers, pure
  compile-time, zero overhead:
  ```c
  typedef struct { __IO uint32_t MODER; … __IO uint32_t BSRR; } GPIO_TypeDef;
  #define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
  GPIOA->BSRR = (1 << 5);   /* atomic set PA5 — one store, no abstraction code */
  ```

* **LL (Low-Layer)** = `__STATIC_INLINE` wrappers that inline to a single
  register write (`LL_GPIO_SetOutputPin` → `WRITE_REG(GPIOx->BSRR, mask)`).
  **Stateless** — no handles, lean.

* **Cube HAL** = thick, handle-based drivers (`HAL_GPIO_Init(GPIOx,
  &init)`, `UART_HandleTypeDef` holding a state machine). It *looks*
  object-oriented but has **no vtable**: `HAL_UART_Transmit(huart, …)` is a
  statically-linked call; the handle's `Instance` pointer is *data* (which
  registers), not dispatch. Portability across families comes from
  **per-family source trees selected by a `-DSTM32F407xx` define** and the
  linker picking one family's `.c` — "polymorphism by separate
  compilation". Overridable behaviour uses **weak symbols** (empty `__weak`
  callbacks; your strong definition wins at link time) at zero runtime
  cost.

* The famous "**HAL is heavy**" criticism: generic + validated + stateful
  ⇒ bigger, slower; LL/CMSIS ⇒ smaller, faster. Standard practice: HAL to
  start, LL on hot paths, mix freely.

> Key takeaway for NavHAL: if you never swap drivers at runtime (you
> don't), per-family/per-vendor source selected at build time + weak
> symbols gives full portability with *zero* indirection — which is
> essentially what NavHAL does today. M9's vtable is therefore justified by
> *dedup*, not portability, and must lean on LTO to match this baseline.

### 5.4 ChibiOS — compile-time HLD→LLD binding + a hand-rolled C vmt for streams

* **Two layers, bound at compile time.** A portable High-Level Driver
  (`pwmStart`, `palSetPad`) calls **fixed-name** Low-Level Driver functions
  (`pwm_lld_start`, `pal_lld_setpad`) *directly* — no function pointers.
  The Makefile's `platform.mk` pulls in exactly one MCU family's LLD `.c`,
  so the names resolve at link time:
  ```c
  void pwmStart(PWMDriver *pwmp, const PWMConfig *config) {
      osalSysLock(); pwmp->config = config;
      pwm_lld_start(pwmp);                 /* direct, fixed-name LLD call */
      osalSysUnlock();
  }
  ```
* **Subsystem on/off via `halconf.h`; peripheral→instance + clocks via
  `mcuconf.h`** — both pure preprocessor, so unused subsystems compile to
  nothing.
* **But streams use a real vmt.** `BaseSequentialStream` / `BaseChannel`
  embed a `const struct …VMT *vmt` as their first field — a hand-rolled C
  vtable — so `chprintf()` can target any stream. Same intent as Zephyr's
  `api`, done in C, and *only* for the cold stream path.

> Key takeaway for NavHAL: a fixed-name LLD contract bound at compile time
> is exactly NavHAL's current model; ChibiOS confirms the pattern scales to
> dozens of MCUs. Runtime indirection appears only for streams.

### 5.5 Mbed OS — fixed-name C contract per target + thin C++ wrappers

* **C HAL = a fixed-name function contract each target fills in**:
  `gpio_init`, `gpio_write`, `gpio_dir`, `serial_init`, … declared in
  `hal/`, implemented per target under `targets/TARGET_<VENDOR>/…`. The
  build compiles only the selected target's directory (driven by
  `targets.json` inheritance → a label set → which `TARGET_*` dirs
  compile), so each symbol has exactly one definition — direct, link-time
  bound, no vtable for primitives.
* **C++ `DigitalOut::write()` is a non-virtual inline** that forwards to
  `gpio_write()` — near-zero overhead.
* **`FileHandle` / stream classes use real C++ virtuals** — again, only the
  file/stream layer is polymorphic, for POSIX-fd and `printf` retargeting.

> Key takeaway for NavHAL: opaque per-target state types (`gpio_t`,
> `serial_t`) + a fixed-name contract is a clean, vtable-free way to let a
> portable header stay vendor-agnostic — and weak symbols are an
> *escape hatch for defaults*, not the core selector.

### 5.6 Comparison

| HAL | Primitive dispatch | Binding time | Runtime vtable? | Used where |
|---|---|---|---|---|
| **NavHAL today** | direct call | compile/link | no | — |
| **NavHAL + M9** | vtable (dedup) | compile/link (+LTO devirt) | yes, but devirtualisable | all migrated drivers |
| **CMSIS** | struct/macro | compile | no | — |
| **STM32 LL** | `static inline` | compile | no | — |
| **STM32 HAL** | direct call + handle | compile/link (per-family) | no (opt-in callbacks) | — |
| **ChibiOS** | fixed-name LLD call | compile/link | only for streams (C vmt) | byte streams |
| **Mbed** | fixed-name C fn | compile/link (per-target dir) | only for files (C++) | files/streams |
| **Arduino GPIO** | runtime table lookup | compile (variant) | no | — |
| **Arduino Stream** | C++ virtual | runtime | yes | byte streams |
| **Zephyr** | `dev->api` vtable | runtime (DT-instantiated) | yes (const, shared) | all drivers |

**The pattern is unanimous:** hardware *primitives* bind at compile/link
time; runtime indirection is reserved for cold, heterogeneous
abstractions. M9 introduces a vtable for primitives, which is the outlier —
*justified only because NavHAL's single-target build + LTO collapses it
back to the compile-time-bound baseline everyone else starts from.*

---

## 6. The compiler — how the indirection becomes free

Everything above hinges on one claim: a `const` vtable call devirtualises
to a direct call (and often inlines away) under the right flags. This
section is how and when that happens, with exact flag names. (The passes
are GCC's target-independent middle-end + GNU linker, so they behave
identically for `arm-none-eabi-gcc` and `avr-gcc`; Clang has analogous
mechanisms with different names.)

### 6.1 Optimisation levels

| Level | Inlining | Devirtualisation | Notes |
|---|---|---|---|
| `-O0` | no | no | every call real, every pointer-load real, locals spilled — deliberately, for debugging |
| `-O1` | only static-called-once | no | |
| `-O2` | **yes** (`-finline-functions`, `-finline-small-functions`, `-findirect-inlining`) | **yes** (`-fdevirtualize`, `-fipa-cp`) | |
| `-O3` | yes, aggressive | yes (+`-fipa-cp-clone`) | clone can *grow* code |
| `-Os` | yes (stingier heuristic) | yes (same as `-O2`) | common MCU release default |
| `-Og` | mostly no | no | best level that preserves debugging |

The decisive fact: **inlining, indirect-call inlining, devirtualisation,
and interprocedural constant propagation all first turn on at `-O2`/`-Os`.**
At `-O0` a `const` table stays a real table and the call through it stays a
real indirect call — which is exactly what the §4.4 `-O0`/`-Os`
measurements show.

### 6.2 Inlining

* `-finline-small-functions` / `-finline-functions` (at `-O2`/`-Os`) let
  the compiler inline even functions you never marked `inline`.
* `static inline` gives internal linkage → the compiler may inline every
  call and emit no out-of-line copy.
* **A body in another `.c` file cannot be inlined in a normal build** — the
  compiler sees only the prototype. This is the gap LTO closes (§6.4).

### 6.3 Devirtualisation — turning `ops->init(...)` into `impl(...)`

The mechanism that matters for a C HAL is constant-propagation +
store-to-load forwarding, not C++ type machinery:

1. `static const hal_gpio_ops_t stm32_gpio_ops = { .init = stm32_gpio_init_impl, … };`
   has a **known address** and **immutable contents**.
2. `-fipa-cp` propagates that constant pointer to the call site.
3. The compiler now knows the loaded value is `stm32_gpio_init_impl`
   (because the table is `const`) and rewrites the indirect call to a
   **direct** call.
4. `-findirect-inlining` may then inline it — erasing the call.

The three properties that unlock this, and why each is necessary:

* **`const`** — guarantees the table members never change, so the loaded
  pointer equals the initialiser. A non-`const` table could be overwritten;
  the compiler must keep the call indirect.
* **internal linkage (`static`)** — the compiler sees every use, so it can
  trust the initialiser is final. (Cross-TU references need LTO instead.)
* **known initialiser** — supplies the concrete target to propagate.

> This is precisely M9's pattern: `static const hal_gpio_ops_t
> stm32_gpio_ops = { … }`. Fully devirtualisable at `-O2` *within* the
> vendor TU. The cross-TU call from the public layer needs LTO.

### 6.4 LTO — devirtualisation *across* `.c` files

This is the crux for M9, because the public layer (`hal_gpio.c`) and the
vendor table (`gpio.c`) are **separate translation units**:

* **Without LTO:** compiling `hal_gpio.c`, the compiler sees only the
  `extern` declaration of `_hal_gpio_ops`, not its initialiser in
  `gpio.c`. It cannot prove the target, so the indirect call **survives** —
  the §4.4 `-O2 (no LTO)` row.
* **With `-flto`:** the compiler emits GIMPLE IR instead of final code;
  at **link time** all TUs are optimised together. IPA-CP propagates the
  table's constant pointers across the file boundary, `-fdevirtualize`
  rewrites the public-layer call to a direct call, and `-findirect-inlining`
  inlines it — the §4.4 `-flto` rows (**zero** survivors, verified by
  disassembly).

Mechanics & caveats: `-flto` must be passed at **both compile and link**;
use the `gcc` driver to link and `*-gcc-ar`/`*-gcc-nm` for archives (slim
LTO objects). `const` tables still help under LTO (they remove the
"could be mutated" obstacle).

### 6.5 Dead-code elimination — and the fat-vtable trap

`-ffunction-sections -fdata-sections` (compile) + `-Wl,--gc-sections`
(link) drop unreferenced functions/data. **But a fat vtable that
references every op anchors all of them**: as long as the table is
reachable, `--gc-sections` must keep every function it points at (the
Zephyr/LWN caveat). Two escapes:

* Gate optional ops behind config `#ifdef` so they never compile in
  (Zephyr's approach), or
* Let LTO devirtualise *and* inline the calls so the table itself becomes
  unreferenced — then `--gc-sections` reclaims it.

`-fno-common` (default since GCC 10) keeps globals in their own sections so
they participate cleanly in section GC.

### 6.6 Practical upshot for M9

| Build | Cross-TU vtable call (`hal_gpio.c` → `gpio.c`) |
|---|---|
| `-O0` (debug) | real indirect call + stack reloads — full cost |
| `-Os`/`-O2`, **no LTO** | **survives** — compiler can't see the table across TUs |
| `-O2 -flto` (release) | **devirtualised + inlined → zero**, table GC'd |

**Zero-cost is real but not automatic.** It requires LTO for any
abstraction that crosses translation units (M9's does), plus
`const`/internal-linkage tables. NavHAL's *current* defaults (`-O0`
Cortex, `-Os` AVR, no LTO) would ship the overhead — so M9 must land an
**LTO release configuration** to deliver the free abstraction.

### 6.7 Zero-cost checklist for the M9 migration

1. Release builds at `-O2`/`-Os` (debug stays `-O0`/`-Og`).
2. `-flto` at **both** compile and link; link via the `gcc` driver, use
   `*-gcc-ar`/`*-gcc-nm` for static libs.
3. Ops tables: `static const struct …` (or `extern const … _hal_gpio_ops;`
   — **the table, not a pointer to it**, per §4.4) with a fully known
   initialiser.
4. Keep `set`/`clear`/`toggle`/`read` `static inline`.
5. `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections`; prefer
   per-subsystem tables; gate optional ops behind Kconfig.
6. **Verify** with `arm-none-eabi-objdump -d` / `avr-objdump -d`: confirm
   no `blx`/`ijmp` survivor on the dispatch path. Devirtualisation is
   heuristic — check, don't assume.

---

## 7. What this means for NavHAL's M9 decision

* The current model is genuinely zero-overhead but pays in **duplicated
  validation and a symbol-only conformance gate** that both worsen with
  every vendor.
* M9's vtable removes that duplication and makes the contract mechanically
  enforceable. Its runtime cost is **a few cycles in debug builds and zero
  under `-flto`** — because NavHAL is single-target, the indirection the
  vtable introduces is an artefact of separate compilation, not genuine
  runtime polymorphism, and LTO collapses it back to the direct call every
  other HAL's primitive layer uses.
* The migration's real, non-obvious work is therefore **not** the vtable
  itself but the surrounding build discipline: an LTO release config, the
  embed-the-table decision, keeping hot paths inline, the AVR SRAM/PROGMEM
  call, and a cycle-counted regression test. Get those right and M9 is the
  rare abstraction that is both cleaner *and* free.

---

## References

NavHAL source: `include/common/hal_gpio.h`, `include/common/hal_features.h`,
`src/vendor/stm32/gpio/gpio.c`, `src/vendor/microchip/gpio/gpio.c`,
`tests/portable/conformance/test_conformance.c`, root `CMakeLists.txt`,
`cmake/arch/armv7e-m.cmake`, `cmake/arch/avr.cmake`. See also @ref roadmap_m9.

**Zephyr** — [Device Driver Model](https://docs.zephyrproject.org/latest/kernel/drivers/index.html),
[System Calls](https://docs.zephyrproject.org/latest/kernel/usermode/syscalls.html),
[device.h](https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/device.h),
[gpio.h](https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/drivers/gpio.h),
[Iterable Sections](https://docs.zephyrproject.org/latest/kernel/iterable_sections/),
[issue #36035 (devices in ROM)](https://github.com/zephyrproject-rtos/zephyr/issues/36035).

**Arduino** — [wiring_digital.c](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/wiring_digital.c),
[Arduino.h macros](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Arduino.h),
[pins_arduino.h](https://github.com/arduino/ArduinoCore-avr/blob/master/variants/standard/pins_arduino.h),
[Print.h](https://github.com/arduino/ArduinoCore-API/blob/master/api/Print.h),
[digitalWrite perf (Timo Denk)](https://blog.timodenk.com/port-manipulation-and-arduino-digitalwrite-performance/).

**STM32Cube / CMSIS / LL** — [UM1725 (HAL & LL)](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf),
[ARM CMSIS-Core peripheral access](https://arm-software.github.io/CMSIS_5/Core/html/group__peripheral__gr.html),
[ST wiki: GPIO](https://wiki.st.com/stm32mcu/wiki/Getting_started_with_GPIO),
[HAL vs LL (ST Community)](https://community.st.com/t5/stm32-mcus-products/hal-vs-ll/td-p/432434).

**ChibiOS** — [HAL design (PlayEmbedded)](https://www.playembedded.org/blog/chibioshal-design-an-object-oriented-approach/),
[hal_streams.h](https://github.com/ChibiOS/ChibiOS/blob/master/os/hal/include/hal_streams.h),
[hal_pwm_lld.h](https://github.com/ChibiOS/ChibiOS/blob/master/os/hal/ports/STM32/LLD/TIMv1/hal_pwm_lld.h).

**Mbed OS** — [GPIO porting](https://os.mbed.com/docs/mbed-os/v6.16/porting/gpio.html),
[DigitalOut.h](https://github.com/ARMmbed/mbed-os/blob/master/drivers/include/drivers/DigitalOut.h),
[FileHandle](https://os.mbed.com/docs/mbed-os/v6.16/mbed-os-api-doxy/classmbed_1_1_file_handle.html),
[adding & configuring targets](https://os.mbed.com/docs/mbed-os/v6.16/program-setup/adding-and-configuring-targets.html).

**Compiler** — [GCC Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html),
[GCC Internals: LTO Overview](https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html),
[GCC Internals: Regular IPA passes](https://gcc.gnu.org/onlinedocs/gccint/Regular-IPA-passes.html),
[Hubička, Devirtualization in C++ pt.1](http://hubicka.blogspot.com/2014/01/devirtualization-in-c-part-1.html) /
[pt.2](http://hubicka.blogspot.com/2014/01/devirtualization-in-c-part-2-low-level.html),
[LWN: link-time GC](https://lwn.net/Articles/741494/),
[GCC 10 porting (-fno-common)](https://gcc.gnu.org/gcc-10/porting_to.html).
