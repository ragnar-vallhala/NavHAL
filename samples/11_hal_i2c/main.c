/**
 * @file main.c
 * @brief Example: Read accel, gyro, mag, and temp from BMX160 over I2C and
 * print via UART2.
 *
 * @details
 * - Initializes SysTick for delay functions.
 * - Configures UART2 at 9600 baud for console output.
 * - Manually unsticks the I2C bus before giving it to the HAL.
 * - Configures I2C1 and GPIO pins for BMX160 communication (PB8=SCL, PB9=SDA).
 * - Periodically reads the 30-byte data block and prints values.
 */

#define CORTEX_M4
#include "navhal.h"

// BMX160 I2C address (7-bit)
#define BMX160_I2C_ADDR 0x68

#define I2C_BUS I2C1
#define I2C_MODE STANDARD_MODE
#define I2C_PIN_1 GPIO_PB08 // SCL
#define I2C_PIN_2 GPIO_PB09 // SDA

#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400UL)

void unstick_i2c_bus(void) {
  // Try to manually clear the I2C bus in case the slave is stuck
  // We temporarily take control of PB8 and PB9
  hal_gpio_setmode(GPIO_PB08, GPIO_OUTPUT, GPIO_PULLUP);
  hal_gpio_setmode(GPIO_PB09, GPIO_OUTPUT, GPIO_PULLUP);
  hal_gpio_set_output_type(GPIO_PB08, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_type(GPIO_PB09, GPIO_OPEN_DRAIN);

  // Set SDA high
  hal_gpio_digitalwrite(GPIO_PB09, GPIO_HIGH);
  for (volatile int i = 0; i < 100; i++)
    ;

  // Toggle SCL 9 times
  for (int i = 0; i < 9; ++i) {
    hal_gpio_digitalwrite(GPIO_PB08, GPIO_LOW);
    for (volatile int j = 0; j < 200; j++)
      ;
    hal_gpio_digitalwrite(GPIO_PB08, GPIO_HIGH);
    for (volatile int j = 0; j < 200; j++)
      ;
  }

  // Stop condition: SCL high, then SDA goes high
  hal_gpio_digitalwrite(GPIO_PB09, GPIO_LOW);
  for (volatile int j = 0; j < 200; j++)
    ;
  hal_gpio_digitalwrite(GPIO_PB08, GPIO_HIGH);
  for (volatile int j = 0; j < 200; j++)
    ;
  hal_gpio_digitalwrite(GPIO_PB09, GPIO_HIGH);
  for (volatile int j = 0; j < 200; j++)
    ;
}

int main(void) {
  systick_init(1000); /**< Initialize SysTick for delays */
  uart2_init(9600);   /**< Initialize UART2 at 9600 baud */

  uart2_write("Initializing BMX160 via HAL...\n\r");

  // Manually unstick bus before starting
  unstick_i2c_bus();

  // I2C configuration
  hal_i2c_config_t i2c_config = {.clock_speed = STANDARD_MODE,
                                 .own_address = I2C_MASTER,
                                 .acknowledge = true};

  // Configure GPIO for I2C1 (PB8=SCL, PB9=SDA)
  hal_gpio_set_alternate_function(I2C_PIN_1, GPIO_FUNC_I2C);
  hal_gpio_set_alternate_function(I2C_PIN_2, GPIO_FUNC_I2C);
  hal_gpio_set_output_type(I2C_PIN_1, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_type(I2C_PIN_2, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_speed(I2C_PIN_1, GPIO_VERY_HIGH_SPEED);
  hal_gpio_set_output_speed(I2C_PIN_2, GPIO_VERY_HIGH_SPEED);

  // Initialize I2C bus
  hal_i2c_status_t status = hal_i2c_init(I2C_BUS, &i2c_config);
  if (status != HAL_I2C_OK) {
    uart2_write("HAL I2C init failed.\n\r");
    while (1)
      ; /**< Stop execution if I2C init fails */
  }

  uint8_t tx_buf[2];

  // Read CHIP ID
  uint8_t chip_id = 0;
  tx_buf[0] = 0x00;
  if (hal_i2c_write_read(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 1, &chip_id, 1) ==
      HAL_I2C_OK) {
    if (chip_id != 0xD8) {
      uart2_write(
          "BMX160 Chip ID mismatch (Not 0xD8). Continuing anyway...\n\r");
    } else {
      uart2_write("BMX160 Chip ID OK: 0xD8\n\r");
    }
  } else {
    uart2_write("Failed to read Chip ID.\n\r");
  }

  // Power up ACC (0x11)
  tx_buf[0] = 0x7E;
  tx_buf[1] = 0x11;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  delay_ms(50);

  // Power up GYRO (0x15)
  tx_buf[1] = 0x15;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  delay_ms(100);

  // Power up MAG (0x19)
  tx_buf[1] = 0x19;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  delay_ms(100);

  uart2_write("Sensors powered up. Starting loop...\n\r");

  uint8_t rx_buf[30]; // 0x04 to 0x21

  while (1) {
    tx_buf[0] = 0x04; // MAG_X_LSB
    status =
        hal_i2c_write_read(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 1, rx_buf, 30);

    if (status != HAL_I2C_OK) {
      uart2_write("I2C Read Error\n\r");
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

      uart2_write("A:");
      uart2_write_int(ax);
      uart2_write(",");
      uart2_write_int(ay);
      uart2_write(",");
      uart2_write_int(az);
      uart2_write(" G:");
      uart2_write_int(gx);
      uart2_write(",");
      uart2_write_int(gy);
      uart2_write(",");
      uart2_write_int(gz);
      uart2_write(" M:");
      uart2_write_int(mx);
      uart2_write(",");
      uart2_write_int(my);
      uart2_write(",");
      uart2_write_int(mz);
      uart2_write(" T:");
      uart2_write_int(temp);
      uart2_write("\n\r");
    }

    delay_ms(500);
  }
}
