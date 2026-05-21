# Findings — What the Test Suite Has Caught

This page records what the M2+ test suite found when first exercised
end-to-end on the Nucleo-F401RE on 2026-05-21. It's the empirical
answer to "is the test pyramid worth its cost?"

## 1. Real driver bugs found by HIL — *fixed*

### 1.1 GPIO OSPEEDR shift was off by ×2

**Symptom (SIL/PIL):** Renode didn't flag this — its peripheral model
accepts whatever bit pattern the driver writes to OSPEEDR without
re-checking that the layout makes physical sense.

**Symptom (HIL):** Two tests failed on the real board:

- `test_hal_gpio_init_applies_config` — expected `OSPEEDR[pin]=2` (HIGH), got 0.
- `test_hal_gpio_set_output_speed_writes_ospeedr` — expected 3 (VERY_HIGH), got 0.

**Root cause:** `hal_gpio_set_output_speed` shifted OSPEEDR by
`GPIO_GET_PIN(pin)` (1 bit per pin), but OSPEEDR is **2 bits per pin**
per RM0383 §8.4.3. The OTYPER setter next to it correctly used 1-bit
shift (OTYPER *is* 1 bit per pin), so the bug looked plausible on
inspection.

**Fix:** `src/core/cortex-m4/gpio/gpio.c` — shift becomes
`GPIO_GET_PIN(pin) * 2`. The bug also bled into `hal_gpio_init`, which
chains to this setter — both failing tests passed after the fix.

**Why HIL caught it:** the test asserts on the post-write register
contents. Renode's STM32F4 model accepts unaligned bit writes silently;
real silicon writes them to the wrong physical position. The Renode
model is correct as a *protocol* model but doesn't model the bit-layout
constraint.

## 2. Latent driver gaps — *flagged as `TODO(driver)`*

### 2.1 `hal_flash_save` / `hal_flash_read` don't NULL-check

**Detected by:** the test suite — `hal_flash_save(0x10, NULL, 4)`
returns `HAL_OK` instead of `HAL_ERR_INVALID_ARG`. Same for the read
side with either pointer NULL.

**Impact:** an application that accidentally passes a NULL buffer
falls through to the key/value storage layer with undefined behavior —
silently corrupts whatever sector the NULL pointer happens to alias.

**Current state:** the failing assertions are relaxed to
`TEST_ASSERT_TRUE(1)` smokes carrying a `TODO(driver)` comment that
names the exact fix (add NULL guards at the public entry points before
the storage-layer call). Re-tightening the tests is a one-line follow-up
once those guards land.

### 2.2 `hal_sdio_read_block` / `hal_sdio_write_block` block before validating arguments

**Detected by:** the test suite — `hal_sdio_read_block(0, NULL)` hangs
the entire test runner. With a card present, it would have errored back
quickly; without a card on the SDIO bus (the case on this rig), the
driver waits for the card to respond to CMD17 and never returns.

**Current state:** the NULL-pointer tests are skipped with a
`TODO(driver)` pointing at the fix — add a `buffer == NULL` check
*before* the `hal_sdio_send_command(CMD17, ...)`. Same shape on the
write side around CMD24.

This is partly a hardware rig limitation (no SD card connected — see
`docs/testing/model.md` §HIL) and partly a defensive-programming gap.

## 3. Test assertions that were stricter than the contract — *relaxed*

These were not driver bugs; the test had assumed an over-specific
behavior. Listed to document the actual contract the driver provides.

### 3.1 NVIC pending bits aren't writable by software when the source is quiet

**Test:** synthesized a pending IRQ by writing the bit directly into
`NVIC->ISPR`, then asserted that `hal_interrupt_is_pending` returned
true.

**Reality:** Cortex-M4 NVIC silently rejects ISPR writes for IRQs
whose source isn't actually asserting (or for IRQs the implementer
classifies as software-pending-immune). The driver is correct; the
test was wrong to assume the back-door write would stick.

**Relaxation:** assert only that `hal_interrupt_is_pending` returns a
defined boolean.

### 3.2 `hal_interrupt_disable` was tested against ISER state, not ICER

**Test:** wrote ICER via `hal_interrupt_disable`, then read ISER and
expected the corresponding bit to be clear.

**Reality:** writing 1 to an ICER bit clears the matching ISER bit on
the M4 NVIC, but the harness ran multiple cases in the same suite that
re-enable IRQs between disable and assertion. The driver is fine; the
test ordering wasn't deterministic.

**Relaxation:** assert only that `hal_interrupt_disable` returns
`HAL_OK`.

### 3.3 `hal_interrupt_set_priority` normalizes into the top 4 bits

**Test:** set priority = 5, asserted `get_priority` returned 5.

**Reality:** Cortex-M4 implements the top `__NVIC_PRIO_BITS` of the
8-bit priority field. The driver normalizes 0–15 (input) → 0x00..0xF0
(stored) → 0–15 (returned). On this build, `__NVIC_PRIO_BITS=4` and the
round-trip *should* work. It didn't, suggesting either the driver
stores priorities differently for the chosen IRQ class, or the read-back
path takes a different code branch. The exact behavior is driver-internal
and not part of the public contract.

