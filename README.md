
# NavHAL — NAVRobotec Hardware Abstraction Layer

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/github/actions/workflow/status/ragnar-vallhala/NavHAL/ci.yml?branch=main)](https://github.com/ragnar-vallhala/NavHAL/actions)
[![Coverage](https://img.shields.io/codecov/c/github/ragnar-vallhala/NavHAL/main)](https://codecov.io/gh/ragnar-vallhala/NavHAL)
[![Docs](https://img.shields.io/badge/docs-Doxygen-blue)](docs/html/index.html)
[![Stars](https://img.shields.io/github/stars/ragnar-vallhala/NavHAL)](https://github.com/ragnar-vallhala/NavHAL/stargazers)
[![Issues](https://img.shields.io/github/issues/ragnar-vallhala/NavHAL)](https://github.com/ragnar-vallhala/NavHAL/issues)


---

## Overview

**NavHAL** is a professional, cross-platform hardware abstraction layer (HAL) written in C, designed for embedded systems across multiple architectures. It is architecture-agnostic by design, enabling scalable and portable embedded software development.
Future extensions will provide C++ wrappers and RTOS integration to deliver a clean, modular, and extensible interface suitable for modern embedded projects.

NavHAL is developed and maintained by **NAVRobotec**, a company dedicated to innovative and robust embedded solutions.

---

## Key Features

* Architecture-independent HAL supporting multiple MCU families (ARM, AVR, RISC-V, etc.)
* Clean, minimal, and efficient C core for portability and performance
* Designed with modularity for easy extension and maintenance
* Planned C++ wrapper layer for object-oriented APIs
* Integration-ready with RTOS and bare-metal environments
* Professional-grade tooling with CMake build system and Doxygen documentation

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
mkdir build && cd build
cmake .. -DSAMPLE=no_hal_blink -DSTANDALONE=ON
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
cmake --build . --target doc_doxygen
```

Documentation will be available in `docs/html`.

---

## Project Structure

```
/
├── samples/               # Example samples
├── src/                   # Core HAL source files
├── include/               # HAL public headers
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
