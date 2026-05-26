/**
 * @file port/cortex-m4/navhal_port_config.h
 * @brief Cortex-M4 / STM32F4 build-time feature flags.
 *
 * @details
 * Bridges the Kconfig-generated @c NAVHAL_HAS_* macros (in @c navhal_target.h)
 * into the legacy @c _*_ENABLED flags that the driver tree #ifdefs on. When a
 * capability is off in Kconfig, the corresponding @c _*_ENABLED flag is not
 * defined and the matching driver source compiles to nothing — matching the
 * AVR port's behaviour, where this header is empty.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */
#ifndef NAVHAL_PORT_CONFIG_H
#define NAVHAL_PORT_CONFIG_H

#include "navhal_target.h"


#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_HAS_FPU
#  ifndef _FPU_ENABLED
#    define _FPU_ENABLED
#  endif
#endif

#if NAVHAL_HAS_DMA
#  ifndef _DMA_ENABLED
#    define _DMA_ENABLED
#  endif
#endif

#if NAVHAL_HAS_CRC_HW
#  ifndef _CRC_HW_ENABLED
#    define _CRC_HW_ENABLED
#  endif
#endif

#if NAVHAL_HAS_CYCLE_COUNTER
#  ifndef _DWT_ENABLED
#    define _DWT_ENABLED
#  endif
#endif

#if NAVHAL_HAS_SDIO
#  ifndef _SDIO_ENABLED
#    define _SDIO_ENABLED
#  endif
#endif

/* Per-driver DMA-backend flags. These let a build keep DRV_DMA on for one
   driver while another opts out (e.g. DRV_DMA=y for SDIO, DRV_UART_DMA=n
   to save flash). Each is the legacy spelling of NAVHAL_HAS_<X>_DMA. */
#if NAVHAL_HAS_UART_DMA
#  ifndef _UART_BACKEND_DMA
#    define _UART_BACKEND_DMA
#  endif
#endif

#if NAVHAL_HAS_I2C_DMA
#  ifndef _I2C_BACKEND_DMA
#    define _I2C_BACKEND_DMA
#  endif
#endif

#if NAVHAL_HAS_SDIO_DMA
#  ifndef _SDIO_BACKEND_DMA
#    define _SDIO_BACKEND_DMA
#  endif
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // NAVHAL_PORT_CONFIG_H
