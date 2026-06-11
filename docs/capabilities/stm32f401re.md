@page cap_stm32f401re Nucleo-F401RE (STM32F4)

# Nucleo-F401RE (STM32F4 / Cortex-M4)

The reference target for NavHAL v1. Everything in the public API has a working implementation here; the cap-contract CI runs against this build.

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)   | `cortex-m4` |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `stm32` |
| Family (`NAVHAL_TARGET_FAMILY`) | `stm32f4` |
| Board (`NAVHAL_TARGET_BOARD`)   | `nucleo_f401re` |
| Toolchain                     | `arm-none-eabi-` (`gcc-arm-none-eabi`) |
| Defconfig                     | [`cmake/defconfigs/cortex-m4_stm32f4_nucleo_f401re.defconfig`](../../cmake/defconfigs/cortex-m4_stm32f4_nucleo_f401re.defconfig) |
| Toolchain file                | [`cmake/toolchains/arm-none-eabi-toolchain.cmake`](../../cmake/toolchains/arm-none-eabi-toolchain.cmake) |
| Default sysclk                | 84 MHz via PLL on HSI (see `hal_clock_init` defaults in `src/vendor/stm32/clock/clock.c`) |

## Capabilities

| `NAVHAL_HAS_*` | Status | Driver | Notes |
|---|---|---|---|
| GPIO              | ✓ | `src/vendor/stm32/gpio/gpio.c`     | PA / PB / PC / PD / PH (PE has no exposed pins on the Nucleo); alternate functions, pull-up/down, output speed. |
| UART              | ✓ | `src/vendor/stm32/uart/uart.c`     | USART1 / USART2 / USART6. Polling + IRQ + DMA backends. |
| I2C               | ✓ | `src/vendor/stm32/i2c/i2c.c`       | I²C1 / I²C2 / I²C3, master only. Standard (100 kHz) and Fast (400 kHz). |
| SPI               | ✓ | `src/vendor/stm32/spi/spi.c`       | Master, 8-bit; bridge sample uses SPI1. |
| TIMER             | ✓ | `src/vendor/stm32/timer/timer.c`   | TIM2 / TIM3 / TIM4 / TIM5; period + callback. |
| PWM               | ✓ | `src/vendor/stm32/pwm/pwm.c`       | Same timer instances; per-channel duty. |
| CLOCK             | ✓ | `src/vendor/stm32/clock/clock.c`   | HSI / PLL configuration; `hal_clock_get_sysclk()` / `_apb1clk()` / etc. |
| INTERRUPT         | ✓ | `src/arch/armv7e-m/interrupt/interrupt.c` | NVIC priority + enable/disable, callback registration. |
| FLASH             | ✓ | `src/vendor/stm32/flash/flash.c`   | Sector-aligned erase, word-aligned program; word & half-word reads. |
| CRC_HW            | ✓ | `src/vendor/stm32/crc/crc.c`       | CRC-32 / MPEG-2 with the F4 hardware unit. |
| CYCLE_COUNTER     | ✓ | `src/arch/armv7e-m/dwt/dwt.c`      | DWT-backed. Adds µs-resolution helpers (`_get_us`, `_delay_us`). |
| FPU               | ✓ | `src/arch/armv7e-m/fpu/fpu.c`      | Hardware FPU enabled via `CONFIG_USE_FPU=y` (also flips `-mfpu=fpv4-sp-d16`). |
| DMA               | ✓ | `src/vendor/stm32/dma/dma.c`       | DMA1 + DMA2, all streams. |
| SDIO              | ✓ | `src/vendor/stm32/sdio/sdio.c`     | 1-bit + 4-bit; polling + async (DMA) block transfers. No SD card present on the Nucleo board itself — bring your own breakout. |
| UART_DMA          | ✓ | (uart.c)                            | `hal_uart_write_dma` etc. Defaults on when UART + DMA are on. |
| I2C_DMA           | ✓ | (i2c.c)                             | `hal_i2c_read_regs_dma`. Defaults on when I²C + DMA are on. |
| SDIO_DMA          | ✓ | (sdio.c)                            | `hal_sdio_*_async` block transfers. Defaults on when SDIO + DMA are on. |

## Default Kconfig state

What `navhal_target.h` contains after `cmake -B b -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake`:

```
NAVHAL_HAS_DMA            1
NAVHAL_HAS_FPU            0   (selectable via CONFIG_USE_FPU)
NAVHAL_HAS_CRC_HW         0   (selectable via CONFIG_DRV_CRC)
NAVHAL_HAS_CYCLE_COUNTER  0   (selectable via CONFIG_DRV_DWT)
NAVHAL_HAS_SDIO           0   (selectable via CONFIG_DRV_SDIO)
NAVHAL_HAS_GPIO           1
NAVHAL_HAS_UART           1
NAVHAL_HAS_I2C            0   (selectable via CONFIG_DRV_I2C)
NAVHAL_HAS_SPI            0   (selectable via CONFIG_DRV_SPI)
NAVHAL_HAS_TIMER          1
NAVHAL_HAS_PWM            0   (selectable via CONFIG_DRV_PWM)
NAVHAL_HAS_CLOCK          1
NAVHAL_HAS_INTERRUPT      1
NAVHAL_HAS_FLASH          0   (selectable via CONFIG_DRV_FLASH)
NAVHAL_HAS_UART_DMA       1
NAVHAL_HAS_I2C_DMA        0
NAVHAL_HAS_SDIO_DMA       0
```

A `0` in the *default* config doesn't mean "unsupported" — it means the driver is opt-in. The matrix `✓` reflects what's possible, not what's default.

## Caveats and known limitations

* `hal_i2c` is master-only; slave mode is unimplemented (`HAL_ERR_IO`).
* `hal_clock_init` busy-waits on PLL/HSE ready flags. Renode's RCC model used to assert these much slower than real silicon, which is why early PIL runs were very slow.
* No DMA buffer-alignment assertion in `hal_uart_write_dma` — caller must ensure the buffer outlives the transfer.
* SDIO timing is calibrated for ~21 MHz post-handshake (CLKCR DIV=2 from 84 MHz SDIOCLK).

## Sample matrix coverage

* CI job: `Build all samples` in `.github/workflows/ci.yml` — builds every sample declared in Kconfig (28 today).
* Iterates: `samples/portable/`, `samples/cortex-m/`, `samples/no_hal/`.
* PIL: the full on-target test ELF runs against this target in `.github/workflows/renode.yml` (`Full suite in Renode` job).
