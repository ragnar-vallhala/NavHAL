/**
 * @file crc_types.h
 * @brief CRC type definitions for NavHAL.
 *
 * Defines enumerations and configuration structures shared between
 * the common CRC HAL interface and the architecture-specific driver.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CRC_TYPES_H
#define CRC_TYPES_H

#include <stdint.h>

/**
 * @enum crc_polynomial_t
 * @brief Supported CRC polynomial presets.
 *
 * Only CRC-32/MPEG-2 (poly 0x04C11DB7) is exposed for now because that is
 * what the STM32F4 hardware unit natively computes.  Extend this enum when
 * support for additional polynomials is added.
 */
typedef enum {
  CRC_POLY_CRC32 = 0, /**< CRC-32/MPEG-2  poly=0x04C11DB7  init=0xFFFFFFFF */
} crc_polynomial_t;

/**
 * @struct crc_config_t
 * @brief CRC module configuration.
 *
 * Pass a fully populated instance to hal_crc_init().
 */
typedef struct {
  crc_polynomial_t polynomial; /**< Polynomial preset to use */
  uint32_t init_value;         /**< Initial accumulator value (typically 0xFFFFFFFF) */
} crc_config_t;

#endif /* CRC_TYPES_H */
