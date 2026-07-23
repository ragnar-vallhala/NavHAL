/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port/x86_64/utils/clock_types.h
 * @brief x86-64 clock types. A PC has no PLL/bus tree — the "clock" is the
 *        invariant TSC, calibrated at init against the 8254 PIT.
 */

#ifndef NAVHAL_PORT_X86_64_CLOCK_TYPES_H
#define NAVHAL_PORT_X86_64_CLOCK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The only clock source on a PC is the CPU timestamp counter. */
typedef enum {
  HAL_CLOCK_SOURCE_TSC = 0, /**< CPU TSC (calibrated against the PIT). */
} hal_clock_source_t;

/** @brief Clock config — no PLL/bus dividers exist on a PC. */
typedef struct {
  hal_clock_source_t source;
} hal_clock_config_t;

#ifdef __cplusplus
}
#endif

#endif /* NAVHAL_PORT_X86_64_CLOCK_TYPES_H */
