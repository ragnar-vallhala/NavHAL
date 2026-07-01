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
 * @file hal_mpu.h
 * @brief Portable HAL interface for the Memory Protection Unit (MPU).
 *
 * @details
 * Backed by the ARMv7-M PMSAv7 MPU shared by Cortex-M4 and Cortex-M7 (the
 * driver lives at @c src/arch/armv7e-m/mpu/mpu.c, a core block alongside
 * @c dwt.c / @c fpu.c). The number of regions is silicon-dependent — 8 on the
 * STM32F401RE (M4), 16 on the STM32F767ZI (M7) — and is read at runtime from
 * @c MPU_TYPE.DREGION via ::hal_mpu_num_regions, never assumed.
 *
 * A region is a power-of-two span [32 B .. 4 GB], based at a size-aligned
 * address, carrying an access permission (privileged/unprivileged split), an
 * execute-never flag, a memory-attribute preset, and an optional 8-way
 * subregion-disable mask. Where two enabled regions overlap, the
 * highest-numbered region wins. A violation raises a MemManage fault.
 *
 * The capability is gated by @c NAVHAL_HAS_MPU (emitted into
 * @c navhal_target.h) which the port config bridges to the @c _MPU_ENABLED
 * driver guard. On a target without an MPU every entry point returns
 * ::HAL_ERR_NOT_SUPPORTED (and ::hal_mpu_present returns @c false).
 */

#ifndef HAL_MPU_H
#define HAL_MPU_H

/**
 * @defgroup HAL_MPU Mpu
 * @ingroup HAL_DRIVERS
 * @brief Memory Protection Unit — region-based access control.
 * @{
 */

#include "common/hal_status.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Region size, encoded as the RASR @c SIZE field value.
 *
 * The raw value is @c log2(bytes) - 1, so the enumerators feed the hardware
 * register directly with no arithmetic. Sizes below 256 B cannot use the
 * subregion-disable mask (see ::hal_mpu_region_t::srd_mask).
 */
typedef enum {
  HAL_MPU_SIZE_32B   = 4,
  HAL_MPU_SIZE_64B   = 5,
  HAL_MPU_SIZE_128B  = 6,
  HAL_MPU_SIZE_256B  = 7,
  HAL_MPU_SIZE_512B  = 8,
  HAL_MPU_SIZE_1KB   = 9,
  HAL_MPU_SIZE_2KB   = 10,
  HAL_MPU_SIZE_4KB   = 11,
  HAL_MPU_SIZE_8KB   = 12,
  HAL_MPU_SIZE_16KB  = 13,
  HAL_MPU_SIZE_32KB  = 14,
  HAL_MPU_SIZE_64KB  = 15,
  HAL_MPU_SIZE_128KB = 16,
  HAL_MPU_SIZE_256KB = 17,
  HAL_MPU_SIZE_512KB = 18,
  HAL_MPU_SIZE_1MB   = 19,
  HAL_MPU_SIZE_2MB   = 20,
  HAL_MPU_SIZE_4MB   = 21,
  HAL_MPU_SIZE_8MB   = 22,
  HAL_MPU_SIZE_16MB  = 23,
  HAL_MPU_SIZE_32MB  = 24,
  HAL_MPU_SIZE_64MB  = 25,
  HAL_MPU_SIZE_128MB = 26,
  HAL_MPU_SIZE_256MB = 27,
  HAL_MPU_SIZE_512MB = 28,
  HAL_MPU_SIZE_1GB   = 29,
  HAL_MPU_SIZE_2GB   = 30,
  HAL_MPU_SIZE_4GB   = 31,
} hal_mpu_size_t;

/**
 * @brief Access permission — the AP[2:0] privileged/unprivileged split.
 */
