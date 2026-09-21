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

/**
 * @file main.c
 * @brief Example application demonstrating the ARMv7-M MPU (hal_mpu).
 *
 * @details
 * - Initializes the clock and HAL_UART_2 for console output.
 * - Reports whether the part implements an MPU and how many regions it has
 *   (8 on the Cortex-M4 / F401, 16 on the Cortex-M7 / F767 — read at runtime).
 * - Programs one region the slow (validating) way with
 *   hal_mpu_configure_region.
 * - Pre-encodes a two-region set with hal_mpu_encode and installs it in bulk
 *   with hal_mpu_apply — the context-switch fast path.
 * - Enables the MPU with the privileged background region on
 *   (PRIVDEFENA = 1), so this privileged sample keeps the default system
 *   memory map and cannot fault itself, then disables it again.
 */

#include "navhal.h"
#include <stdint.h>

/** @brief PLL configuration: 16 MHz HSI -> 84 MHz system clock. */
hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSI,
    .pll_m = 16,
    .pll_n = 336,
    .pll_p = 4,
    .pll_q = 7,
};

/** @brief System clock source configuration. */
hal_clock_config_t clock_cfg = {
    .source = HAL_CLOCK_SOURCE_PLL,
    .hpre_div = 1,
    .ppre1_div = 2,
    .ppre2_div = 1,
};

/* A 1 KB block of SRAM we will guard as read-only for unprivileged code. */
static volatile uint8_t guarded_block[1024] __attribute__((aligned(1024)));

static void report(const char *label, hal_status_t st) {
  hal_uart_print(HAL_UART_2, label);
  hal_uart_print(HAL_UART_2, (st == HAL_OK) ? " OK\r\n" : " FAILED\r\n");
}

int main(void) {
  clock_cfg.pll = pll_cfg;
  hal_clock_init(&clock_cfg);
  hal_timebase_init(1000);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate = 9600});

  hal_uart_print(HAL_UART_2, "\r\nNavHAL MPU Sample\r\n");

  if (!hal_mpu_present()) {
    hal_uart_print(HAL_UART_2, "No MPU on this part — nothing to do.\r\n");
    while (1) {
      hal_delay_ms(1000);
    }
  }

  hal_uart_print(HAL_UART_2, "MPU regions: ");
  hal_uart_print(HAL_UART_2, hal_mpu_num_regions());
  hal_uart_print(HAL_UART_2, "\r\n");

  /* 1) Validating path: guard the 1 KB block as read-only, normal write-back
   *    memory, execute-never, in region 0. */
  hal_mpu_region_t ro = {
      .base = (uint32_t)(uintptr_t)guarded_block,
      .size = HAL_MPU_SIZE_1KB,
      .ap = HAL_MPU_AP_RO,
      .mem = HAL_MPU_MEM_NORMAL_WB,
      .executable = false,
      .shareable = false,
      .srd_mask = 0u,
  };
  report("configure_region(0):", hal_mpu_configure_region(0, &ro));

  /* 2) Fast path: pre-encode a two-region set once, then install it in bulk.
   *    Region 1 = the same RO block; region 2 = device memory for the GPIOA
   *    MMIO window (privileged RW, non-cacheable, execute-never). */
  hal_mpu_region_t dev = {
      .base = 0x40020000u,
      .size = HAL_MPU_SIZE_1KB,
      .ap = HAL_MPU_AP_PRIV_RW,
      .mem = HAL_MPU_MEM_DEVICE,
      .executable = false,
      .shareable = true,
      .srd_mask = 0u,
  };
  hal_mpu_encoded_t set[2];
  report("encode(region 1):", hal_mpu_encode(1, &ro, &set[0]));
  report("encode(region 2):", hal_mpu_encode(2, &dev, &set[1]));
  report("apply(set, 2):", hal_mpu_apply(set, 2));

  /* 3) Turn the MPU on with the privileged background region enabled, so this
   *    privileged code keeps default access to everything else, then off. */
  report("enable(bg_priv):", hal_mpu_enable(true));
  report("disable():", hal_mpu_disable());

  hal_uart_print(HAL_UART_2, "MPU sample done.\r\n");

  while (1) {
    hal_delay_ms(2000);
  }
}
