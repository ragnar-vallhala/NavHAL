/**
 * @file main.c
 * @brief Example: Read raw temperature from BMP180 over I2C and print via UART2.
 *
 * @details
 * - Initializes SysTick for delay functions.
 * - Configures UART2 at 9600 baud for console output.
 * - Configures I2C1 and GPIO pins for BMP180 communication (PB8=SCL, PB9=SDA).
 * - Periodically starts a temperature measurement, reads the raw value, and prints it.
 *
 * @note Delay between measurement and reading follows BMP180 datasheet (~4.5 ms).
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */

#define CORTEX_M4
#include "navhal.h"

// BMP180 I2C address (7-bit)
#define BMP180_ADDR 0x77

// BMP180 registers & commands
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT  0xF6
#define BMP180_CMD_TEMP    0x34

#define I2C_BUS I2C1

int main(void) {
    systick_init(1000); /**< Initialize SysTick for delays */
    uart2_init(9600);   /**< Initialize UART2 at 9600 baud */

    // I2C configuration
    hal_i2c_config_t i2c_config = {
        .clock_speed = STANDARD_MODE,
        .own_address = I2C_MASTER,
        .acknowledge = true
    };

    // Configure GPIO for I2C1 (PB8=SCL, PB9=SDA)
    hal_gpio_set_alternate_function(GPIO_PB08, GPIO_FUNC_I2C);
    hal_gpio_set_alternate_function(GPIO_PB09, GPIO_FUNC_I2C);
    hal_gpio_set_output_type(GPIO_PB08, GPIO_OPEN_DRAIN);
    hal_gpio_set_output_type(GPIO_PB09, GPIO_OPEN_DRAIN);
    hal_gpio_set_output_speed(GPIO_PB08, GPIO_VERY_HIGH_SPEED);
    hal_gpio_set_output_speed(GPIO_PB09, GPIO_VERY_HIGH_SPEED);

    // Initialize I2C bus
    hal_i2c_status_t status = hal_i2c_init(I2C_BUS, &i2c_config);
    if (status != HAL_I2C_OK) {
        uart2_write("HAL I2C init failed: ");
        uart2_write(status);
        uart2_write("\n");
        while (1); /**< Stop execution if I2C init fails */
    }

    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    while (1) {
        // Start temperature measurement
        tx_buf[0] = BMP180_REG_CONTROL;
        tx_buf[1] = BMP180_CMD_TEMP;
        status = hal_i2c_write(I2C_BUS, BMP180_ADDR, tx_buf, 2);
        if (status != HAL_I2C_OK) {
            uart2_write("Failed to start temperature measurement\n");
            continue;
        }

        delay_ms(5); /**< Wait for conversion (~4.5 ms for temperature) */

        // Read temperature result (2 bytes)
        tx_buf[0] = BMP180_REG_RESULT;
        status = hal_i2c_write_read(I2C_BUS, BMP180_ADDR, tx_buf, 1, rx_buf, 2);
        if (status != HAL_I2C_OK) {
            uart2_write("Failed to read temperature\n");
            continue;
        }

        // Combine MSB and LSB
        int16_t temp_raw = (rx_buf[0] << 8) | rx_buf[1];

        // Print raw temperature value
        uart2_write_int(temp_raw); /**< Assumes uart2_write_int exists */
        uart2_write("\r\n");

        delay_ms(1000); /**< Wait 1 second before next measurement */
    }
}
