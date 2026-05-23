/**
 * @file navhal.h
 * @brief Root header file for the NavHAL project by NavRobotec.
 *
 * NavHAL is the official hardware abstraction layer (HAL) developed by
 * NavRobotec. It provides a clean, modular, and extensible C interface to
 * abstract peripheral control (such as GPIO, UART, timers) across multiple
 * microcontroller architectures.
 *
 * Include this file in your application to access the full HAL API.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#ifndef NAVHAL_H
#define NAVHAL_H

/* Release version (SemVer) — the version of this NavHAL distribution. */
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0
#define VERSION "0.1.0"

/**
 * @brief NavHAL public-API contract version.
 *
 * An integer, independent of the SemVer release version above. It is the
 * frozen v1 `hal_*` API surface described in `docs/api_standardization.md`;
 * it is bumped only on a breaking change to a public header. Application
 * code may test it with `#if HAL_API_VERSION >= N`. When a second port
 * lands (M6, AVR), each port asserts the contract version it implements
 * against this value.
 */
#define HAL_API_VERSION 1


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup NAVHAL Core HAL
 * @brief Top-level includes for the NavHAL framework.
 * @{
 */

/// Common GPIO HAL header (architecture-agnostic interface)
#include "common/hal_gpio.h"

/// Common Clock HAL header (architecture-agnostic interface)
#include "common/hal_clock.h"

/// Common UART HAL header (architecture-agnostic interface)
#include "common/hal_uart.h"

/// Common UART HAL header (architecture-agnostic interface)
#include "common/hal_timer.h"
/** @} */ // end of NAVHAL

#include "common/hal_pwm.h"

#include "common/hal_i2c.h"

#include "common/hal_spi.h"

#include "common/hal_interrupt.h"

#include "common/hal_fpu.h"

#include "common/hal_config.h"

#include "common/hal_crc.h"
#include "common/hal_diskio.h"
#include "common/hal_dma.h"
#include "common/hal_dwt.h"
#include "common/hal_flash.h"
#include "common/hal_sdio.h"


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // NAVHAL_H
