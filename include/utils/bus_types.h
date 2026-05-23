#ifndef BUS_TYPES_H
#define BUS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
    HAL_APB1,
    HAL_APB2,
    HAL_AHB1,
    HAL_AHB2,
} hal_bus_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !BUS_TYPES_H