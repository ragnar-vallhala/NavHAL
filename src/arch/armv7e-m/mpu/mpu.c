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
 * @file mpu.c
 * @brief Standardized HAL Memory Protection Unit driver for ARMv7-M.
 *
 * @details
 * Implements the `hal_mpu_*` API declared in `common/hal_mpu.h` against the
 * PMSAv7 MPU shared by Cortex-M4 and Cortex-M7. The register block lives in the
 * System Control Space and is identical across both cores, so the driver is a
 * single arch-level unit (no per-family register header, matching `fpu.c`). The
 * region count is the only silicon difference and is always read from
 * `MPU_TYPE.DREGION` rather than assumed.
 */

#include "navhal_port_config.h"
#if NAVHAL_CONFIG_DRV_MPU

#include "common/hal_mpu.h"
#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- *
 * Register block — System Control Space, 0xE000ED90.
 * -------------------------------------------------------------------------- */
typedef struct {
  volatile uint32_t TYPE; /**< 0x00 (RO): DREGION in bits [15:8].          */
  volatile uint32_t CTRL; /**< 0x04: ENABLE[0], HFNMIENA[1], PRIVDEFENA[2].*/
  volatile uint32_t RNR;  /**< 0x08: region number select.                 */
  volatile uint32_t RBAR; /**< 0x0C: ADDR[31:5], VALID[4], REGION[3:0].    */
  volatile uint32_t RASR; /**< 0x10: EN[0],SIZE[5:1],SRD[15:8],attrs,AP,XN.*/
} mpu_regs_t;

#define MPU ((mpu_regs_t *)0xE000ED90UL)

/* RBAR / RASR / CTRL field positions. */
#define MPU_RBAR_ADDR_MASK 0xFFFFFFE0u
#define MPU_RBAR_VALID     (1u << 4)
#define MPU_RBAR_REGION_MASK 0x0Fu

#define MPU_RASR_ENABLE    (1u << 0)
#define MPU_RASR_SIZE_POS  1
#define MPU_RASR_SRD_POS   8
#define MPU_RASR_B         (1u << 16)
#define MPU_RASR_C         (1u << 17)
#define MPU_RASR_S         (1u << 18)
#define MPU_RASR_TEX_POS   19
#define MPU_RASR_AP_POS    24
#define MPU_RASR_XN        (1u << 28)

#define MPU_CTRL_ENABLE    (1u << 0)
#define MPU_CTRL_PRIVDEFENA (1u << 2)

#define MPU_TYPE_DREGION_POS  8
#define MPU_TYPE_DREGION_MASK 0xFFu

static inline void mpu_barrier(void) {
  __asm volatile("dsb");
  __asm volatile("isb");
}

/* -------------------------------------------------------------------------- *
 * Field encoders — pure, no hardware access.
 * -------------------------------------------------------------------------- */

/** Combined TEX/C/B bits (already shifted into RASR position) for a preset. */
static uint32_t attr_bits(hal_mpu_mem_t mem) {
  switch (mem) {
  case HAL_MPU_MEM_STRONGLY_ORDERED:
    return 0u;                                /* TEX=0 C=0 B=0            */
  case HAL_MPU_MEM_DEVICE:
    return MPU_RASR_B;                         /* TEX=0 C=0 B=1 (shared)  */
  case HAL_MPU_MEM_NORMAL_WT:
    return MPU_RASR_C;                         /* TEX=0 C=1 B=0           */
  case HAL_MPU_MEM_NORMAL_WB:
    return MPU_RASR_C | MPU_RASR_B;            /* TEX=0 C=1 B=1           */
  case HAL_MPU_MEM_NORMAL_NONCACHE:
    return (1u << MPU_RASR_TEX_POS);           /* TEX=1 C=0 B=0           */
  default:
    return 0u;
  }
}

/** AP[2:0] field value for an access-permission preset. */
static uint32_t ap_field(hal_mpu_ap_t ap) {
  switch (ap) {
  case HAL_MPU_AP_NONE:              return 0x0u;
  case HAL_MPU_AP_PRIV_RW:           return 0x1u;
  case HAL_MPU_AP_PRIV_RW_UNPRIV_RO: return 0x2u;
  case HAL_MPU_AP_RW:                return 0x3u;
  case HAL_MPU_AP_PRIV_RO:           return 0x5u;
  case HAL_MPU_AP_RO:                return 0x6u;
  default:                           return 0x0u;
  }
}

static uint32_t encode_rasr(const hal_mpu_region_t *r) {
  uint32_t rasr = MPU_RASR_ENABLE;
  rasr |= ((uint32_t)r->size & 0x1Fu) << MPU_RASR_SIZE_POS;
  rasr |= (uint32_t)r->srd_mask << MPU_RASR_SRD_POS;
  rasr |= attr_bits(r->mem);
  if (r->shareable)
    rasr |= MPU_RASR_S;
  rasr |= ap_field(r->ap) << MPU_RASR_AP_POS;
  if (!r->executable)
    rasr |= MPU_RASR_XN;
  return rasr;
}

static uint32_t encode_rbar(uint32_t idx, const hal_mpu_region_t *r) {
  return (r->base & MPU_RBAR_ADDR_MASK) | MPU_RBAR_VALID |
         (idx & MPU_RBAR_REGION_MASK);
}

