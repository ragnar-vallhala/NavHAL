/**
 * @file core/cortex-m4/i2c.h
 * @brief Cortex-M4 I²C HAL interface.
 *
 * @details
 * This header defines types, structures, and function prototypes for
 * I²C communication on Cortex-M4 microcontrollers. It provides functions
 * for initializing the I²C peripheral, reading/writing data, and performing
 * combined write-read transactions.
 *
 * Supported features:
 * - Standard and Fast mode I²C communication.
 * - Master mode operations.
 * - Error handling with status codes.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_I2C_H
#define CORTEX_M4_I2C_H

#include "utils/gpio_types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief I²C operation status codes.
 */
typedef enum {
    HAL_I2C_OK = 0,          /**< Operation successful */
    HAL_I2C_ERR_TIMEOUT,     /**< Timeout occurred */
    HAL_I2C_ERR_BUS,         /**< Bus error */
    HAL_I2C_ERR_NACK,        /**< NACK received */
    HAL_I2C_ERR_REINIT       /**< Reinitialization required */
} hal_i2c_status_t;

/**
 * @brief Supported I²C bus instances.
 */
typedef enum {
    I2C1 = 0, /**< I²C bus 1 */
    I2C2 = 1, /**< I²C bus 2 */
    I2C3 = 2  /**< I²C bus 3 */
} hal_i2c_bus_t;

/**
 * @brief I²C speed modes.
 */
typedef enum {
    STANDARD_MODE = 0, /**< 100 kHz standard mode */
    FAST_MODE = 1      /**< 400 kHz fast mode */
} hal_i2c_speed_t;

/** Master mode identifier */
#define I2C_MASTER 0

/** GPIO alternate function for I²C pins */
#define GPIO_FUNC_I2C GPIO_AF04

/**
 * @brief I²C configuration structure.
 */
typedef struct {
    hal_i2c_speed_t clock_speed; /**< Clock speed in Hz */
    uint8_t own_address;         /**< 7-bit device address (0 if master) */
    bool acknowledge;            /**< Enable/disable ACK */
} hal_i2c_config_t;

/**
 * @brief Initialize the I²C peripheral.
 *
 * @param bus The I²C bus instance to initialize.
 * @param config Pointer to the configuration structure.
 * @return Status code of the initialization operation.
 */
hal_i2c_status_t hal_i2c_init(hal_i2c_bus_t bus, hal_i2c_config_t *config);

/**
 * @brief Write data to an I²C device.
 *
 * @param bus The I²C bus instance.
 * @param dev_addr The 7-bit address of the target device.
 * @param data Pointer to data buffer to transmit.
 * @param len Number of bytes to transmit.
 * @return Status code of the write operation.
 */
hal_i2c_status_t hal_i2c_write(uint8_t bus, uint8_t dev_addr,
                               const uint8_t *data, uint16_t len);

/**
 * @brief Read data from an I²C device.
 *
 * @param bus The I²C bus instance.
 * @param dev_addr The 7-bit address of the target device.
 * @param data Pointer to buffer for received data.
 * @param len Number of bytes to read.
 * @return Status code of the read operation.
 */
hal_i2c_status_t hal_i2c_read(uint8_t bus, uint8_t dev_addr, uint8_t *data,
                              uint16_t len);

/**
 * @brief Write to a device register and read back data.
 *
 * @param bus The I²C bus instance.
 * @param dev_addr The 7-bit address of the target device.
 * @param tx_data Pointer to data to write.
 * @param tx_len Number of bytes to write.
 * @param rx_data Pointer to buffer for received data.
 * @param rx_len Number of bytes to read.
 * @return Status code of the write-read operation.
 */
hal_i2c_status_t hal_i2c_write_read(uint8_t bus, uint8_t dev_addr,
                                    const uint8_t *tx_data, uint16_t tx_len,
                                    uint8_t *rx_data, uint16_t rx_len);

/**
 * @brief Get the initialization status of the I²C peripheral.
 *
 * @return 0 if not initialized, non-zero if initialized.
 */
uint8_t hal_i2c_get_init_status(void);

#endif // !CORTEX_M4_I2C_H
