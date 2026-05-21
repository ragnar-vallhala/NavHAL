/**
 * @file hal_features.h
 * @brief NavHAL capability (feature) macros.
 *
 * @details
 * Each `NAVHAL_HAS_<feature>` macro expands to `1` when the current target
 * build includes that optional capability and `0` otherwise. Optional driver
 * code and the interrupt-/DMA-mode API variants are gated on these macros
 * (see Section 8 of `docs/api_standardization.md`).
 *
 * Always test with `#if NAVHAL_HAS_<feature>` — the macros are always defined,
 * never just `#ifdef`-present.
 *
 * @par M1 status
 * This header currently *bridges* the pre-standardization `_<feature>_ENABLED`
 * build flags (defined in the per-target `config.h`). In M4 the source of
 * truth becomes a Kconfig-generated `navhal_target.h`; the `NAVHAL_HAS_*`
 * names defined here are the stable contract and will not change.
 *
 * @par Owning Kconfig symbols
 * - `NAVHAL_HAS_DMA`            &larr; `CONFIG_DRV_DMA`
 * - `NAVHAL_HAS_FPU`            &larr; `CONFIG_USE_FPU` / `CONFIG_DRV_FPU`
 * - `NAVHAL_HAS_CRC_HW`         &larr; `CONFIG_DRV_CRC`
 * - `NAVHAL_HAS_CYCLE_COUNTER`  &larr; `CONFIG_DRV_DWT`
 * - `NAVHAL_HAS_SDIO`           &larr; `CONFIG_DRV_SDIO`
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_FEATURES_H
#define HAL_FEATURES_H

#include "common/hal_config.h" /* pulls in the target's _<feature>_ENABLED flags */


#ifdef __cplusplus
extern "C" {
#endif
/** @brief 1 if a DMA controller and the DMA-mode API are available. */
#if defined(_DMA_ENABLED)
#define NAVHAL_HAS_DMA 1
#else
#define NAVHAL_HAS_DMA 0
#endif

/** @brief 1 if a hardware floating-point unit is available. */
#if defined(_FPU_ENABLED)
#define NAVHAL_HAS_FPU 1
#else
#define NAVHAL_HAS_FPU 0
#endif

/** @brief 1 if a hardware CRC peripheral is available. */
#if defined(_CRC_HW_ENABLED)
#define NAVHAL_HAS_CRC_HW 1
#else
#define NAVHAL_HAS_CRC_HW 0
#endif

/** @brief 1 if a cycle counter (Cortex DWT, or equivalent) is available. */
#if defined(_DWT_ENABLED)
#define NAVHAL_HAS_CYCLE_COUNTER 1
#elif !defined(NAVHAL_HAS_CYCLE_COUNTER)
#define NAVHAL_HAS_CYCLE_COUNTER 0
#endif

/** @brief 1 if an SDIO / SD-card peripheral is available. */
#if defined(_SDIO_ENABLED)
#define NAVHAL_HAS_SDIO 1
#elif !defined(NAVHAL_HAS_SDIO)
#define NAVHAL_HAS_SDIO 0
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_FEATURES_H */
