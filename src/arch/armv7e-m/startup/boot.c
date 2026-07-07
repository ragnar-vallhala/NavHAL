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
 * @file boot.c
 * @brief Shared Cortex-M C reset path (M4/M7).
 *
 * @details
 * The per-device startup.s keeps only the vector table (its peripheral half is
 * MCU-specific); the reset routine itself lives here, once, for every ARMv7E-M
 * target. On Cortex-M the core loads SP from vector[0] before Reset_Handler
 * runs, so this can be plain C with a valid stack from the first instruction.
 *
 * Memory bring-up is data-driven: the linker emits a copy table
 * (@c __copy_table_start__ .. @c __copy_table_end__, triples of
 * {flash-src, ram-dst, byte-len}) and a zero table (pairs of {ram-dst,
 * byte-len}). This routine just walks them, so F7's ITCM/DTCM regions are
 * extra rows rather than special-case code and F4 (no TCM) runs the same path.
 *
 * Two hook layers bracket the bring-up, split on the one hard line in a reset
 * path — whether the C runtime exists yet:
 *   - INNER (@c navhal_boot_pre / @c navhal_boot_post): HAL/device territory.
 *     boot_pre runs BEFORE .data/.bss are live, so an override MUST NOT touch
 *     initialized/zeroed globals (e.g. TCM enable, flash latency, cache
 *     invalidate). Left weak-empty by default.
 *   - OUTER (@c SystemInit / @c navhal_post_main): user/RTOS territory, run with
 *     the runtime up. SystemInit is the CMSIS-standard pre-main seam — VTOR
 *     relocation, early clock, RTOS pre-scheduler work. Weak-empty by default,
 *     so the boot's out-of-the-box behaviour is unchanged (vectors stay at the
 *     reset alias until an override relocates VTOR).
 */

#include <stdint.h>

extern int main(void);

/* Linker-emitted tables (see the board / tests/arch linker scripts). Copy
 * entries are triples {src_lma, dst_vma, len_bytes}; zero entries are pairs
 * {dst_vma, len_bytes}. Every region is word-aligned and word-multiple-sized. */
extern uint32_t __copy_table_start__[];
extern uint32_t __copy_table_end__[];
extern uint32_t __zero_table_start__[];
extern uint32_t __zero_table_end__[];

/* --- INNER hooks: HAL/device, run around memory init (global-free). --- */
__attribute__((weak)) void navhal_boot_pre(void) {}
__attribute__((weak)) void navhal_boot_post(void) {}

/* --- OUTER hooks: user/RTOS, run with the C runtime up. --- */
__attribute__((weak)) void SystemInit(void) {}
__attribute__((weak)) void navhal_post_main(void) {}

/* -nostdlib links no libc, so there is no memcpy/memset. The volatile pointers
 * also stop the compiler lowering these loops back into a memcpy/memset call
 * that would then fail to link. Lengths are byte counts, always /4. */
static void copy_words(volatile uint32_t *dst, volatile const uint32_t *src,
                       uint32_t len) {
  for (uint32_t n = len >> 2; n; n--) {
    *dst++ = *src++;
  }
}

static void zero_words(volatile uint32_t *dst, uint32_t len) {
  for (uint32_t n = len >> 2; n; n--) {
    *dst++ = 0u;
  }
}

__attribute__((used)) void Reset_Handler(void) {
  /* SP is already valid (hardware loaded it from vector[0]). */
  navhal_boot_pre(); /* inner pre: device setup that must precede the runtime */

  for (uint32_t *e = __copy_table_start__; e < __copy_table_end__; e += 3) {
    copy_words((volatile uint32_t *)e[1], (volatile const uint32_t *)e[0], e[2]);
  }
  for (uint32_t *z = __zero_table_start__; z < __zero_table_end__; z += 2) {
    zero_words((volatile uint32_t *)z[0], z[1]);
  }

  navhal_boot_post(); /* inner post: caches / FPU / MPU baseline (globals live) */
  SystemInit();       /* outer pre: VTOR, clock, RTOS (weak; user override) */

  __asm volatile("cpsie i" ::: "memory"); /* match the prior boot's behaviour */

  main();

  navhal_post_main(); /* outer post */
  for (;;) {
  }
}
