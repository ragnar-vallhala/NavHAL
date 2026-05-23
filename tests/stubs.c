#define CORTEX_M4
#include "navhal.h"

int putchar(int ch) {
  hal_uart_write_char(HAL_UART_2, ch);
  return ch;
}

void abort(void) {
  while (1) {
  }
}

void *memcpy(void *dest, const void *src, unsigned int n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}
