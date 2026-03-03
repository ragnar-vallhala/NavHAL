/**
 * @file hal_types.h
 * @brief Hardware Abstraction Layer (HAL) common type definitions.
 *
 * @details
 * This header file defines basic macros and types used throughout the
 * HAL and low-level drivers:
 * - `__IO` : Volatile qualifier for memory-mapped I/O registers.
 * - `NULL` : Null pointer definition for pointer initialization and checks.
 *
 * These definitions help standardize low-level coding practices across
 * different architectures and modules.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_TYPES
#define HAL_TYPES
#include <stdint.h>

#define __IO volatile /**< Defines volatile memory access for I/O registers.   \
                       */
#ifndef NULL
#define NULL (void *)0 /**< Defines the null pointer constant. */
#endif                 // !NULL
#ifndef byte
#define byte uint8_t
#endif
#ifndef __UNUSED
#define __UNUSED __attribute__((unused))
#endif
#endif // !HAL_TYPES
