/**
 * @file i2c_types.h
 * @brief I²C bus-instance identifier — Cortex-M4 / STM32F4 port.
 *
 * @details
 * The set of valid I²C buses is target-defined, so ::hal_i2c_bus_t is
 * port-resolved: each port ships its own @c utils/i2c_types.h selected by the
 * `-I include/port/<processor>` path. Portable I²C concepts that are not
 * target-shaped (speed modes, the config struct) stay in
 * @c common/hal_i2c.h.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef I2C_TYPES_H
#define I2C_TYPES_H

#include "common/navhal_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I²C bus instance identifier (STM32F401RE: I2C1/2/3).
 */
typedef enum {
  HAL_I2C_1 = 0, /**< I²C bus 1. */
  HAL_I2C_2 = 1, /**< I²C bus 2. */
  HAL_I2C_3 = 2, /**< I²C bus 3. */
  I2C1 NAVHAL_DEPRECATED("use HAL_I2C_1") = 0,
  I2C2 NAVHAL_DEPRECATED("use HAL_I2C_2") = 1,
  I2C3 NAVHAL_DEPRECATED("use HAL_I2C_3") = 2,
} hal_i2c_bus_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* I2C_TYPES_H */
