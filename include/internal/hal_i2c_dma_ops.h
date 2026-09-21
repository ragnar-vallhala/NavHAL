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
 * @file internal/hal_i2c_dma_ops.h
 * @brief HAL-internal I2C-over-DMA backend interface.
 *
 * @details
 * A sibling of ::hal_i2c_ops_t, gated by @c DRV_I2C_DMA, on the same
 * reasoning as the UART DMA table: a port supplies the whole thing or none of
 * it, so the completeness check stays meaningful.
 *
 * Two entries, and they are there for different reasons.
 *
 * ::default_binding is the hardware fact the caller used to have to supply.
 * Before this, @c hal_i2c_read_regs_dma took a @c hal_dma_config_t from the
 * application, so application code had to know which controller, stream and
 * channel its I2C bus was wired to. That is a reference-manual property of the
 * silicon, not a decision a caller should be making.
 *
 * ::read_regs stays a backend entry rather than being lifted, because the
 * register sequence around the transfer -- START, address, ADDR clear,
 * restart, then hand off -- genuinely differs between the F4 legacy I2C and
 * the F7 TIMINGR/CR2 generation. That is two implementations of one
 * operation, which is what a vtable entry is for.
 */

#ifndef NAVHAL_INTERNAL_HAL_I2C_DMA_OPS_H
#define NAVHAL_INTERNAL_HAL_I2C_DMA_OPS_H

#include "common/hal_dma.h"
#include "common/hal_i2c.h"
#include "common/hal_status.h"

#include <stdbool.h>
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port I2C-over-DMA operations table. */
typedef struct {
  /**
   * The wiring this bus uses by default, per the reference manual.
   *
   * @param tx true for the transmit request, false for receive.
   * @return ::HAL_ERR_NOT_SUPPORTED on a port whose mapping is not
   *         established, or ::HAL_ERR_INVALID_ARG for a bus it does not have.
   */
  hal_status_t (*default_binding)(hal_i2c_bus_t bus, bool tx,
                                  hal_dma_binding_t *out);

  /** Run a register read into @p cfg's destination, then call @p callback. */
  hal_status_t (*read_regs)(hal_i2c_bus_t bus, uint8_t dev_addr, uint8_t reg,
                            const hal_dma_config_t *cfg,
                            void (*callback)(void));
} hal_i2c_dma_ops_t;

/** @brief The active port's I2C DMA backend. */
extern const hal_i2c_dma_ops_t _hal_i2c_dma_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA */

#endif /* NAVHAL_INTERNAL_HAL_I2C_DMA_OPS_H */
