#ifndef HAL_UTIL_H
#define HAL_UTIL_H
#include <stdint.h>

void hal_memcpy(void *dest, const void *src, uint32_t len);

#endif // !HAL_UTIL_H