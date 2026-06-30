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
| CLOCK             | ◐ | `src/vendor/stm32/clock/clock.c`         | HSI / HSE / PLL up to ~180 MHz. Over-drive (`PWR_CR1` ODEN/ODSWEN) + VOS for 216 MHz **not yet** wired — do not target >180 MHz. |
| INTERRUPT         | ✓ | `src/arch/armv7e-m/interrupt/interrupt.c`| NVIC; shared ARMv7E-M arch code. |
| UART              | ✗ | (pending)                                 | Hardware present (USART3 on ST-LINK VCP). Driver pending: F7 USART IP uses `ISR`/`RDR`/`TDR`/`ICR`, not the F4 `SR`/`DR` that `uart.c` targets. Disabled in defconfig. |
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
NAVHAL_HAS_UART          0   (held off — F7 USART IP differs; see port plan F7-2)
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

## Caveats and known limitations

* HSI-only by default; the over-drive/VOS sequence for >180 MHz is not yet
  implemented (port-plan milestone F7-3).
* `uart_reg.h` is a placeholder copy of the F4 header and must be rewritten to
  the F7 USART register model before `DRV_UART` is enabled.
* L1 caches are disabled; enabling them later requires DMA-buffer cache
  maintenance.
* Not yet wired into CI (sample matrix / PIL) — milestone F7-7.
