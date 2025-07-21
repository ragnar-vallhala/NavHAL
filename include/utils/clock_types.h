#ifndef CLOCK_TYPES_H
#define CLOCK_TYPES_H
#include <stdint.h>

typedef enum
{
    HAL_CLOCK_SOURCE_HSI,
    HAL_CLOCK_SOURCE_HSE,
    HAL_CLOCK_SOURCE_PLL
} hal_clock_source_t;

typedef struct {
    hal_clock_source_t source;
} hal_clock_config_t;



#endif // !CLOCK_TYPES_H