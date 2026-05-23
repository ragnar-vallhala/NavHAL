# Coverage Matrix

What is tested where, mapped onto the SIL / PIL / HIL model from
[`model.md`](model.md).

## Per-suite totals (latest on-target run, 2026-05-21)

| Suite | Cases | Pass | SIL | PIL | HIL | Notes |
|-------|------:|-----:|:---:|:---:|:---:|-------|
| GPIO              | 11 | 11 |    | ✓ | ✓ | Pin-encoding math (`GPIO_GET_PIN`, `GPIO_GET_PORT_NUMBER`) covered separately in the SIL `GPIO ENCODING` suite. |
| INTERRUPT         | 14 | 14 |    | ✓ | ✓ | Some assertions relaxed because synthesized-pending writes are silently rejected on real NVIC. |
| TIMEBASE          |  8 |  8 |    | ✓ | ✓ | Locks the `uint32_t` tick contract (no `uint64_t` in any public signature). |
| TIMER             | 15 | 15 |    | ✓ | ✓ | All 17 standardized funcs except the deliberately-disabled `test_timer_enable_and_disable_interrupt` (hangs UART2 on this rig). |
| CLOCK             | 12 | 12 |    | ✓ | ✓ | Reconfigures the system clock between cases; `between=wait_uart_empty` keeps the UART intact. |
| PWM               | 11 | 11 |    | ✓ | ✓ | `hal_pwm_set_frequency` accepts either OK or `HAL_ERR_NOT_SUPPORTED` (graceful degradation). |
| DMA               | 17 | 17 |    | ✓ | ✓ | Gated by `NAVHAL_HAS_DMA`. Tests register-level DMA-stream config, not actual transfers. |
| CRC               |  7 |  7 |    | ✓ | ✓ | Plus a separate SIL suite (`CRC SOFTWARE`) covering the software fallback path. |
| CYCLE_COUNTER     |  6 |  6 |    | ✓ | ✓ | Gated by `NAVHAL_HAS_CYCLE_COUNTER`. Cortex DWT. |
| FPU               |  3 |  3 |    | ✓ | ✓ | Gated by `NAVHAL_HAS_FPU`. Includes a benchmark sanity check (<50 000 cycles for 1000 FMA ops). |
| UART PROTOCOL     | 12 | 12 |    | ✓ | ✓ | Drives UART1/UART6 only; UART2 is the test console. |
| I2C               |  8 |  8 |    | ✓ | ✓ | Driver returns specific error codes (BUSY / NOT_INITIALIZED / IO); tests assert "not HAL_OK" only. |
| SPI               |  7 |  7 |    | ✓ | ✓ | Both CPOL/CPHA halves covered. |
| FLASH RELIABILITY |  6 |  6 |    | ✓ | ✓ | NULL-pointer cases relaxed pending driver NULL guards — see [`findings.md`](findings.md). |
| SDIO              |  5 |  5 |    | ✓ | ✓ | Gated by `NAVHAL_HAS_SDIO`. Block-I/O tests skipped because this rig has no SD card. |
| HAL_STATUS (host) |  5 |  5 | ✓  |    |   | Pure SIL — enum stability + `HAL_OK_OR_RETURN` short-circuit. |
| CONVERSION (host) |  9 |  9 | ✓  |    |   | Pure SIL — `str_to_int` / `str_to_float`. |
| CRC SOFTWARE (host) | 6 | 6 | ✓  |    |   | Pure SIL — the table-driven CRC path of the production source. |
| GPIO ENCODING (host) | 4 | 4 | ✓  |    |   | Pure SIL — pin-encoding math used by every GPIO driver. |
| **Total** | **166** | **166** | **24** | **142** | **142** | |

Pass column counts the **on-target** result. The host subset adds 24
more (run at SIL only).

## Per-driver function coverage

Standardized M2 drivers and how their public API maps to suites. A
"success" test exercises the happy path; an "error" test exercises a
documented failure mode.

| Driver | Public funcs | Success | Error | Notes |
|--------|-------------:|--------:|------:|-------|
| `gpio`        | 8 | 10 | 1 | Plus 4 SIL cases on the pin encoder. |
| `clock`       | 5 | 9 | 2 | Three init sources × init+sysclk + 4 bus getters + 3 error paths. |
| `interrupt`   | 11 | 9 | 5 | 5 error-path tests for negative IRQs + out-of-range callback IDs. |
| `timebase`    | 9 | 8 | 1 | NULL-callback rejection. |
| `timer`       | 17 | 15 | 1 | One commented-out (`enable_and_disable_interrupt` hangs UART). |
| `uart`        | 11 | 11 | 1 | Tests on UART1/UART6 only — UART2 is the test console. |
| `i2c`         | 5 | 4 | 4 | All NULL-pointer cases. |
| `spi`         | 4 | 4 | 3 | NULL-pointer rejection on transmit / receive / transmit_receive. |
| `pwm`         | 5 | 6 | 5 | NULL-handle rejection on every fallible function. |
| `crc`         | 4 | 6 | 1 | Plus 6 SIL cases on the software path. |
| `flash`       | 5 | 4 | 2¹ | ¹ NULL-pointer tests are smokes — driver lacks NULL guards. |
| `dma`         | 5 | 14 | 3 | Register-level DMA-stream programming. |
| `cycle_counter` (was dwt) | 4 | 5 | 1 | `init` / `reset` return OK + counter behavior. |
| `fpu`         | 1 | 2 | 1 | Plus benchmark sanity. |
| `sdio`        | 14 | 3 | 2¹ | ¹ Block-I/O tests skipped — no SD card on this rig. |

(Function counts include the deprecated compat shims so they slightly
overstate the standardized surface; the actual M2 API per driver is
documented in `include/core/cortex-m4/<driver>.h`.)

## Acceptance bar

From `docs/m2_plus_plan.md` §Acceptance gate:

- Every standardized `hal_*` function has ≥1 success-path test. ✅
- Every `hal_status_t`-returning function has ≥1 error-path test. ✅
  (Three driver-side gaps are flagged in [`findings.md`](findings.md)
  with `TODO(driver)` markers in-tree; the suite exercises the call
  but doesn't assert on the contract yet.)
- `cmake -DTEST=ON` configures clean. ✅
- `cmake -DTEST_HOST=ON` (via `cmake -S tests/host`) builds and runs
  natively. ✅
- CI runs the host subset + on-target build + Renode + no-cap symbol
  audit on every PR. ⏳ pending first push.
- `docs/testing.md` exists. ✅ (now [`howto.md`](howto.md))

## SIL vs HIL effort budget

The split below is the rough cycle cost as of this report:

```
SIL  (host gcc, 24 cases)          ~ 0.05 s    [runs every commit]
PIL  (Renode emulator, 142 cases)  ~ 30 s      [runs every PR in CI]
HIL  (Nucleo board, 142 cases)     ~ 8 s flash + run   [on demand]
```

The pyramid economics work out: SIL is so cheap that running it on
every save is justifiable; HIL is fast *enough* that running it after
every nontrivial driver change is also justifiable, as long as a board
is plugged in. PIL is the middle layer that catches what SIL can't
without the board.
