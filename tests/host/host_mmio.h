/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file host_mmio.h
 * @brief Simulated memory-mapped I/O for the host driver suite.
 *
 * @details
 * Maps the STM32 peripheral region (0x40000000) and flash (0x08000000) at
 * their real addresses with `mmap(MAP_FIXED)`, so the actual vendor driver
 * sources — which dereference fixed peripheral addresses — run unmodified
 * under the host compiler and write into this backing store. Tests then assert
 * on the resulting register/memory state and pre-seed the "ready" flags that
 * the drivers' on-target busy-waits poll.
 *
 * This is a *logic* harness, not a hardware model: it does not simulate
 * peripheral side effects (a write to a control register does not move data on
 * a bus). It exercises the driver's register-manipulation and control-flow
 * logic — where most port bugs live — at host speed, in CI, with no board.
 */
#ifndef HOST_MMIO_H
#define HOST_MMIO_H

#include <stdint.h>

/** @brief Map the peripheral + flash regions (call once at startup). Aborts on
 *  failure (e.g. the fixed address is unavailable). */
void host_mmio_setup(void);

/** @brief Reset the backing store to a power-on-like state: peripherals zeroed,
 *  flash set to 0xFF (erased). Call between tests for isolation. */
void host_mmio_reset(void);

/** @brief Set bits in a 32-bit peripheral register (helper to pre-seed the
 *  hardware "ready"/status flags a driver busy-waits on). */
void host_reg_set(uintptr_t addr, uint32_t bits);

/** @brief Clear bits in a 32-bit peripheral register. */
void host_reg_clear(uintptr_t addr, uint32_t bits);

#endif /* HOST_MMIO_H */
