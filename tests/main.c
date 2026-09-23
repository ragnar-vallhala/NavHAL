/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/hal_features.h"
#include "navhal_port_fpu.h"
#include "navhal_port_uart.h"
#include "navhal.h"
#include "navtest/navtest.h"
#include "navtest_target.h"

/* Test suite headers, grouped by tier:
 *   portable/   — run on every supported arch
 *   cap/<X>/    — run wherever NAVHAL_CONFIG_DRV_<X> is 1
 *   arch/<X>/   — run only on the matching arch (white-box, register pokes)
 */
#include "portable/test_crc.h"
#include "portable/test_flash_raw.h"
#include "portable/test_timebase.h"
#include "portable/conformance/test_conformance.h"
#include "portable/conformance/test_vtable.h"

#include "cap/boot/test_boot.h"
#include "cap/cache/test_cache.h"
#include "cap/dma/test_dma.h"
#include "cap/eth/test_eth.h"
#include "cap/i2c_dma/test_i2c_dma.h"
#include "cap/cycle_counter/test_dwt.h"
#include "cap/rtc/test_rtc.h"
#include "cap/fpu/test_fpu_accel.h"
#include "cap/mpu/test_mpu.h"
#include "cap/reset/test_reset.h"
#include "cap/sdio/test_sdio.h"
#include "cap/tcm/test_tcm.h"
#include "cap/uart_dma/test_uart_dma.h"
#include "cap/watchdog/test_watchdog.h"
#include "cap/usb_cdc/test_usb_cdc.h"

/* White-box, register-poke suites are per-processor. Only the Cortex-M4 set
 * exists today; a cortex-m7 build skips this tier (its registers differ — e.g.
 * the F7 USART) and runs the portable + cap + conformance tiers below. Add a
 * parallel NAVTEST_ARCH_CORTEX_M7 block when tests/arch/cortex-m7/ lands. */
#if defined(NAVTEST_ARCH_CORTEX_M4)
#include "arch/cortex-m4/test_adc.h"
#include "arch/cortex-m4/test_clock.h"
#include "arch/cortex-m4/test_gpio.h"
#include "arch/cortex-m4/test_i2c.h"
#include "arch/cortex-m4/test_interrupt.h"
#include "arch/cortex-m4/test_pwm.h"
#include "arch/cortex-m4/test_spi.h"
#include "arch/cortex-m4/test_timer.h"
#include "arch/cortex-m4/test_uart_protocol.h"
#endif

/* Cortex-M7 white-box suites: GPIO (incl. contiguous-port-indexing), TIMER,
 * CLOCK (clock_f7, HSI/PLL), INTERRUPT (NVIC + the F767 USART3 vector slot),
 * and UART PROTOCOL (F7 USART). The F7-6 buses (i2c/spi/pwm) are still to come. */
#if defined(NAVTEST_ARCH_CORTEX_M7)
#include "arch/cortex-m7/test_gpio.h"
#include "arch/cortex-m7/test_timer.h"
#include "arch/cortex-m7/test_clock.h"
#include "arch/cortex-m7/test_interrupt.h"
#include "arch/cortex-m7/test_uart_protocol.h"
#include "arch/cortex-m7/test_pwm.h"
#include "arch/cortex-m7/test_spi.h"
#include "arch/cortex-m7/test_i2c.h"
#include "arch/cortex-m7/test_adc.h"
#endif

