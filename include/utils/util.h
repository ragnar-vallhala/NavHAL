#ifndef HAL_UTIL_H
#define HAL_UTIL_H
#include <stdint.h>

void hal_memcpy(void *dest, const void *src, uint32_t len);
void hal_memset(void *dest, int val, uint32_t len);
int hal_memcmp(const void *s1, const void *s2, uint32_t len);
uint32_t hal_strlen(const char *s);
char *hal_strchr(const char *s, int c);

#endif // !HAL_UTIL_H