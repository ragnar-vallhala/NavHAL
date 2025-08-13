/**
 * @file types.h
 * @brief Centralized type definitions include for NavHAL.
 *
 * This header acts as a single include point for all
 * common and peripheral-specific type definitions used
 * throughout the NavHAL project.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#ifndef TYPES_H
#define TYPES_H

#include "clock_types.h" /**< Clock-specific type definitions */
#include "gpio_types.h"  /**< GPIO-specific type definitions */
#include "timer_types.h" /**< Timer-specific type definitions */
#include <stdbool.h>     /**< Standard boolean types */
#include <stdint.h>      /**< Standard fixed-width integer types */
#endif // TYPES_H