static const navtest_suite_t *const all_suites[] = {
/* Cortex-M-only white-box suites — the underlying .c files are gated
 * the same way; this branch keeps the symbol references out of the
 * AVR-targeted link. Portable HAL-only test suites (timebase, crc,
 * flash) stay outside the gate so they run on every supported arch. */
#if defined(NAVTEST_ARCH_CORTEX_M4)
    &test_gpio_suite,
    &test_interrupt_suite,
    &test_timer_suite,
    &test_clock_suite,
    &test_pwm_suite,
    &test_uart_protocol_suite,
    &test_i2c_suite,
    &test_spi_suite,
    &test_adc_suite,
#endif
#if defined(NAVTEST_ARCH_CORTEX_M7)
    &test_gpio_suite,
    &test_timer_suite,
    &test_clock_suite,
    &test_interrupt_suite,
    &test_uart_protocol_suite,
    &test_pwm_suite,
    &test_spi_suite,
    &test_i2c_suite,
    &test_adc_suite,
#endif
    &test_conformance_suite,   /* portable HAL-contract assertions; runs
                                  on every arch (navtest PROGMEM keeps
                                  __FILE__/msg strings out of AVR .data). */
    &test_vtable_suite,        /* every linked ops table is fully filled in */
    &test_timebase_suite,
#if NAVHAL_CONFIG_BOOT_SNIFFER
    &test_boot_suite,
#endif
#if NAVHAL_CONFIG_DRV_DMA
    &test_dma_suite,
#endif
#if NAVHAL_CONFIG_DRV_UART_DMA
    &test_uart_dma_suite,
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
    &test_i2c_dma_suite,
#endif
#if NAVHAL_CONFIG_DRV_ETH
    &test_eth_suite,
#endif
    &test_crc_suite,
#if NAVHAL_CONFIG_DRV_DWT
    &test_dwt_suite,
#endif
#if NAVHAL_CONFIG_DRV_RTC
    &test_rtc_suite,
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
    &test_usb_cdc_suite,
#endif
#if NAVHAL_CONFIG_DRV_RESET
    &test_reset_suite,
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
    &test_watchdog_suite,
#endif
#if NAVHAL_CONFIG_USE_FPU
    &test_fpu_suite,
#endif
#if NAVHAL_CONFIG_DRV_MPU
    &test_mpu_suite,
#endif
#if NAVHAL_CONFIG_DRV_CACHE
    &test_cache_suite,
#endif
#if NAVHAL_CONFIG_USE_TCM
    &test_tcm_suite,
#endif
    &test_flash_suite,
#if NAVHAL_CONFIG_DRV_SDIO
    &test_sdio_suite,
#endif
};

static void print_startup_message(void) {
  hal_uart_write_char(NAVTEST_UART, 0x1B); // ESC
  hal_uart_write_char(NAVTEST_UART, '[');
  hal_uart_write_char(NAVTEST_UART, '2');
  hal_uart_write_char(NAVTEST_UART, 'J');

  hal_uart_write_char(NAVTEST_UART, 0x1B); // ESC
  hal_uart_write_char(NAVTEST_UART, '[');
  hal_uart_write_char(NAVTEST_UART, 'H');
  /* _NT_PSTR keeps the banner in flash on AVR: a plain literal is copied into
   * SRAM at startup, and on a 2 KB part a quarter kilobyte of box drawing is
   * not a good use of it. */
  navtest_write_P(_NT_PSTR("\r\n"
                           "|========================================|\r\n"
                           "|    NAVrobotec Private Limited          |\r\n"
                           "|          Project: NAVHAL               |\r\n"
                           "|     Starting Unit Tests...             |\r\n"
                           "|========================================|\r\n"));
}

int main(void) {
#if NAVHAL_CONFIG_DRV_CACHE
  hal_icache_enable(); /* hazard-free perf win; do it before anything else */
  /* Invalidates then enables. The DMA drivers maintain their own buffers:
   * eth cleans and invalidates its descriptors and frames, sdio and i2c go
   * through navhal_dma_{tx_prepare,rx_guard,rx_finish}, and uart does the same
   * plus hal_uart_dma_rx_sync for its ring. */
  hal_dcache_enable();
#endif
  hal_uart_init(NAVTEST_UART, &(hal_uart_config_t){.baudrate=9600});
#if NAVHAL_CONFIG_USE_FPU
  hal_fpu_enable();
#endif
  print_startup_message();

  int failed = 0;
  uint32_t total_tests = 0;
  const size_t n_suites = sizeof(all_suites) / sizeof(all_suites[0]);
  for (size_t i = 0; i < n_suites; i++) {
    failed += navtest_run_suite(all_suites[i]);
    total_tests += all_suites[i]->count;
  }

  navtest_write_P(_NT_PSTR("\n\n=========== FINAL RESULTS ===========\n\n"));
  navtest_write_P(_NT_PSTR("Total tests run: "));
  _navtest_print_uint32(total_tests);
  navtest_write_P(_NT_PSTR("\nTotal failures:  "));
  _navtest_print_uint32((uint32_t)failed);
  navtest_write_P(_NT_PSTR("\n"));

  return failed;
}
