# M5 — Per-driver conformance audit (WI5.1)

Audit of all 14 drivers against the **Section 14 conformance checklist** in
`docs/api_standardization.md`. The checklist's 11 items:

1. Public header is declarations-only, no `#ifdef` target conditionals.
2. Public symbols use the `hal_<p>_` prefix and `snake_case` verbs.
3. `hal_<p>_init` / `hal_<p>_deinit` exist; init takes `const hal_<p>_config_t*`.
4. Every fallible function returns `hal_status_t`; data out via pointer.
5. Instances addressed by a typed id enum (no bare `uint8_t`).
6. Buffers passed as `(ptr, len)`; timeouts in ms.
7. IT/DMA variants use `_it` / `_dma` suffixes, gated by `NAVHAL_HAS_*`.
8. No `IRQHandler` / `*_Handler` / `*_reg.h` symbols in the public header.
9. Events delivered via a registered `hal_<p>_callback_t`.
10. Doxygen `@file` / `@ingroup` correct; one example in the header.
11. Unit test in `tests/` updated to the new signatures.

Status legend: **P** pass · **D** accepted v1 deviation · **F** fixed in the
WI5.1 PR · **n/a** not applicable to this driver.

## Per-driver result

| Driver    | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|-----------|---|---|---|---|---|---|---|---|---|----|----|
| gpio      | D | P | D | P | P | n/a | n/a | P | n/a | D | P |
| uart      | D | P | D | P | P | D | D | P | n/a | D | P |
| clock     | D | P | D | P | n/a | n/a | n/a | P | n/a | D | P |
| timer     | D | P | D | P | P | n/a | n/a | P | P | D | P |
| pwm       | D | P | D | P | P | n/a | n/a | P | n/a | D | P |
| i2c       | D | P | D | P | P | D | D | P | n/a | D | P |
| spi       | F | P | D | P | P | P | n/a | P | n/a | D | P |
| dma       | F | P | D | P | n/a | n/a | P | P | n/a | D | P |
| crc       | D | P | D | P | n/a | P | n/a | P | n/a | D | P |
| flash     | D | P | D | P | n/a | P | n/a | P | n/a | D | P |
| dwt       | D | D | D | P | n/a | n/a | n/a | P | n/a | D | P |
| fpu       | D | P | D | P | n/a | n/a | n/a | P | n/a | D | P |
| sdio      | D | P | D | D | n/a | P | D | P | P | D | P |
| interrupt | D | P | n/a | P | P | n/a | n/a | D | P | D | P |

## Fixed in the WI5.1 PR

- **spi, dma — item 1.** Neither had an `include/common/hal_<p>.h` public
  header; both were reachable only through the port header and were absent
  from `navhal.h`. Added `common/hal_spi.h` and `common/hal_dma.h` as
  declarations-only prototype headers (mirroring the M4 pattern of the other
  12 drivers), with the port header reduced to its port-specific bits. Both
  are now included by `navhal.h`. All 14 drivers are now structurally uniform.

## Accepted v1 deviations

Each deviation below is a conscious choice for v1, not an oversight. D2 and
D3 were the two costly-to-revisit-post-freeze items; both were **confirmed
accepted for v1** before `HAL_API_VERSION` was set to 1 in WI5.4.

- **D1 — header location.** Public headers live in `include/common/`, not
  `include/navhal/`. Deliberate (M4); the checklist text predates the decision.
- **D2 — no `hal_<p>_deinit`. (confirmed accepted for v1)** No driver exposes
  a teardown entry point. NavHAL targets configure-once bare-metal use:
  peripherals are set up at boot and run for the device's lifetime; no driver,
  sample, or test needs teardown. Adding 14 teardown functions would freeze
  unused surface into v1. Can be added later as an additive (non-breaking)
  change if a low-power / re-init use case appears.
- **D3 — no UART/I2C timeouts. (confirmed accepted for v1)** `hal_uart_*` and
  `hal_i2c_*` blocking transfers take no timeout argument (SPI does). They are
  polling-mode blocking calls; adding a timeout is a signature change deferred
  past v1.
- **D4 — DMA gating macro.** DMA-capability `#ifdef`s use `_DMA_ENABLED` (a
  build `-D` flag), not `NAVHAL_HAS_DMA`. `navhal_target.h` (WI4.3) now emits
  `NAVHAL_HAS_DMA`; migrating the gates is a mechanical, behaviour-neutral
  follow-up.
- **D5 — SDIO status type.** `hal_sdio_*` returns `hal_sdio_error_t`, not
  `hal_status_t` — the async model needs `HAL_SDIO_PENDING`, which the standard
  enum cannot express. Already documented; flagged at the M4 gate.
- **D6 — PWM init signature.** `hal_pwm_init(handle, frequency, duty_cycle)`
  takes two scalars instead of a `const hal_pwm_config_t*`. Two scalar
  parameters don't warrant a struct.
- **D7 — cycle-counter prefix.** The DWT-backed driver uses the
  `hal_cycle_counter_*` prefix, not `hal_dwt_*`, to keep the API arch-neutral
  (DWT is a Cortex implementation detail).
- **D8 — drivers without an init.** `flash` (self-initializing key/value
  store), `fpu` (one-shot `hal_fpu_enable`) and `interrupt` (NVIC needs no
  setup) have no `hal_<p>_init`. Correct for those peripherals.
- **D9 — Doxygen groups.** `@ingroup` tags are absent and in-header `@code`
  examples exist only on uart/crc/dma. Deferred to WI5.5, which defines the
  Doxygen group structure holistically.
- **D10 — SDIO async suffix.** SDIO's DMA-backed calls use the `_async`
  suffix, not `_dma`; `_async` describes the caller-visible contract better.

## Conclusion

All 14 drivers expose the standardized `hal_<p>_` API, return `hal_status_t`
from fallible functions (SDIO excepted, D5), carry no `#ifdef` target
conditionals in their public headers, and have unit tests. The structural gap
(spi/dma missing a `common/` header) is fixed. The remaining deviations are
documented and accepted for v1; D2 and D3 were confirmed before
`HAL_API_VERSION` was frozen at 1 in WI5.4.