/**
 * @brief Validate a descriptor against the hardware and the encoding rules.
 *
 * Assumes the caller has already confirmed the MPU is present. Collapses the
 * design's ALIGN/SIZE/INDEX/NULL causes onto ::HAL_ERR_INVALID_ARG per the
 * unified-status convention.
 */
static hal_status_t validate(uint32_t idx, const hal_mpu_region_t *r) {
  if (r == NULL)
    return HAL_ERR_INVALID_ARG;
  if (idx >= hal_mpu_num_regions())
    return HAL_ERR_INVALID_ARG;
  if (r->size < HAL_MPU_SIZE_32B || r->size > HAL_MPU_SIZE_4GB)
    return HAL_ERR_INVALID_ARG;

  /* base must be aligned to the region size. 64-bit math so the 4 GB case
   * (mask 0xFFFFFFFF) does not overflow. */
  uint64_t bytes = (uint64_t)1u << ((uint32_t)r->size + 1u);
  if (((uint64_t)r->base & (bytes - 1u)) != 0u)
    return HAL_ERR_INVALID_ARG;

  /* Subregion-disable needs each subregion >= 32 B, i.e. region >= 256 B. */
  if (r->srd_mask != 0u && r->size < HAL_MPU_SIZE_256B)
    return HAL_ERR_INVALID_ARG;

  return HAL_OK;
}

/* -------------------------------------------------------------------------- *
 * Query
 * -------------------------------------------------------------------------- */

bool hal_mpu_present(void) {
  return ((MPU->TYPE >> MPU_TYPE_DREGION_POS) & MPU_TYPE_DREGION_MASK) != 0u;
}

uint32_t hal_mpu_num_regions(void) {
  return (MPU->TYPE >> MPU_TYPE_DREGION_POS) & MPU_TYPE_DREGION_MASK;
}

/* -------------------------------------------------------------------------- *
 * Global enable / disable
 * -------------------------------------------------------------------------- */

hal_status_t hal_mpu_enable(bool bg_priv) {
  if (!hal_mpu_present())
    return HAL_ERR_NOT_SUPPORTED;

  uint32_t ctrl = MPU_CTRL_ENABLE; /* HFNMIENA left 0: MPU off in HardFault/NMI */
  if (bg_priv)
    ctrl |= MPU_CTRL_PRIVDEFENA;

  MPU->CTRL = ctrl;
  mpu_barrier();
  return HAL_OK;
}

hal_status_t hal_mpu_disable(void) {
  if (!hal_mpu_present())
    return HAL_ERR_NOT_SUPPORTED;

  __asm volatile("dsb"); /* complete outstanding accesses before dropping MPU */
  MPU->CTRL = 0u;
  mpu_barrier();
  return HAL_OK;
}

/* -------------------------------------------------------------------------- *
 * Per-region programming (validating path)
 * -------------------------------------------------------------------------- */

hal_status_t hal_mpu_configure_region(uint32_t idx,
                                      const hal_mpu_region_t *r) {
  if (!hal_mpu_present())
    return HAL_ERR_NOT_SUPPORTED;

  hal_status_t st = validate(idx, r);
  if (st != HAL_OK)
    return st;

  /* Writing RBAR with VALID=1 also latches REGION into RNR; RASR then applies
   * to that region. Disable the region first so a half-written descriptor is
   * never briefly live. */
  MPU->RNR = idx;
  MPU->RASR = 0u;
  MPU->RBAR = encode_rbar(idx, r);
  MPU->RASR = encode_rasr(r);
  mpu_barrier();
  return HAL_OK;
}

hal_status_t hal_mpu_disable_region(uint32_t idx) {
  if (!hal_mpu_present())
    return HAL_ERR_NOT_SUPPORTED;
  if (idx >= hal_mpu_num_regions())
    return HAL_ERR_INVALID_ARG;

  MPU->RNR = idx;
  MPU->RASR &= ~MPU_RASR_ENABLE;
  mpu_barrier();
  return HAL_OK;
}

/* -------------------------------------------------------------------------- *
 * Pre-encode / bulk apply (context-switch fast path)
 * -------------------------------------------------------------------------- */

hal_status_t hal_mpu_encode(uint32_t idx, const hal_mpu_region_t *r,
                            hal_mpu_encoded_t *out) {
  if (!hal_mpu_present())
    return HAL_ERR_NOT_SUPPORTED;
  if (out == NULL)
    return HAL_ERR_INVALID_ARG;

  hal_status_t st = validate(idx, r);
  if (st != HAL_OK)
    return st;

  out->rbar = encode_rbar(idx, r);
  out->rasr = encode_rasr(r);
  return HAL_OK;
}

hal_status_t hal_mpu_apply(const hal_mpu_encoded_t *set, uint32_t count) {
  if (!hal_mpu_present())
    return HAL_ERR_NOT_SUPPORTED;
  if (set == NULL)
    return HAL_ERR_INVALID_ARG;
  if (count > hal_mpu_num_regions())
    return HAL_ERR_INVALID_ARG;

  /* No per-region validation or packing here — each pair is written straight
   * through. The VALID bit in rbar selects the target region, so RNR need not
   * be touched between entries. */
  for (uint32_t i = 0; i < count; i++) {
    MPU->RBAR = set[i].rbar;
    MPU->RASR = set[i].rasr;
  }
  mpu_barrier();
  return HAL_OK;
}

#endif /* NAVHAL_CONFIG_DRV_MPU */
