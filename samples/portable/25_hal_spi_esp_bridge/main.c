#define CORTEX_M4
#include "navhal_port_config.h"
#include "navhal_port_spi.h"
#include "navhal.h"
#include <stddef.h>

size_t _strlen(const char *s) {
  size_t i = 0;
  while (s[i])
    i++;
  return i;
}

/*
 * Sample: SPI communication with ESP8266 (Slave)
 * STM32F401 (Master) -> ESP8266 (Slave, bit-banging)
 *
 * Pins:
 * - PA4: CS (Manual GPIO)
 * - PA5: SCK (SPI1)
 * - PA6: MISO (SPI1)
 * - PA7: MOSI (SPI1)
 */

#define CS_PIN GPIO_PA04

void esp_cs_select(void) {
  hal_gpio_write(CS_PIN, HAL_GPIO_LOW);
  hal_delay_us(10); // Small delay for bit-banging slave to recognize
}

void esp_cs_deselect(void) {
  hal_delay_us(10);
  hal_gpio_write(CS_PIN, HAL_GPIO_HIGH);
}

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=115200});

  hal_uart_write_string(HAL_UART_2, "SPI ESP8266 Bridge Sample Starting...\r\n");

  // Configure CS pin
  hal_gpio_enable_clock(CS_PIN);
  hal_gpio_set_mode(CS_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(CS_PIN, HAL_GPIO_HIGH);

  // Initialize SPI1
  // ESP8266 bit-banging is slow, using highest prescaler (DIV256)
  hal_spi_config_t spi_cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV256,
                              .cpol = HAL_SPI_CPOL_LOW,
                              .cpha = HAL_SPI_CPHA_1EDGE,
                              .datasize = HAL_SPI_DATASIZE_8BIT,
                              .firstbit = HAL_SPI_FIRSTBIT_MSB};

  if (hal_spi_init(HAL_SPI_1, &spi_cfg) != HAL_OK) {
    hal_uart_write_string(HAL_UART_2, "SPI Initialization Failed!\r\n");
    while (1)
      ;
  }

  hal_uart_write_string(HAL_UART_2, "SPI Initialized. Waiting for UART input...\r\n");

  char tx_buf[128];
  char rx_buf[sizeof(tx_buf)];

  while (1) {
    uint16_t len = 0;

    // Read all available bytes from HAL_UART_2, batching fast-arriving bytes
    while (hal_uart_available(HAL_UART_2) && len < (sizeof(tx_buf) - 1)) {
      tx_buf[len++] = hal_uart_read_char(HAL_UART_2);
      hal_delay_ms(2); // Small delay to allow the next byte to arrive
    }

    if (len > 0) {
      esp_cs_select();

      hal_status_t status = hal_spi_transmit_receive(
          HAL_SPI_1, (uint8_t *)tx_buf, (uint8_t *)rx_buf, len, 100);

      esp_cs_deselect();

      if (status == HAL_OK) {
        for (uint16_t i = 0; i < len; i++) {
          // Print valid characters ignoring empty/dummy bytes from slave
          uint8_t c = rx_buf[i];
          if (c == '\r' || c == '\n' || c == '\t' || (c >= 32 && c <= 126)) {
            hal_uart_write_char(HAL_UART_2, c);
          }
        }
      } else {
        hal_uart_write_string(HAL_UART_2, "SPI Transfer Error!\r\n");
      }
    } else {
      // Continuously poll SPI to read asynchronous data from the slave
      uint8_t dummy_tx = 0x00;
      uint8_t dummy_rx = 0x00;

      esp_cs_select();
      hal_status_t status =
          hal_spi_transmit_receive(HAL_SPI_1, &dummy_tx, &dummy_rx, 1, 100);
      esp_cs_deselect();

      if (status == HAL_OK) {
        // Only print valid ASCII text (filters out weird idle bus states)
        if (dummy_rx == '\r' || dummy_rx == '\n' || dummy_rx == '\t' ||
            (dummy_rx >= 32 && dummy_rx <= 126)) {
          hal_uart_write_char(HAL_UART_2, dummy_rx);
        }
      }

      hal_delay_ms(5); // Increased delay slightly to avoid spamming the bus
    }
  }

  return 0;
}
