/**
 * @file main.c
 * @brief Benchmark: 1000 Iteration BMX160 DMA fast-read.
 */

#define CORTEX_M4
#include "core/cortex-m4/clock.h"
#include "navhal.h"

#define BMX160_I2C_ADDR 0x68
#define I2C_BUS I2C1
#define I2C_PIN_1 GPIO_PB08 // SCL
#define I2C_PIN_2 GPIO_PB09 // SDA

volatile bool dma_rx_complete = false;

void on_dma_complete(void) { dma_rx_complete = true; }

void unstick_i2c_bus(void) {
  hal_gpio_setmode(GPIO_PB08, GPIO_OUTPUT, GPIO_PULLUP);
  hal_gpio_setmode(GPIO_PB09, GPIO_OUTPUT, GPIO_PULLUP);
  hal_gpio_set_output_type(GPIO_PB08, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_type(GPIO_PB09, GPIO_OPEN_DRAIN);

  hal_gpio_digitalwrite(GPIO_PB09, GPIO_HIGH);
  for (volatile int i = 0; i < 100; i++)
    ;

  for (int i = 0; i < 9; ++i) {
    hal_gpio_digitalwrite(GPIO_PB08, GPIO_LOW);
    for (volatile int j = 0; j < 200; j++)
      ;
    hal_gpio_digitalwrite(GPIO_PB08, GPIO_HIGH);
    for (volatile int j = 0; j < 200; j++)
      ;
  }

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

  systick_init(1000);
  uart2_init(9600);

  delay_ms(100);
  uart2_write("Initializing BMX160 for DMA Benchmark...\n\r");

  unstick_i2c_bus();

  hal_i2c_config_t i2c_config = {
      .clock_speed = FAST_MODE, .own_address = I2C_MASTER, .acknowledge = true};

  hal_gpio_set_alternate_function(I2C_PIN_1, GPIO_FUNC_I2C);
  hal_gpio_set_alternate_function(I2C_PIN_2, GPIO_FUNC_I2C);
  hal_gpio_set_output_type(I2C_PIN_1, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_type(I2C_PIN_2, GPIO_OPEN_DRAIN);
  hal_gpio_set_output_speed(I2C_PIN_1, GPIO_VERY_HIGH_SPEED);
  hal_gpio_set_output_speed(I2C_PIN_2, GPIO_VERY_HIGH_SPEED);

  hal_i2c_init(I2C_BUS, &i2c_config);

  uint8_t tx_buf[2];

  tx_buf[0] = 0x7E;
  tx_buf[1] = 0x11; // ACC
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  delay_ms(50);

  tx_buf[1] = 0x15; // GYRO
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  delay_ms(100);

  tx_buf[1] = 0x19; // MAG
  hal_i2c_write(I2C_BUS, BMX160_I2C_ADDR, tx_buf, 2);
  delay_ms(100);

  uart2_write("Sensors ready. Executing 1000 iteration DMA read...\n\r");

  uint8_t rx_buf[30];

  // Configure DMA for I2C1 RX (DMA1, Stream 0, Channel 1)
  dma_config_t i2c_dma_cfg = {
      .controller = DMA_CONTROLLER_1,
      .stream = 0,
      .channel = 1,
      .direction = DMA_DIR_P2M,
      .src_addr = (uint32_t)(0x40005400 + 0x10), // I2C1_BASE + DR Offset
      .dst_addr = (uint32_t)rx_buf,
      .data_count = 30,
      .src_inc = 0,
      .dst_inc = 1,
      .data_width = DMA_DATA_WIDTH_8,
      .priority = DMA_PRIORITY_HIGH,
      .circular = 0};

  uint32_t start_time = hal_get_tick();

  for (int i = 0; i < 1000; i++) {
    dma_rx_complete = false;

    hal_i2c_status_t stat = hal_i2c_read_regs_dma(
        I2C_BUS, BMX160_I2C_ADDR, 0x04, &i2c_dma_cfg, on_dma_complete);
    if (stat != HAL_I2C_OK) {
      uart2_write("DMA Transaction start failed on iteration: ");
      uart2_write_int(i);
      uart2_write(" with error code: ");
      uart2_write_int(stat);
      uart2_write("\n\r");
      break;
    }

    while (!dma_rx_complete) {
      // Wait for interrupt
    }
  }

  uint32_t end_time = hal_get_tick();

  // Print results for 1 iteration simply to prove reading succeeded
  int16_t ax = (int16_t)((rx_buf[15] << 8) | rx_buf[14]);
  int16_t temp = (int16_t)((rx_buf[29] << 8) | rx_buf[28]);

  uart2_write("\n\r--- Benchmark Complete ---\n\r");
  uart2_write("Final Sample - A_X: ");
  uart2_write_int(ax);
  uart2_write(" TEMP: ");
  uart2_write_int(temp);
  uart2_write("\n\r");
  uart2_write("Total Time for 1000 reads: ");
  uart2_write_int(end_time - start_time);
  uart2_write(" ms\n\r");

  while (1) {
    delay_ms(1000);
  }
}
