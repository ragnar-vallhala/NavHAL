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
 * @file rtc_reg.h
 * @brief RTC peripheral register map for STM32F4.
 *
 * @details
 * The RTC lives in the backup domain: its registers survive a system reset and
 * are write-protected twice over — by PWR_CR.DBP for the domain, and by the
 * RTC's own key register for the calendar. Reference: RM0368 §21.
 */

#ifndef CORTEX_M4_RTC_REG_H
#define CORTEX_M4_RTC_REG_H

#include "common/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of 32-bit backup registers in the RTC block. */
#define RTC_BACKUP_REG_COUNT 20

typedef struct {
  __IO uint32_t TR;       /**< 0x00: Time (BCD) */
  __IO uint32_t DR;       /**< 0x04: Date (BCD) */
  __IO uint32_t CR;       /**< 0x08: Control */
  __IO uint32_t ISR;      /**< 0x0C: Initialization and status */
  __IO uint32_t PRER;     /**< 0x10: Prescaler */
  __IO uint32_t WUTR;     /**< 0x14: Wakeup timer */
  __IO uint32_t CALIBR;   /**< 0x18: Calibration (legacy) */
  __IO uint32_t ALRMAR;   /**< 0x1C: Alarm A */
  __IO uint32_t ALRMBR;   /**< 0x20: Alarm B */
  __IO uint32_t WPR;      /**< 0x24: Write protection key */
  __IO uint32_t SSR;      /**< 0x28: Sub second */
  __IO uint32_t SHIFTR;   /**< 0x2C: Shift control */
  __IO uint32_t TSTR;     /**< 0x30: Timestamp time */
  __IO uint32_t TSDR;     /**< 0x34: Timestamp date */
  __IO uint32_t TSSSR;    /**< 0x38: Timestamp sub second */
  __IO uint32_t CALR;     /**< 0x3C: Calibration */
  __IO uint32_t TAFCR;    /**< 0x40: Tamper and alternate function */
  __IO uint32_t ALRMASSR; /**< 0x44: Alarm A sub second */
  __IO uint32_t ALRMBSSR; /**< 0x48: Alarm B sub second */
  uint32_t RESERVED0;     /**< 0x4C */
  __IO uint32_t BKPR[RTC_BACKUP_REG_COUNT]; /**< 0x50-0x9C: Backup registers */
} RTC_Typedef;

#define RTC_BASE_ADDR 0x40002800UL
#define RTC ((RTC_Typedef *)RTC_BASE_ADDR)

/* RTC_TR / RTC_DR are BCD; these are the field shifts. */
#define RTC_TR_SECOND_SHIFT 0
#define RTC_TR_MINUTE_SHIFT 8
#define RTC_TR_HOUR_SHIFT 16
#define RTC_DR_DAY_SHIFT 0
#define RTC_DR_MONTH_SHIFT 8
#define RTC_DR_WEEKDAY_SHIFT 13
#define RTC_DR_YEAR_SHIFT 16

/* RTC_CR */
#define RTC_CR_WUCKSEL_MASK (0x7U << 0)
#define RTC_CR_WUCKSEL_DIV16 (0x0U << 0) /**< Wakeup counts RTCCLK/16 */
#define RTC_CR_WUCKSEL_SPRE (0x4U << 0)  /**< Wakeup counts the 1 Hz tick */
#define RTC_CR_BYPSHAD (1U << 5) /**< Read the counters, not the shadow regs */
#define RTC_CR_FMT (1U << 6)     /**< 0 = 24-hour clock, 1 = AM/PM */
#define RTC_CR_ALRAE (1U << 8)   /**< Alarm A enable */
#define RTC_CR_ALRBE (1U << 9)   /**< Alarm B enable */
#define RTC_CR_WUTE (1U << 10)   /**< Wakeup timer enable */
#define RTC_CR_ALRAIE (1U << 12) /**< Alarm A interrupt enable */
#define RTC_CR_ALRBIE (1U << 13) /**< Alarm B interrupt enable */
#define RTC_CR_WUTIE (1U << 14)  /**< Wakeup timer interrupt enable */

/* RTC_ISR */
#define RTC_ISR_ALRAWF (1U << 0) /**< Alarm A registers may be written */
#define RTC_ISR_ALRBWF (1U << 1) /**< Alarm B registers may be written */
#define RTC_ISR_WUTWF (1U << 2)  /**< Wakeup timer registers may be written */
#define RTC_ISR_INITS (1U << 4) /**< Calendar has been initialized */
#define RTC_ISR_RSF (1U << 5)   /**< Shadow registers synchronized */
#define RTC_ISR_INITF (1U << 6) /**< Initialization mode entered */
#define RTC_ISR_INIT (1U << 7)  /**< Request initialization mode */
#define RTC_ISR_ALRAF (1U << 8) /**< Alarm A fired */
#define RTC_ISR_ALRBF (1U << 9) /**< Alarm B fired */
#define RTC_ISR_WUTF (1U << 10) /**< Wakeup timer fired */

/* RTC_ALRMxR — the MSKn bits mean "ignore this field when matching". */
#define RTC_ALRM_SECOND_SHIFT 0
#define RTC_ALRM_MINUTE_SHIFT 8
#define RTC_ALRM_HOUR_SHIFT 16
#define RTC_ALRM_DAY_SHIFT 24
#define RTC_ALRM_MSK_SECOND (1U << 7)
#define RTC_ALRM_MSK_MINUTE (1U << 15)
#define RTC_ALRM_MSK_HOUR (1U << 23)
#define RTC_ALRM_MSK_DAY (1U << 31)
#define RTC_ALRM_WDSEL (1U << 30) /**< Match a weekday, not a date */

/* RTC_PRER */
#define RTC_PRER_SYNC(x) ((x) & 0x7FFFU)
#define RTC_PRER_ASYNC(x) (((x) & 0x7FU) << 16)

/* RTC_WPR — the two-key sequence that unlocks the calendar registers. */
#define RTC_WPR_KEY1 0xCAU
#define RTC_WPR_KEY2 0x53U
#define RTC_WPR_LOCK 0xFFU

/* RCC_BDCR — backup domain control (the RTC's clock source lives here). */
#define RCC_BDCR_LSEON (1U << 0)
#define RCC_BDCR_LSERDY (1U << 1)
#define RCC_BDCR_RTCSEL_SHIFT 8
#define RCC_BDCR_RTCSEL_MASK (0x3U << RCC_BDCR_RTCSEL_SHIFT)
#define RCC_BDCR_RTCSEL_LSE (0x1U << RCC_BDCR_RTCSEL_SHIFT)
#define RCC_BDCR_RTCSEL_LSI (0x2U << RCC_BDCR_RTCSEL_SHIFT)
#define RCC_BDCR_RTCEN (1U << 15)
#define RCC_BDCR_BDRST (1U << 16)

/* RCC_CSR — the low-speed internal oscillator. */
#define RCC_CSR_LSION (1U << 0)
#define RCC_CSR_LSIRDY (1U << 1)

/* PWR_CR.DBP gates every write to the backup domain, RTC included. */
#define PWR_BASE_ADDR 0x40007000UL
#define PWR_CR (*(volatile uint32_t *)(PWR_BASE_ADDR + 0x00))
#define PWR_CR_DBP (1U << 8)
#define RCC_APB1ENR_PWREN (1U << 28)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORTEX_M4_RTC_REG_H */
