/**
 * @file hal_types.h
 * @brief NavHAL common low-level type definitions and I/O qualifiers.
 *
 * @details
 * Single include point for the fixed-width integer types, boolean type, and
 * the memory-mapped I/O access qualifiers (`__I` / `__O` / `__IO`) used
 * throughout the HAL and low-level drivers. Compiler-attribute shims are
 * provided via @ref navhal_compiler.h.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

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
#endif /* HAL_TYPES_H */
