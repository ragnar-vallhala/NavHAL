# STM32F767ZI Port Plan (Nucleo-F767ZI)

> Status: **in progress** — build wiring + GPIO/CLOCK/TIMER/INTERRUPT bring-up
> landed; UART / FLASH / hardware-FPU / DMA / high-frequency clock are scoped
> follow-ups. See [Milestones](#milestones).

## Goal

Add a third implemented NavHAL port: **ARM Cortex-M7 / STM32F7 / Nucleo-F767ZI**,
alongside the existing Cortex-M4 / STM32F4 / Nucleo-F401RE and AVR / ATmega328P
ports. The port is purely additive — it drops fragment files into `src/arch`,
`src/vendor`, `src/board`, `include/port`, `cmake/` per the M7 modular-build
contract, with no edits to the root build graph beyond Kconfig `depends on`
widening.

## Target identity

| Axis | Value |
|---|---|
| Arch (`CMAKE_SYSTEM_PROCESSOR`) | `cortex-m7` |
| ISA layer (`ARCH_ISA`) | `armv7e-m` (shared with Cortex-M4 — same startup/NVIC/SysTick) |
| Vendor (`VENDOR`) | `stm32` |
| Family (`FAMILY`) | `stm32f7` |
| Board (`BOARD`) | `nucleo_f767zi` |
| MCU | STM32F767ZI — Cortex-M7, 2 MB flash, 512 KB SRAM (DTCM 128 KB + SRAM1 368 KB + SRAM2 16 KB) |
| Toolchain | `arm-none-eabi-` |
| Flasher | `st-flash`, flash base `0x08000000` |
| Defconfig | `cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig` |
| Toolchain file | `cmake/toolchains/arm-none-eabi-f767-toolchain.cmake` |

### Board specifics (Nucleo-144, UM1974)

| Function | Pin / Instance |
|---|---|
| LD1 green LED (`LED_BUILTIN`) | `PB0` |
| LD2 blue LED | `PB7` |
| LD3 red LED | `PB14` |
| User button B1 | `PC13` (active-high on this board) |
| Console UART (ST-LINK VCP) | `USART3` — TX `PD8`, RX `PD9`, AF7 |
| HSE | 8 MHz from ST-LINK MCO (solder-bridge default) |
| HSI | 16 MHz internal RC |

## Why the ISA layer is reused

Cortex-M7 and Cortex-M4 are both ARMv7E-M. The startup vector glue, NVIC
interrupt controller, SysTick timebase, DWT cycle counter and FPU enable
sequence are core-architectural and identical at the register level, so the
existing `src/arch/armv7e-m/` and `include/port/cortex-m7/` (copied from
`cortex-m4`) layers serve M7 unchanged. The two cores diverge only in:

- **FPU width** — M4 is single-precision (`fpv4-sp-d16`), M7 is
  double-precision (`fpv5-d16`). Handled in `cmake/arch/armv7e-m.cmake` by
  branching the `-mfpu=` flag on `CMAKE_SYSTEM_PROCESSOR`.
- **L1 cache** — M7 adds optional I-cache/D-cache (not present on M4). Left
  disabled for v1 bring-up; enabling it later requires cache-maintenance around
  DMA buffers (see follow-ups).

Everything else that differs between the targets is **family** (STM32F7 vs
STM32F4 register maps) or **board** (pinout), which the layered tree already
isolates.

## F4 → F7 deltas that matter

The shared vendor drivers under `src/vendor/stm32/*.c` reference family register
structs/macros by name; the family register headers
(`src/vendor/stm32/family/<family>/include/family/*_reg.h`, selected via the
`INCLUDE_FAMILY` path) are the abstraction boundary. Where the register
*layout/semantics* match, the driver is reused verbatim with an F7 header; where
they diverge, the driver needs a family-conditional path.

| Peripheral | F4 vs F7 | Reuse strategy |
|---|---|---|
| **GPIO** | Identical IP; same base `0x40020000`. F7 exposes contiguous ports A–G (+H), F401 jumps PE→PH. | Reuse `gpio.c`. F7 `gpio_reg.h` uses contiguous `n>>4` port indexing and lists all bases. ✅ done |
| **CLOCK / RCC** | Same `CR`/`PLLCFGR`/`CFGR` layout and base `0x40023800`. F7 adds over-drive (`PWR_CR1` ODEN/ODSWEN) + VOS scaling for >180 MHz, and PLLSAI/DCKCFGR. | Reuse `clock.c` for HSI/HSE/PLL up to ~180 MHz. Over-drive to reach 216 MHz is a follow-up. ✅ basic |
| **FLASH** | Base `0x40023C00`, same `ACR`/`KEYR`/`CR`/`SR`. Wait-state count and **sector map differ** (F767: 32 KB×4, 128 KB×1, 256 KB×3 = 2 MB single bank). ART accelerator + prefetch. | F7 `flash_reg.h` carries the F767 sector map. `flash.c` reuse pending sector-map verification. ⏳ follow-up |
| **USART** | **Major divergence.** F4 uses `SR`/`DR`; F7 uses the modern IP: `ISR` (RO) / `ICR` / `RDR` / `TDR`, plus `BRR` oversampling differences. `uart.c` writes `usart->SR`/`->DR` directly. | Implemented as a separate `src/vendor/stm32/uart/uart_f7.c`, selected by the vendor CMakeLists when `CONFIG_FAMILY_STM32F7` (frozen F4 `uart.c` untouched). Polling TX/RX verified on USART3. DMA backend still pending (F7-5). ✅ done (polling) |
| **TIMER** | General-purpose timers (TIM2–5, TIM1/9/10/11) identical layout and bases. | Reuse `timer.c` with F7 `timer_reg.h` (copy of F4). ✅ done |
| **INTERRUPT / NVIC** | Core peripheral, identical. F7 has more IRQ lines; vector table grows. | Reuse arch `interrupt.c` + `startup.s`. ✅ |
| **DMA** | F7 DMA controller is stream/channel-compatible with F4 but cache coherency matters on M7. | Reuse pending; gated off by default. ⏳ follow-up |
| **CRC / SDIO / SPI / I2C / PWM** | Mostly compatible register maps; I2C IP differs (F7 uses the timing-register I2C like F0/L4). | Bring up after UART. ⏳ follow-up |

## What this lands now (basic bring-up)

A flashable **`01_hal_blink`** on the Nucleo-F767ZI driving LD1 (PB0), running on
the reset-default HSI (16 MHz, 0 flash wait-states), using:

- `DRV_GPIO` → `src/vendor/stm32/gpio/gpio.c` + F7 `gpio_reg.h`
- `DRV_TIMER` → `src/vendor/stm32/timer/timer.c` + SysTick timebase
  (`src/arch/armv7e-m/timebase/timebase.c`)
- `DRV_CLOCK` → `src/vendor/stm32/clock/clock.c` + F7 `rcc_reg.h` / `flash_reg.h`
- `DRV_INTERRUPT` → `src/arch/armv7e-m/interrupt/interrupt.c` + `startup.s`

### Files added / touched

```
docs/stm32f767zi_port_plan.md                         (this file)
docs/capabilities/stm32f767zi.md                      (capability matrix)

src/arch/armv7e-m/Kconfig            (+ARCH_CORTEX_M7 identity defaults)
src/arch/armv7e-m/Kconfig.choice     (+ARCH_CORTEX_M7)
src/arch/armv7e-m/Kconfig.features   (FPU/DMA/DWT/FPU-drv depends widened to M7)
cmake/arch/armv7e-m.cmake            (M7 -mfpu=fpv5-d16 branch)

src/vendor/stm32/Kconfig.choice                  (VENDOR_STM32 visible on M7)
src/vendor/stm32/family/stm32f4/Kconfig.choice   (gate to ARCH_CORTEX_M4)
src/vendor/stm32/family/stm32f7/Kconfig          (FAMILY="stm32f7")
src/vendor/stm32/family/stm32f7/Kconfig.choice   (FAMILY_STM32F7, M7-gated)
src/vendor/stm32/family/stm32f7/include/family/*_reg.h  (F7 register maps;
                                            uart_reg.h is the real F7 USART model)
src/vendor/stm32/uart/uart_f7.c            (F7 USART driver — ISR/RDR/TDR)
src/vendor/stm32/CMakeLists.txt            (select uart_f7.c for FAMILY_STM32F7)

src/board/nucleo_f767zi/Kconfig            (BOARD="nucleo_f767zi")
src/board/nucleo_f767zi/Kconfig.choice     (BOARD_NUCLEO_F767ZI)
src/board/nucleo_f767zi/board.h            (pin aliases / oscillators)
src/board/nucleo_f767zi/linker.ld          (2 MB flash / 512 KB RAM)

include/port/cortex-m7/**                  (copied from cortex-m4; gpio_types
                                            contiguous ports, uart_types +HAL_UART_3)

cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig
cmake/toolchains/arm-none-eabi-f767-toolchain.cmake

Kconfig                              (HAL Drivers menu depends widened to M7)
```

## How to build / flash

```bash
cmake -G Ninja -B build-f767 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake \
  -DSAMPLE=01_hal_blink
cmake --build build-f767
cmake --build build-f767 --target flash      # st-flash write to 0x08000000
```

(The no-toolchain-file path also works because the arch Kconfig sets
`TOOLCHAIN_PREFIX=arm-none-eabi-`; the toolchain file is what seeds the
`cortex-m7` defconfig into a fresh `.config`.)

## Testing

### Current state (this PR)

- **Host suite passes** — `tools/run_host_tests.sh` reports **24/24** after the
  port changes. It is arch-agnostic (system gcc, pure-logic HAL tests:
  `hal_status`, conversion, software CRC, GPIO pin encoding), so it confirms the
  port edits did not regress the shared HAL / `common` code. The port adds no
  host-buildable code, so there is nothing new for this tier to cover yet.
- **No on-target or PIL tests for F767 yet.**

### On-target test prerequisite (UART) — now satisfied

The on-target test ELF reports its pass/fail results **exclusively over
`NAVTEST_UART`** (`tests/navtest_state.c` → `hal_uart_print`; `tests/main.c`
screen-clear + result print). This made on-target testing gated on **F7-2
(UART)**, which is now **done** (`uart_f7.c`, polling TX/RX verified on USART3).
The remaining work to actually run the suite on F767 is the test-build
generalization below (items 2–4) plus the PIL/CI wiring (item 6).

### Changes required to enable on-target + PIL tests (milestone F7-7)

1. **UART (F7-2)** — ✅ done; provides the `NAVTEST_UART` output path.
2. **Processor-generic test linker** — `cmake/arch/armv7e-m.cmake` hardcodes
   `NAVHAL_TEST_LINKER_FLAGS` to `tests/arch/cortex-m4/linker.ld`. Parametrize it
   to `tests/arch/${CMAKE_SYSTEM_PROCESSOR}/linker.ld` and add
   `tests/arch/cortex-m7/linker.ld` (2 MB flash / 512 KB RAM, like the board
   script). Without this an M7 test ELF links against the F401's 512 KB/96 KB map.
3. **Arch-generic harness** — `tests/main.c` hardcodes
   `#include "arch/cortex-m4/test_uart_protocol.h"` and registers the M4
   white-box suites by name. Gate the arch-specific includes / suite
   registrations per `CMAKE_SYSTEM_PROCESSOR` (or route them through a
   `tests/navtest_target.h` indirection) so a cortex-m7 build pulls the M7 suites.
4. **White-box tier** — add `tests/arch/cortex-m7/` (start by adapting the M4
   register tests). Note: the GPIO test must assert **contiguous** port indexing
   for F7, not the F4 `PE→PH` skip — the existing host test
   `test_gpio_get_port_number_skips_to_h` encodes the M4 behaviour and is
   M4-specific.
5. **Portable + cap tiers** — `tests/portable/*` and `tests/cap/*` are HAL
   black-box and self-gate on `NAVHAL_HAS_*`, so they run on F767 unchanged once
   UART exists; no per-test edits needed.
6. **PIL / CI** —
   - `tools/pil/boards/nucleo_f767zi.conf` (mirror `nucleo_f401re.conf`),
   - a Renode platform / `.resc` for F767 (mirror `tools/renode/navhal_f401re.resc`),
   - extend the `.github/workflows/` sample-matrix and PIL jobs with the F767
     defconfig.

## Milestones

| # | Scope | Status |
|---|---|---|
| F7-1 | Build wiring + GPIO/CLOCK/TIMER/INTERRUPT + flashable blink on HSI | **done** |
| F7-2 | UART (USART3 console) — `uart_f7.c` for the F7 ISR/RDR/TDR IP; polling TX/RX verified on hardware | **done** |
| F7-3 | High-frequency clock — PWR over-drive + VOS + correct flash wait-states (up to 216 MHz) | next |
| F7-4 | FLASH driver — verify F767 sector map, adapt `flash.c` | follow |
| F7-5 | DMA + hardware FPU (`fpv5-d16`) + DWT + cache-coherent DMA buffers | follow |
| F7-6 | I2C (new timing-register IP), SPI, PWM, CRC, SDIO | follow |
| F7-7 | Test enablement + CI: processor-generic test linker/harness, `tests/arch/cortex-m7/`, run portable+cap tiers on-target, PIL board profile (`tools/pil/boards/nucleo_f767zi.conf`) + Renode F767 platform, sample-matrix defconfig, capability-doc parity. UART prerequisite (F7-2) is met; remaining work is the test-build generalization. See [Testing](#testing). | follow |

## Risks / notes

- **HSI-only bring-up** keeps flash wait-states at 0 and avoids the over-drive
  sequence, so blink is safe before F7-3. Do **not** call `hal_clock_init` with
  a PLL target above 180 MHz until over-drive lands.
- UART is **polling-only** on F7 today (`uart_f7.c`). The DMA backend
  (`hal_uart_write_dma` etc.) is not yet ported — it needs Cortex-M7 cache
  coherency handling (F7-5), so `DRV_UART_DMA` stays off.
- L1 D-cache stays **off** until DMA work (F7-5); enabling it earlier would
  require cache maintenance around any DMA/peripheral-shared buffer.
- `flash_reg.h` sector sizes/addresses are still the **F4 placeholder** map;
  `flash.c`'s storage-sector selection must be re-reviewed against the F767's
  256 KB sectors before `DRV_FLASH` is enabled (F7-4).
