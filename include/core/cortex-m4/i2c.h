// i2c.h
#ifndef CORTEX_M4_I2C_H
#define CORTEX_M4_I2C_H
#include "utils/gpio_types.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  HAL_I2C_OK = 0,
  HAL_I2C_ERR_TIMEOUT,
  HAL_I2C_ERR_BUS,
  HAL_I2C_ERR_NACK,
  HAL_I2C_ERR_REINIT
} hal_i2c_status_t;
typedef enum { I2C1 = 0, I2C2 = 1, I2C3 = 2 } hal_i2c_bus_t;
typedef enum { STANDARD_MODE = 0, FAST_MODE = 1 } hal_i2c_speed_t;

#define I2C_MASTER 0

#define GPIO_FUNC_I2C GPIO_AF04

typedef struct {
  hal_i2c_speed_t clock_speed; // in Hz
  uint8_t own_address;         // 7-bit address (if in slave mode, 0 if master)
  bool acknowledge;            // enable/disable ACK
} hal_i2c_config_t;

// Initializes the I2C peripheral
hal_i2c_status_t hal_i2c_init(hal_i2c_bus_t bus, hal_i2c_config_t *config);

// Writes data to a device
hal_i2c_status_t hal_i2c_write(uint8_t bus, uint8_t dev_addr,
                               const uint8_t *data, uint16_t len);

// Reads data from a device
hal_i2c_status_t hal_i2c_read(uint8_t bus, uint8_t dev_addr, uint8_t *data,
                              uint16_t len);

// Writes to a device register and reads back data
hal_i2c_status_t hal_i2c_write_read(uint8_t bus, uint8_t dev_addr,
                                    const uint8_t *tx_data, uint16_t tx_len,
                                    uint8_t *rx_data, uint16_t rx_len);
uint8_t hal_i2c_get_init_status(void);
#endif // !CORTEX_M4_I2C_H
