/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#define VERSION_MINOR 4
#define VERSION_PATCH 1
#define VERSION "0.4.1"

/**
 * @brief The release version under NavHAL's own prefix, for application code.
 *
 * Derived from the four lines above, which stay the single source of truth
 * (the CMake and SemVer-gate checks parse them). @c NAVHAL_VERSION packs the
 * three parts as 0xMMmmpp, so a caller can bridge an API rename with
 * `#if NAVHAL_VERSION >= NAVHAL_VERSION_ENCODE(0, 4, 1)`.
 */
#define NAVHAL_VERSION_MAJOR  VERSION_MAJOR
/** @brief Minor part of the release version. */
#define NAVHAL_VERSION_MINOR  VERSION_MINOR
/** @brief Patch part of the release version. */
#define NAVHAL_VERSION_PATCH  VERSION_PATCH
/** @brief The release version as a string, e.g. "0.4.1". */
#define NAVHAL_VERSION_STRING VERSION
/** @brief Pack a version as 0xMMmmpp, comparable with @c NAVHAL_VERSION. */
#define NAVHAL_VERSION_ENCODE(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))
/** @brief This release, packed by @c NAVHAL_VERSION_ENCODE. */
#define NAVHAL_VERSION \
    NAVHAL_VERSION_ENCODE(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

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

#include "common/hal_adc.h"

#include "common/hal_interrupt.h"

#include "common/hal_fpu.h"

#include "common/hal_config.h"
#include "common/hal_console.h"

#include "common/hal_cache.h"
#include "common/hal_crc.h"
#include "common/hal_diskio.h"
#include "common/hal_boot.h"
#include "common/hal_dma.h"
#include "common/hal_dwt.h"
#include "common/hal_eth.h"
#include "common/hal_flash.h"
#include "common/hal_mpu.h"
#include "common/hal_reset.h"
#include "common/hal_rtc.h"
#include "common/hal_sdio.h"
#include "common/hal_tcm.h"
#include "common/hal_usb_cdc.h"
#include "common/hal_watchdog.h"


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // NAVHAL_H
