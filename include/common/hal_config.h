#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef CORTEX_M4
#include "core/cortex-m4/config.h"
#endif // CORTEX_M4

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // HAL_CONFIG_H