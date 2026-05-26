# ATmega328P (AVR8)

The M6 target. An 8-bit AVR — none of the optional Cortex-M caps (DMA, FPU, DWT, SDIO) exist; the cap-gated symbols are absent from any AVR build by construction.

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)   | `avr` |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `microchip` |
| Family (`NAVHAL_TARGET_FAMILY`) | `atmega328p` |
| Board (`NAVHAL_TARGET_BOARD`)   | `atmega328p` |
| Toolchain                     | `avr-` (`gcc-avr`, `avr-libc`, `binutils-avr`) |
| Defconfig                     | [`cmake/defconfigs/avr_atmega328p.defconfig`](../../cmake/defconfigs/avr_atmega328p.defconfig) |
| Toolchain file                | [`cmake/toolchains/avr-toolchain.cmake`](../../cmake/toolchains/avr-toolchain.cmake) |
| Default sysclk                | 16 MHz external crystal (Arduino-Uno wiring), reflected by the `F_CPU=16000000UL` define set in `CMakeLists.txt`. |

## Capabilities

| `NAVHAL_HAS_*` | Status | Driver | Notes |
|---|---|---|---|
| GPIO              | ✓   | `src/vendor/microchip/gpio/gpio.c`     | All ports (B, C, D); hot-path accessors inlined for zero-cost. |
| UART              | ✓   | `src/vendor/microchip/uart/uart.c`     | USART0 only (the only USART on this part); polling. |
| I2C               | ✓   | `src/vendor/microchip/i2c/i2c.c`       | TWI master; standard / fast mode. |
| SPI               | ✓   | `src/vendor/microchip/spi/spi.c`       | Master, 8-bit. |
| TIMER             | ✓   | `src/vendor/microchip/timer/timer.c`   | Timer0 / Timer1 / Timer2; period + callback. |
| PWM               | ✓   | `src/vendor/microchip/pwm/pwm.c`       | Channels via Timer0/1/2 output compare. |
| CLOCK             | ◐   | `src/vendor/microchip/clock/clock.c`   | Reports the build-time `F_CPU`; no runtime reconfig (the AVR's CLKPR isn't exposed in the HAL today). |
| INTERRUPT         | ✓   | `src/arch/avr/interrupt/interrupt.c`   | AVR's flat IRQ table; enable/disable + callback. |
| FLASH             | ◐   | `src/vendor/microchip/flash/flash.c`   | Read works freely; programming uses self-write (SPM) and requires running from the bootloader section. Document this in your application. |
| CRC_HW            | s/w | `src/vendor/stm32/crc/crc.c` (s/w fallback) | No hardware CRC peripheral; the software CRC table fulfils the `hal_crc_*` API. `NAVHAL_HAS_CRC_HW == 0`. |
| CYCLE_COUNTER     | —   | n/a                                    | No DWT or equivalent. `NAVHAL_HAS_CYCLE_COUNTER == 0`; cycle-counter symbols absent at link time. Time can still be measured via the Timer driver. |
| FPU               | —   | n/a                                    | No hardware FPU; `NAVHAL_HAS_FPU == 0`. Floating-point operations use soft float from avr-libc. |
| DMA               | —   | n/a                                    | No DMA controller. `NAVHAL_HAS_DMA == 0`. |
| SDIO              | —   | n/a                                    | No SDIO peripheral. SD-card access would need SPI + an FAT FS in user code. |
| UART_DMA          | —   | n/a                                    | Requires DMA. |
| I2C_DMA           | —   | n/a                                    | Requires DMA. |
| SDIO_DMA          | —   | n/a                                    | Requires SDIO + DMA. |

## Default Kconfig state

What `navhal_target.h` contains after `cmake -B b -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-toolchain.cmake`:

```
NAVHAL_HAS_DMA            0
NAVHAL_HAS_FPU            0
NAVHAL_HAS_CRC_HW         0
NAVHAL_HAS_CYCLE_COUNTER  0
NAVHAL_HAS_SDIO           0
NAVHAL_HAS_GPIO           1
NAVHAL_HAS_UART           1
NAVHAL_HAS_I2C            0   (selectable via CONFIG_DRV_I2C)
NAVHAL_HAS_SPI            0   (selectable via CONFIG_DRV_SPI)
NAVHAL_HAS_TIMER          1
NAVHAL_HAS_PWM            0   (selectable via CONFIG_DRV_PWM)
NAVHAL_HAS_CLOCK          1
NAVHAL_HAS_INTERRUPT      1
NAVHAL_HAS_FLASH          0   (selectable via CONFIG_DRV_FLASH)
NAVHAL_HAS_UART_DMA       0
NAVHAL_HAS_I2C_DMA        0
NAVHAL_HAS_SDIO_DMA       0
```

The `—` rows are hard-wired at 0 — even if a user sets `CONFIG_DRV_DMA=y` in `.config` for an AVR build, the `if ARCH_CORTEX_M4` guards in `Kconfig` and the absent driver sources mean nothing changes.

## Caveats and known limitations

* `hal_clock` doesn't reconfigure the prescaler; it reports `F_CPU`. If your application needs to slow the CPU at runtime, write CLKPR yourself and re-build with the new `F_CPU`.
* `hal_flash` write requires the application to run from the bootloader section so SPM works. Out-of-the-box samples that touch flash assume this.
* The AVR port is recent (M6) — peripheral edge cases will surface as samples in `samples/portable/` exercise them. See [`docs/m5_avr_readiness_review.md`](../m5_avr_readiness_review.md) for the readiness audit and [`docs/m5_conformance_audit.md`](../m5_conformance_audit.md) for the per-driver conformance check.

## Sample matrix coverage

* CI job: `Build all AVR samples` in `.github/workflows/ci.yml` — builds every sample under `samples/portable/` against the AVR toolchain.
* Iterates: `samples/portable/` only. Samples under `samples/cortex-m/` and `samples/no_hal/` are not portable to AVR and are skipped by design.
* No PIL: there's no Renode (or simavr) job for AVR yet. AVR samples are build-tested only; runtime verification is HIL on a real board.
