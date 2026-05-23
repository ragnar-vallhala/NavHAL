/**
 * @file hal_dma.h
 * @brief Portable HAL interface for the DMA controller.
 *
 * @details
 * Standardized DMA API (see @c docs/api_standardization.md). Configures and
 * controls DMA streams. The entire API is compiled only when @c _DMA_ENABLED
 * is defined (see ::NAVHAL_HAS_DMA).
 *
 * ### Typical usage
 * @code
 * hal_dma_config_t cfg = {
 *     .controller = HAL_DMA_CONTROLLER_1,
 *     .stream     = 6,
 *     .channel    = 4,
 *     .direction  = HAL_DMA_DIR_M2P,
 *     .src_addr   = (uint32_t)my_buffer,
 *     .dst_addr   = (uint32_t)&USART2->DR,
 *     .data_count = len,
 *     .src_inc    = 1,
 *     .dst_inc    = 0,
 *     .data_width = HAL_DMA_DATA_WIDTH_8,
 *     .priority   = HAL_DMA_PRIORITY_HIGH,
 * };
 * hal_dma_init(&cfg);
 * hal_dma_start(&cfg);
 * while (!hal_dma_transfer_complete(&cfg));
 * @endcode
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_DMA_H
#define HAL_DMA_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DMA_ENABLED

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief DMA controller selection. */
typedef enum {
  HAL_DMA_CONTROLLER_1 = 1, /**< DMA1. */
  HAL_DMA_CONTROLLER_2 = 2, /**< DMA2. */
  DMA_CONTROLLER_1 NAVHAL_DEPRECATED("use HAL_DMA_CONTROLLER_1") = 1,
  DMA_CONTROLLER_2 NAVHAL_DEPRECATED("use HAL_DMA_CONTROLLER_2") = 2,
} hal_dma_controller_t;

/** @brief Transfer direction. */
typedef enum {
  HAL_DMA_DIR_P2M = 0, /**< Peripheral to memory. */
  HAL_DMA_DIR_M2P = 1, /**< Memory to peripheral. */
  HAL_DMA_DIR_M2M = 2, /**< Memory to memory. */
  DMA_DIR_P2M NAVHAL_DEPRECATED("use HAL_DMA_DIR_P2M") = 0,
  DMA_DIR_M2P NAVHAL_DEPRECATED("use HAL_DMA_DIR_M2P") = 1,
  DMA_DIR_M2M NAVHAL_DEPRECATED("use HAL_DMA_DIR_M2M") = 2,
} hal_dma_direction_t;

/** @brief Data width applied to both peripheral and memory sides. */
typedef enum {
  HAL_DMA_DATA_WIDTH_8 = 0,  /**< 8-bit. */
  HAL_DMA_DATA_WIDTH_16 = 1, /**< 16-bit. */
  HAL_DMA_DATA_WIDTH_32 = 2, /**< 32-bit. */
  DMA_DATA_WIDTH_8 NAVHAL_DEPRECATED("use HAL_DMA_DATA_WIDTH_8") = 0,
  DMA_DATA_WIDTH_16 NAVHAL_DEPRECATED("use HAL_DMA_DATA_WIDTH_16") = 1,
  DMA_DATA_WIDTH_32 NAVHAL_DEPRECATED("use HAL_DMA_DATA_WIDTH_32") = 2,
} hal_dma_data_width_t;

/** @brief Stream priority level. */
typedef enum {
  HAL_DMA_PRIORITY_LOW = 0,
  HAL_DMA_PRIORITY_MEDIUM = 1,
  HAL_DMA_PRIORITY_HIGH = 2,
  HAL_DMA_PRIORITY_VERY_HIGH = 3,
  DMA_PRIORITY_LOW NAVHAL_DEPRECATED("use HAL_DMA_PRIORITY_LOW") = 0,
  DMA_PRIORITY_MEDIUM NAVHAL_DEPRECATED("use HAL_DMA_PRIORITY_MEDIUM") = 1,
  DMA_PRIORITY_HIGH NAVHAL_DEPRECATED("use HAL_DMA_PRIORITY_HIGH") = 2,
  DMA_PRIORITY_VERY_HIGH NAVHAL_DEPRECATED("use HAL_DMA_PRIORITY_VERY_HIGH") = 3,
} hal_dma_priority_t;

/** @brief Burst transfer configuration. */
typedef enum {
  HAL_DMA_BURST_SINGLE = 0,
  HAL_DMA_BURST_INCR4 = 1,
  HAL_DMA_BURST_INCR8 = 2,
  HAL_DMA_BURST_INCR16 = 3,
  DMA_BURST_SINGLE NAVHAL_DEPRECATED("use HAL_DMA_BURST_SINGLE") = 0,
  DMA_BURST_INCR4 NAVHAL_DEPRECATED("use HAL_DMA_BURST_INCR4") = 1,
  DMA_BURST_INCR8 NAVHAL_DEPRECATED("use HAL_DMA_BURST_INCR8") = 2,
  DMA_BURST_INCR16 NAVHAL_DEPRECATED("use HAL_DMA_BURST_INCR16") = 3,
} hal_dma_burst_t;

