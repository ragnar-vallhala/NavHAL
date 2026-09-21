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

#include "test_vtable.h"

#include "common/hal_features.h"

#if NAVHAL_CONFIG_DRV_ADC
#include "internal/hal_adc_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
#include "internal/hal_clock_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_CRC
#include "internal/hal_crc_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_FLASH
#include "internal/hal_flash_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_GPIO
#include "internal/hal_gpio_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_I2C
#include "internal/hal_i2c_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA
#include "internal/hal_i2c_dma_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
#include "internal/hal_interrupt_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_PWM
#include "internal/hal_pwm_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_RESET
#include "internal/hal_reset_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_SPI
#include "internal/hal_spi_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
#include "internal/hal_timebase_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_TIMER
#include "internal/hal_timer_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_UART
#include "internal/hal_uart_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA
#include "internal/hal_uart_dma_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
#include "internal/hal_watchdog_ops.h"
#endif
#if NAVHAL_CONFIG_DRV_WWDG
#include "internal/hal_wwdg_ops.h"
#endif

#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Every ops table is a struct of function pointers and nothing else, so a
 * complete one contains no null word. Walking the bytes is what makes this
 * suite worth having: a table that grows a seventeenth entry is covered the
 * day it grows, not the day someone remembers to come back here and name it.
 *
 * That works only because optional capabilities are their own tables --
 * DRV_WWDG, DRV_UART_DMA, DRV_I2C_DMA are siblings, not nullable entries in
 * their parent. If a nullable entry is ever added to a table, this suite is
 * the thing it breaks, and the design decision is what should be revisited.
 *
 * Function pointers are read through memcpy rather than a cast: they are not
 * object pointers, and the alignment a cast would assume is not guaranteed
 * to hold for every member of every table on every target.
 * ------------------------------------------------------------------------- */
typedef void (*_vt_fn_t)(void);

static bool _vtable_is_complete(const void *table, size_t bytes) {
  const unsigned char *p = (const unsigned char *)table;

  /* A table whose size is not a whole number of function pointers is not a
   * table of function pointers -- fail rather than quietly skip the tail. */
  if (bytes == 0u || (bytes % sizeof(_vt_fn_t)) != 0u) {
    return false;
  }

  for (size_t off = 0u; off < bytes; off += sizeof(_vt_fn_t)) {
    _vt_fn_t fn;
    memcpy(&fn, p + off, sizeof fn);
    if (fn == NULL) {
      return false;
    }
  }
  return true;
}

#define ASSERT_TABLE_COMPLETE(tbl)                                             \
  TEST_ASSERT_TRUE(_vtable_is_complete(&(tbl), sizeof(tbl)))

/* Each table gets its own case so a failure names the port's gap in the test
 * name itself, which is the whole diagnostic a NULL entry otherwise denies
 * you. Gates mirror the shared layer's: a table is asserted exactly when the
 * build links the driver that defines it. */

#if NAVHAL_CONFIG_DRV_ADC
void test_vtable_adc_is_complete(void) { ASSERT_TABLE_COMPLETE(_hal_adc_ops); }
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
void test_vtable_clock_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_clock_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_CRC
void test_vtable_crc_is_complete(void) { ASSERT_TABLE_COMPLETE(_hal_crc_ops); }
#endif
#if NAVHAL_CONFIG_DRV_FLASH
void test_vtable_flash_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_flash_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_GPIO
void test_vtable_gpio_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_gpio_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_I2C
void test_vtable_i2c_is_complete(void) { ASSERT_TABLE_COMPLETE(_hal_i2c_ops); }
#endif
#if NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA
void test_vtable_i2c_dma_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_i2c_dma_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
void test_vtable_interrupt_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_interrupt_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_PWM
void test_vtable_pwm_is_complete(void) { ASSERT_TABLE_COMPLETE(_hal_pwm_ops); }
#endif
#if NAVHAL_CONFIG_DRV_RESET
void test_vtable_reset_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_reset_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_SPI
void test_vtable_spi_is_complete(void) { ASSERT_TABLE_COMPLETE(_hal_spi_ops); }
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
void test_vtable_timebase_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_timebase_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_TIMER
void test_vtable_timer_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_timer_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_UART
void test_vtable_uart_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_uart_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA
void test_vtable_uart_dma_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_uart_dma_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
void test_vtable_watchdog_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_watchdog_ops);
}
#endif
#if NAVHAL_CONFIG_DRV_WWDG
void test_vtable_wwdg_is_complete(void) {
  ASSERT_TABLE_COMPLETE(_hal_wwdg_ops);
}
#endif

