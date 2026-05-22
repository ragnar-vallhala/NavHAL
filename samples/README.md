# NavHAL samples

Samples are organised into three tiers by what they need from the hardware.
Build any of them with `-DSAMPLE=<short-name>` (see the table below); the
build resolves the short name to its tier directory.

## Tiers

| Tier | Directory | What it is | Targets |
|------|-----------|------------|---------|
| **Portable** | `samples/portable/` | HAL samples that use only peripherals every NavHAL port has (GPIO, UART, timer, clock, software CRC). The *same source* is meant to build for every target via the board-layer aliases (`LED_BUILTIN`, `BOARD_CONSOLE_UART`). | Cortex-M4 + AVR |
| **Cortex-M** | `samples/cortex-m/` | HAL samples that need an optional capability the ATmega328P lacks — DMA, hardware FPU, the DWT cycle counter, or SDIO. Their Kconfig entries `depend on ARCH_CORTEX_M4`, so they are simply not offered on AVR. | Cortex-M4 only |
| **No-HAL** | `samples/no_hal/` | Register-level teaching samples that bypass the HAL entirely. Inherently per-MCU (current set is STM32F4). | Cortex-M4 only |

## Portable tier — AVR migration status

A portable sample is *architecturally* portable, but its source must use the
board aliases (not `GPIO_PA05` / `HAL_UART_2`) and the matching AVR driver
must exist. Migration is incremental:

- **Verified on AVR** (board-alias source, ATmega328P driver done):
  `hal_blink`, `hal_uart_tx`.
- **Pending** — awaiting board-alias source migration and/or the remaining
  ATmega328P drivers (timer, pwm, i2c, spi, flash — see execution-plan
  WI6.2): `hal_pupd`, `hal_timer`, `hal_systick`, `hal_blink_delay`,
  `hal_pwm`, `hal_clock`, `hal_i2c`, `hal_uart_int`, `hal_flash`, `hal_crc`,
  `hal_spi_esp_bridge`, `hal_blink_cpp`.

These build for Cortex-M4 today; the AVR column flips to verified as each
is migrated.

## Sample index

### Portable (`samples/portable/`)
`hal_blink` · `hal_pupd` · `hal_uart_tx` · `hal_timer` · `hal_systick` ·
`hal_blink_delay` · `hal_pwm` · `hal_clock` · `hal_i2c` · `hal_flash` ·
`hal_uart_int` · `hal_crc` · `hal_spi_esp_bridge` · `hal_blink_cpp`

### Cortex-M (`samples/cortex-m/`)
| Sample | Needs |
|--------|-------|
| `hal_uart_dma`, `hal_dma_polling_uart`, `hal_dma_i2c`, `hal_uart_dma_bridge` | DMA |
| `hal_fpu` | hardware FPU |
| `hal_dwt` | DWT cycle counter |
| `hal_sdio`, `hal_sdio_block`, `hal_sdio_perf`, `hal_fatfs_posix` | SDIO |

### No-HAL (`samples/no_hal/`)
`no_hal_blink` · `no_hal_uart_tx` · `no_hal_pwm` · `no_hal_i2c` ·
`no_hal_flash`

## Adding a sample

1. Create `samples/<tier>/<NN_name>/` with `main.c` + `CMakeLists.txt`.
2. Add the short name + tier-relative path to the parallel lists in
   `samples/CMakeLists.txt`.
3. Add a `config SAMPLE_<NN>_<NAME>` entry to `Kconfig`; for a Cortex-M or
   No-HAL sample add `depends on ARCH_CORTEX_M4`.
4. Portable samples must use board aliases and the v1 `hal_*` API only.
