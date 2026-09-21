/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clock.c
 * @brief HAL clock driver for the PC — TSC frequency calibrated against the PIT.
 *
 * @details
 * A PC has no PLL/bus tree to configure, so "clock init" means *measuring* the
 * CPU timestamp-counter rate, not programming a clock source. We gate the 8254
 * PIT channel 2 for a known interval and count TSC cycles across it. The
 * measured value is deliberately not a compile-time constant — real TSC rates
 * vary per machine and QEMU host, so it must be sampled at boot.
 */

#include "common/hal_clock.h"
#include "internal/hal_clock_ops.h"
#include "pc_io.h"

#define PIT_INPUT_HZ 1193182u /* 8254 input clock */

static uint64_t g_tsc_hz;

/* Measure TSC Hz over a ~40 ms PIT channel-2 one-shot (no interrupts needed). */
static uint64_t calibrate_tsc_hz(void) {
  const uint16_t count = 47727; /* 47727 / 1193182 Hz ~= 40 ms */

  /* PIT ch2 gate on (bit0), speaker off (bit1). */
  uint8_t p61 = (pc_inb(0x61) & 0xFC) | 0x01;
  pc_outb(0x61, p61);

  /* ch2, lobyte/hibyte access, mode 0 (interrupt on terminal count), binary. */
  pc_outb(0x43, 0xB0);
  pc_outb(0x42, (uint8_t)(count & 0xFF));
  pc_outb(0x42, (uint8_t)(count >> 8));

  /* Restart the count by toggling the gate low->high. */
  p61 = pc_inb(0x61) & 0xFE;
  pc_outb(0x61, p61);
  pc_outb(0x61, p61 | 0x01);

  uint64_t t0 = pc_rdtsc();
  while (!(pc_inb(0x61) & 0x20)) { /* wait: OUT high = terminal count */ }
  uint64_t t1 = pc_rdtsc();

  return (t1 - t0) * (uint64_t)PIT_INPUT_HZ / count;
}

uint64_t pc_tsc_hz(void) {
  if (!g_tsc_hz) g_tsc_hz = calibrate_tsc_hz();
  return g_tsc_hz;
}

static hal_status_t pc_clock_init(const hal_clock_config_t *cfg) {
  if (!cfg) return HAL_ERR_INVALID_ARG;
  g_tsc_hz = calibrate_tsc_hz();
  return HAL_OK;
}

static uint32_t pc_clock_get_sysclk(void) { return (uint32_t)pc_tsc_hz(); }

/* A PC has no AHB/APB bus hierarchy; report the core (TSC) rate uniformly. */

/** @brief The PC clock backend. A PC has no bus hierarchy, so the bus
 *  accessors all report the core clock. */
/* No bus hierarchy: every peripheral runs from the core clock, so there is
 * nothing for hal_clock_get_bus_clock to report. Callers use get_sysclk. */
static uint8_t pc_clock_get_bus_count(void) { return 0u; }

static uint32_t pc_clock_get_bus_clock(uint8_t bus) {
  (void)bus;
  return 0u;
}

const hal_clock_ops_t _hal_clock_ops = {
    .init = pc_clock_init,
    .get_sysclk = pc_clock_get_sysclk,
    .get_bus_count = pc_clock_get_bus_count,
    .get_bus_clock = pc_clock_get_bus_clock,
};
