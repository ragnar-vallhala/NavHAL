/**
 * @file hal_crc.h
 * @brief Portable HAL interface for CRC calculation.
 *
 * @details
 * Standardized CRC API (see @c docs/api_standardization.md). On the Cortex-M4
 * port this is backed either by the STM32F4 hardware CRC unit (when
 * @c _CRC_HW_ENABLED is defined) or by a software fallback — the API is
 * identical in both cases.
 *
 * ### Typical usage
 * @code
 * hal_crc_config_t cfg = {
 *     .polynomial = HAL_CRC_POLY_CRC32,
 *     .init_value = 0xFFFFFFFF,
 * };
 * hal_crc_init(&cfg);
 *
 * uint32_t crc = hal_crc_compute(buf, len);
 *
 * hal_crc_reset();
 * uint32_t partial = hal_crc_accumulate(chunk1, len1);
 * uint32_t result  = hal_crc_accumulate(chunk2, len2);
 * @endcode
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_CRC_H
#define HAL_CRC_H

#include "common/hal_status.h"
#include "utils/crc_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the CRC module.
 *
 * Hardware path: enables the RCC clock for the CRC peripheral and resets the
 * accumulator. Software path: stores @p cfg->init_value for subsequent calls.
 *
 * @param cfg Fully populated configuration; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_crc_init(const hal_crc_config_t *cfg);

/**
 * @brief Compute the CRC of a byte buffer from scratch.
 *
 * Equivalent to ::hal_crc_reset followed by ::hal_crc_accumulate.
 *
 * @param data Pointer to input data (need not be word-aligned).
 * @param len  Number of bytes.
 * @return Final CRC-32 value.
 */
uint32_t hal_crc_compute(const uint8_t *data, uint32_t len);

/**
 * @brief Feed more bytes into the running CRC accumulator.
 *
 * The accumulator is NOT reset before processing, allowing a CRC to be
 * computed over a stream split into multiple chunks.
 *
 * @param data Pointer to input data.
 * @param len  Number of bytes.
 * @return Accumulated CRC-32 value after processing @p data.
 */
uint32_t hal_crc_accumulate(const uint8_t *data, uint32_t len);

/**
 * @brief Reset the CRC accumulator to the configured init value.
 * @return ::HAL_OK.
 */
hal_status_t hal_crc_reset(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits (currently none for CRC; header retained for the
 * existing @c #include "navhal_port_crc.h" path). */
#include "navhal_port_crc.h"

#endif /* HAL_CRC_H */
