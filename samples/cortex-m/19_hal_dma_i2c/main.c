/**
 * @file main.c
 * @brief Benchmark: 1000 Iteration BMX160 DMA fast-read.
 */

#define CORTEX_M4
#include "navhal_port_clock.h"
#include "navhal.h"

#define BMX160_I2C_ADDR 0x68
#define I2C_BUS HAL_I2C_1
#define I2C_PIN_1 GPIO_PB08 // SCL
#define I2C_PIN_2 GPIO_PB09 // SDA

volatile bool dma_rx_complete = false;

void on_dma_complete(void) { dma_rx_complete = true; }

void unstick_i2c_bus(void) {
  hal_gpio_set_mode(GPIO_PB08, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_UP);
  hal_gpio_set_mode(GPIO_PB09, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_UP);
  hal_gpio_set_output_type(GPIO_PB08, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_type(GPIO_PB09, HAL_GPIO_OTYPE_OPEN_DRAIN);

  hal_gpio_write(GPIO_PB09, HAL_GPIO_HIGH);
  for (volatile int i = 0; i < 100; i++)
    ;

  for (int i = 0; i < 9; ++i) {
    hal_gpio_write(GPIO_PB08, HAL_GPIO_LOW);
    for (volatile int j = 0; j < 200; j++)
      ;
    hal_gpio_write(GPIO_PB08, HAL_GPIO_HIGH);
    for (volatile int j = 0; j < 200; j++)
      ;
  }

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

  hal_timebase_init(1000);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});

  hal_delay_ms(100);
  hal_uart_print(HAL_UART_2, "Initializing BMX160 for DMA Benchmark...\n\r");

  unstick_i2c_bus();

  hal_i2c_config_t i2c_config = {
      .clock_speed = HAL_I2C_SPEED_FAST, .own_address = I2C_MASTER, .acknowledge = true};

  hal_gpio_set_alternate_function(I2C_PIN_1, GPIO_FUNC_I2C);
  hal_gpio_set_alternate_function(I2C_PIN_2, GPIO_FUNC_I2C);
  hal_gpio_set_output_type(I2C_PIN_1, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_type(I2C_PIN_2, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_speed(I2C_PIN_1, HAL_GPIO_SPEED_VERY_HIGH);
  hal_gpio_set_output_speed(I2C_PIN_2, HAL_GPIO_SPEED_VERY_HIGH);

  hal_i2c_init(I2C_BUS, &i2c_config);

  uint8_t tx_buf[2];

  tx_buf[0] = 0x7E;
  tx_buf[1] = 0x11; // ACC
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(50);

  tx_buf[1] = 0x15; // GYRO
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(100);

  tx_buf[1] = 0x19; // MAG
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  hal_delay_ms(100);

  hal_uart_print(HAL_UART_2, "Sensors ready. Executing 1000 iteration DMA read...\n\r");

  uint8_t rx_buf[30];

  // Configure DMA for HAL_I2C_1 RX (DMA1, Stream 0, Channel 1)
  hal_dma_config_t i2c_dma_cfg = {
      .controller = HAL_DMA_CONTROLLER_1,
      .stream = 0,
      .channel = 1,
      .direction = HAL_DMA_DIR_P2M,
      .src_addr = (uint32_t)(0x40005400 + 0x10), // I2C1_BASE + DR Offset
      .dst_addr = (uint32_t)rx_buf,
      .data_count = 30,
      .src_inc = 0,
      .dst_inc = 1,
      .data_width = HAL_DMA_DATA_WIDTH_8,
      .priority = HAL_DMA_PRIORITY_HIGH,
      .circular = 0};

  uint32_t start_time = hal_timebase_get_tick();

  for (int i = 0; i < 1000; i++) {
    dma_rx_complete = false;

    hal_status_t stat = hal_i2c_read_regs_dma(
        I2C_BUS, BMX160_I2C_ADDR, 0x04, &i2c_dma_cfg, on_dma_complete);
    if (stat != HAL_OK) {
      hal_uart_print(HAL_UART_2, "DMA Transaction start failed on iteration: ");
      hal_uart_write_int(HAL_UART_2, i);
      hal_uart_print(HAL_UART_2, " with error code: ");
      hal_uart_write_int(HAL_UART_2, stat);
      hal_uart_print(HAL_UART_2, "\n\r");
      break;
    }

    while (!dma_rx_complete) {
      // Wait for interrupt
    }
  }

  uint32_t end_time = hal_timebase_get_tick();

  // Print results for 1 iteration simply to prove reading succeeded
  int16_t ax = (int16_t)((rx_buf[15] << 8) | rx_buf[14]);
  int16_t temp = (int16_t)((rx_buf[29] << 8) | rx_buf[28]);

  hal_uart_print(HAL_UART_2, "\n\r--- Benchmark Complete ---\n\r");
  hal_uart_print(HAL_UART_2, "Final Sample - A_X: ");
  hal_uart_write_int(HAL_UART_2, ax);
  hal_uart_print(HAL_UART_2, " TEMP: ");
  hal_uart_write_int(HAL_UART_2, temp);
  hal_uart_print(HAL_UART_2, "\n\r");
  hal_uart_print(HAL_UART_2, "Total Time for 1000 reads: ");
  hal_uart_write_int(HAL_UART_2, end_time - start_time);
  hal_uart_print(HAL_UART_2, " ms\n\r");

  while (1) {
    hal_delay_ms(1000);
  }
}
