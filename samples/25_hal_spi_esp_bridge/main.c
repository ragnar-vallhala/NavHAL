#define CORTEX_M4
#include "core/cortex-m4/config.h"
#include "core/cortex-m4/spi.h"
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
  hal_gpio_digitalwrite(CS_PIN, GPIO_LOW);
  delay_us(10); // Small delay for bit-banging slave to recognize
}

void esp_cs_deselect(void) {
  delay_us(10);
  hal_gpio_digitalwrite(CS_PIN, GPIO_HIGH);
}

int main(void) {
  systick_init(1000);
  uart2_init(115200);

  uart2_write_string("SPI ESP8266 Bridge Sample Starting...\r\n");

  // Configure CS pin
  hal_gpio_enable_rcc(CS_PIN);
  hal_gpio_setmode(CS_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  hal_gpio_digitalwrite(CS_PIN, GPIO_HIGH);

  // Initialize SPI1
  // ESP8266 bit-banging is slow, using highest prescaler (DIV256)
  hal_spi_config_t spi_cfg = {.baudrate = SPI_BAUDRATE_DIV256,
                              .cpol = SPI_CPOL_LOW,
                              .cpha = SPI_CPHA_1EDGE,
                              .datasize = SPI_DATASIZE_8BIT,
                              .firstbit = SPI_FIRSTBIT_MSB};

  if (hal_spi_init(SPI_1, &spi_cfg) != HAL_SPI_OK) {
    uart2_write_string("SPI Initialization Failed!\r\n");
    while (1)
      ;
  }

  uart2_write_string("SPI Initialized. Waiting for UART input...\r\n");

  char tx_buf[128];
  char rx_buf[sizeof(tx_buf)];

  while (1) {
    uint16_t len = 0;

    // Read all available bytes from UART2, batching fast-arriving bytes
    while (uart2_available() && len < (sizeof(tx_buf) - 1)) {
      tx_buf[len++] = uart2_read_char();
      delay_ms(2); // Small delay to allow the next byte to arrive
    }

    if (len > 0) {
      esp_cs_select();

      hal_spi_status_t status = hal_spi_transmit_receive(
          SPI_1, (uint8_t *)tx_buf, (uint8_t *)rx_buf, len, 100);

      esp_cs_deselect();

      if (status == HAL_SPI_OK) {
        for (uint16_t i = 0; i < len; i++) {
          // Print valid characters ignoring empty/dummy bytes from slave
          uint8_t c = rx_buf[i];
          if (c == '\r' || c == '\n' || c == '\t' || (c >= 32 && c <= 126)) {
            uart_write_char(c, UART2);
          }
        }
      } else {
        uart2_write_string("SPI Transfer Error!\r\n");
      }
    } else {
      // Continuously poll SPI to read asynchronous data from the slave
      uint8_t dummy_tx = 0x00;
      uint8_t dummy_rx = 0x00;

      esp_cs_select();
      hal_spi_status_t status =
          hal_spi_transmit_receive(SPI_1, &dummy_tx, &dummy_rx, 1, 100);
      esp_cs_deselect();

      if (status == HAL_SPI_OK) {
        // Only print valid ASCII text (filters out weird idle bus states)
        if (dummy_rx == '\r' || dummy_rx == '\n' || dummy_rx == '\t' ||
            (dummy_rx >= 32 && dummy_rx <= 126)) {
          uart_write_char(dummy_rx, UART2);
        }
      }

      delay_ms(5); // Increased delay slightly to avoid spamming the bus
    }
  }

  return 0;
}
