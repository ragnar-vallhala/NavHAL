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
 * @file wdg_reg.h
 * @brief IWDG and WWDG register maps for STM32F7.
 *
 * @details
 * Two unrelated peripherals that happen to answer the same question from
 * opposite ends. The IWDG runs off the LSI and only cares that firmware is
 * still alive; the WWDG runs off PCLK1 and also cares that firmware is not
 * running early.
 *
 * The F7 IWDG adds a WINR window register the F4 has no equivalent for;
 * it is left out here because the window contract is the WWDG's job in this
 * HAL, and a second way to express it would only be a second thing to keep
 * in agreement.
 *
 * Reference: RM0410 §21 (IWDG), §22 (WWDG).
 */

#ifndef CORTEX_M7_WDG_REG_H
#define CORTEX_M7_WDG_REG_H

#include "common/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- IWDG --- */

typedef struct {
  __IO uint32_t KR;  /**< 0x00: Key (write-only) */
  __IO uint32_t PR;  /**< 0x04: Prescaler */
  __IO uint32_t RLR; /**< 0x08: Reload */
  __IO uint32_t SR;  /**< 0x0C: Status */
} IWDG_Typedef;

#define IWDG_BASE_ADDR 0x40003000UL
#define IWDG ((IWDG_Typedef *)IWDG_BASE_ADDR)

/* The key register is the whole access-control scheme: PR and RLR ignore
 * writes unless UNLOCK was the last key written, and re-lock on the next
 * RELOAD or START. */
#define IWDG_KEY_RELOAD 0x0000AAAAU /**< Reload the counter; also re-locks. */
#define IWDG_KEY_UNLOCK 0x00005555U /**< Allow writes to PR and RLR. */
#define IWDG_KEY_START 0x0000CCCCU  /**< Start counting. No key stops it. */

#define IWDG_PR_MASK 0x07U     /**< Prescaler is 3 bits: /4 << PR. */
#define IWDG_PR_MAX 6U         /**< 6 = /256, the slowest divider. */
#define IWDG_RLR_MASK 0x0FFFU  /**< Reload is 12 bits. */

/* Writes to PR and RLR take a few LSI cycles to cross into the watchdog's own
 * clock domain; the register reads back the old value until they land. */
#define IWDG_SR_PVU (1U << 0) /**< Prescaler update in progress. */
#define IWDG_SR_RVU (1U << 1) /**< Reload update in progress. */

/** @brief Nominal LSI frequency (RC, ±several percent over temperature). */
#define IWDG_LSI_HZ 32000U

/* ---------------------------------------------------------------- WWDG --- */

typedef struct {
  __IO uint32_t CR;  /**< 0x00: Control */
  __IO uint32_t CFR; /**< 0x04: Configuration */
  __IO uint32_t SR;  /**< 0x08: Status */
} WWDG_Typedef;

#define WWDG_BASE_ADDR 0x40002C00UL
#define WWDG ((WWDG_Typedef *)WWDG_BASE_ADDR)

/* T[6:0] counts down; bit 6 falling from 1 to 0 is what resets the part, so
 * the counter is only ever loaded in the range 0x40..0x7F. */
#define WWDG_CR_T_MASK 0x7FU
#define WWDG_CR_T_MIN 0x40U   /**< Below this the reset has already happened. */
#define WWDG_CR_WDGA (1U << 7) /**< Activate. Set-only — no key clears it. */

#define WWDG_CFR_W_MASK 0x7FU  /**< Window value, compared against T. */
#define WWDG_CFR_WDGTB_BIT 7   /**< Timebase: PCLK1/4096 >> WDGTB. */
#define WWDG_CFR_WDGTB_MASK (0x3U << WWDG_CFR_WDGTB_BIT)
#define WWDG_CFR_EWI (1U << 9) /**< Early-wakeup interrupt at T == 0x40. */

#define WWDG_SR_EWIF (1U << 0) /**< Early-wakeup flag. */

/** @brief The WWDG counter always divides PCLK1 by this before WDGTB. */
#define WWDG_PCLK_DIV 4096U

/** @brief WWDG clock-enable bit in RCC_APB1ENR. */
#define RCC_APB1ENR_WWDGEN (1U << 11)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORTEX_M7_WDG_REG_H */
