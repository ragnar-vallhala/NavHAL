#define CORTEX_M4
#include "navhal.h"

// BMP180 I2C address (7-bit)
#define BMP180_ADDR 0x77

// BMP180 registers & commands
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT 0xF6
#define BMP180_CMD_TEMP 0x34

#define I2C_BUS I2C1

int main(void) {
  systick_init(1000); // SysTick for delays
  uart2_init(9600);   // UART2 for debugging

  // I2C configuration
  hal_i2c_config_t i2c_config = {.clock_speed = STANDARD_MODE,
                                 .own_address = I2C_MASTER,
                                 .acknowledge = true};

  // Configure GPIO for I2C1 (PB8 = SCL, PB9 = SDA)
  hal_gpio_set_alternate_function(GPIO_PB08, GPIO_FUNC_I2C);
  hal_gpio_set_alternate_function(GPIO_PB09, GPIO_FUNC_I2C);
  hal_gpio_set_output_type(GPIO_PB08, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_type(GPIO_PB09, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_speed(GPIO_PB08, GPIO_VERY_HIGH_SPEED);
  hal_gpio_set_output_speed(GPIO_PB09, GPIO_VERY_HIGH_SPEED);

  // Initialize I2C
  hal_i2c_status_t status = hal_i2c_init(I2C_BUS, &i2c_config);
  status = hal_i2c_init(I2C_BUS, &i2c_config);
  if (status != HAL_I2C_OK) {
    uart2_write("HAL I2C init status: ");
    uart2_write(status);
    uart2_write("\n");
    while (1)
      ;
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

    // Wait for conversion (datasheet: 4.5ms for temp)
    delay_ms(5);

    // Read temperature result (2 bytes)
    tx_buf[0] = BMP180_REG_RESULT;
    status = hal_i2c_write_read(I2C_BUS, BMP180_ADDR, tx_buf, 1, rx_buf, 2);
    if (status != HAL_I2C_OK) {
      uart2_write("Failed to read temperature\n");
      continue;
    }

    // Combine MSB/LSB
    int16_t temp_raw = (rx_buf[0] << 8) | rx_buf[1];

    // Print raw value
    uart2_write_int(temp_raw); // assuming uart2_write_int exists
    uart2_write("\r\n");

    delay_ms(10); // wait 1s before next read
  }
}
