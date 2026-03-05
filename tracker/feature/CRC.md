# CRC Module Implementation

## Overview
The CRC module provides cyclic redundancy check functionality for the NavHAL framework. It includes both hardware-accelerated CRC (for STM32F4) and a software fallback implementation, ensuring portability and optimal performance depending on the system configuration.

## Architecture
The module follows the NavHAL standard 3-layer architecture:
1. **Utility Types**: `include/utils/crc_types.h` defines the `crc_polynomial_t` and `crc_config_t` structs.
2. **Arch Definitions**: `include/core/cortex-m4/crc_reg.h` defines the STM32F4 memory-mapped registers for the CRC peripheral (Base address `0x40023000`).
3. **Arch Interface**: `include/core/cortex-m4/crc.h` provides the function prototypes for Cortex-M4.
4. **Common Dispatch API**: `include/common/hal_crc.h` acts as the portable inclusion point for the module across architectures.

## Features & Implementation Details
- **Hardware Acceleration**: When `_CRC_HW_ENABLED` is defined, the module utilizes the built-in STM32F4 CRC32 block. The hardware unit calculates the standard CRC-32 (Ethernet/MPEG-2) with polynomial `0x04C11DB7`.
- **Software Fallback**: A pure C table-driven software implementation is provided when hardware CRC is disabled. It uses a 256-entry lookup table to calculate the identical CRC-32/MPEG-2 checksum.
- **Data Alignment**: The STM32F4 hardware CRC peripheral strictly accepts 32-bit words. The implementation of `hal_crc_accumulate` correctly buffers and packs unaligned bytes or lengths that are not multiples of 4 into 32-bit words before writing them to the `CRC->DR` register.

## Core API
- `void hal_crc_init(crc_config_t *config)`: Initializes the CRC module/peripheral.
- `uint32_t hal_crc_compute(const uint8_t *buffer, uint32_t length)`: Resets the CRC accumulator and computes the CRC for the given buffer.
- `uint32_t hal_crc_accumulate(const uint8_t *buffer, uint32_t length)`: Appends the buffer contents to the ongoing CRC accumulation.
- `void hal_crc_reset(void)`: Resets the internal CRC ongoing accumulator back to the initial value (typically `0xFFFFFFFF`).

## Testing
The module is fully validated with the `navtest` framework (`tests/test_crc.c`).
Tests include:
- `test_crc_empty_returns_init`: Ensures that accumulating zero bytes returns the initialization value.
- `test_crc_single_byte`: Validates CRC over a 1-byte payload.
- `test_crc_known_vector`: Validates CRC over standard ASCII string `"123456789"`, asserting it matches the expected vector `0x0376E6E7`.
- `test_crc_accumulate_matches_compute`: Ensures chunked accumulation yields the identical result as computing the whole chunk at once.
- `test_crc_reset_restores_init`: Ensures `hal_crc_reset` successfully returns the state to empty.
