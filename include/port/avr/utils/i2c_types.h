/**
 * @file i2c_types.h
 * @brief I²C bus-instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single TWI (two-wire) peripheral, exposed as
 * ::HAL_I2C_0.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef I2C_TYPES_H
#define I2C_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief I²C bus instance identifier (ATmega328P: the TWI peripheral). */
typedef enum {
  HAL_I2C_0 = 0, /**< TWI — the only I²C bus on the ATmega328P. */
} hal_i2c_bus_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* I2C_TYPES_H */
