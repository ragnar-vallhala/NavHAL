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
 * @file common/hal_i2c_dma.c
 * @brief Shared I2C-over-DMA layer: bindings, and the transfer front door.
 *
 * @details
 * Splits a hardware fact from a policy choice. The backend reports the
 * reference-manual default for a bus; this layer remembers an override if the
 * application sets one, and builds the transfer descriptor either way. The
 * caller no longer passes a @c hal_dma_config_t, so application code does not
 * have to know its bus's DMA wiring.
 *
 * Several I2C requests have a second stream option on STM32 -- I2C1_RX is on
 * DMA1 stream 0 or stream 5, for instance -- so the override exists for the
 * case where something else already wants the default stream.
 */

#include "common/hal_i2c.h"
#include "internal/hal_i2c_dma_ops.h"

#if NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA

#include "common/hal_dma.h"

#include <stddef.h>

/* Bus ids are small and dense enough to index directly; [bus][tx]. */
#define I2C_BUS_SLOTS 4u

static hal_dma_binding_t s_override[I2C_BUS_SLOTS][2];
static bool s_has_override[I2C_BUS_SLOTS][2];

hal_status_t hal_i2c_dma_get_binding(hal_i2c_bus_t bus, bool tx,
                                     hal_dma_binding_t *out) {
  if (out == NULL || (unsigned)bus >= I2C_BUS_SLOTS)
    return HAL_ERR_INVALID_ARG;

  if (s_has_override[bus][tx ? 1u : 0u]) {
    *out = s_override[bus][tx ? 1u : 0u];
    return HAL_OK;
  }
  return _hal_i2c_dma_ops.default_binding(bus, tx, out);
}

hal_status_t hal_i2c_dma_set_binding(hal_i2c_bus_t bus, bool tx,
                                     const hal_dma_binding_t *binding) {
  if ((unsigned)bus >= I2C_BUS_SLOTS)
    return HAL_ERR_INVALID_ARG;

  /* NULL clears the override and restores the hardware default. */
  if (binding == NULL) {
    s_has_override[bus][tx ? 1u : 0u] = false;
    return HAL_OK;
  }

  /* Refuse an override on a port that has no mapping to override. */
  hal_dma_binding_t probe;
  hal_status_t st = _hal_i2c_dma_ops.default_binding(bus, tx, &probe);
  if (st != HAL_OK)
    return st;

  s_override[bus][tx ? 1u : 0u] = *binding;
  s_has_override[bus][tx ? 1u : 0u] = true;
  return HAL_OK;
}

hal_status_t hal_i2c_read_regs_dma(hal_i2c_bus_t bus, uint8_t dev_addr,
                                   uint8_t reg, uint8_t *buffer, uint16_t length,
                                   void (*callback)(void)) {
  if (buffer == NULL || length == 0u)
    return HAL_ERR_INVALID_ARG;

  hal_dma_binding_t b;
  hal_status_t st = hal_i2c_dma_get_binding(bus, false, &b);
  if (st != HAL_OK)
    return st;

  hal_dma_config_t cfg = {
      .controller = b.controller,
      .stream = b.stream,
      .channel = b.channel,
      .direction = HAL_DMA_DIR_P2M,
      .src_addr = b.periph_addr,
      .dst_addr = (uint32_t)buffer,
      .data_count = length,
      .src_inc = 0u,
      .dst_inc = 1u,
      .data_width = HAL_DMA_DATA_WIDTH_8,
      .priority = HAL_DMA_PRIORITY_HIGH,
      .circular = 0u,
  };

  return _hal_i2c_dma_ops.read_regs(bus, dev_addr, reg, &cfg, callback);
}

#endif /* NAVHAL_CONFIG_DRV_I2C && NAVHAL_CONFIG_DRV_I2C_DMA */