/** @brief FIFO threshold selection. */
typedef enum {
  HAL_DMA_FIFO_THRESHOLD_1_4 = 0,
  HAL_DMA_FIFO_THRESHOLD_1_2 = 1,
  HAL_DMA_FIFO_THRESHOLD_3_4 = 2,
  HAL_DMA_FIFO_THRESHOLD_FULL = 3,
  DMA_FIFO_THRESHOLD_1_4 NAVHAL_DEPRECATED("use HAL_DMA_FIFO_THRESHOLD_1_4") = 0,
  DMA_FIFO_THRESHOLD_1_2 NAVHAL_DEPRECATED("use HAL_DMA_FIFO_THRESHOLD_1_2") = 1,
  DMA_FIFO_THRESHOLD_3_4 NAVHAL_DEPRECATED("use HAL_DMA_FIFO_THRESHOLD_3_4") = 2,
  DMA_FIFO_THRESHOLD_FULL NAVHAL_DEPRECATED("use HAL_DMA_FIFO_THRESHOLD_FULL") =
      3,
} hal_dma_fifo_threshold_t;

/**
 * @brief DMA stream configuration.
 *
 * Fill in all fields and pass to ::hal_dma_init, then ::hal_dma_start.
 */
typedef struct {
  hal_dma_controller_t controller;       /**< DMA controller. */
  uint8_t stream;                        /**< Stream index [0..7]. */
  uint8_t channel;                       /**< Channel selection [0..7]. */
  hal_dma_direction_t direction;         /**< Transfer direction. */
  uint32_t src_addr;                     /**< Source address. */
  uint32_t dst_addr;                     /**< Destination address. */
  uint16_t data_count;                   /**< Number of data items. */
  uint8_t src_inc;                       /**< 1 = increment source address. */
  uint8_t dst_inc;                       /**< 1 = increment dest address. */
  hal_dma_data_width_t data_width;       /**< Data size. */
  hal_dma_priority_t priority;           /**< Stream priority. */
  uint8_t circular;                      /**< 1 = circular mode. */
  uint8_t pfctrl;                        /**< 1 = peripheral flow control. */
  uint8_t fifo_mode;                     /**< 1 = FIFO mode enabled. */
  hal_dma_fifo_threshold_t fifo_threshold; /**< FIFO threshold. */
  hal_dma_burst_t mburst;                /**< Memory burst configuration. */
  hal_dma_burst_t pburst;                /**< Peripheral burst configuration. */
} hal_dma_config_t;

/**
 * @brief Initialize a DMA stream from @p cfg.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_init(const hal_dma_config_t *cfg);

/**
 * @brief Enable a previously initialized DMA stream.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_start(const hal_dma_config_t *cfg);

/**
 * @brief Disable a DMA stream immediately.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_stop(const hal_dma_config_t *cfg);

/**
 * @brief Check whether the transfer-complete flag is set.
 * @return true if the transfer is complete, false otherwise.
 */
bool hal_dma_transfer_complete(const hal_dma_config_t *cfg);

/**
 * @brief Clear all interrupt flags for the configured stream.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_clear_flags(const hal_dma_config_t *cfg);

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization DMA type names. Retained as a
 * backward-compat alias behind NAVHAL_DEPRECATED.
 * -------------------------------------------------------------------------- */
typedef hal_dma_controller_t dma_controller_t
    NAVHAL_DEPRECATED("use hal_dma_controller_t");
typedef hal_dma_direction_t dma_direction_t
    NAVHAL_DEPRECATED("use hal_dma_direction_t");
typedef hal_dma_data_width_t dma_data_width_t
    NAVHAL_DEPRECATED("use hal_dma_data_width_t");
typedef hal_dma_priority_t dma_priority_t
    NAVHAL_DEPRECATED("use hal_dma_priority_t");
typedef hal_dma_burst_t dma_burst_t NAVHAL_DEPRECATED("use hal_dma_burst_t");
typedef hal_dma_fifo_threshold_t dma_fifo_threshold_t
    NAVHAL_DEPRECATED("use hal_dma_fifo_threshold_t");
typedef hal_dma_config_t dma_config_t NAVHAL_DEPRECATED("use hal_dma_config_t");

#endif /* _DMA_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits: register map, deprecated-function compat shim. */
#include "navhal_port_dma.h"

#endif /* HAL_DMA_H */
