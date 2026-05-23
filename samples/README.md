# NavHAL samples

Samples are organised into three tiers by what they need from the hardware.
Build any of them with `-DSAMPLE=<short-name>`; the build resolves the short
name to its tier directory.

## Tiers

| Tier | Directory | What it is | Targets |
|------|-----------|------------|---------|
| **Portable** | `samples/portable/` | HAL samples that use only peripherals every NavHAL port has (GPIO, UART, timer, clock, PWM, I²C, SPI, CRC). The *same source* builds for every target via the board-layer aliases (`LED_BUILTIN`, `BOARD_CONSOLE_UART`, …). | Cortex-M4 + AVR |
| **Cortex-M** | `samples/cortex-m/` | HAL samples that need hardware the ATmega328P lacks — DMA, a hardware FPU, the DWT cycle counter, SDIO, or substantially more of a peripheral than the ATmega328P has (many hardware timers, an STM32 clock-tree/PLL). Their Kconfig entries `depend on ARCH_CORTEX_M4`, so they are not offered on AVR. | Cortex-M4 only |
| **No-HAL** | `samples/no_hal/` | Register-level teaching samples that bypass the HAL entirely. Inherently per-MCU (current set is STM32F4). | Cortex-M4 only |

## Portable tier — verified

All 12 portable samples build for **both** the Nucleo-F401RE and the
ATmega328P from a single Kconfig switch, using the board-layer aliases and
the v1 `hal_*` API only — no `#ifdef`, no target-specific identifiers in the
sample source.

## Sample index

### Portable (`samples/portable/`)
| Sample | Exercises |
|--------|-----------|
| `hal_blink`, `hal_pupd`, `hal_blink_delay` | GPIO |
| `hal_uart_tx`, `hal_uart_int` | UART (polled, interrupt) |
| `hal_timer` | general-purpose timer + callbacks |
| `hal_pwm` | PWM output |
| `hal_i2c` | I²C master |
| `hal_spi_esp_bridge` | SPI master |
| `hal_crc` | CRC-32 |
| `hal_flash` | flash/EEPROM key-value store |
| `hal_blink_cpp` | the above, from a C++17 translation unit |

### Cortex-M (`samples/cortex-m/`)
| Sample | Needs |
|--------|-------|
| `hal_uart_dma`, `hal_dma_polling_uart`, `hal_dma_i2c`, `hal_uart_dma_bridge` | DMA |
| `hal_fpu` | hardware FPU |
| `hal_dwt` | DWT cycle counter |
| `hal_sdio`, `hal_sdio_block`, `hal_sdio_perf`, `hal_fatfs_posix` | SDIO |
| `hal_systick` | five concurrent hardware timers |
| `hal_clock` | the STM32 PLL clock tree |

### No-HAL (`samples/no_hal/`)
`no_hal_blink` · `no_hal_uart_tx` · `no_hal_pwm` · `no_hal_i2c` ·
`no_hal_flash`

## Adding a sample

1. Create `samples/<tier>/<NN_name>/` with `main.c` + `CMakeLists.txt`.
2. Add the short name + tier-relative path to the parallel lists in
   `samples/CMakeLists.txt`.
3. Add a `config SAMPLE_<NN>_<NAME>` entry to `Kconfig`; for a Cortex-M or
   No-HAL sample add `depends on ARCH_CORTEX_M4`, and `select` the driver(s)
   the sample needs so choosing it pulls them in.
4. Portable samples must use board aliases and the v1 `hal_*` API only.
