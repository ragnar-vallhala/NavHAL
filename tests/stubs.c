/**
 * @file stubs.c
 * @brief System-level function implementations for Cortex-M4
 * @details
 * This file provides minimal implementations of standard library functions
 * required for the NAVHAL framework. It includes:
 * - Character output (putchar)
 * - Program termination (abort)
 * - Memory operations (memcpy)
 *
 * These implementations are optimized for embedded use and avoid dependencies
 * on the standard C library.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

/**
 * @brief Write a character to the standard output
 * @param[in] ch Character to output
 * @return The character written
 * @details
 * Implements the standard putchar() function by redirecting output
 * to UART2. This enables printf() functionality when using the
 * standard library.
 */
int putchar(int ch) {
  uart2_write_char(ch);
  return ch;
}

/**
 * @brief Abnormal program termination
 * @details
 * Implements the standard abort() function by entering an infinite loop.
 * This is the default behavior for unrecoverable errors in the
 * embedded environment.
 */
void abort(void) {
  while (1) {
  }
}

/**
 * @brief Copy memory area
 * @param[out] dest Pointer to destination memory
 * @param[in] src Pointer to source memory
 * @param[in] n Number of bytes to copy
 * @return Pointer to destination memory
 * @details
 * Implements the standard memcpy() function with byte-by-byte copy.
 * This is a minimal implementation without alignment optimizations.
 */
void *memcpy(void *dest, const void *src, unsigned int n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}