/**
 @mainpage NavHAL Documentation
 
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
 3. Flash to your board:
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
 
 
 GitHub Pages: https://ragnar-vallhala.github.io/NavHAL/


 