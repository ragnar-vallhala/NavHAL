/**
 * @file main.c
 * @brief Example: Read accel, gyro, mag, and temp from BMX160 over I2C and
 * print via HAL_UART_2.
 *
 * @details
 * - Initializes SysTick for delay functions.
 * - Configures HAL_UART_2 at 9600 baud for console output.
 * - Manually unsticks the I2C bus before giving it to the HAL.
 * - Configures HAL_I2C_1 and GPIO pins for BMX160 communication (PB8=SCL, PB9=SDA).
 * - Periodically reads the 30-byte data block and prints values.
 */

#define CORTEX_M4
#include "navhal.h"

// BMX160 I2C address (7-bit)
#define BMX160_I2C_ADDR 0x68

#define I2C_BUS HAL_I2C_1
#define I2C_MODE HAL_I2C_SPEED_STANDARD
#define I2C_PIN_1 GPIO_PB08 // SCL
#define I2C_PIN_2 GPIO_PB09 // SDA

#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400UL)

void unstick_i2c_bus(void) {
  // Try to manually clear the I2C bus in case the slave is stuck
  // We temporarily take control of PB8 and PB9
  hal_gpio_set_mode(GPIO_PB08, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_UP);
  hal_gpio_set_mode(GPIO_PB09, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_UP);
  hal_gpio_set_output_type(GPIO_PB08, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_type(GPIO_PB09, HAL_GPIO_OTYPE_OPEN_DRAIN);

  // Set SDA high
  hal_gpio_write(GPIO_PB09, HAL_GPIO_HIGH);
  for (volatile int i = 0; i < 100; i++)
    ;

  // Toggle SCL 9 times
  for (int i = 0; i < 9; ++i) {
    hal_gpio_write(GPIO_PB08, HAL_GPIO_LOW);
    for (volatile int j = 0; j < 200; j++)
      ;
    hal_gpio_write(GPIO_PB08, HAL_GPIO_HIGH);
    for (volatile int j = 0; j < 200; j++)
      ;
  }

  // Stop condition: SCL high, then SDA goes high
  hal_gpio_write(GPIO_PB09, HAL_GPIO_LOW);
  for (volatile int j = 0; j < 200; j++)
    ;
  hal_gpio_write(GPIO_PB08, HAL_GPIO_HIGH);
  for (volatile int j = 0; j < 200; j++)
    ;
  hal_gpio_write(GPIO_PB09, HAL_GPIO_HIGH);
  for (volatile int j = 0; j < 200; j++)
    ;
}

int main(void) {
  hal_timebase_init(1000); /**< Initialize SysTick for delays */
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});   /**< Initialize HAL_UART_2 at 9600 baud */

  hal_uart_print(HAL_UART_2, "Initializing BMX160 via HAL...\n\r");

  // Manually unstick bus before starting
  unstick_i2c_bus();

  // I2C configuration
  hal_i2c_config_t i2c_config = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                                 .own_address = I2C_MASTER,
                                 .acknowledge = true};

  // Configure GPIO for HAL_I2C_1 (PB8=SCL, PB9=SDA)
  hal_gpio_set_alternate_function(I2C_PIN_1, GPIO_FUNC_I2C);
  hal_gpio_set_alternate_function(I2C_PIN_2, GPIO_FUNC_I2C);
  hal_gpio_set_output_type(I2C_PIN_1, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_type(I2C_PIN_2, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_speed(I2C_PIN_1, HAL_GPIO_SPEED_VERY_HIGH);
  hal_gpio_set_output_speed(I2C_PIN_2, HAL_GPIO_SPEED_VERY_HIGH);

  // Initialize I2C bus
  hal_status_t status = hal_i2c_init(I2C_BUS, &i2c_config);
  if (status != HAL_OK) {
    hal_uart_print(HAL_UART_2, "HAL I2C init failed.\n\r");
    while (1)
      ; /**< Stop execution if I2C init fails */
  }

  uint8_t tx_buf[2];

  // Read CHIP ID
  uint8_t chip_id = 0;
  tx_buf[0] = 0x00;
  if (hal_i2c_write_read(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 1, &chip_id, 1) ==
      HAL_OK) {
    if (chip_id != 0xD8) {
      hal_uart_print(HAL_UART_2, 
          "BMX160 Chip ID mismatch (Not 0xD8). Continuing anyway...\n\r");
    } else {
      hal_uart_print(HAL_UART_2, "BMX160 Chip ID OK: 0xD8\n\r");
    }
  } else {
    hal_uart_print(HAL_UART_2, "Failed to read Chip ID.\n\r");
  }

  // Power up ACC (0x11)
  tx_buf[0] = 0x7E;
  tx_buf[1] = 0x11;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(50);

  // Power up GYRO (0x15)
  tx_buf[1] = 0x15;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(100);

  // Power up MAG (0x19)
  tx_buf[1] = 0x19;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(100);

  hal_uart_print(HAL_UART_2, "Sensors powered up. Starting loop...\n\r");

  uint8_t rx_buf[30]; // 0x04 to 0x21

  while (1) {
    tx_buf[0] = 0x04; // MAG_X_LSB
    status =
        hal_i2c_write_read(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 1, rx_buf, 30);

    if (status != HAL_OK) {
      hal_uart_print(HAL_UART_2, "I2C Read Error\n\r");
    } else {
      // MAG is 0x04-0x0B (Index 0-7)
      int16_t mx = (int16_t)((rx_buf[1] << 8) | rx_buf[0]);
      int16_t my = (int16_t)((rx_buf[3] << 8) | rx_buf[2]);
      int16_t mz = (int16_t)((rx_buf[5] << 8) | rx_buf[4]);

      // GYR is 0x0C-0x11 (Index 8-13)
      int16_t gx = (int16_t)((rx_buf[9] << 8) | rx_buf[8]);
      int16_t gy = (int16_t)((rx_buf[11] << 8) | rx_buf[10]);
      int16_t gz = (int16_t)((rx_buf[13] << 8) | rx_buf[12]);

      // ACC is 0x12-0x17 (Index 14-19)
      int16_t ax = (int16_t)((rx_buf[15] << 8) | rx_buf[14]);
      int16_t ay = (int16_t)((rx_buf[17] << 8) | rx_buf[16]);
      int16_t az = (int16_t)((rx_buf[19] << 8) | rx_buf[18]);

      // TEMP is 0x20-0x21 (Index 28-29)
      int16_t temp = (int16_t)((rx_buf[29] << 8) | rx_buf[28]);

      hal_uart_print(HAL_UART_2, "A:");
      hal_uart_write_int(HAL_UART_2, ax);
      hal_uart_print(HAL_UART_2, ",");
      hal_uart_write_int(HAL_UART_2, ay);
      hal_uart_print(HAL_UART_2, ",");
      hal_uart_write_int(HAL_UART_2, az);
      hal_uart_print(HAL_UART_2, " G:");
      hal_uart_write_int(HAL_UART_2, gx);
      hal_uart_print(HAL_UART_2, ",");
      hal_uart_write_int(HAL_UART_2, gy);
      hal_uart_print(HAL_UART_2, ",");
      hal_uart_write_int(HAL_UART_2, gz);
      hal_uart_print(HAL_UART_2, " M:");
      hal_uart_write_int(HAL_UART_2, mx);
      hal_uart_print(HAL_UART_2, ",");
      hal_uart_write_int(HAL_UART_2, my);
      hal_uart_print(HAL_UART_2, ",");
      hal_uart_write_int(HAL_UART_2, mz);
      hal_uart_print(HAL_UART_2, " T:");
      hal_uart_write_int(HAL_UART_2, temp);
      hal_uart_print(HAL_UART_2, "\n\r");
    }

    hal_delay_ms(500);
  }
}
