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
 *        compiles the vendor drivers directly, so the capabilities here only
 *        need to satisfy navhal_port_config.h. The DMA-backend caps are off so
 *        no DMA code paths are pulled into the host build.
 */
#ifndef NAVHAL_TARGET_H
#define NAVHAL_TARGET_H

#define NAVHAL_HAS_GPIO 1
#define NAVHAL_HAS_UART 1
#define NAVHAL_HAS_I2C 1
#define NAVHAL_HAS_SPI 1
#define NAVHAL_HAS_CLOCK 1
#define NAVHAL_HAS_TIMER 1
#define NAVHAL_HAS_INTERRUPT 1
#define NAVHAL_HAS_FLASH 1
#define NAVHAL_HAS_CRC_HW 1
#define NAVHAL_HAS_DMA 0
#define NAVHAL_HAS_FPU 0
#define NAVHAL_HAS_CYCLE_COUNTER 0
#define NAVHAL_HAS_SDIO 0
#define NAVHAL_HAS_UART_DMA 0
#define NAVHAL_HAS_I2C_DMA 0
#define NAVHAL_HAS_SDIO_DMA 0

#define NAVHAL_TARGET_ARCH "cortex-m7"
#define NAVHAL_TARGET_VENDOR "stm32"
#define NAVHAL_TARGET_FAMILY "stm32f7"
#define NAVHAL_TARGET_BOARD "host"

#endif /* NAVHAL_TARGET_H */
