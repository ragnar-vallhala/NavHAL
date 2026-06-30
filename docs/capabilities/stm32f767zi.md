@page cap_stm32f767ze Nucleo-F767ZI (STM32F7)

# Nucleo-F767ZI (STM32F7 / Cortex-M7)

The third NavHAL port, in **initial bring-up**. The build system, GPIO, clock,
timer and interrupt layers are implemented and verified on real hardware (a
flashed `hal_blink` toggles LD1). The remaining peripherals are scoped
follow-ups — see [`../stm32f767zi_port_plan.md`](../stm32f767zi_port_plan.md).

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)     | `cortex-m7` |
| ISA layer (`ARCH_ISA`)          | `armv7e-m` (shared with Cortex-M4) |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `stm32` |
| Family (`NAVHAL_TARGET_FAMILY`) | `stm32f7` |
| Board (`NAVHAL_TARGET_BOARD`)   | `nucleo_f767zi` |
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
| PWM               | ✗ | (pending)                                 | Depends on the shared timer driver; enable after timer validation. |
| FLASH             | ✗ | (pending)                                 | F7 sector map differs (32 KB×4, 128 KB×1, 256 KB×3 single 2 MB bank); `flash_reg.h` carries a placeholder F4 map. |
| CRC_HW            | ✗ | (pending)                                 | F7 CRC unit present; driver not validated. |
| CYCLE_COUNTER     | ✗ | `src/arch/armv7e-m/dwt/dwt.c`            | Arch code exists; not enabled/validated on M7 yet. |
| FPU               | ✗ | `src/arch/armv7e-m/fpu/fpu.c`            | M7 is double-precision (`-mfpu=fpv5-d16`, wired in the cmake arch fragment); runtime enable not validated. |
| DMA               | ✗ | (pending)                                 | M7 needs L1 D-cache coherency handling around DMA buffers; cache left off for bring-up. |
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
NAVHAL_HAS_DMA           0
NAVHAL_HAS_FPU           0   (selectable via CONFIG_USE_FPU once validated)
NAVHAL_HAS_I2C/SPI/PWM/FLASH/CRC_HW/CYCLE_COUNTER/SDIO  0
```

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
* Clock brought to **216 MHz** (HSI→PLL, over-drive engaged): captured
  `sysclk=216000000 ahb=216000000 apb1=54000000 apb2=108000000` over USART3 while
  the UART kept running — confirming VOS/over-drive/WS=7 and the APB prescalers.

## Caveats and known limitations

* Reset default is HSI 16 MHz; call `hal_clock_init` with a PLL config to scale
  up (up to 216 MHz — `clock_f7.c` does VOS/over-drive/wait-states for you).
* UART is polling-only; the DMA backend (`hal_uart_write_dma`) is not yet ported
  to F7 (needs M7 cache-coherency handling — F7-5), so `DRV_UART_DMA` stays off.
* L1 caches are disabled; enabling them later requires DMA-buffer cache
  maintenance.
* Not yet wired into CI (sample matrix / PIL) — milestone F7-7.
