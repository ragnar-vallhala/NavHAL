@mainpage NavHAL™ Documentation

**NavHAL™** is a hardware abstraction layer for embedded systems, offering a standardized, frozen C interface for GPIO, UART, I²C, SPI, timers, PWM, DMA, flash, CRC, SDIO, FPU and a cycle counter. The public `hal_*` API is at `HAL_API_VERSION 1`.

The codebase is laid out so adding a new MCU adds directories, not build-system changes — arch / vendor / family / board are explicit Kconfig axes. v1 ships two implemented ports: **STM32F401RE (Cortex-M4)** and **ATmega328P (AVR)**.

## Quick links

| | |
|---|---|
| **Drivers, types, utilities** | See the **Topics** tab (HAL Drivers → GPIO / UART / I²C / …) |
| **Source files** | See the **Files** tab |
| **Per-MCU capability table** | @ref capabilities |
| **How to build & flash a sample** | @ref contributing "Contributing — Building" |
| **How `nav` ecosystem modules interoperate** | @ref module_abi |

## Getting started

```bash
git clone https://github.com/ragnar-vallhala/NavHAL.git
cd NavHAL
cmake -B build -DSAMPLE=hal_blink
cmake --build build
cmake --build build --target flash    # Nucleo-F401RE connected
```

For AVR: pass `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-toolchain.cmake`. Full guide in @ref contributing.

## Documentation map

The Related Pages tab is structured top-down — start here:

- **Project contribution**
  - @subpage contributing
  - @subpage code_of_conduct

- **API reference**
  - @subpage api_standardization
  - @subpage naming_conventions
  - @subpage module_abi

- **Per-target capabilities**
  - @subpage capabilities

- **Testing & verification**
  - @subpage testing

- **Project planning (historical)**
  - @subpage exec_plan
  - @subpage m2_plus_plan
  - @subpage m5_avr_readiness
  - @subpage m5_conformance

## Supported toolchains

- `arm-none-eabi-gcc` for the Cortex-M4 / STM32F4 port (`apt install gcc-arm-none-eabi`).
- `avr-gcc` for the AVR / ATmega328P port (`apt install gcc-avr avr-libc`).
- CMake ≥ 3.20, Kconfig (via `pip install kconfiglib`).

GitHub Pages: https://ragnar-vallhala.github.io/NavHAL/ &nbsp;·&nbsp;
Repo: https://github.com/ragnar-vallhala/NavHAL
