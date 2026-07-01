@page capabilities Capability Matrix

# NavHAL — capability matrix

What the HAL capability contract reports for each supported MCU. The canonical gate is `NAVHAL_CONFIG_DRV_*` (a 1:1 mirror of Kconfig, force-included into every TU); the `NAVHAL_HAS_*` names below are the **deprecated** aliases kept for out-of-tree consumers. Macro definitions and the contract semantics live in [`../api_standardization.md`](../api_standardization.md); this directory only tracks per-target availability and implementation status.

**Per-MCU detail pages:** @subpage cap_stm32f401re &nbsp;·&nbsp; @subpage cap_atmega328p &nbsp;·&nbsp; @subpage cap_stm32f767zi

## Symbol legend

| Symbol | Meaning |
|---|---|
| ✓     | Hardware supports it AND the driver is implemented; `NAVHAL_HAS_X == 1` in the default config. |
| ◐     | Hardware supports it but the driver is partial / has documented caveats (see the per-MCU page). |
| —     | Hardware doesn't have the peripheral; `NAVHAL_HAS_X == 0`. Symbols are absent at link time. |
| ✗     | Hardware *does* have it but the driver isn't implemented yet. Treated the same as `—` at compile time. |
| s/w   | No hardware peripheral, but the public `hal_*` API is satisfied by a software fallback (only `hal_crc_*` today). |

## Matrix

This is a **silicon + driver** view: every row is a hardware block one of the
MCUs carries, whether or not NavHAL drives it yet. A `NAVHAL_HAS_*` macro exists
only for the blocks NavHAL actually exposes (the `✓`/`◐`/`s/w` rows); silicon
that has no driver yet shows `✗` and carries no macro (`—` in that column).

| Class | Capability | `NAVHAL_HAS_*` | [F401RE](stm32f401re.md) | [ATmega328P](atmega328p.md) | [F767ZI](stm32f767zi.md) |
|---|---|---|---|---|---|
| Core   | Interrupt ctrl (NVIC)     | `INTERRUPT`     | ✓ | ✓ | ✓ |
| Core   | Cycle counter (DWT)       | `CYCLE_COUNTER` | ✓ | — | ✓ |
| Core   | FPU                       | `FPU`           | ✓ † | — | ✓ † |
| Core   | MPU (memory protection) § | `MPU`           | ✓ (8-region) | — | ✓ (16-region) |
| Core   | L1 I-cache / D-cache      | `CACHE`         | — | — | ◐ |
| Core   | DTCM / ITCM               | `TCM`           | — | — | ✓ |
| System | Clock subsystem           | `CLOCK`         | ✓ | ◐ | ✓ |
| System | Flash (KV store)          | `FLASH`         | ✓ | ◐ | ✓ |
| System | DMA controller            | `DMA`           | ✓ | — | ✓ |
| System | Hardware CRC              | `CRC_HW`        | ✓ | s/w | ✓ |
| System | RTC                       | *(none)*        | ✗ | — | ✗ |
| I/O    | GPIO                      | `GPIO`          | ✓ | ✓ | ✓ |
| I/O    | Timer                     | `TIMER`         | ✓ | ✓ | ✓ |
| I/O    | PWM                       | `PWM`           | ✓ | ✓ | ✓ |
| Bus    | UART                      | `UART`          | ✓ | ✓ | ✓ |
| Bus    | UART → DMA backend        | `UART_DMA`      | ✓ | — | ✓ |
| Bus    | I²C                       | `I2C`           | ✓ | ✓ | ◐ |
| Bus    | I²C → DMA backend         | `I2C_DMA`       | ✓ | — | ◐ ¶ |
| Bus    | SPI                       | `SPI`           | ✓ | ✓ | ◐ |
| Bus    | SDIO / SDMMC              | `SDIO`          | ✓ (1×) | — | ◐ (2×) |
| Bus    | SDIO async (DMA)          | `SDIO_DMA`      | ✓ | — | ◐ ¶ |
| Bus    | USB OTG FS                | *(none)*        | ✗ | — | ✗ |
| Bus    | USB OTG HS                | *(none)*        | — | — | ✗ |
| Bus    | Ethernet MAC (10/100)     | *(none)*        | — | — | ✗ |
| Bus    | CAN (bxCAN)               | *(none)*        | — | — | ✗ (3×) |
| Bus    | QUAD-SPI                  | *(none)*        | — | — | ✗ |
| Bus    | FMC (ext-memory ctrl)     | *(none)*        | — | — | ✗ |
| Bus    | SAI (serial audio)        | *(none)*        | — | — | ✗ (2×) |
| Bus    | SPDIFRX                   | *(none)*        | — | — | ✗ |
| Analog | ADC                       | *(none)*        | ✗ (1×12-bit) | ✗ (10-bit) | ✗ (3×12-bit) |
| Analog | DAC                       | *(none)*        | — | — | ✗ (2-ch) |
| Video  | DCMI (camera)             | *(none)*        | — | — | ✗ |
| Video  | LTDC (LCD-TFT)            | *(none)*        | — | — | ✗ |
| Video  | DMA2D (Chrom-ART)         | *(none)*        | — | — | ✗ |
| Crypto | RNG (true RNG)            | *(none)*        | — | — | ✗ |

