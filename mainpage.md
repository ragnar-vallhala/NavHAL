/**
 @mainpage NavHAL Documentation
 
 NavHAL is a hardware abstraction layer for embedded systems,
 offering a standardized C interface for GPIO, UART, timers, and more.

 The public `hal_*` API is frozen at `HAL_API_VERSION 1`
 (see `docs/api_standardization.md`).

 - Standardized, versioned API contract
 - Minimal dependencies
 - Lightweight; layered design built to host multiple ports
 - C++-compatible headers (`extern "C"` guarded)


 ## Getting Started

 1. Clone the repository:
 ```
 git clone https://github.com/ragnar-vallhala/NavHAL.git
 ```
 2. Build a sample:
 ```
 # =====BUILD NO HAL VERSION=====

mkdir build && cd build
cmake .. -DSAMPLE=no_hal_blink -DSTANDALONE=ON
cmake --build .
 ```
 ```
#=====BUILD HAL VERSION=====

mkdir build && cd build
cmake .. -DSAMPLE=hal_blink -DSTANDALONE=OFF
cmake --build .
 ```
 3. Flash to your board:
 ```
 cmake --build . --target flash
 ```

 ## Supported Platforms

### Implemented (v1)
- **Board:** ST Nucleo-F401RE
- **MCU family:** STM32F4 (STM32F401RE)
- **Architecture:** ARM Cortex-M4 (ARMv7E-M)

### Planned
- AVR / ATmega328P — next milestone (M6); the Kconfig schema already
  models `ARCH_AVR8` / `VENDOR_MICROCHIP` / `BOARD_ATMEGA328P`.

### Operating Modes
Bare-metal.

### Supported Toolchains
 - `arm-none-eabi-gcc` (GCC ARM)
 - CMake-based builds, configured via Kconfig

## Contribution & support

```
Contributions are welcome! Submit pull request, open issues, or suggest features.

Join discussions on GitHub or contact via email.
```
 
 
 GitHub Pages: https://ragnar-vallhala.github.io/NavHAL/


 