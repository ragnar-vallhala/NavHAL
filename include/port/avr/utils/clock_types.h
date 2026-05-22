/**
 * @file clock_types.h
 * @brief Clock HAL type definitions — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P clock source is selected by fuse bits, not at runtime;
 * `hal_clock_config_t::source` is therefore informational. The one runtime
 * knob is the system clock prescaler (CLKPR / CLKPS), expressed here as a
 * power-of-two divider exponent.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CLOCK_TYPES_H
#define CLOCK_TYPES_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief System clock source (fuse-selected on ATmega328P). */
typedef enum {
  HAL_CLOCK_SOURCE_INTERNAL_RC,   /**< Internal ~8 MHz RC oscillator. */
  HAL_CLOCK_SOURCE_EXTERNAL_XTAL, /**< External crystal / resonator. */
} hal_clock_source_t;

/**
 * @brief System clock configuration.
 *
 * @c source documents the fuse-selected oscillator; @c prescaler_log2 is the
 * runtime CLKPS divider exponent (0 = /1, 1 = /2, ... 8 = /256).
 */
typedef struct {
  hal_clock_source_t source; /**< Informational; actual source is fuse-set. */
  uint8_t prescaler_log2;    /**< CLKPS divider exponent, 0..8. */
} hal_clock_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* CLOCK_TYPES_H */
