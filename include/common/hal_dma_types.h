/**
 * @file hal_dma_types.h
 * @brief DMA types, split out so the ops table and the port header can
 *        have them without including the API.
 *
 * common/hal_dma.h ends by including the port header, and everything behind
 * that header -- the family code and the ops table -- needs these types. Taking
 * them from the API header made the include graph circular, which clang-tidy
 * reports at every call site that opens one. Include guards made it compile;
 * they did not make it a graph anyone could follow.
 */
#ifndef HAL_DMA_TYPES_H
#define HAL_DMA_TYPES_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Transfer-completion callback for one stream.
 *
 * Runs from the stream's interrupt, after the transfer and before the driver
 * clears the stream's flags.
 */
typedef void (*hal_dma_callback_t)(void);

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
/** @brief DMA controller selection. */
typedef enum {
  HAL_DMA_CONTROLLER_1 = 1, /**< DMA1. */
  HAL_DMA_CONTROLLER_2 = 2, /**< DMA2. */
  DMA_CONTROLLER_1 NAVHAL_DEPRECATED("use HAL_DMA_CONTROLLER_1") = 1,
  DMA_CONTROLLER_2 NAVHAL_DEPRECATED("use HAL_DMA_CONTROLLER_2") = 2,
} hal_dma_controller_t;
/** @brief Data width applied to both peripheral and memory sides. */
typedef enum {
  HAL_DMA_DATA_WIDTH_8 = 0,  /**< 8-bit. */
  HAL_DMA_DATA_WIDTH_16 = 1, /**< 16-bit. */
  HAL_DMA_DATA_WIDTH_32 = 2, /**< 32-bit. */
  DMA_DATA_WIDTH_8 NAVHAL_DEPRECATED("use HAL_DMA_DATA_WIDTH_8") = 0,
  DMA_DATA_WIDTH_16 NAVHAL_DEPRECATED("use HAL_DMA_DATA_WIDTH_16") = 1,
  DMA_DATA_WIDTH_32 NAVHAL_DEPRECATED("use HAL_DMA_DATA_WIDTH_32") = 2,
} hal_dma_data_width_t;
/** @brief Transfer direction. */
typedef enum {
  HAL_DMA_DIR_P2M = 0, /**< Peripheral to memory. */
  HAL_DMA_DIR_M2P = 1, /**< Memory to peripheral. */
  HAL_DMA_DIR_M2M = 2, /**< Memory to memory. */
  DMA_DIR_P2M NAVHAL_DEPRECATED("use HAL_DMA_DIR_P2M") = 0,
  DMA_DIR_M2P NAVHAL_DEPRECATED("use HAL_DMA_DIR_M2P") = 1,
  DMA_DIR_M2M NAVHAL_DEPRECATED("use HAL_DMA_DIR_M2M") = 2,
} hal_dma_direction_t;
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_DMA_TYPES_H */
