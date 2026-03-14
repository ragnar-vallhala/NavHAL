#include "utils/util.h"

void hal_memcpy(void *dest, const void *src, uint32_t len) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  while (len--)
    *d++ = *s++;
}
void hal_memset(void *dest, int val, uint32_t len) {
  uint8_t *d = (uint8_t *)dest;
  while (len--)
    *d++ = (uint8_t)val;
}

int hal_memcmp(const void *s1, const void *s2, uint32_t len) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;
  while (len--) {
    if (*p1 != *p2)
      return (int)*p1 - (int)*p2;
    p1++;
    p2++;
  }
  return 0;
}

uint32_t hal_strlen(const char *s) {
  uint32_t len = 0;
  while (*s++)
    len++;
  return len;
}

char *hal_strchr(const char *s, int c) {
  while (*s != (char)c) {
    if (!*s)
      return 0;
    s++;
  }
  return (char *)s;
}
