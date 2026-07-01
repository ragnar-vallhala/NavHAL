@page capabilities Capability Matrix

# NavHAL — capability matrix

What the HAL contract (`NAVHAL_HAS_*`) reports for each supported MCU. Macro definitions and the contract semantics live in [`../api_standardization.md`](../api_standardization.md); this directory only tracks per-target availability and implementation status.

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

| Capability        | `NAVHAL_HAS_*`        | [STM32F401RE](stm32f401re.md) | [ATmega328P](atmega328p.md) | [STM32F767ZI](stm32f767zi.md) |
|---|---|---|---|---|
| GPIO              | `GPIO`                 | ✓ | ✓ | ✓ |
| UART              | `UART`                 | ✓ | ✓ | ◐ |
| I²C               | `I2C`                  | ✓ | ✓ | ◐ |
| SPI               | `SPI`                  | ✓ | ✓ | ◐ |
| Timer             | `TIMER`                | ✓ | ✓ | ✓ |
| PWM               | `PWM`                  | ✓ | ✓ | ✓ |
| Clock subsystem   | `CLOCK`                | ✓ | ◐ | ✓ |
| Interrupt ctrl    | `INTERRUPT`            | ✓ | ✓ | ✓ |
| Flash             | `FLASH`                | ✓ | ◐ | ✓ |
| Hardware CRC      | `CRC_HW`               | ✓ | s/w | ✓ |
| Cycle counter     | `CYCLE_COUNTER`        | ✓ | — | ✓ |
| FPU               | `FPU`                  | ✓ | — | ✓ |
| DMA controller    | `DMA`                  | ✓ | — | ✓ |
| SDIO              | `SDIO`                 | ✓ | — | ◐ |
| UART → DMA backend| `UART_DMA`             | ✓ | — | ✗ |
| I²C → DMA backend | `I2C_DMA`              | ✓ | — | ✗ |
| SDIO async (DMA)  | `SDIO_DMA`             | ✓ | — | ✗ |

A `✓` here is a statement about both *hardware presence* and *current driver completeness*. It does **not** mean the cap is on by default in the shipped Kconfig — most non-core caps default to `n` and must be selected explicitly. See `Kconfig` and each MCU's detail page for the default state and the `select` cascade.

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
