/**
 * @file core/cortex-m4/dma.h
 * @brief DMA HAL interface for STM32F4.
 *
 * @details
 * Provides a high-level API to configure and control DMA1/DMA2 streams.
 * Only compiled into the project when @c _DMA_ENABLED is defined.
 *
 * ### Typical usage
 * @code
 * dma_config_t cfg = {
 *     .controller   = DMA_CONTROLLER_1,
 *     .stream       = 6,
 *     .channel      = 4,
 *     .direction    = DMA_DIR_M2P,
 *     .src_addr     = (uint32_t)my_buffer,
 *     .dst_addr     = (uint32_t)&USART2->DR,
 *     .data_count   = strlen(my_buffer),
 *     .src_inc      = 1,
 *     .dst_inc      = 0,
 *     .data_width   = DMA_DATA_WIDTH_8,
 *     .priority     = DMA_PRIORITY_HIGH,
 *     .circular     = 0,
 * };
 * dma_init(&cfg);
 * dma_start(&cfg);
 * while (!dma_transfer_complete(&cfg));
 * @endcode
 *
 * @note Requires @c _DMA_ENABLED compile definition.
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_DMA_H
#define CORTEX_M4_DMA_H

#ifdef _DMA_ENABLED

#include "core/cortex-m4/dma_reg.h"
#include <stdint.h>

/*---------------------------------------------------------------------------
 * Enumerations
 *---------------------------------------------------------------------------*/

/**
 * @brief Select DMA controller.
 */
typedef enum {
  DMA_CONTROLLER_1 = 1, /**< DMA1 (base 0x40026000) */
  DMA_CONTROLLER_2 = 2, /**< DMA2 (base 0x40026400) */
} dma_controller_t;

/**
 * @brief Transfer direction.
 */
typedef enum {
  DMA_DIR_P2M = 0, /**< Peripheral → Memory */
  DMA_DIR_M2P = 1, /**< Memory → Peripheral */
  DMA_DIR_M2M = 2, /**< Memory → Memory (DMA2 only) */
} dma_direction_t;

/**
 * @brief Data width for both peripheral and memory sides.
 */
typedef enum {
  DMA_DATA_WIDTH_8 = 0,  /**< Byte (8-bit) */
  DMA_DATA_WIDTH_16 = 1, /**< Half-word (16-bit) */
  DMA_DATA_WIDTH_32 = 2, /**< Word (32-bit) */
} dma_data_width_t;

/**
 * @brief Stream priority level.
 */
typedef enum {
  DMA_PRIORITY_LOW = 0,
  DMA_PRIORITY_MEDIUM = 1,
  DMA_PRIORITY_HIGH = 2,
  DMA_PRIORITY_VERY_HIGH = 3,
} dma_priority_t;

/*---------------------------------------------------------------------------
 * Configuration structure
 *---------------------------------------------------------------------------*/

/**
 * @brief DMA stream configuration.
 *
 * Fill in all fields and pass to dma_init(), then dma_start().
 */
typedef struct {
  dma_controller_t controller; /**< Which DMA controller (1 or 2) */
  uint8_t stream;              /**< Stream index [0..7] */
  uint8_t channel;             /**< Channel selection [0..7] */
  dma_direction_t direction;   /**< Transfer direction */
  uint32_t
      src_addr; /**< Source address (memory in M2P/M2M, peripheral in P2M) */
  uint32_t dst_addr;   /**< Destination address */
  uint16_t data_count; /**< Number of data items to transfer */
  uint8_t src_inc;     /**< 1 = increment source address after each transfer */
  uint8_t dst_inc; /**< 1 = increment destination address after each transfer */
  dma_data_width_t data_width; /**< Data size (applied to both src and dst) */
  dma_priority_t priority;     /**< Stream priority */
  uint8_t circular;            /**< 1 = circular (ring-buffer) mode */
} dma_config_t;

/*---------------------------------------------------------------------------
 * API
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize a DMA stream.
 *
 * Enables the peripheral clock, disables the stream, waits for it to stop,
 * clears any pending interrupt flags, and programs all CR/NDTR/PAR/M0AR
 * registers according to @p cfg.
 *
 * @param cfg  Pointer to a fully populated dma_config_t.
 */
void dma_init(const dma_config_t *cfg);

/**
 * @brief Enable a previously initialized DMA stream.
 *
 * Clears interrupt flags and sets DMA_SxCR_EN.
 *
 * @param cfg  Same config used with dma_init().
 */
void dma_start(const dma_config_t *cfg);

/**
 * @brief Disable a DMA stream immediately.
 *
 * @param cfg  Same config used with dma_init().
 */
void dma_stop(const dma_config_t *cfg);

/**
 * @brief Check whether the transfer-complete flag is set.
 *
 * @param cfg  Same config used with dma_init().
 * @return 1 if transfer is complete, 0 otherwise.
 */
int dma_transfer_complete(const dma_config_t *cfg);

/**
 * @brief Clear all interrupt flags for the configured stream.
 *
 * @param cfg  Same config used with dma_init().
 */
void dma_clear_flags(const dma_config_t *cfg);

#endif /* _DMA_ENABLED */

#endif /* CORTEX_M4_DMA_H */
