/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file timebase.c
 * @brief HAL timebase for the PC — TSC-backed, polled (no interrupts).
 *
 * @details
 * hal_timebase_get_micros/millis and hal_delay_* read the TSC directly and
 * scale by the calibrated frequency (clock.c). This needs no IRQs. The
 * *periodic* tick and its callback are IRQ-driven (PIT IRQ0) and land with the
 * interrupt slice; they are kept API-complete here (the callback store works,
 * and hal_timebase_tick() dispatches it if called).
 *
 * @note hal_timebase_init() must be called (after the clock is calibrated)
 *       before the get_micros/millis and delay helpers return meaningful values.
 */

#include "common/hal_timer.h"
#include "pc_io.h"

static uint64_t g_tsc0;      /* TSC snapshot at hal_timebase_init */
static uint32_t g_tick_us;
static uint64_t g_cyc_per_us = 1;
static uint64_t g_cyc_per_ms = 1;
static hal_timebase_callback_t g_cb;

hal_status_t hal_timebase_init(uint32_t tick_us) {
  if (tick_us == 0) return HAL_ERR_INVALID_ARG;
  uint64_t hz = pc_tsc_hz();
  g_cyc_per_us = hz / 1000000u;
  g_cyc_per_ms = hz / 1000u;
  if (!g_cyc_per_us) g_cyc_per_us = 1;
  if (!g_cyc_per_ms) g_cyc_per_ms = 1;
  g_tick_us = tick_us;
  g_tsc0 = pc_rdtsc();
  return HAL_OK;
}

uint32_t hal_timebase_get_micros(void) {
  return (uint32_t)((pc_rdtsc() - g_tsc0) / g_cyc_per_us);
}

uint32_t hal_timebase_get_millis(void) {
  return (uint32_t)((pc_rdtsc() - g_tsc0) / g_cyc_per_ms);
}

uint32_t hal_timebase_get_tick(void) {
  if (!g_tick_us) return 0;
  return hal_timebase_get_micros() / g_tick_us;
}

uint32_t hal_timebase_get_tick_duration_us(void) { return g_tick_us; }

/* No SysTick reload register on x86. */
uint32_t hal_timebase_get_reload_value(void) { return 0; }

void hal_delay_us(uint32_t us) {
  uint64_t start = pc_rdtsc();
  uint64_t target = (uint64_t)us * g_cyc_per_us;
  while ((pc_rdtsc() - start) < target) __asm__ volatile("pause");
}

void hal_delay_ms(uint32_t ms) {
  uint64_t start = pc_rdtsc();
  uint64_t target = (uint64_t)ms * g_cyc_per_ms;
  while ((pc_rdtsc() - start) < target) __asm__ volatile("pause");
}

void hal_timebase_tick(void) {
  if (g_cb) g_cb();
}

hal_status_t hal_timebase_set_callback(hal_timebase_callback_t cb) {
  g_cb = cb;
  return HAL_OK;
}
