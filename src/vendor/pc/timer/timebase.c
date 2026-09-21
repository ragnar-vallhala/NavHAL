/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file timebase.c
 * @brief HAL timebase for the PC — TSC for time queries, PIT IRQ for the tick.
 *
 * @details
 * hal_timebase_get_micros/millis and hal_delay_* read the TSC directly and
 * scale by the calibrated frequency (clock.c) — accurate and available without
 * interrupts. When the interrupt driver is present, hal_timebase_init also
 * programs 8254 PIT channel 0 for a periodic IRQ0 that drives
 * hal_timebase_tick(), so pc_timebase_get_tick() and the registered tick
 * callback advance from a real hardware interrupt (as SysTick does on Cortex-M).
 */

#include "common/hal_timer.h"
#include "internal/hal_timebase_ops.h"

/* Forward declarations: several of these call each other (micros from tick,
 * delays from both), and they are static now. */
static hal_status_t pc_timebase_init(uint32_t tick_us);
static uint32_t pc_timebase_get_tick(void);
static uint32_t pc_timebase_get_tick_duration_us(void);
static uint32_t pc_timebase_get_reload_value(void);
static uint32_t pc_timebase_get_micros(void);
static uint32_t pc_timebase_get_millis(void);
static void pc_timebase_delay_us(uint32_t us);
static void pc_timebase_delay_ms(uint32_t ms);
static hal_status_t pc_timebase_set_callback(hal_timebase_callback_t cb);
#include "pc_io.h"

#if NAVHAL_CONFIG_DRV_INTERRUPT
#include "common/hal_interrupt.h"

#define PIT_CH0 0x40
#define PIT_CMD 0x43
#define PIT_INPUT_HZ 1193182u

static void pit_start_periodic(uint32_t freq_hz) {
  uint32_t div = PIT_INPUT_HZ / (freq_hz ? freq_hz : 1000u);
  if (div == 0) div = 1;
  if (div > 0xFFFF) div = 0xFFFF;
  pc_outb(PIT_CMD, 0x34); /* ch0, lo/hi access, mode 2 (rate generator), binary */
  pc_outb(PIT_CH0, (uint8_t)(div & 0xFF));
  pc_outb(PIT_CH0, (uint8_t)(div >> 8));
}
#endif

static uint64_t g_tsc0;      /* TSC snapshot at hal_timebase_init */
static uint32_t g_tick_us;
static uint64_t g_cyc_per_us = 1;
static uint64_t g_cyc_per_ms = 1;
static volatile uint32_t g_ticks;
static hal_timebase_callback_t g_cb;

static hal_status_t pc_timebase_init(uint32_t tick_us) {
  if (tick_us == 0) return HAL_ERR_INVALID_ARG;
  uint64_t hz = pc_tsc_hz();
  g_cyc_per_us = hz / 1000000u;
  g_cyc_per_ms = hz / 1000u;
  if (!g_cyc_per_us) g_cyc_per_us = 1;
  if (!g_cyc_per_ms) g_cyc_per_ms = 1;
  g_tick_us = tick_us;
  g_ticks = 0;
  g_tsc0 = pc_rdtsc();
#if NAVHAL_CONFIG_DRV_INTERRUPT
  pit_start_periodic(1000000u / tick_us);
  hal_interrupt_attach_callback(HAL_IRQ_TIMER, hal_timebase_tick);
  hal_interrupt_enable(HAL_IRQ_TIMER);
#endif
  return HAL_OK;
}

static uint32_t pc_timebase_get_micros(void) {
  return (uint32_t)((pc_rdtsc() - g_tsc0) / g_cyc_per_us);
}

static uint32_t pc_timebase_get_millis(void) {
  return (uint32_t)((pc_rdtsc() - g_tsc0) / g_cyc_per_ms);
}

static uint32_t pc_timebase_get_tick(void) {
#if NAVHAL_CONFIG_DRV_INTERRUPT
  return g_ticks; /* incremented by the PIT IRQ */
#else
  if (!g_tick_us) return 0;
  return pc_timebase_get_micros() / g_tick_us;
#endif
}

static uint32_t pc_timebase_get_tick_duration_us(void) { return g_tick_us; }

/* No SysTick reload register on x86. */
static uint32_t pc_timebase_get_reload_value(void) { return 0; }

static void pc_timebase_delay_us(uint32_t us) {
  uint64_t start = pc_rdtsc();
  uint64_t target = (uint64_t)us * g_cyc_per_us;
  while ((pc_rdtsc() - start) < target) __asm__ volatile("pause");
}

static void pc_timebase_delay_ms(uint32_t ms) {
  uint64_t start = pc_rdtsc();
  uint64_t target = (uint64_t)ms * g_cyc_per_ms;
  while ((pc_rdtsc() - start) < target) __asm__ volatile("pause");
}

/* Tick handler: invoked from the PIT IRQ (or manually in a polled build). */
void hal_timebase_tick(void) {
  g_ticks++;
  if (g_cb) g_cb();
}

static hal_status_t pc_timebase_set_callback(hal_timebase_callback_t cb) {
  g_cb = cb;
  return HAL_OK;
}


/** @brief The PC TSC/PIT timebase backend. */
const hal_timebase_ops_t _hal_timebase_ops = {
    .init = pc_timebase_init,
    .get_tick = pc_timebase_get_tick,
    .get_tick_duration_us = pc_timebase_get_tick_duration_us,
    .get_reload_value = pc_timebase_get_reload_value,
    .get_micros = pc_timebase_get_micros,
    .get_millis = pc_timebase_get_millis,
    .delay_us = pc_timebase_delay_us,
    .delay_ms = pc_timebase_delay_ms,
    .set_callback = pc_timebase_set_callback,
};
