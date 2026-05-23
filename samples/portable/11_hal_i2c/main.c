/**
 * @file main.c
 * @brief Read a BMX160 IMU over I²C and print accel/gyro/mag/temp.
 *
 * @details
 * Target-agnostic: the console UART and the I²C bus / SCL / SDA pins are
 * named by the board-layer aliases (::BOARD_CONSOLE_UART, ::BOARD_I2C_BUS,
 * ::BOARD_I2C_SCL, ::BOARD_I2C_SDA), so the same source builds for the
 * Nucleo-F401RE and the ATmega328P.
 *
 * Manually unsticks the bus, brings up the sensor, then prints its 30-byte
 * data block every 500 ms.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "board.h"
#include "navhal.h"

/** @brief BMX160 7-bit I²C address. */
#define BMX160_I2C_ADDR 0x68

#define I2C_BUS BOARD_I2C_BUS
#define I2C_PIN_SCL BOARD_I2C_SCL
#define I2C_PIN_SDA BOARD_I2C_SDA

/** @brief Best-effort recovery of a stuck I²C bus before the HAL takes it. */
void unstick_i2c_bus(void) {
  hal_gpio_set_mode(I2C_PIN_SCL, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_UP);
  hal_gpio_set_mode(I2C_PIN_SDA, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_UP);
  hal_gpio_set_output_type(I2C_PIN_SCL, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_type(I2C_PIN_SDA, HAL_GPIO_OTYPE_OPEN_DRAIN);

  hal_gpio_write(I2C_PIN_SDA, HAL_GPIO_HIGH);
  for (volatile int i = 0; i < 100; i++)
    ;

  /* Clock out up to nine pulses to free a slave holding SDA low. */
  for (int i = 0; i < 9; ++i) {
    hal_gpio_write(I2C_PIN_SCL, HAL_GPIO_LOW);
    for (volatile int j = 0; j < 200; j++)
      ;
    hal_gpio_write(I2C_PIN_SCL, HAL_GPIO_HIGH);
    for (volatile int j = 0; j < 200; j++)
      ;
  }

  /* Stop condition: SDA rises while SCL is high. */
  hal_gpio_write(I2C_PIN_SDA, HAL_GPIO_LOW);
  for (volatile int j = 0; j < 200; j++)
    ;
  hal_gpio_write(I2C_PIN_SCL, HAL_GPIO_HIGH);
  for (volatile int j = 0; j < 200; j++)
    ;
  hal_gpio_write(I2C_PIN_SDA, HAL_GPIO_HIGH);
  for (volatile int j = 0; j < 200; j++)
    ;
}

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});

  hal_uart_print(BOARD_CONSOLE_UART, "Initializing BMX160 via HAL...\n\r");

  unstick_i2c_bus();

  hal_i2c_config_t i2c_config = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                                 .own_address = I2C_MASTER,
                                 .acknowledge = true};

  /* On Cortex-M the SCL/SDA pins are routed through a GPIO alternate
   * function; on the AVR the TWI peripheral owns its fixed pins, so these
   * calls are harmless no-ops there. */
  hal_gpio_set_alternate_function(I2C_PIN_SCL, GPIO_FUNC_I2C);
  hal_gpio_set_alternate_function(I2C_PIN_SDA, GPIO_FUNC_I2C);
  hal_gpio_set_output_type(I2C_PIN_SCL, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_type(I2C_PIN_SDA, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_speed(I2C_PIN_SCL, HAL_GPIO_SPEED_VERY_HIGH);
  hal_gpio_set_output_speed(I2C_PIN_SDA, HAL_GPIO_SPEED_VERY_HIGH);

  hal_status_t status = hal_i2c_init(I2C_BUS, &i2c_config);
  if (status != HAL_OK) {
    hal_uart_print(BOARD_CONSOLE_UART, "HAL I2C init failed.\n\r");
    while (1)
      ;
  }

  uint8_t tx_buf[2];

  uint8_t chip_id = 0;
  tx_buf[0] = 0x00;
  if (hal_i2c_write_read(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 1, &chip_id, 1) ==
      HAL_OK) {
    if (chip_id != 0xD8) {
      hal_uart_print(BOARD_CONSOLE_UART,
                     "BMX160 Chip ID mismatch (Not 0xD8). Continuing...\n\r");
    } else {
      hal_uart_print(BOARD_CONSOLE_UART, "BMX160 Chip ID OK: 0xD8\n\r");
    }
  } else {
    hal_uart_print(BOARD_CONSOLE_UART, "Failed to read Chip ID.\n\r");
  }

  /* Power up ACC, GYRO, MAG (register 0x7E command interface). */
  tx_buf[0] = 0x7E;
  tx_buf[1] = 0x11;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(50);
  tx_buf[1] = 0x15;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(100);
  tx_buf[1] = 0x19;
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(100);

  hal_uart_print(BOARD_CONSOLE_UART, "Sensors powered up. Starting loop...\n\r");

  uint8_t rx_buf[30]; /* registers 0x04..0x21 */

  while (1) {
    tx_buf[0] = 0x04; /* MAG_X_LSB */
    status =
        hal_i2c_write_read(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 1, rx_buf, 30);

    if (status != HAL_OK) {
      hal_uart_print(BOARD_CONSOLE_UART, "I2C Read Error\n\r");
    } else {
      int16_t mx = (int16_t)((rx_buf[1] << 8) | rx_buf[0]);
      int16_t my = (int16_t)((rx_buf[3] << 8) | rx_buf[2]);
      int16_t mz = (int16_t)((rx_buf[5] << 8) | rx_buf[4]);
      int16_t gx = (int16_t)((rx_buf[9] << 8) | rx_buf[8]);
      int16_t gy = (int16_t)((rx_buf[11] << 8) | rx_buf[10]);
      int16_t gz = (int16_t)((rx_buf[13] << 8) | rx_buf[12]);
      int16_t ax = (int16_t)((rx_buf[15] << 8) | rx_buf[14]);
      int16_t ay = (int16_t)((rx_buf[17] << 8) | rx_buf[16]);
      int16_t az = (int16_t)((rx_buf[19] << 8) | rx_buf[18]);
      int16_t temp = (int16_t)((rx_buf[29] << 8) | rx_buf[28]);

      hal_uart_print(BOARD_CONSOLE_UART, "A:");
      hal_uart_write_int(BOARD_CONSOLE_UART, ax);
      hal_uart_print(BOARD_CONSOLE_UART, ",");
      hal_uart_write_int(BOARD_CONSOLE_UART, ay);
      hal_uart_print(BOARD_CONSOLE_UART, ",");
      hal_uart_write_int(BOARD_CONSOLE_UART, az);
      hal_uart_print(BOARD_CONSOLE_UART, " G:");
      hal_uart_write_int(BOARD_CONSOLE_UART, gx);
      hal_uart_print(BOARD_CONSOLE_UART, ",");
      hal_uart_write_int(BOARD_CONSOLE_UART, gy);
      hal_uart_print(BOARD_CONSOLE_UART, ",");
      hal_uart_write_int(BOARD_CONSOLE_UART, gz);
      hal_uart_print(BOARD_CONSOLE_UART, " M:");
      hal_uart_write_int(BOARD_CONSOLE_UART, mx);
      hal_uart_print(BOARD_CONSOLE_UART, ",");
      hal_uart_write_int(BOARD_CONSOLE_UART, my);
      hal_uart_print(BOARD_CONSOLE_UART, ",");
      hal_uart_write_int(BOARD_CONSOLE_UART, mz);
      hal_uart_print(BOARD_CONSOLE_UART, " T:");
      hal_uart_write_int(BOARD_CONSOLE_UART, temp);
      hal_uart_print(BOARD_CONSOLE_UART, "\n\r");
    }

    hal_delay_ms(500);
  }
}
