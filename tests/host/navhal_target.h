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
 * @file navhal_target.h (host driver-suite stub)
 * @brief The embedded build generates this from Kconfig; the host driver suite
 *        compiles the vendor drivers directly, so the capabilities here are
 *        hand-maintained. tests/host/CMakeLists.txt force-includes this file
 *        into every host TU (mirroring the root build's force-include of the
 *        generated header). The DMA-backend caps are off so no DMA code paths
 *        are pulled into the host build.
 *
 * Keep in the shape the generator emits: NAVHAL_CONFIG_DRV_* is the truth guards
 * key off; NAVHAL_HAS_* are deprecated aliases kept for legacy consumers.
 */
#ifndef NAVHAL_TARGET_H
#define NAVHAL_TARGET_H

/* ===== Resolved Kconfig symbols (NAVHAL_CONFIG_*) ===== */
#define NAVHAL_CONFIG_DRV_GPIO      1
#define NAVHAL_CONFIG_DRV_UART      1
#define NAVHAL_CONFIG_DRV_I2C       1
#define NAVHAL_CONFIG_DRV_SPI       1
#define NAVHAL_CONFIG_DRV_CLOCK     1
#define NAVHAL_CONFIG_DRV_TIMER     1
#define NAVHAL_CONFIG_DRV_INTERRUPT 1
#define NAVHAL_CONFIG_DRV_FLASH     1
/* CRC hardware OFF on host: the pure-logic suite (test_crc_sw.c) exercises the
 * software fallback in crc.c's #else branch; the HW path would touch real CRC
 * registers and fault on x86. */
#define NAVHAL_CONFIG_DRV_CRC       0
#define NAVHAL_CONFIG_DRV_DMA       0
#define NAVHAL_CONFIG_USE_FPU       0
#define NAVHAL_CONFIG_DRV_DWT       0
#define NAVHAL_CONFIG_DRV_SDIO      0
#define NAVHAL_CONFIG_DRV_UART_DMA  0
#define NAVHAL_CONFIG_DRV_I2C_DMA   0
#define NAVHAL_CONFIG_DRV_SDIO_DMA  0

/* ===== DEPRECATED capability aliases (NAVHAL_HAS_* -> NAVHAL_CONFIG_*) ===== */
#define NAVHAL_HAS_GPIO          NAVHAL_CONFIG_DRV_GPIO
#define NAVHAL_HAS_UART          NAVHAL_CONFIG_DRV_UART
#define NAVHAL_HAS_I2C           NAVHAL_CONFIG_DRV_I2C
#define NAVHAL_HAS_SPI           NAVHAL_CONFIG_DRV_SPI
#define NAVHAL_HAS_CLOCK         NAVHAL_CONFIG_DRV_CLOCK
#define NAVHAL_HAS_TIMER         NAVHAL_CONFIG_DRV_TIMER
#define NAVHAL_HAS_INTERRUPT     NAVHAL_CONFIG_DRV_INTERRUPT
#define NAVHAL_HAS_FLASH         NAVHAL_CONFIG_DRV_FLASH
#define NAVHAL_HAS_CRC_HW        NAVHAL_CONFIG_DRV_CRC
#define NAVHAL_HAS_DMA           NAVHAL_CONFIG_DRV_DMA
#define NAVHAL_HAS_FPU           NAVHAL_CONFIG_USE_FPU
#define NAVHAL_HAS_CYCLE_COUNTER NAVHAL_CONFIG_DRV_DWT
#define NAVHAL_HAS_SDIO          NAVHAL_CONFIG_DRV_SDIO
#define NAVHAL_HAS_UART_DMA      NAVHAL_CONFIG_DRV_UART_DMA
#define NAVHAL_HAS_I2C_DMA       NAVHAL_CONFIG_DRV_I2C_DMA
#define NAVHAL_HAS_SDIO_DMA      NAVHAL_CONFIG_DRV_SDIO_DMA

#define NAVHAL_TARGET_ARCH "cortex-m7"
#define NAVHAL_TARGET_VENDOR "stm32"
#define NAVHAL_TARGET_FAMILY "stm32f7"
#define NAVHAL_TARGET_BOARD "host"

#endif /* NAVHAL_TARGET_H */
