/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pc_io.h
 * @brief Shared low-level PC primitives: port I/O and the TSC.
 */

#ifndef NAVHAL_PC_IO_H
#define NAVHAL_PC_IO_H

#include <stdint.h>

static inline void pc_outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t pc_inb(uint16_t port) {
  uint8_t r;
  __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
  return r;
}

static inline uint64_t pc_rdtsc(void) {
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

/** @brief Calibrated TSC frequency in Hz (measured once, in clock.c). */
uint64_t pc_tsc_hz(void);

#endif /* NAVHAL_PC_IO_H */