typedef enum {
  HAL_MPU_AP_NONE = 0, /**< No access in either mode.                     */
  HAL_MPU_AP_PRIV_RW,  /**< Privileged RW, unprivileged no access.        */
  HAL_MPU_AP_PRIV_RW_UNPRIV_RO, /**< Privileged RW, unprivileged RO.      */
  HAL_MPU_AP_RW,       /**< Privileged RW, unprivileged RW (full access). */
  HAL_MPU_AP_PRIV_RO,  /**< Privileged RO, unprivileged no access.        */
  HAL_MPU_AP_RO,       /**< Privileged RO, unprivileged RO.               */
} hal_mpu_ap_t;

/**
 * @brief Memory-attribute preset (the TEX/C/B/S encoding), named by intent.
 *
 * Covers the presets a task/kernel split actually needs; the raw TEX/C/B/S
 * bits are not exposed to keep callers away from invalid combinations.
 */
typedef enum {
  HAL_MPU_MEM_STRONGLY_ORDERED = 0, /**< No caching/buffering; ordered.    */
  HAL_MPU_MEM_DEVICE,               /**< Shared device (MMIO).             */
  HAL_MPU_MEM_NORMAL_WT,            /**< Normal, cacheable write-through.  */
  HAL_MPU_MEM_NORMAL_WB,            /**< Normal, cacheable write-back.     */
  HAL_MPU_MEM_NORMAL_NONCACHE,      /**< Normal, non-cacheable (DMA-safe). */
} hal_mpu_mem_t;

/**
 * @brief Human-facing region descriptor (pre-encode form).
 *
 * ::hal_mpu_configure_region and ::hal_mpu_encode validate @c base against
 * @c size (must be size-aligned) and reject an out-of-range index.
 */
typedef struct {
  uint32_t base;         /**< Region base address; MUST be @c size aligned. */
  hal_mpu_size_t size;   /**< Region span (power of two, 32 B .. 4 GB).     */
  hal_mpu_ap_t ap;       /**< Access permission (priv/unpriv split).        */
  hal_mpu_mem_t mem;     /**< Memory-attribute preset.                      */
  bool executable;       /**< Instruction fetch allowed (XN = !executable). */
  bool shareable;        /**< Shareable (S bit) — set for shared data/MMIO. */
  uint8_t srd_mask;      /**< Subregion-disable mask: bit n disables the nth
                              eighth. Requires @c size >= 256 B; 0 = none.   */
} hal_mpu_region_t;

/**
 * @brief Pre-encoded region: the exact RBAR/RASR word pair for one region.
 *
 * Produced by ::hal_mpu_encode away from the critical path and written by
 * ::hal_mpu_apply, so a context switch programs regions with no per-region
 * validation or field packing. @c rbar already carries the VALID bit and the
 * region number.
 */
typedef struct {
  uint32_t rbar; /**< MPU_RBAR value (base | VALID | region number). */
  uint32_t rasr; /**< MPU_RASR value (enable | size | AP | attrs | SRD). */
} hal_mpu_encoded_t;

/* -------------------------------------------------------------------------- *
 * Query
 * -------------------------------------------------------------------------- */

/**
 * @brief Whether this silicon implements an MPU.
 * @return @c true if @c MPU_TYPE.DREGION is non-zero, else @c false.
 */
bool hal_mpu_present(void);

/**
 * @brief Number of hardware regions on this target.
 * @return Region count from @c MPU_TYPE.DREGION (8 on M4, 16 on M7); 0 if the
 *         MPU is absent.
 */
uint32_t hal_mpu_num_regions(void);

/* -------------------------------------------------------------------------- *
 * Global enable / disable
 * -------------------------------------------------------------------------- */

/**
 * @brief Enable the MPU.
 *
 * Sets @c MPU_CTRL.ENABLE with a DSB/ISB barrier pair so protection is live
 * before the next access. @p bg_priv sets @c PRIVDEFENA: when @c true,
 * privileged code retains the default system memory map for addresses not
 * covered by any enabled region; when @c false, any uncovered access faults.
 *
 * @param bg_priv Enable the privileged background region (@c PRIVDEFENA).
 * @return ::HAL_OK on success, or ::HAL_ERR_NOT_SUPPORTED if no MPU is present.
 */
