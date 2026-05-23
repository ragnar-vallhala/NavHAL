/**
 * @file hal_i2c.h
 * @brief Portable HAL interface for I²C communication.
 *
 * @details
 * Standardized I²C API (see @c docs/api_standardization.md). All public
 * functions use the @c hal_i2c_ prefix, take the I²C bus as their first
 * argument (typed ::hal_i2c_bus_t), and return ::hal_status_t. Master-mode
 * standard/fast-speed operation.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_I2C_H
#define HAL_I2C_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/gpio_types.h"
#include "utils/i2c_types.h" /* port-resolved ::hal_i2c_bus_t instance enum */
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I²C speed modes.
 */
typedef enum {
  HAL_I2C_SPEED_STANDARD = 0, /**< 100 kHz standard mode. */
  HAL_I2C_SPEED_FAST = 1,     /**< 400 kHz fast mode. */
  STANDARD_MODE NAVHAL_DEPRECATED("use HAL_I2C_SPEED_STANDARD") = 0,
  FAST_MODE NAVHAL_DEPRECATED("use HAL_I2C_SPEED_FAST") = 1,
} hal_i2c_speed_t;

/** @brief Own-address value selecting master mode. */
#define I2C_MASTER 0

/** @brief GPIO alternate function for I²C pins. */
#define GPIO_FUNC_I2C HAL_GPIO_AF4

/**
 * @brief I²C configuration structure.
 */
typedef struct {
  hal_i2c_speed_t clock_speed; /**< Bus speed mode. */
  uint8_t own_address;         /**< 7-bit own address (I2C_MASTER if master). */
  bool acknowledge;            /**< Enable/disable ACK. */
} hal_i2c_config_t;

/**
 * @brief Initialize an I²C bus.
 * @param bus    I²C bus instance.
 * @param config Configuration; must not be NULL.
 * @return ::HAL_OK on success; ::HAL_ERR_NOT_INITIALIZED if the bus is already
 *         initialized; ::HAL_ERR_IO if slave mode is requested (unsupported).
 */
hal_status_t hal_i2c_init(hal_i2c_bus_t bus, const hal_i2c_config_t *config);

/**
 * @brief Write a byte buffer to an I²C device (blocking).
 * @param bus      I²C bus instance.
 * @param dev_addr 7-bit device address.
 * @param data     Source buffer.
 * @param len      Number of bytes to transmit.
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_i2c_write(hal_i2c_bus_t bus, uint8_t dev_addr,
                           const uint8_t *data, uint16_t len);

/**
 * @brief Read a byte buffer from an I²C device (blocking).
 * @param bus      I²C bus instance.
 * @param dev_addr 7-bit device address.
 * @param data     Destination buffer.
 * @param len      Number of bytes to read.
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_i2c_read(hal_i2c_bus_t bus, uint8_t dev_addr, uint8_t *data,
                          uint16_t len);

/**
 * @brief Write then read in a single combined transaction (blocking).
 * @param bus      I²C bus instance.
 * @param dev_addr 7-bit device address.
 * @param tx_data  Bytes to write.
 * @param tx_len   Number of bytes to write.
 * @param rx_data  Destination buffer for the read.
 * @param rx_len   Number of bytes to read.
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_i2c_write_read(hal_i2c_bus_t bus, uint8_t dev_addr,
                                const uint8_t *tx_data, uint16_t tx_len,
                                uint8_t *rx_data, uint16_t rx_len);

/**
 * @brief Get the per-bus initialization bitmask.
 * @return Bitmask of initialized buses (0 if none).
 */
uint8_t hal_i2c_get_init_status(void);

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization I²C status type and codes. The status enum
 * is now ::hal_status_t; these aliases are retained as a backward-compat
 * surface behind NAVHAL_DEPRECATED.
 * -------------------------------------------------------------------------- */
typedef hal_status_t hal_i2c_status_t NAVHAL_DEPRECATED("use hal_status_t");
#define HAL_I2C_OK HAL_OK                       /**< @deprecated HAL_OK */
#define HAL_I2C_ERR_TIMEOUT HAL_ERR_TIMEOUT     /**< @deprecated HAL_ERR_TIMEOUT */
#define HAL_I2C_ERR_BUS HAL_ERR_IO              /**< @deprecated HAL_ERR_IO */
#define HAL_I2C_ERR_NACK HAL_ERR_IO             /**< @deprecated HAL_ERR_IO */
#define HAL_I2C_ERR_REINIT HAL_ERR_NOT_INITIALIZED /**< @deprecated HAL_ERR_NOT_INITIALIZED */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits: DMA-backed I²C API behind @c _DMA_ENABLED. */
#include "navhal_port_i2c.h"

#endif /* HAL_I2C_H */
