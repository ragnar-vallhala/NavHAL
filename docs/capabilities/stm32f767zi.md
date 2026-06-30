@page cap_stm32f767zi STM32F767ZI (Cortex-M7)

# STM32F767ZI (STM32F7 / Cortex-M7)

The third NavHAL port, in **initial bring-up**. The build system, GPIO, clock,
timer and interrupt layers are implemented and verified on real hardware (a
flashed `hal_blink` toggles the LED). The remaining peripherals are scoped
follow-ups — see [`../stm32f767zi_port_plan.md`](../stm32f767zi_port_plan.md).
The reference board used for bring-up is the ST Nucleo-F767ZI, but the port
targets the STM32F767ZI MCU; other boards built on the same MCU are supported
by adding a board layer.

## Target identity

| | |
|---|---|
| MCU                             | `STM32F767ZI` (2 MB flash, 512 KB SRAM) |
| Arch (`NAVHAL_TARGET_ARCH`)     | `cortex-m7` |
| ISA layer (`ARCH_ISA`)          | `armv7e-m` (shared with Cortex-M4) |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `stm32` |
| Family (`NAVHAL_TARGET_FAMILY`) | `stm32f7` |
| Reference board (`NAVHAL_TARGET_BOARD`) | `nucleo_f767zi` |
| Toolchain                       | `arm-none-eabi-` (`gcc-arm-none-eabi`) |
| Defconfig                       | [`cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig`](../../cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig) |
| Toolchain file                  | [`cmake/toolchains/arm-none-eabi-f767-toolchain.cmake`](../../cmake/toolchains/arm-none-eabi-f767-toolchain.cmake) |
| Default sysclk                  | 16 MHz (reset-default HSI; no `hal_clock_init` in the blink path) |

## Capabilities

| `NAVHAL_HAS_*` | Status | Driver | Notes |
|---|---|---|---|
| GPIO              | ✓ | `src/vendor/stm32/gpio/gpio.c`            | Reuses the F4 driver; F7 `gpio_reg.h` uses contiguous port indexing (A–G + H). Verified on LD1 (PB0). |
| TIMER             | ✓ | `src/vendor/stm32/timer/timer.c`         | TIM2–5 / TIM1 / TIM9–11; same register layout as F4. |
| CLOCK             | ✓ | `src/vendor/stm32/clock/clock_f7.c`      | HSI / HSE / PLL up to **216 MHz**, verified on hardware. VOS Scale 1, PWR over-drive (>180 MHz), HCLK-scaled flash wait states + ART/prefetch, APB1 ≤54 / APB2 ≤108 MHz prescalers. |
| INTERRUPT         | ✓ | `src/arch/armv7e-m/interrupt/interrupt.c`| NVIC; shared ARMv7E-M arch code. |
| UART              | ◐ | `src/vendor/stm32/uart/uart_f7.c`        | USART1/2/3/6, polling TX/RX. USART3 (ST-LINK VCP, PD8/PD9) verified on hardware at 115200. F7-specific driver (ISR/RDR/TDR), selected by `CONFIG_FAMILY_STM32F7`. DMA backend not yet ported (F7-5). |
| I2C               | ✗ | (pending)                                 | F7 uses the timing-register I2C IP (like F0/L4); needs a new driver path. |
| SPI               | ✗ | (pending)                                 | Register-compatible with F4; bring-up after UART. |
| PWM               | ✓ | `src/vendor/stm32/pwm/pwm.c`             | Reuses the shared timer-based driver. Opt-in via `CONFIG_DRV_PWM`; `test_pwm` (11) passes on hardware. |
| FLASH             | ✓ | `src/vendor/stm32/flash/flash.c`        | Key/value store on sectors 6/7 (256 KB each) of the real F767 12-sector 2 MB map. Opt-in via `CONFIG_DRV_FLASH`; `test_flash_raw` (6) passes on hardware. Bring-up fixed two `flash.c` bugs (M7 write-buffer `DSB`; NULL guard). |
| CRC_HW            | ✓ | `src/vendor/stm32/crc/crc.c`            | Hardware CRC-32; default polynomial is register-compatible with F4. Opt-in via `CONFIG_DRV_CRC`; the CRC suite (7) passes via the hardware unit on F767. |
| CYCLE_COUNTER     | ✓ | `src/arch/armv7e-m/dwt/dwt.c`            | DWT-backed; shared ARMv7E-M arch code. Opt-in via `CONFIG_DRV_DWT`; `test_dwt` (6) passes on hardware. |
| FPU               | ✓ | `src/arch/armv7e-m/fpu/fpu.c`            | Hardware **double-precision** FPU (`-mfpu=fpv5-d16`, hard float) via `CONFIG_USE_FPU` + `CONFIG_DRV_FPU`. `test_fpu_accel` (3) passes on hardware. |
| DMA               | ✓ | `src/vendor/stm32/dma/dma.c`            | DMA1/DMA2 stream controller (register-compatible with F4). Opt-in via `CONFIG_DRV_DMA`; `test_dma` (17) passes on hardware. Coherent while the L1 D-cache stays off (see caveats); a DMA UART backend is still pending. |
| SDIO              | ✗ | (pending)                                 | After DMA. |
| UART_DMA / I2C_DMA / SDIO_DMA | ✗ | (pending)                     | Follow their base drivers. |

