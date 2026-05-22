/**
 * @file main.c
 * @brief CRC Hardware Example for Cortex-M4 (STM32F4)
 *
 * Demonstrates how to initialize the CRC hardware peripheral, compute CRC32
 * in a single shot, and compute it incrementally across multiple buffers.
 */

#define CORTEX_M4
#include "navhal.h"
#include <stdint.h>

/**
 * @brief Simple delay function
 */
void delay(volatile uint32_t count) {
  while (count--) {
    __asm("nop");
  }
}

/**
 * @brief Helper function to print a 32-bit hex value to UART
 */
static void uart2_write_hex32(uint32_t val) {
  const char hex_digits[] = "0123456789ABCDEF";
  hal_uart_print(HAL_UART_2, "0x");
  for (int i = 28; i >= 0; i -= 4) {
    hal_uart_write_char(HAL_UART_2, hex_digits[(val >> i) & 0xF]);
  }
}

int main(void) {
  // 1. Initialize system clock and UART
  hal_clock_config_t clock_cfg = {
      .source = HAL_CLOCK_SOURCE_PLL,
      .hpre_div = RCC_CFGR_HPRE_DIV1,
      .ppre1_div = RCC_CFGR_PPRE_DIV2, // APB1 at 42MHz
      .ppre2_div = RCC_CFGR_PPRE_DIV1  // APB2 at 84MHz
  };

  hal_pll_config_t pll_cfg = {
      .input_src = HAL_CLOCK_SOURCE_HSI,
      .pll_m = 16,
      .pll_n = 336,
      .pll_p = 4, // Sysclk = 84MHz
      .pll_q = 7  // USB/SDIO/RNG clock
  };

  hal_clock_init(&clock_cfg, &pll_cfg);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});

  hal_uart_print(HAL_UART_2, "\r\n\r\n================================\r\n");
  hal_uart_print(HAL_UART_2, "NavHAL CRC Module Example\r\n");
  hal_uart_print(HAL_UART_2, "================================\r\n");

  // 2. Initialize CRC Unit
  crc_config_t crc_cfg = {.polynomial = CRC_POLY_CRC32, // Standard 0x04C11DB7
                          .init_value = 0xFFFFFFFF};

#ifdef _CRC_HW_ENABLED
  hal_uart_print(HAL_UART_2, "Mode: Hardware Accelerated\r\n\r\n");
#else
  hal_uart_print(HAL_UART_2, "Mode: Software Fallback\r\n\r\n");
#endif

  hal_crc_init(&crc_cfg);

  // 3. Single-shot computation
  hal_uart_print(HAL_UART_2, "1. Single-Shot Computation\r\n");
  const uint8_t message[] = "123456789";

  hal_uart_print(HAL_UART_2, "   Message: '123456789'\r\n");

  uint32_t single_crc = hal_crc_compute(message, 9);

  hal_uart_print(HAL_UART_2, "   CRC32 Result:   ");
  uart2_write_hex32(single_crc);
  hal_uart_print(HAL_UART_2, "\r\n   Expected:       0x0376E6E7\r\n");

  if (single_crc == 0x0376E6E7) {
    hal_uart_print(HAL_UART_2, "   Status:         PASS\r\n\r\n");
  } else {
    hal_uart_print(HAL_UART_2, "   Status:         FAIL\r\n\r\n");
  }

  // 4. Incremental / Chunked computation
  hal_uart_print(HAL_UART_2, "2. Incremental Computation (Chunked)\r\n");

  const uint8_t chunk1[] = "1234";
  const uint8_t chunk2[] = "567";
  const uint8_t chunk3[] = "89";

  hal_crc_reset(); // Reset internal accumulator to 0xFFFFFFFF

  uint32_t part1 = hal_crc_accumulate(chunk1, 4);
  uint32_t part2 = hal_crc_accumulate(chunk2, 3);
  uint32_t final_crc = hal_crc_accumulate(chunk3, 2);

  hal_uart_print(HAL_UART_2, "   Parts appended: '1234', '567', '89'\r\n");
  hal_uart_print(HAL_UART_2, "   Intermediate 1: ");
  uart2_write_hex32(part1);
  hal_uart_print(HAL_UART_2, "\r\n");
  hal_uart_print(HAL_UART_2, "   Intermediate 2: ");
  uart2_write_hex32(part2);
  hal_uart_print(HAL_UART_2, "\r\n");
  hal_uart_print(HAL_UART_2, "   Final CRC32:    ");
  uart2_write_hex32(final_crc);
  hal_uart_print(HAL_UART_2, "\r\n");

  if (final_crc == 0x0376E6E7) {
    hal_uart_print(HAL_UART_2, "   Status:         PASS\r\n\r\n");
  } else {
    hal_uart_print(HAL_UART_2, "   Status:         FAIL\r\n\r\n");
  }

  hal_uart_print(HAL_UART_2, "CRC Example Finished Successfully. Halting.\r\n");

  while (1) {
    // Main application loop
    delay(1000000);
  }

  return 0;
}
