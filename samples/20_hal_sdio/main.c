/**
 * @file main.c
 * @brief Example application: Initialize SD card using SDIO in 4-bit mode.
 *
 * @details
 * - Initializes UART2 for logging.
 * - Configures SDIO for 4-bit mode and 400kHz clock (init phase).
 * - Sends CMD0 (GO_IDLE_STATE) to reset the card.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

/**
 * @brief PLL configuration for STM32F401RE (assuming 16MHz HSI)
 * Target: 84MHz SYSCLK, 48MHz SDIO
 * M=16 (Input 1MHz), N=336 (VCO 336MHz), P=4 (SYS 84MHz), Q=7 (48MHz)
 */
hal_pll_config_t pll_cfg = {.input_src = HAL_CLOCK_SOURCE_HSI,
                            .pll_m = 16,
                            .pll_n = 336,
                            .pll_p = 4,
                            .pll_q = 7};

hal_clock_config_t clk_cfg = {.source = HAL_CLOCK_SOURCE_PLL};

int main(void) {
  /* Initialize System Clocks (Required for SDIO 48MHz clock) */
  hal_clock_init(&clk_cfg, &pll_cfg);

  /* Initialize System Tick and UART2 for logging */
  systick_init(1000);
  uart2_init(115200);
  uart2_write_string("\n\r--- NavHAL SDIO Init Sample ---\n\r");

  /* SDIO Configuration:
   * Clock Div = 118 (for 400kHz from 48MHz SDIO Clock)
   * Bus Width = 0 (1-bit mode initially for card detection)
   */
  hal_sdio_config_t sd_config = {.clock_div = 118, .bus_width = 0};

  uart2_write_string("Initializing SDIO peripheral...\n\r");
  if (sdio_init(&sd_config) == HAL_SDIO_OK) {
    uart2_write_string("SDIO peripheral initialized.\n\r");
  } else {
    uart2_write_string("SDIO initialization failed!\n\r");
  }

  /* Send CMD0: GO_IDLE_STATE (Reset) */
  uart2_write_string("Sending CMD0 (Reset)...\n\r");
  hal_sdio_error_t err =
      sdio_send_command(0, 0x00, 0); // CMD0, Arg 0, No response

  if (err == HAL_SDIO_OK) {
    uart2_write_string("CMD0 sent successfully.\n\r");
  } else {
    uart2_write_string("CMD0 failed or timed out.\n\r");
  }

  uart2_write_string("SDIO test finished.\n\r");

  while (1) {
    /* Loop forever */
  }
}
