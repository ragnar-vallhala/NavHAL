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
 * `docs/api_standardization.md` and `docs/execution_plan.md`.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
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

#else /* non-GCC: degrade to no-ops */

#define NAVHAL_INLINE        static inline
#define NAVHAL_ALWAYS_INLINE static inline
#define NAVHAL_UNUSED
#define NAVHAL_USED
#define NAVHAL_WEAK
#define NAVHAL_PACKED
#define NAVHAL_NORETURN
#define NAVHAL_DEPRECATED(msg)

#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_COMPILER_H */
