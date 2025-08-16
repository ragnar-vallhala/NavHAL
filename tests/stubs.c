#define CORTEX_M4
#include "navhal.h"

int putchar(int ch) {
  if (ch == '\n')
    uart2_write_char('\r');
  uart2_write_char(ch);
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