/* The detector has to be able to fail, or this suite is 17 assertions that
 * only ever say yes. A stand-in table with a hole in it proves it doesn't. */
void test_vtable_detects_a_hole(void) {
  struct {
    _vt_fn_t a;
    _vt_fn_t b;
    _vt_fn_t c;
  } filled = {test_vtable_detects_a_hole, test_vtable_detects_a_hole,
              test_vtable_detects_a_hole},
    holed = {test_vtable_detects_a_hole, NULL, test_vtable_detects_a_hole};

  TEST_ASSERT_TRUE(_vtable_is_complete(&filled, sizeof filled));
  TEST_ASSERT_FALSE(_vtable_is_complete(&holed, sizeof holed));
  TEST_ASSERT_FALSE(_vtable_is_complete(&filled, 0u));
}

#if NAVHAL_CONFIG_DRV_ADC
NAVTEST_CASE_DECL(test_vtable_adc_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
NAVTEST_CASE_DECL(test_vtable_clock_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_CRC
NAVTEST_CASE_DECL(test_vtable_crc_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_FLASH
NAVTEST_CASE_DECL(test_vtable_flash_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_GPIO
NAVTEST_CASE_DECL(test_vtable_gpio_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_vtable_i2c_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA
NAVTEST_CASE_DECL(test_vtable_i2c_dma_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
NAVTEST_CASE_DECL(test_vtable_interrupt_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_PWM
NAVTEST_CASE_DECL(test_vtable_pwm_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_RESET
NAVTEST_CASE_DECL(test_vtable_reset_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_vtable_spi_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
NAVTEST_CASE_DECL(test_vtable_timebase_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_vtable_timer_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_vtable_uart_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA
NAVTEST_CASE_DECL(test_vtable_uart_dma_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
NAVTEST_CASE_DECL(test_vtable_watchdog_is_complete);
#endif
#if NAVHAL_CONFIG_DRV_WWDG
NAVTEST_CASE_DECL(test_vtable_wwdg_is_complete);
#endif
NAVTEST_CASE_DECL(test_vtable_detects_a_hole);

static const navtest_case_t vtable_cases[] = {
    NAVTEST_CASE(test_vtable_detects_a_hole),
#if NAVHAL_CONFIG_DRV_ADC
    NAVTEST_CASE(test_vtable_adc_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
    NAVTEST_CASE(test_vtable_clock_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_CRC
    NAVTEST_CASE(test_vtable_crc_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_FLASH
    NAVTEST_CASE(test_vtable_flash_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_GPIO
    NAVTEST_CASE(test_vtable_gpio_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_vtable_i2c_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA
    NAVTEST_CASE(test_vtable_i2c_dma_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
    NAVTEST_CASE(test_vtable_interrupt_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_PWM
    NAVTEST_CASE(test_vtable_pwm_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_RESET
    NAVTEST_CASE(test_vtable_reset_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_vtable_spi_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
    NAVTEST_CASE(test_vtable_timebase_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_vtable_timer_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_vtable_uart_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA
    NAVTEST_CASE(test_vtable_uart_dma_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
    NAVTEST_CASE(test_vtable_watchdog_is_complete),
#endif
#if NAVHAL_CONFIG_DRV_WWDG
    NAVTEST_CASE(test_vtable_wwdg_is_complete),
#endif
};

const navtest_suite_t test_vtable_suite = {
    .name = "VTABLE COMPLETENESS",
    .cases = vtable_cases,
    .count = sizeof(vtable_cases) / sizeof(vtable_cases[0]),
    .between = NULL,
};
