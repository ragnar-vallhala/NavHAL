# Test Model — SIL / PIL / HIL for NavHAL

The classical verification ladder for embedded / control systems has
four rungs. NavHAL collapses the top rung (MIL) into SIL because the
algorithms *are* the implementation — there's no separate executable
model to validate against. The remaining three rungs are all active.

```
      MIL  (Model-in-the-Loop)        →  not applicable (no separate model)
       │
       ▼
      SIL  (Software-in-the-Loop)     →  tests/host/ on the build host gcc
       │
       ▼
      PIL  (Processor-in-the-Loop)    →  tests/ ELF in Renode (STM32F4 model)
       │
       ▼
      HIL  (Hardware-in-the-Loop)     →  tests/ ELF on a Nucleo-F401RE
```

Each rung uses the **same** navtest harness — same suite tables, same
assertion macros, same `Total failures:` exit contract — so a test
written once is portable across levels (subject to the level's
constraints below). The split is purely about the substrate the
firmware executes on.

---

## SIL — Software in the loop

| | |
|---|---|
| **Substrate** | The build host (`gcc` on Linux/macOS). |
| **What runs** | Driver code that is purely algorithmic, with no register access — e.g. the software CRC-32/MPEG-2 lookup-table path in `src/core/cortex-m4/crc/crc.c`, and the `utils/conversion.c` helpers. |
| **Entry point** | `tools/run_host_tests.sh` → `build-host/tests_host`. |
| **Speed** | < 30 s end-to-end, no toolchain download. |
| **Output** | navtest's text format to `stdout` (routed via `tests/host/host_backend.c` instead of UART2). |
| **What it catches** | Algorithmic regressions (CRC table corruption, conversion edge cases), API-contract invariants (`HAL_OK == 0`, `HAL_OK_OR_RETURN` short-circuit semantics), pin-encoding math (`GPIO_GET_PIN` / `GPIO_GET_PORT_NUMBER`). |
| **What it can't catch** | Anything that depends on memory-mapped peripherals, Cortex-M4 ISA instructions, real interrupt delivery, or wall-clock timing. |

The host build deliberately compiles a subset of `src/core/cortex-m4/` —
only the files that are register-free. It does **not** define
`_CRC_HW_ENABLED`, so the CRC driver builds in its software path; this
lets us run the very same `hal_crc_*` API on the host and get
algorithmically meaningful coverage. Same idea would apply to any other
driver that has a software fallback.

---

## PIL — Processor in the loop

| | |
|---|---|
| **Substrate** | The on-target ELF (cross-compiled with `arm-none-eabi-gcc`) running inside Renode against a modeled STM32F4 machine. |
| **What runs** | The full `tests/` suite — the same binary that flashes to a real board. |
| **Entry point** | `tools/renode/run_tests.sh build/tests` (locally), `Full suite in Renode` job in `.github/workflows/renode.yml` (CI — nightly + on demand). |
| **Speed** | Slow. The full 142-test suite takes ~90 min wall-clock. Renode advances emulated time slower than wall-clock when the firmware busy-waits, and our `hal_clock_init` paths busy-wait on PLL/HSE ready flags that the Renode RCC model is slow to assert. |
| **Output** | USART2 captured to a file by Renode's `CreateFileBackend`; the wrapper greps for `Total failures:` and exits with that count. |
| **What it catches** | Register-write logic (does `hal_gpio_set_mode` actually flip the right bits in MODER?), ISA-level math (32-bit cycle counter wraparound, byte order), build-time integration (does the linker script + startup code actually run the suite to completion?). |
| **What it can't catch** | Peripheral edge cases the model glosses over — e.g. real I²C arbitration timing, true SPI clock-data alignment, ADC noise, electrical issues, current draw, the exact NVIC pending-write behavior on real silicon. |

Renode is intentionally **not** wired as a per-PR gate — the
wall-clock cost is too high. Instead, it runs on a nightly schedule
and on-demand via `workflow_dispatch`. Per-PR CI (`ci.yml`) still
catches build breaks via `build-on-target` and the host subset via
`host-tests`; the nightly Renode run is the slower follow-up that
provides the "M3 refactor was behavior-preserving" signal M2+ planned
for, just on a less aggressive cadence.

The Renode `.resc` script (`tools/renode/navhal_f401re.resc`) currently
loads the `stm32f4_discovery` board model. The F401RE shares its silicon
family with the Discovery; the model exposes USART2, GPIO, the RCC and
timers used by the suite. Limitations of the model are documented inline
in the `.resc` file as they're discovered — and in
`docs/testing/findings.md` once they show up in test runs.

---

## HIL — Hardware in the loop

| | |
|---|---|
| **Substrate** | Real silicon — currently a Nucleo-F401RE attached over its built-in ST-Link/V2-1. |
| **What runs** | The same on-target ELF that PIL uses, this time flashed to the chip's internal flash and executing on the actual Cortex-M4. |
| **Entry point** | `tools/run_target_tests.sh [port] [baud] [timeout]` — configures `TEST=ON`, builds, opens the serial console, starts `tools/uart_capture.py` in the background, then runs `make flash_tests` which performs a software reset that kicks off the test run. |
| **Speed** | A few seconds to build + flash + run the suite to completion. |
| **Output** | USART2 from the chip → ST-Link VCP → host `/dev/ttyACM0` → captured to stdout. Exit code is the navtest failure count. |
| **What it catches** | Real-peripheral semantics — bit-positions in registers that the Renode model abstracts over (the GPIO OSPEEDR bug that PR10 fixed was found here), real NVIC pending-bit acceptance, ST-Link / RCC clock-tree quirks, actual interrupt latency. |
| **What it can't catch** | Anything that requires *additional* hardware not wired to the bare Nucleo: I²C devices, SPI peripherals, an SD card on the SDIO bus, a UART loopback for byte-level verification. This rig has none of those. |

This is the level where the test pyramid earns its name: HIL is the
narrow top — most expensive per run, but the only level that can
distinguish "the model says my code is right" from "the silicon agrees."

---

## What sits *above* this triangle

A few classes of test are still on the wishlist and don't fit any of
the three levels above:

- **Extended HIL** — wiring an I²C sensor + SPI peripheral + SD card
  + UART loopback to the Nucleo so we can do byte-level bus traffic
  verification. The current rig does not have these, so several driver
  paths (e.g. `hal_sdio_read_block` on a card with data) are skipped.
- **Soak / fault-injection** — pulling the reset line during a flash
  write, browning out VDD during an SDIO transfer, etc. Useful for the
  `hal_flash_*` reliability story but expensive to automate.

These are tracked as future work; this report scopes only the three
active levels.
