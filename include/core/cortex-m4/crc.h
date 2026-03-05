/**
 * @file core/cortex-m4/crc.h
 * @brief CRC HAL interface for Cortex-M4 (STM32F4).
 *
 * @details
 * Provides CRC-32/MPEG-2 computation via the STM32F4 hardware CRC unit
 * when @c _CRC_HW_ENABLED is defined, or via a pure-C table-driven software
 * implementation otherwise.  The API is identical in both cases.
 *
 * ### Typical usage
 * @code
 * crc_config_t cfg = {
 *     .polynomial  = CRC_POLY_CRC32,
 *     .init_value  = 0xFFFFFFFF,
 * };
 * hal_crc_init(&cfg);
 *
 * // Single-shot
 * uint32_t crc = hal_crc_compute(buf, len);
 *
 * // Incremental (stream chunks)
 * hal_crc_reset();
 * uint32_t partial = hal_crc_accumulate(chunk1, len1);
 * uint32_t result  = hal_crc_accumulate(chunk2, len2);
 * @endcode
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_CRC_H
#define CORTEX_M4_CRC_H

#include "utils/crc_types.h"
#include <stdint.h>

/**
 * @brief Initialise the CRC module.
 *
 * Hardware path: enables the RCC clock for the CRC peripheral and resets the
 * accumulator to @p cfg->init_value.
 * Software path: stores @p cfg->init_value for use by subsequent calls.
 *
 * @param cfg  Pointer to a fully populated crc_config_t.
 */
void hal_crc_init(const crc_config_t *cfg);

/**
 * @brief Compute the CRC of a byte buffer from scratch.
 *
 * Equivalent to calling hal_crc_reset() followed by hal_crc_accumulate().
 * The internal accumulator is reset to init_value before processing.
 *
 * @param data  Pointer to input data (need not be word-aligned).
 * @param len   Number of bytes.
 * @return      Final CRC-32 value.
 */
uint32_t hal_crc_compute(const uint8_t *data, uint32_t len);

/**
 * @brief Feed more bytes into the running CRC accumulator.
 *
 * The accumulator is NOT reset before processing, allowing CRC to be
 * calculated over a stream split into multiple chunks.
 *
 * @param data  Pointer to input data.
 * @param len   Number of bytes.
 * @return      Accumulated CRC-32 value after processing @p data.
 */
uint32_t hal_crc_accumulate(const uint8_t *data, uint32_t len);

/**
 * @brief Reset the CRC accumulator to the configured init_value.
 *
 * Hardware path: writes the RESET bit in CRC_CR.
 * Software path: restores the stored init_value.
 */
void hal_crc_reset(void);

#endif /* CORTEX_M4_CRC_H */
