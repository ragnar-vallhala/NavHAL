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

#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @enum hal_crc_polynomial_t
 * @brief Supported CRC polynomial presets.
 *
 * Only CRC-32/MPEG-2 (poly 0x04C11DB7) is exposed for now because that is
 * what the STM32F4 hardware unit natively computes. Extend this enum when
 * support for additional polynomials is added.
 */
typedef enum {
  HAL_CRC_POLY_CRC32 = 0, /**< CRC-32/MPEG-2  poly=0x04C11DB7  init=0xFFFFFFFF */
  CRC_POLY_CRC32 NAVHAL_DEPRECATED("use HAL_CRC_POLY_CRC32") = 0,
} hal_crc_polynomial_t;

/**
 * @struct hal_crc_config_t
 * @brief CRC module configuration.
 *
 * Pass a fully populated instance to hal_crc_init().
 */
typedef struct {
  hal_crc_polynomial_t polynomial; /**< Polynomial preset to use. */
  uint32_t init_value;             /**< Initial accumulator value. */
} hal_crc_config_t;

/* Deprecated pre-standardization CRC type names — retained as a backward-compat alias. */
typedef hal_crc_polynomial_t crc_polynomial_t
    NAVHAL_DEPRECATED("use hal_crc_polynomial_t");
typedef hal_crc_config_t crc_config_t NAVHAL_DEPRECATED("use hal_crc_config_t");


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* CRC_TYPES_H */
