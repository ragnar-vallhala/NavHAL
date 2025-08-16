#include "core/cortex-m4/uart.h"

int main(void) {
  // Initialize UART2 at 115200 baud
  uart2_init(115200);

  uart2_write_string("UART2 Echo Started...\r\n");

  while (1) {
    // Wait for a character
    char c = uart2_read_char();

    // Echo it back
    if (c != '\r' && c != '\n')
      c++;
    uart2_write_char(c);
  }

  return 0; // Never reached
}
