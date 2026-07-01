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
 * @file hal_tcm.h
 * @brief Placement attributes for the Cortex-M7 tightly-coupled memories.
 *
 * @details
 * The Cortex-M7 has two 0-wait-state memories wired directly to the core:
 * **ITCM** (instruction TCM) and **DTCM** (data TCM). Unlike a cache, TCM is
 * just fast memory at a fixed address — there is no coherency maintenance — so
 * placement is an explicit, compile-time decision made through these
 * attributes. They are gated by @c NAVHAL_CONFIG_USE_TCM (a Cortex-M7-only
 * Kconfig option); when the cap is off, or on a target without TCM (M4, AVR),
 * every macro expands to nothing, so portable code keeps compiling and the
 * symbol simply lands in normal flash/RAM.
 *
 * The build copies `.itcm` and initialized `.dtcm` from flash at reset (in the
 * board startup) and zeroes `.dtcm_bss`, exactly like `.data`/`.bss`.
 *
 * @code
 * NAVHAL_ITCM int  hot_isr_helper(int x) { ... }   // runs 0-wait from ITCM
 * NAVHAL_DTCM float filter_state[64] = {0};        // 0-wait data in DTCM
 * NAVHAL_DTCM_NOINIT uint8_t dma_buf[512];         // DTCM buffer (coherent,
 *                                                  //  no D-cache needed)
 * @endcode
 *
 * DTCM is a coherency-free place for DMA buffers: because it is not cached,
 * a buffer there stays coherent with a DMA engine without any clean/invalidate.
 */

#ifndef HAL_TCM_H
#define HAL_TCM_H

#if defined(NAVHAL_CONFIG_USE_TCM) && NAVHAL_CONFIG_USE_TCM

/** Place a function in ITCM (copied from flash at reset). `long_call` so the
 *  branch from flash reaches ITCM at 0x0; `noinline` so placement is kept. */
#define NAVHAL_ITCM __attribute__((section(".itcm"), long_call, noinline))

/** Place initialized data in DTCM (copied from flash at reset). */
#define NAVHAL_DTCM __attribute__((section(".dtcm")))

/** Place uninitialized data / buffers in DTCM (zeroed at reset, no flash
 *  backing) — the DTCM analogue of `.bss`. */
#define NAVHAL_DTCM_NOINIT __attribute__((section(".dtcm_bss")))

#else /* TCM unavailable — attributes are inert, symbol stays in flash/RAM. */

#define NAVHAL_ITCM
#define NAVHAL_DTCM
#define NAVHAL_DTCM_NOINIT

#endif

#endif /* HAL_TCM_H */
