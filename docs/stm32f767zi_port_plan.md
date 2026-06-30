# STM32F767ZI Port Plan

> Status: **in progress** — build wiring + GPIO/CLOCK/TIMER/INTERRUPT bring-up
> landed; UART / FLASH / hardware-FPU / DMA / high-frequency clock are scoped
> follow-ups. See [Milestones](#milestones).

## Goal

Add a third implemented NavHAL port: **ARM Cortex-M7 / STM32F7 / STM32F767ZI**
(reference board: ST Nucleo-F767ZI),
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
| **CLOCK / RCC** | Same `CR`/`PLLCFGR`/`CFGR` layout and base `0x40023800`. F7 adds over-drive (`PWR_CR1` ODEN/ODSWEN) + VOS scaling for >180 MHz, frequency-scaled flash wait states, and APB bus limits (APB1 ≤54, APB2 ≤108 MHz). | Implemented in `src/vendor/stm32/clock/clock_f7.c` (family-selected): VOS Scale 1, over-drive >180 MHz, WS by HCLK, ART+prefetch, bus-limit prescalers. Verified at **216 MHz** on hardware. ✅ done |
| **FLASH** | Base `0x40023C00`, same `ACR`/`KEYR`/`CR`/`SR`; 5-bit `SNB`. **Sector map differs** (F767: 32 KB×4, 128 KB×1, 256 KB×7 = 2 MB single bank, 12 sectors). | F7 `flash_reg.h` carries the real F767 sector map (KV store on sectors 6/7); shared `flash.c` reused. Bring-up surfaced two real-hardware bugs in `flash.c` (M7 write-buffer needs a `DSB`; missing NULL guard faulted on M7) — both fixed. `test_flash_raw` (6) passes. ✅ done |
| **USART** | **Major divergence.** F4 uses `SR`/`DR`; F7 uses the modern IP: `ISR` (RO) / `ICR` / `RDR` / `TDR`, plus `BRR` oversampling differences. `uart.c` writes `usart->SR`/`->DR` directly. | Implemented as a separate `src/vendor/stm32/uart/uart_f7.c`, selected by the vendor CMakeLists when `CONFIG_FAMILY_STM32F7` (frozen F4 `uart.c` untouched). Polling TX/RX verified on USART3. DMA backend still pending (F7-5). ✅ done (polling) |
| **TIMER** | General-purpose timers (TIM2–5, TIM1/9/10/11) identical layout and bases. | Reuse `timer.c` with F7 `timer_reg.h` (copy of F4). ✅ done |
| **INTERRUPT / NVIC** | NVIC programmer's model is identical, but the **peripheral vector table is MCU-specific**: the F767's USART3 sits at IRQ 39, which is a literal `0` ("Reserved") in the F401-based arch `startup.s`. Enabling an interrupt-driven peripheral the F401 lacks would vector to address 0 and fault. | Reuse arch `interrupt.c`; the F767 ships its **own** `src/board/nucleo_f767zi/startup.s` with the full STM32F767xx vector table (every usable IRQ → a dispatch-backed handler, no `0` traps). The build prefers a board `startup.s` when present. ✅ done |
| **DMA** | F7 DMA controller is stream/channel-compatible with F4; cache coherency matters on M7 only when the D-cache is on (kept off here). | Reuses `dma.c` with the F7 `dma_reg.h`. Opt-in via `CONFIG_DRV_DMA`; `test_dma` (17) passes on hardware with the D-cache off. ✅ done |
| **PWM** | Built on the shared timer IP. | Reuses `pwm.c`. `test_pwm` (11) passes on hardware. ✅ done |
| **CRC** | Hardware CRC-32; default polynomial register-compatible with F4. | Reuses `crc.c`. CRC suite (7) passes via the HW unit. ✅ done |
| **SPI** | F7 moved frame size to `CR2.DS` (+`FRXTH`, byte-`DR` FIFO); F4's `CR1.DFF` is gone. | Separate `spi_f7.c`. `test_spi` (7) passes — init register-verified; FIFO transfer untested (no device). ◐ |
| **I2C** | **Different IP generation** — F7 `TIMINGR`/`ISR`-`ICR`/CR2-framed/`RXDR`-`TXDR` vs F4 legacy. | Separate `i2c_f7.c` + real F7 `i2c_reg.h`. `test_i2c` (8) passes — init register-verified; transfers untested (no device). ◐ |
| **SDIO** | F767 SDMMC1 is **register-identical** to the F4 SDIO (same base `0x40012C00`, APB2ENR bit 11, AF12 pinmux, vector slot 49). The "SDMMC rename" is cosmetic for the registers the driver touches. | Shared `sdio.c` runs unchanged once `DRV_SDIO` is un-gated for M7. `test_sdio` (6) passes; a polled block write/read round-trip is validated in PIL vs a Renode `STM32FSDMMC` + card. DMA async backend stays M4-only. ✅ done (polled) |

## What this lands now (basic bring-up)

A flashable **`01_hal_blink`** on the STM32F767ZI driving the user LED (PB0, LD1
on the Nucleo), running on
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
src/vendor/stm32/clock/clock_f7.c          (F7 clock — over-drive/WS/APB limits)
src/vendor/stm32/CMakeLists.txt            (select *_f7.c for FAMILY_STM32F7)

src/board/nucleo_f767zi/Kconfig            (BOARD="nucleo_f767zi")
src/board/nucleo_f767zi/Kconfig.choice     (BOARD_NUCLEO_F767ZI)
src/board/nucleo_f767zi/board.h            (pin aliases / oscillators)
src/board/nucleo_f767zi/linker.ld          (2 MB flash / 512 KB RAM)
src/board/nucleo_f767zi/startup.s          (STM32F767 vector table + reset)
CMakeLists.txt + src/vendor/stm32/CMakeLists.txt  (prefer a board startup.s)

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

### Current state

- **Host suite passes** — `tools/run_host_tests.sh` reports **24/24**. It is
  arch-agnostic (system gcc, pure-logic HAL tests: `hal_status`, conversion,
  software CRC, GPIO pin encoding), so it confirms the port edits did not regress
  the shared HAL / `common` code.
- **On-target suite passes on real F767 hardware** — the TEST ELF builds for
  cortex-m7, flashes to the STM32F767ZI, and reports over USART3 @9600:
  **30 tests, 0 failures** (conformance 15, timebase 8, CRC 7). The F4 white-box
  tier and the raw-flash suite are intentionally skipped (see below); the F4 TEST
  ELF still builds unchanged (no reference-target regression).
- **Not yet:** the M7 white-box arch tier and PIL/CI wiring (items 4 and 6).

### How the test build was generalized for cortex-m7

The on-target test ELF reports results **exclusively over `NAVTEST_UART`**
(`tests/navtest_state.c` → `hal_uart_print`), so on-target testing was gated on
F7-2 (UART), now done. The following made the (previously M4-hardcoded) harness
build and run for cortex-m7:

1. **UART (F7-2)** — ✅ done; provides the `NAVTEST_UART` output path
   (`NAVTEST_UART = HAL_UART_3` for F767, added to `tests/navtest_target.h`).
2. **Processor-generic test linker** — ✅ `cmake/arch/armv7e-m.cmake` now uses
   `tests/arch/${CMAKE_SYSTEM_PROCESSOR}/linker.ld`, and
   `tests/arch/cortex-m7/linker.ld` (2 MB / 512 KB) was added.
3. **Arch-generic harness** — ✅ `tests/main.c` dropped its vestigial
   `#define CORTEX_M4`; the `arch/cortex-m4/*` includes and white-box suite
   registrations are now gated on `NAVTEST_ARCH_CORTEX_M4` (a per-processor
   define set by the root `CMakeLists.txt` TEST block). A cortex-m7 build skips
   that tier.
   - The TEST build globs **all** vendor `.c` regardless of Kconfig, so the
     CMakeLists now drops `uart.c` (F4) / `uart_f7.c` (F7) to keep only the one
     matching the target family — otherwise both define `hal_uart_*` and the F4
     file fails against the F7 register header.
4. **White-box tier** — ✅ `tests/arch/cortex-m7/` has GPIO (incl. a
   contiguous-port-indexing assertion), TIMER, CLOCK (`clock_f7`, HSI/PLL — HSE
   omitted, board availability unconfirmed), INTERRUPT (NVIC + a vector-table
   assertion that USART3/IRQ 39 resolves to a real handler, guarding the
   startup fix) and UART PROTOCOL (F7 USART, `BRR` on UART1/6). Registered under
   `NAVTEST_ARCH_CORTEX_M7`; all pass on hardware (GPIO 12, TIMER 15, CLOCK 10,
   INTERRUPT 15, UART 12 → **100/0 total** with the portable tiers). i2c/spi/pwm
   white-box belong to F7-6.
5. **Portable + cap tiers** — ✅ run unchanged on F767 (HAL black-box, self-gate
   on `NAVHAL_HAS_*`). The raw-flash suite is now **re-enabled** on M7 (F7-4
   corrected the sector map and fixed two `flash.c` hardware bugs) and passes
   (6/6).
6. **CI / PIL** — ✅
   - Sample-matrix CI: `tools/build_all_f767_samples.sh` builds the portable
     sample tier under the F767 toolchain (12/12); wired into `ci.yml` as the
     `sample-matrix-f767` job, the `ci-required` aggregate, and the dispatch
     path filter.
   - PIL: `tools/pil/boards/nucleo_f767zi.conf` + `tools/renode/navhal_f767zi.resc`
     (and `stm32f767zi.repl`, which reuses Renode's mainline STM32F746 CPU model
     with SRAM widened to the F767's 512 KB). `run_tests.sh`/`run.sh` now take a
     per-board `RESC` override. The F767 ELF boots and runs the full suite over
     USART3 in Renode; the SPI `CR2.DS` and I²C `TIMINGR` register read-backs the
     Renode models don't reflect are `NAVTEST_SKIP_ON_PIL()`-gated (they stay
     strict on HIL), so PIL is green.

## Milestones

| # | Scope | Status |
|---|---|---|
| F7-1 | Build wiring + GPIO/CLOCK/TIMER/INTERRUPT + flashable blink on HSI | **done** |
| F7-2 | UART (USART3 console) — `uart_f7.c` for the F7 ISR/RDR/TDR IP; polling TX/RX verified on hardware | **done** |
| F7-3 | High-frequency clock — PWR over-drive + VOS + flash wait-states + APB limits, in `clock_f7.c`; verified at 216 MHz on hardware | **done** |
| F7-4 | FLASH — real F767 sector map in `flash_reg.h`; fixed two `flash.c` bugs found on hardware (DSB after program, NULL guard); flash test re-enabled and passing (36/36) | **done** |
| F7-5 | DMA + hardware FPU (`fpv5-d16`) + DWT — all verified on hardware (56-test run). No new code needed: register-compatible with M4 and the `fpv5-d16` flag landed in F7-1. D-cache stays off (DMA coherent); enabling it + a DMA UART backend remain. | **done** (cache off) |
| F7-6 | Peripherals. **Done:** PWM + HW CRC (reuse, verified), SPI (`spi_f7.c`) + I2C (`i2c_f7.c`) rewrites (init register-verified; transfers PIL-validated against modelled devices), SDIO (shared `sdio.c`, polled; un-gated for M7; PIL block round-trip vs a Renode SD card). **Remaining:** SDIO DMA async backend (still M4-only, pending L1-cache validation). | done |
| F7-7 | Test enablement + CI. **Done:** processor-generic test linker/harness; on-target tiers + white-box GPIO/TIMER/CLOCK/INTERRUPT/UART/PWM/SPI/I2C; `sample-matrix-f767` CI job; **PIL** — `tools/pil/boards/nucleo_f767zi.conf` + Renode `.resc`/`.repl` (F746 model, SRAM widened to 512 KB), boots & runs the suite over USART3. See [Testing](#testing). | **done** |

## Risks / notes

- `clock_f7.c` handles VOS, over-drive (>180 MHz), flash wait states and APB
  bus limits automatically inside `hal_clock_init`, so a PLL target up to 216 MHz
  is safe. Samples that never call `hal_clock_init` simply run on the reset HSI
  (16 MHz, 0 wait states).
- UART is **polling-only** on F7 today (`uart_f7.c`). The DMA-backed UART API
  (`hal_uart_write_dma` etc.) is not yet ported, so `DRV_UART_DMA` stays off —
  this is a `uart_f7.c` gap, not a DMA-driver one (the DMA driver itself is
  verified working).
- The **L1 D-cache is kept off**, which is what makes the verified DMA path
  coherent. Turning the D-cache on later (for performance) needs cache
  clean/invalidate around any DMA/peripheral-shared buffer.
- The flash KV store uses the two 256 KB sectors 6/7. They are far from code
  (safe), but 256 KB is a coarse erase granularity for a small key/value store,
  so compaction erases are slow (~seconds). A future tweak could move storage to
  the 32 KB sectors when the application image is known to be small.
- `flash.c` now issues a `DSB` after each program store (required on Cortex-M7;
  harmless on M4) and NULL-guards `hal_flash_read`. Both were latent bugs that
  only manifested on real M7 silicon — worth knowing if porting the driver
  elsewhere.
