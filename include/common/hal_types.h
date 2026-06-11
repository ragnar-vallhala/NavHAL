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
 * @file hal_types.h
 * @brief NavHAL common low-level type definitions and I/O qualifiers.
 *
 * @details
 * Single include point for the fixed-width integer types, boolean type, and
 * the memory-mapped I/O access qualifiers (`__I` / `__O` / `__IO`) used
 * throughout the HAL and low-level drivers. Compiler-attribute shims are
 * provided via @ref navhal_compiler.h.
 */

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

/**
 * @defgroup HAL_TYPES Types
 * @ingroup HAL_CORE
 * @brief Cross-driver shared type aliases.
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/navhal_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif
/** @name Memory-mapped I/O access qualifiers
 *  @{ */
#define __I  volatile const /**< Read-only register access. */
#define __O  volatile       /**< Write-only register access. */
#define __IO volatile       /**< Read/write register access. */
/** @} */

#ifndef NULL
#define NULL ((void *)0) /**< Null pointer constant. */
#endif

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization aliases.
 *
 * Retained so existing drivers keep building, as a backward-compat alias.
 * New code MUST use the standard names instead.
 * -------------------------------------------------------------------------- */
#ifndef byte
#define byte uint8_t /**< @deprecated Use uint8_t. */
#endif
#ifndef __UNUSED
#define __UNUSED NAVHAL_UNUSED /**< @deprecated Use NAVHAL_UNUSED. */
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_TYPES */
#endif /* HAL_TYPES_H */