A `✓` is a statement about both *hardware presence* and *current driver completeness*. It does **not** mean the cap is on by default in the shipped Kconfig — most non-core caps default to `n` and must be selected explicitly. See `Kconfig` and each MCU's detail page for the default state and the `select` cascade.

Most `✗` rows are silicon NavHAL simply hasn't scoped a driver for yet (roadmap, not a commitment). The `HASH`/AES crypto block is intentionally absent from the table — none of these three MCUs carry it (it lives only on the STM32F777/F779 crypto parts).

`†` The FPU **module** is shared across the M4 and M7 ports; the FPU
*precision/variant* differs by core — see each MCU's page.

`§` MPU support is a shared ARMv7-M core driver (`hal_mpu`), gated on
`NAVHAL_CONFIG_DRV_MPU` (deprecated alias `NAVHAL_HAS_MPU`). Per-core region
counts and validation status are on each MCU's page.

`¶` **Implemented but not yet hardware-validated.** The DMA backend is compiled
and register-correct by inspection, but the bench has no device to prove a
transfer (no I²C sensor / SD card), and Renode does not model the peripheral→DMA
request path (the polled path works there; the DMA path times out). It awaits
validation on a wired rig.

The **L1 cache** (`CACHE`) is Cortex-M7 only: `hal_cache` drives the instruction
cache (`hal_icache_enable`); the data cache is a later phase (it needs
clean/invalidate maintenance), so the F767 cell is `◐`. Detail on the
[STM32F767ZI page](stm32f767zi.md).

**DTCM / ITCM** (`TCM`) is Cortex-M7 only: `common/hal_tcm.h` exposes explicit
placement attributes (`NAVHAL_ITCM` / `NAVHAL_DTCM` / `NAVHAL_DTCM_NOINIT`, opt-in
`CONFIG_USE_TCM`) that pin code/data into the 0-wait TCMs, with the reset-time
copy in the board startup — complete and hardware-validated. There is no dynamic
TCM allocator / heap-in-TCM (placement is static, by attribute). Detail on the
[STM32F767ZI page](stm32f767zi.md).

> **Accuracy note:** the silicon-presence cells (MPU regions, ADC/DAC/CAN/SAI
> counts, USB HS/FS, Ethernet, DCMI/LTDC/DMA2D, QUAD-SPI, FMC, SPDIFRX, RNG)
> come from the ST product datasheets — F767: 3× CAN, 3× 12-bit ADC (24 ch),
> 2× DAC, 2× SAI, MPU present; F401: 1× ADC, USB FS, SDIO, MPU, and *no*
> CAN/Ethernet/DAC/RNG.

## Adding a new MCU

1. Copy `_template.md` → `docs/capabilities/<board>.md`. Fill in the metadata, the per-capability rows, and any caveats. Each capability row must justify its symbol — link to the datasheet section, name the driver file, or cite the limitation.
2. Add a column to the matrix above (right of the existing columns).
3. Reference the new page from any existing rows that gain a status (e.g. partial-implementation footnotes).
4. Add the MCU's defconfig under `cmake/defconfigs/` and the toolchain file under `cmake/toolchains/` (see [CONTRIBUTING.md](../../CONTRIBUTING.md#adding-a-new-port-eg-new-mcu-family)).
5. Wire the build into CI: extend `.github/workflows/ci.yml` with a `Build all <arch> samples` job (template: the existing `sample-matrix-avr` job).
6. The matrix must agree with what `navhal_target.h` actually emits for that MCU — spot-check by running `cmake -B b -DCMAKE_TOOLCHAIN_FILE=<toolchain>` and grepping `NAVHAL_HAS_` in `b/navhal_target.h`.

## Keeping the matrix honest

The values here are not auto-generated. If a driver lands or a peripheral gets exposed, the matrix has to be updated by hand. Two places where it's most likely to drift:

* A new driver in `src/vendor/<vendor>/<peripheral>/` that flips a `✗` to `✓`.
* A Kconfig `select` change that flips a default — though this affects the *default `.config`*, not what the matrix says (the matrix is hardware-and-implementation, not config defaults).

A future enhancement is generating the per-MCU column from `navhal_target.h` directly. Not done yet — the manual matrix is also a useful place for caveats and footnotes that a generated table couldn't carry.