hal_status_t hal_mpu_enable(bool bg_priv);

/**
 * @brief Disable the MPU.
 *
 * Clears @c MPU_CTRL.ENABLE with a DSB/ISB barrier pair. Region contents are
 * left intact for a later re-enable.
 *
 * @return ::HAL_OK on success, or ::HAL_ERR_NOT_SUPPORTED if no MPU is present.
 */
hal_status_t hal_mpu_disable(void);

/* -------------------------------------------------------------------------- *
 * Per-region programming (validating, convenience path)
 * -------------------------------------------------------------------------- */

/**
 * @brief Validate and program a single region, marking it enabled.
 *
 * Encodes @p r and writes @c MPU_RNR / @c MPU_RBAR / @c MPU_RASR. Intended for
 * setup code, not the context-switch fast path — see ::hal_mpu_encode +
 * ::hal_mpu_apply for that.
 *
 * @param idx Region index in @c [0, hal_mpu_num_regions()).
 * @param r   Region descriptor; must be non-NULL and size-aligned.
 * @retval HAL_OK              Region programmed.
 * @retval HAL_ERR_NOT_SUPPORTED No MPU on this target.
 * @retval HAL_ERR_INVALID_ARG @p r is NULL, @p idx out of range, @c base not
 *                             aligned to @c size, or an illegal @c srd_mask
 *                             for a sub-256 B region.
 */
hal_status_t hal_mpu_configure_region(uint32_t idx,
                                      const hal_mpu_region_t *r);

/**
 * @brief Disable a single region (clears its RASR enable bit).
 *
 * @param idx Region index in @c [0, hal_mpu_num_regions()).
 * @retval HAL_OK              Region disabled.
 * @retval HAL_ERR_NOT_SUPPORTED No MPU on this target.
 * @retval HAL_ERR_INVALID_ARG @p idx out of range.
 */
hal_status_t hal_mpu_disable_region(uint32_t idx);

/* -------------------------------------------------------------------------- *
 * Pre-encode / bulk apply (context-switch fast path)
 * -------------------------------------------------------------------------- */

/**
 * @brief Encode one region into its RBAR/RASR word pair without touching the
 *        hardware.
 *
 * All validation (alignment, size, index, NULL) happens here, off the critical
 * path, so a per-task region set can be prepared once at domain-setup time and
 * replayed cheaply on every switch.
 *
 * @param idx Region index baked into the RBAR region-number field.
 * @param r   Region descriptor; must be non-NULL and size-aligned.
 * @param out Destination for the encoded pair; must be non-NULL.
 * @retval HAL_OK              Encoded into @p out.
 * @retval HAL_ERR_NOT_SUPPORTED No MPU on this target.
 * @retval HAL_ERR_INVALID_ARG @p r or @p out NULL, @p idx out of range, or an
 *                             alignment/size/SRD violation in @p r.
 */
hal_status_t hal_mpu_encode(uint32_t idx, const hal_mpu_region_t *r,
                            hal_mpu_encoded_t *out);

/**
 * @brief Apply a pre-encoded region set to the hardware.
 *
 * Writes each pair straight to @c MPU_RBAR / @c MPU_RASR — no per-region
 * validation or field packing — with a single DSB/ISB pair after the batch.
 * This is the intended PendSV / context-switch entry point. A single bound
 * check confirms @p count does not exceed ::hal_mpu_num_regions.
 *
 * @param set   Array of ::hal_mpu_encoded_t (from ::hal_mpu_encode).
 * @param count Number of entries in @p set.
 * @retval HAL_OK              Region set applied.
 * @retval HAL_ERR_NOT_SUPPORTED No MPU on this target.
 * @retval HAL_ERR_INVALID_ARG @p set is NULL, or @p count exceeds the region
 *                             count.
 */
hal_status_t hal_mpu_apply(const hal_mpu_encoded_t *set, uint32_t count);

#ifdef __cplusplus
} /* extern "C" */
#endif


/** @} */ /* end of group HAL_MPU */
#endif /* HAL_MPU_H */
