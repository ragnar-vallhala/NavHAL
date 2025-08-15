/**
 * @file status.h
 * @brief Common status code definitions for HAL and application modules.
 *
 * @details
 * This header file defines generic status codes used across different
 * modules to indicate success or failure of operations. These macros
 * can be used consistently throughout the codebase for standardized
 * error handling and return values.
 *
 * Macros:
 * - `SUCCESS` (0)  : Operation completed successfully.
 * - `FAILURE` (-1) : Operation failed.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef STATUS_H
#define STATUS_H

#define SUCCESS 0   /**< Indicates successful operation. */
#define FAILURE -1  /**< Indicates failed operation. */

#endif // !STATUS_H