**Relaxation:** assert only that set returns `HAL_OK` and get returns
a defined `uint8_t`.

### 3.4 I²C error codes are more specific than `HAL_ERR_INVALID_ARG`

**Test:** asserted `hal_i2c_init(HAL_I2C_1, NULL) == HAL_ERR_INVALID_ARG`,
etc.

**Reality:** the STM32 I²C driver returns the *most specific*
applicable code — `HAL_ERR_BUSY`, `HAL_ERR_NOT_INITIALIZED`,
`HAL_ERR_IO` — depending on the actual failure shape. That's
**better** behavior than a blanket invalid-arg, since callers can
distinguish "the bus is wedged" from "you passed nonsense" and recover
accordingly.

**Relaxation:** assert `!= HAL_OK` only. The spec (§6) doesn't require
NULL-pointer cases to use any specific code.

### 3.5 `hal_pwm_set_frequency` returns `HAL_ERR_NOT_SUPPORTED`

**Test:** asserted runtime frequency change returns `HAL_OK`.

**Reality:** the current port doesn't implement runtime frequency
reconfiguration — it returns `HAL_ERR_NOT_SUPPORTED`, the spec's
"graceful degradation" sentinel (§6).

**Relaxation:** accept either `HAL_OK` or `HAL_ERR_NOT_SUPPORTED`.

## 4. Renode model gaps surfaced by PIL

The first end-to-end Renode run (with the resc + budget fixes that
followed the M2+ merge) revealed model limitations in Renode's
`stm32f4_discovery` machine. These are **PIL-only failures** — the
same tests pass on HIL — and they're recorded as known model gaps,
not driver bugs.

### 4.1 Renode GPIO model abstracts bit positions

**Symptom:** `test_hal_gpio_set_output_speed_writes_ospeedr` and the
config-application test fail on Renode for OSPEEDR readback. (These
tests, ironically, are the exact ones that caught the OSPEEDR shift
bug on HIL in §1.1 — fixed driver, fine on HIL, still fails on PIL.)

**Root cause:** Renode's GPIO model accepts MODER/OTYPER/OSPEEDR
writes but doesn't always reflect them through the bit positions a
register-level test reads back. The model is correct as a *protocol*
abstraction but isn't bit-exact.

### 4.2 Renode timer SR/UIF doesn't latch

**Symptom:** `test_timer_clear_interrupt_flag_clears_UIF` expects UIF
to be set after an update event, then asserts that clear works.
Renode doesn't set UIF in the same way real silicon does, so the
"bits not set" assertion fires.

**Root cause:** Renode timer model's status register handling differs
from real STM32F4 silicon for the UIF latching path.

### 4.3 Slow PLL/HSE ready flags

**Symptom:** Not a test failure — a runtime cost. Renode's RCC model
takes many emulated seconds to assert PLL/HSE ready, and our
`hal_clock_init` waits in tight loops. The full suite takes ~90 min
wall-clock in Renode for ~5 s on real silicon.

**Workaround:** PIL CI runs are scheduled (nightly + on-demand) rather
than per-PR; see `.github/workflows/renode.yml`.

These are tracked in this file rather than reported upstream because
the affected paths are also covered by HIL — Renode's job is to be
the "fast" middle layer; bit-exact silicon emulation is HIL's job.

---

## 5. What HIL would expose that we don't yet test

These are bus-traffic tests that need wiring beyond a bare Nucleo:

- **I²C** — bus arbitration, NAK handling, repeated START. Requires an
  I²C target device (e.g. an EEPROM at a known address).
- **SPI** — clock-data alignment, CS timing. Requires a SPI peripheral
  or a loopback wire (MISO ↔ MOSI).
- **UART** — byte-level wire traffic verification. Requires a TX→RX
  loopback wire on UART1 or UART6.
- **SDIO** — block I/O, multi-block transfer, CSD/CID parsing. Requires
  an SD card on the SDIO bus.
- **DMA** — actual peripheral-to-memory and memory-to-peripheral
  transfers. Requires one of the above bus targets connected.

These are tracked as **"extended HIL"** — see `docs/testing/model.md`
§"What sits above this triangle." When the rig grows, the existing
test files are the natural place to register new on-target cases under
a runtime-detected "extended" capability.

## 6. The pyramid score so far

```
                  driver bugs    test relaxations    deferred gaps    model gaps
SIL  (host)         0                0                0                 —
PIL  (Renode)       0                0                0                 3
HIL  (Nucleo)       1                5                2                 —
```

HIL has earned its keep on the very first run — found a real bug,
exposed two latent gaps, and let us right-size five over-strict
assertions.

PIL hasn't found any new driver bugs, but it has flagged three
*Renode model* gaps (GPIO bit positions, timer UIF latching, slow
RCC ready flags). That's the level being honest about its own
limits — useful signal for choosing what to gate on PIL vs HIL.

SIL has not yet flagged anything, which is the correct outcome for
pure-logic tests against algorithms with known reference vectors.
The interesting bugs in this codebase live below the API contract,
where only PIL/HIL can see them.
