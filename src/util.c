#include "utils/util.h"

void hal_memcpy(void *dest, const void *src, uint32_t len)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    while (len--)
        *d++ = *s++;
}
