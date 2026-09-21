/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port/x86_64/utils/timer_types.h
 * @brief x86-64 timer instance enum (::hal_timer_t).
 *
 * The timebase (delay/millis) is TSC-backed and needs no timer instance. The
 * general-purpose hal_timer_* API maps to the 8254 PIT and lands with the
 * interrupt slice; the channel enum is stubbed here so hal_timer.h compiles.
 */

#ifndef NAVHAL_PORT_X86_64_TIMER_TYPES_H
#define NAVHAL_PORT_X86_64_TIMER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Timer instances (8254 PIT channels). */
typedef enum {
  HAL_TIMER_PIT0 = 0, /**< PIT channel 0 — periodic IRQ0 (Slice 4). */
} hal_timer_t;

#ifdef __cplusplus
}
#endif

#endif /* NAVHAL_PORT_X86_64_TIMER_TYPES_H */
