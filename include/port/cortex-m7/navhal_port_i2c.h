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
 * @file port/cortex-m4/navhal_port_i2c.h
 * @brief Cortex-M4 / STM32F4 I²C port header.
 *
 * @details
 * The public I²C API lives in @c common/hal_i2c.h, which includes this
 * header. This file carries the DMA-backed I²C prototype (available only
 * when the DMA backend is enabled).
 */

#ifndef NAVHAL_PORT_I2C_H
#define NAVHAL_PORT_I2C_H

#include "common/hal_i2c.h"
#include "navhal_port_config.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _I2C_BACKEND_DMA
#include "navhal_port_dma.h"

/**
 * @brief Write a register address then read it back via DMA (non-blocking).
 * @param bus      I²C bus instance.
 * @param dev_addr 7-bit device address.
 * @param reg      Target register address.
 * @param dma_cfg  Fully populated DMA configuration.
 * @param callback Invoked when the DMA transfer completes.
 * @return ::HAL_OK once the sequence is started, or an error status.
 */
hal_status_t hal_i2c_read_regs_dma(hal_i2c_bus_t bus, uint8_t dev_addr,
                                   uint8_t reg,
                                   const hal_dma_config_t *dma_cfg,
                                   void (*callback)(void));
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_PORT_I2C_H */
