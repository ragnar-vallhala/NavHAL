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

#define __IO volatile /**< Defines volatile memory access for I/O registers. */
#define NULL 0        /**< Defines the null pointer constant. */

#endif // !HAL_TYPES
