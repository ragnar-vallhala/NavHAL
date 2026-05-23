
# NavHAL — NAVRobotec Hardware Abstraction Layer

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/github/actions/workflow/status/ragnar-vallhala/NavHAL/ci.yml?branch=main)](https://github.com/ragnar-vallhala/NavHAL/actions)
[![Coverage](https://img.shields.io/codecov/c/github/ragnar-vallhala/NavHAL/main)](https://codecov.io/gh/ragnar-vallhala/NavHAL)
[![Docs](https://img.shields.io/badge/docs-Doxygen-blue)](docs/html/index.html)
[![Stars](https://img.shields.io/github/stars/ragnar-vallhala/NavHAL)](https://github.com/ragnar-vallhala/NavHAL/stargazers)
[![Issues](https://img.shields.io/github/issues/ragnar-vallhala/NavHAL)](https://github.com/ragnar-vallhala/NavHAL/issues)


---

## Overview

**NavHAL** is a hardware abstraction layer (HAL) written in C for embedded
systems. Its API is architecture-agnostic *by design* — the layered
arch / vendor / family / board structure and the Kconfig-driven build are
built to host multiple targets — but the v1 release ships **one implemented
port: ARM Cortex-M4 / STM32F4, on the Nucleo-F401RE.** An AVR / ATmega328P
port is the next milestone (M6); the Kconfig schema already models it.

The public `hal_*` API is frozen at `HAL_API_VERSION 1` (see
[`docs/api_standardization.md`](docs/api_standardization.md)).

NavHAL is developed and maintained by **NAVRobotec**, a company dedicated to innovative and robust embedded solutions.

---

## Key Features

* Standardized, versioned `hal_*` C API (`HAL_API_VERSION 1`) — frozen contract
* Implemented today on ARM Cortex-M4 / STM32F4 (Nucleo-F401RE)
* Layered arch / vendor / family / board source tree, Kconfig-driven build —
  designed so a new port adds directories, not build-system changes
* Clean, minimal, and efficient C core for portability and performance
* C++-compatible headers (`extern "C"`-guarded); a C++ sample builds on-target
* Comprehensive unit-test suite (host subset + on-target runs)
* Professional-grade tooling with a CMake build system and Doxygen documentation

---

## Project Brief

> *A cross-platform hardware abstraction layer for embedded systems, enabling scalable and architecture-agnostic development.*

---

## Getting Started

### Prerequisites

* ARM GCC Toolchain (`arm-none-eabi-gcc`) or other toolchains depending on target architecture
* CMake (version 3.15 or higher recommended)
* Ninja or Make build system
* Flashing tools (e.g., `st-flash` for STM32 boards)
* Doxygen (for generating documentation)

### Building a Sample

```bash
# =====BUILD NO HAL VERSION=====

mkdir build && cd build
cmake .. -DSAMPLE=no_hal_blink -DSTANDALONE=ON
cmake --build .
```

```bash
#=====BUILD HAL VERSION=====

mkdir build && cd build
cmake .. -DSAMPLE=hal_blink -DSTANDALONE=OFF
cmake --build .
```
### Flashing Firmware

```bash
cmake --build . --target flash
```

---

## Documentation

Generate API documentation using:

```bash
cmake --build . --target doc
```

Documentation will be available in `docs/html`.

Other docs in this repository:

- [`docs/api_standardization.md`](docs/api_standardization.md) — the v1 public-API contract.
- [`docs/execution_plan.md`](docs/execution_plan.md) — milestone breakdown (M0 → M6, AVR port).
- [`docs/m2_plus_plan.md`](docs/m2_plus_plan.md) — M2+ test-suite execution plan.
- [`docs/testing/`](docs/testing/README.md) — test report (SIL/PIL/HIL framework,
  coverage matrix, findings from real-board runs) and the build/run how-to.

---

## Project Structure

```
/
├── samples/               # Example samples
├── src/                   # Core HAL source files
├── include/               # HAL public headers
├── cmake                  # cmake build configuration 
├── datasheets             # reference datasheets
├── tests                  # unit tests
├── CMakeLists.txt         # Build configuration
├── Doxyfile               # Doxygen config
└── README.md              # This file

```

---

## Contributing

Contributions, bug reports, and feature requests are welcome. Please follow the coding style and submit pull requests.

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

## Contact

NAVRobotec — Man Meets Machine

Email: [support@navrobotec.com](mailto:support@navrobotec.com)

Website: [https://navrobotec.com](https://navrobotec.com)

---

Thank you for exploring NavHAL — the future of portable embedded hardware abstraction!
