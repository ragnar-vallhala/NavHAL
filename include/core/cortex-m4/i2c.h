/**
 * @file core/cortex-m4/i2c.h
 * @brief Cortex-M4 / STM32F4 I²C port header.
 *
 * @details
 * The public I²C API lives in @c common/hal_i2c.h, which includes this
 * header. This file carries the DMA-backed I²C prototype (available only
 * when the DMA backend is enabled).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_I2C_H
#define CORTEX_M4_I2C_H

#include "common/hal_i2c.h"
#include "core/cortex-m4/config.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DMA_ENABLED
#include "core/cortex-m4/dma.h"

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
                                   uint8_t reg, const dma_config_t *dma_cfg,
                                   void (*callback)(void));
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORTEX_M4_I2C_H */