`✗` here means the silicon has the peripheral but the NavHAL driver isn't
validated for F7 yet — treated like `—` at link time.

## Default Kconfig state

What `navhal_target.h` contains after
`cmake -B b -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake -DSAMPLE=hal_blink`:

```
NAVHAL_HAS_GPIO          1
NAVHAL_HAS_TIMER         1
NAVHAL_HAS_CLOCK         1
NAVHAL_HAS_INTERRUPT     1
NAVHAL_HAS_UART          1   (uart_f7.c — polling; DMA backend off)
NAVHAL_HAS_DMA           0   (opt-in via CONFIG_DRV_DMA — verified working)
NAVHAL_HAS_FPU           0   (opt-in via CONFIG_USE_FPU+DRV_FPU — verified working)
NAVHAL_HAS_CYCLE_COUNTER 0   (opt-in via CONFIG_DRV_DWT — verified working)
NAVHAL_HAS_FLASH         0   (opt-in via CONFIG_DRV_FLASH — verified working)
NAVHAL_HAS_I2C/SPI/PWM/CRC_HW/SDIO  0
```

DMA / FPU / DWT are off in the *default* config (opt-in), but all three are
implemented and pass their on-target test suites — see the bring-up record.

## Hardware bring-up record

* Board probed: `st-info --probe` → `STM32F76x_F77x`, chipid `0x451`,
  2048 KiB flash / 512 KiB SRAM (matches the linker script).
* `hal_blink` (LD1 / PB0) built (`text 9892 / data 4 / bss 524`), flashed to
  `0x08000000` and verified by `st-flash`. Erase reported sector 0 = 0x8000
  (32 KB), confirming the F7 sector map differs from the F4's 16 KB sector 0.
* `hal_uart_tx` flashed; USART3 output captured on `/dev/ttyACM0` at 115200 —
  100×`Hello World` + `UART TX NO DMA Test finished: 115 ticks`, correctly
  framed (confirms the F7 ISR/TDR path and the BRR baud calc from APB1=16 MHz).
* On-target test ELF (`-DTEST=ON`) flashed; results captured over USART3 @9600:
  **30 tests, 0 failures** (conformance 15, timebase 8, CRC 7). M4 white-box
  and raw-flash suites intentionally skipped on M7.
* Vector table: the F767 ships its own `src/board/nucleo_f767zi/startup.s`
  (STM32F767xx layout). Verified that vector slot 39 (USART3) resolves to the
  dispatch handler `0x...2254` rather than the literal `0` the F401-based arch
  table left there — so interrupt-driven USART3 dispatches instead of faulting.
* Clock brought to **216 MHz** (HSI→PLL, over-drive engaged): captured
  `sysclk=216000000 ahb=216000000 apb1=54000000 apb2=108000000` over USART3 while
  the UART kept running — confirming VOS/over-drive/WS=7 and the APB prescalers.
* Test ELF rebuilt with `CONFIG_USE_FPU` + `DRV_FPU` + `DRV_DWT` + `DRV_DMA`
  (hard-float `fpv5-d16`); on-target run reported **56 tests, 0 failures** —
  adds DMA (17), CYCLE_COUNTER/DWT (6) and FPU (3) to the 30 above.
* Flash KV store (F7-4): after correcting the sector map and fixing the M7
  write-buffer `DSB` + NULL-guard bugs, the `test_flash_raw` suite (6) passes —
  a clean-boot run reports **36 tests, 0 failures**. A probe confirmed
  erase→save→read round-trips real data on sector 6 (0x08080000).

## Caveats and known limitations

* Reset default is HSI 16 MHz; call `hal_clock_init` with a PLL config to scale
  up (up to 216 MHz — `clock_f7.c` does VOS/over-drive/wait-states for you).
* UART is polling-only; the DMA-backed UART API (`hal_uart_write_dma`) is not yet
  ported to F7, so `DRV_UART_DMA` stays off — even though the DMA driver itself
  works. (Wiring `uart_f7.c` to DMA is the remaining UART-DMA task.)
* The **L1 D-cache is kept disabled**, which is what makes DMA buffers coherent
  today. The DMA driver is verified in this configuration. Enabling the D-cache
  later for performance will require cache clean/invalidate around any
  DMA/peripheral-shared buffer (`SCB_CleanDCache_by_Addr` / `InvalidateDCache`).
* Not yet wired into CI (sample matrix / PIL) — milestone F7-7.
