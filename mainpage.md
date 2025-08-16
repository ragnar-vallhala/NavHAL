/**
 @mainpage NavHAL Documentation
 # NavHAL — NAVRobotec Hardware Abstraction Layer

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE.md)
[![Build Status](https://img.shields.io/github/actions/workflow/status/ragnar-vallhala/NavHAL/ci.yml?branch=main)](https://github.com/ragnar-vallhala/NavHAL/actions)
[![Coverage](https://img.shields.io/codecov/c/github/ragnar-vallhala/NavHAL/main)](https://codecov.io/gh/ragnar-vallhala/NavHAL)
[![Docs](https://img.shields.io/badge/docs-Doxygen-blue)](docs/html/index.html)
[![Stars](https://img.shields.io/github/stars/ragnar-vallhala/NavHAL)](https://github.com/ragnar-vallhala/NavHAL/stargazers)
[![Issues](https://img.shields.io/github/issues/ragnar-vallhala/NavHAL)](https://github.com/ragnar-vallhala/NavHAL/issues)
[![GitHub contributors](https://img.shields.io/github/contributors/ragnar-vallhala/NavHAL)]
 
 NavHAL is a hardware abstraction layer for embedded systems,
 offering clean interfaces for GPIO, UART, and more.
 
 - Easy to port
 - Minimal dependencies
 - Lightweight and architecture-agnostic
 - C++ wrapper layer and RTOS integration planned


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
 3. Building Tests 

 ```
#=====BUILD TEST (example: clock_test)=====

mkdir build && cd build
cmake .. -DTEST=clock_test -DSTANDALONE=ON
cmake --build .
 ```

 ```
# =====BUILD ANOTHER TEST (example: systick_test)=====

mkdir build && cd build
cmake .. -DTEST=systick_test -DSTANDALONE=OFF
cmake --build .
ctest --output-on-failure
 ```

 4. Flash to your board:
 ```
 cmake --build . --target flash
 ```

 ## Supported Platforms:



### Microcontrollers: 
- STM32 (HAL & LL)
- Raspberry Pi Pico
- AVR ATmega series(planned)
- ESP32 (planned)

### Architectures: 


- ARM Cortex-M 
- AVR (planned)
- RISC-V (planned)
- Other 32-bit/8-bit MCUs (planned)


### Operating Modes: 
Bare-metal and RTOS-integrated environments

### Supported Toolchains:
 - GCC ARM
 - CMAKE-based builds

## Contribution & support

```
Contributions are welcome! Submit pull request, open issues, or suggest features.

Join discussions on GitHub or contact via email.
```
 ---
## Contributors

Special thanks to our contributors who made this possible:

<div style="display:flex;flex-wrap:wrap;gap:12px;align-items:center;"> <a href="https://github.com/ragnar-vallhala" title="ragnar-vallhala" target="_blank" rel="noopener noreferrer" style="display:inline-block;text-decoration:none;"> <img src="https://avatars.githubusercontent.com/ragnar-vallhala?s=128" alt="ragnar-vallhala" style="width:64px;height:64px;border-radius:50%;display:block;object-fit:cover;border:2px solid #e6e6e6;"> </a> <a href="https://github.com/nipunsingh27" title="nipunsingh27" target="_blank" rel="noopener noreferrer" style="display:inline-block;text-decoration:none;"> <img src="https://avatars.githubusercontent.com/nipunsingh27?s=128" alt="nipunsingh27" style="width:64px;height:64px;border-radius:50%;display:block;object-fit:cover;border:2px solid #e6e6e6;"> </a> </div>


---
 
 GitHub Link: https://ragnar-vallhala.github.io/NavHAL/


 