/**
 * @file main.c
 * @brief UART <-> SPI bridge to a bit-banging SPI slave (e.g. an ESP8266).
 *
 * @details
 * Target-agnostic: the console UART, the SPI bus and the chip-select pin are
 * named by the board-layer aliases (::BOARD_CONSOLE_UART, ::BOARD_SPI_BUS,
 * ::BOARD_SPI_CS), so the same source builds for the Nucleo-F401RE and the
 * ATmega328P.
 *
 * Bytes arriving on the console UART are forwarded to the SPI slave; the
 * slave's full-duplex response is echoed back to the UART.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "board.h"
#include "navhal.h"

#define CS_PIN BOARD_SPI_CS

static void esp_cs_select(void) {
  hal_gpio_write(CS_PIN, HAL_GPIO_LOW);
  hal_delay_us(10); /* let a bit-banging slave notice the edge. */
}

static void esp_cs_deselect(void) {
  hal_delay_us(10);
  hal_gpio_write(CS_PIN, HAL_GPIO_HIGH);
}

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});
  hal_uart_write_string(BOARD_CONSOLE_UART,
                        "SPI ESP8266 Bridge Sample Starting...\r\n");

  /* Chip-select is a plain GPIO output, idle high. */
  hal_gpio_enable_clock(CS_PIN);
  hal_gpio_set_mode(CS_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(CS_PIN, HAL_GPIO_HIGH);

  /* The slave bit-bangs SPI, so use the slowest available clock. */
  hal_spi_config_t spi_cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV256,
                              .cpol = HAL_SPI_CPOL_LOW,
                              .cpha = HAL_SPI_CPHA_1EDGE,
                              .datasize = HAL_SPI_DATASIZE_8BIT,
                              .firstbit = HAL_SPI_FIRSTBIT_MSB};
  if (hal_spi_init(BOARD_SPI_BUS, &spi_cfg) != HAL_OK) {
    hal_uart_write_string(BOARD_CONSOLE_UART, "SPI Initialization Failed!\r\n");
    while (1)
      ;
  }
  hal_uart_write_string(BOARD_CONSOLE_UART,
                        "SPI Initialized. Waiting for UART input...\r\n");

  char tx_buf[128];
  char rx_buf[sizeof(tx_buf)];

  while (1) {
    uint16_t len = 0;

    /* Batch fast-arriving console bytes into one SPI transfer. */
    while (hal_uart_available(BOARD_CONSOLE_UART) &&
           len < (sizeof(tx_buf) - 1)) {
      tx_buf[len++] = hal_uart_read_char(BOARD_CONSOLE_UART);
      hal_delay_ms(2);
    }

    if (len > 0) {
      esp_cs_select();
      hal_status_t status = hal_spi_transmit_receive(
          BOARD_SPI_BUS, (uint8_t *)tx_buf, (uint8_t *)rx_buf, len, 100);
      esp_cs_deselect();

      if (status == HAL_OK) {
        for (uint16_t i = 0; i < len; i++) {
          uint8_t c = (uint8_t)rx_buf[i];
          if (c == '\r' || c == '\n' || c == '\t' || (c >= 32 && c <= 126))
            hal_uart_write_char(BOARD_CONSOLE_UART, (char)c);
        }
      } else {
        hal_uart_write_string(BOARD_CONSOLE_UART, "SPI Transfer Error!\r\n");
      }
    } else {
      /* Poll the slave for asynchronous data with a dummy frame. */
      uint8_t dummy_tx = 0x00;
      uint8_t dummy_rx = 0x00;

      esp_cs_select();
      hal_status_t status = hal_spi_transmit_receive(BOARD_SPI_BUS, &dummy_tx,
                                                     &dummy_rx, 1, 100);
      esp_cs_deselect();

      if (status == HAL_OK) {
        if (dummy_rx == '\r' || dummy_rx == '\n' || dummy_rx == '\t' ||
            (dummy_rx >= 32 && dummy_rx <= 126))
          hal_uart_write_char(BOARD_CONSOLE_UART, (char)dummy_rx);
      }
      hal_delay_ms(5);
    }
  }
  return 0;
}
