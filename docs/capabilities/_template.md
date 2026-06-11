# <BOARD_NAME> (<MCU_FAMILY> / <ARCH>)

> Template for a new MCU's capability page. Copy to `docs/capabilities/<board_slug>.md` and fill in. Then add a column to `docs/capabilities/README.md`'s matrix.

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)   | _e.g. `cortex-m4`, `avr`_ |
| Vendor (`NAVHAL_TARGET_VENDOR`) | _e.g. `stm32`, `microchip`_ |
| Family (`NAVHAL_TARGET_FAMILY`) | _e.g. `stm32f4`, `atmega328p`_ |
| Board (`NAVHAL_TARGET_BOARD`)   | _e.g. `nucleo_f401re`, `atmega328p`_ |
| Toolchain                     | _e.g. `arm-none-eabi-`, `avr-`_ |
| Defconfig                     | `cmake/defconfigs/<slug>.defconfig` |
| Toolchain file                | `cmake/toolchains/<slug>-toolchain.cmake` |
| Default sysclk                | _Hz, and how it's reached (HSI, PLL, external xtal)_ |

## Capabilities

| `NAVHAL_HAS_*` | Status | Driver | Notes |
|---|---|---|---|
| GPIO              | _✓/◐/✗/—_ | `src/vendor/<vendor>/gpio/gpio.c`         | |
| UART              |           | `src/vendor/<vendor>/uart/uart.c`         | _baud rates, USART instances_ |
| I2C               |           | `src/vendor/<vendor>/i2c/i2c.c`           | _master/slave, max speed_ |
| SPI               |           | `src/vendor/<vendor>/spi/spi.c`           | |
| TIMER             |           | `src/vendor/<vendor>/timer/timer.c`       | _instances, max freq_ |
| PWM               |           | `src/vendor/<vendor>/pwm/pwm.c`           | _channels_ |
| CLOCK             |           | `src/vendor/<vendor>/clock/clock.c`       | _PLL / HSI / HSE_ |
| INTERRUPT         |           | `src/arch/<arch>/interrupt/interrupt.c`   | _NVIC / native IRQ count_ |
| FLASH             |           | `src/vendor/<vendor>/flash/flash.c`       | _block size, write granularity_ |
| CRC_HW            |           | `src/vendor/<vendor>/crc/crc.c` or s/w    | _CRC poly_ |
| CYCLE_COUNTER     |           | `src/arch/<arch>/dwt/dwt.c` (Cortex-M)    | _DWT / equivalent_ |
| FPU               |           | `src/arch/<arch>/fpu/fpu.c`               | _hard / soft float ABI_ |
| DMA               |           | `src/vendor/<vendor>/dma/dma.c`           | _stream count_ |
| SDIO              |           | `src/vendor/<vendor>/sdio/sdio.c`         | _bus width_ |
| UART_DMA          |           | (in uart.c)                                | |
| I2C_DMA           |           | (in i2c.c)                                 | |
| SDIO_DMA          |           | (in sdio.c)                                | |

## Default Kconfig state

What `cmake -B b -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/<slug>-toolchain.cmake` produces in `b/navhal_target.h` immediately after seed (i.e. before the user changes anything):

```
<paste the NAVHAL_HAS_* block from b/navhal_target.h here>
```

## Caveats and known limitations

_Anything that turns a clean `✓` into a `◐`. Examples: only 7-bit I²C addressing supported, FLASH writes restricted to word-aligned chunks, UART RX has no IRQ-driven mode yet, etc. Link to the relevant issue or the spec section that documents the limit._

## Sample matrix coverage

* CI job: `Build all <arch> samples` in `.github/workflows/ci.yml`.
* Iterates samples from: _`samples/portable/` only_ / _`samples/portable/` + `samples/<vendor>/`_.
