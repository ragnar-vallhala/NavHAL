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
 * @file navhal_compiler.h
 * @brief Compiler / toolchain abstraction shims for NavHAL.
 *
 * @details
 * Internal foundation header. Provides portable wrappers over
 * compiler-specific attributes so HAL and driver code stays toolchain-neutral
 * as the project expands to new ISAs.
 *
 * Both currently supported toolchains — `arm-none-eabi-gcc` (Cortex-M4) and
 * the planned `avr-gcc` (ATmega328p) — are GCC-compatible, so the GCC branch
 * covers them. The fallback branch lets the headers still compile on other
 * compilers (attributes simply become no-ops).
 *
 * Part of the M1 standardization foundations — see
 * `docs/api_standardization.md`.
 */

#ifndef NAVHAL_COMPILER_H
#define NAVHAL_COMPILER_H


#ifdef __cplusplus
extern "C" {
#endif
#if defined(__GNUC__)

#define NAVHAL_INLINE        static inline                              /**< Internal-linkage inline function. */
#define NAVHAL_ALWAYS_INLINE static inline __attribute__((always_inline)) /**< Force inlining. */
#define NAVHAL_UNUSED        __attribute__((unused))                    /**< Suppress unused-symbol warnings. */
#define NAVHAL_USED          __attribute__((used))                      /**< Keep symbol even if unreferenced. */
#define NAVHAL_WEAK          __attribute__((weak))                      /**< Weak symbol (overridable). */
#define NAVHAL_PACKED        __attribute__((packed))                    /**< Remove struct padding. */
#define NAVHAL_NORETURN      __attribute__((noreturn))                  /**< Function never returns. */
#define NAVHAL_DEPRECATED(msg) __attribute__((deprecated(msg)))         /**< Mark symbol deprecated. */
#define NAVHAL_ALIGNED(n)    __attribute__((aligned(n)))                /**< Align a symbol to @p n bytes. */

#else /* non-GCC: degrade to no-ops */

#define NAVHAL_INLINE        static inline
#define NAVHAL_ALWAYS_INLINE static inline
#define NAVHAL_UNUSED
#define NAVHAL_USED
#define NAVHAL_WEAK
#define NAVHAL_PACKED
#define NAVHAL_NORETURN
#define NAVHAL_DEPRECATED(msg)
#define NAVHAL_ALIGNED(n)

#endif

/**
 * @brief L1 cache line size (bytes) on the widest-cache target we build for.
 *
 * The Cortex-M7 D-cache line is 32 bytes; DMA buffers must be aligned to and
 * sized in multiples of this so a clean/invalidate on one buffer never touches
 * a cache line shared with unrelated data. Targets without a data cache still
 * use this as the DMA-buffer alignment granularity (harmless over-alignment).
 * Boards may override it (e.g. a future 64-byte-line core) before this header.
 */
#ifndef NAVHAL_CACHE_LINE
#define NAVHAL_CACHE_LINE 32U
#endif

/**
 * @brief Align a DMA buffer to a cache line.
 *
 * Apply to any buffer handed to a DMA engine so it is safe to clean/invalidate
 * once the L1 D-cache is enabled. Works on statics *and* stack locals (unlike a
 * section attribute). The caller must still pad the buffer's *size* to a
 * multiple of ::NAVHAL_CACHE_LINE — alignment fixes the start, padding the end.
 *
 * @code
 * uint8_t rx[NAVHAL_CACHE_LINE] NAVHAL_DMA_ALIGN;   // 32-aligned, 32 bytes
 * @endcode
 */
#define NAVHAL_DMA_ALIGN NAVHAL_ALIGNED(NAVHAL_CACHE_LINE)


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_COMPILER_H */
