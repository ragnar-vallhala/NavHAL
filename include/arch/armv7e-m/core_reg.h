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
 * @file arch/armv7e-m/core_reg.h
 * @brief ARMv7E-M core peripherals: NVIC, SCB and DWT.
 *
 * @details
 * These blocks belong to the Cortex-M core, not to any silicon vendor. They
 * sit at architecturally fixed addresses in the 0xE000_0000 private
 * peripheral bus and are identical on every ARMv7E-M part, so every vendor
 * on this architecture sees the same definitions from here rather than
 * shipping its own copy.
 *
 * What stays with the vendor is the interrupt *numbering*: which peripheral
 * owns IRQ 37 is an MCU property, and lives in @c family/interrupt_reg.h.
 */

#ifndef NAVHAL_ARCH_ARMV7EM_CORE_REG_H
#define NAVHAL_ARCH_ARMV7EM_CORE_REG_H

#include "common/hal_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- NVIC (0xE000E100) ------------------------------------------------- */

/**
 * @brief NVIC register map.
 *
 * @details
 * Set/clear enable, set/clear pending, active bits and priorities.
 */
typedef struct {
    /* Each NVIC register array is 0x20 bytes (8 words). The ARM Cortex-M4
     * NVIC layout (PM0214 §4.3) leaves a 0x60-byte (24-word) reserved gap
     * between successive arrays. Without these RESERVED slots in the struct,
     * accesses to ICER / ISPR / ICPR / IABR / IPR land in the gap region
     * and never reach the actual peripheral. */
    __IO uint32_t ISER[8];       /**< Interrupt Set-Enable Registers (0xE000E100 - 0xE000E11C) */
    uint32_t RESERVED0[24];      /**< 0xE000E120 - 0xE000E17F */
    __IO uint32_t ICER[8];       /**< Interrupt Clear-Enable Registers (0xE000E180 - 0xE000E19C) */
    uint32_t RESERVED1[24];      /**< 0xE000E1A0 - 0xE000E1FF */
    __IO uint32_t ISPR[8];       /**< Interrupt Set-Pending Registers (0xE000E200 - 0xE000E21C) */
    uint32_t RESERVED2[24];      /**< 0xE000E220 - 0xE000E27F */
    __IO uint32_t ICPR[8];       /**< Interrupt Clear-Pending Registers (0xE000E280 - 0xE000E29C) */
    uint32_t RESERVED3[24];      /**< 0xE000E2A0 - 0xE000E2FF */
    __IO uint32_t IABR[8];       /**< Interrupt Active Bit Registers (0xE000E300 - 0xE000E31C) */
    uint32_t RESERVED4[56];      /**< 0xE000E320 - 0xE000E3FF */
    __IO uint8_t IPR[240];       /**< Interrupt Priority Registers (0xE000E400 - 0xE000E4EF) */
} NVIC_Typedef;

/** NVIC base address */
#define NVIC_BASE_ADDR 0xE000E100UL
/** NVIC instance pointer */
#define NVIC ((NVIC_Typedef *)NVIC_BASE_ADDR)

/* ---- SCB (0xE000ED00) -------------------------------------------------- */

/** @brief Application Interrupt and Reset Control Register. */
#define SCB_AIRCR (*(__IO uint32_t *)0xE000ED0CUL)
/** @brief Writes to SCB_AIRCR without this key are ignored. */
#define SCB_AIRCR_VECTKEY 0x05FA0000U
/** @brief Request a system reset. */
#define SCB_AIRCR_SYSRESETREQ (1U << 2)
/** @brief Vector Table Offset Register. */
#define SCB_VTOR (*(__IO uint32_t *)0xE000ED08UL)

/* ---- DWT + CoreDebug (0xE0001000 / 0xE000EDF0) ------------------------- */

typedef struct {
  __IO uint32_t CTRL;      /**< 0x00: Control Register */
  __IO uint32_t CYCCNT;    /**< 0x04: Cycle Count Register */
  __IO uint32_t CPICNT;    /**< 0x08: CPI Count Register */
  __IO uint32_t EXCCNT;    /**< 0x0C: Exception Overhead Count Register */
  __IO uint32_t SLEEPCNT;  /**< 0x10: Sleep Count Register */
  __IO uint32_t LSUCNT;    /**< 0x14: LSU Count Register */
  __IO uint32_t FOLDCNT;   /**< 0x18: Folded Instruction Count Register */
  __IO uint32_t PCSR;      /**< 0x1C: Program Counter Sample Register */
  __IO uint32_t COMP[16];  /**< 0x20: Comparator Registers */
  __IO uint32_t MASK[16];  /**< 0x60: Mask Registers */
  __IO uint32_t FUNCTION[16]; /**< 0xA0: Function Registers */
} DWT_Typedef;

/** DWT base address */
#define DWT_BASE_ADDR 0xE0001000UL
/** DWT instance pointer */
#define DWT ((DWT_Typedef *)DWT_BASE_ADDR)

/** DWT Control Register bits */
#define DWT_CTRL_CYCCNTENA_BIT (1 << 0) /**< Enable cycle counter */

/** DWT CoreSight Lock Access Register (offset 0xFB0). Writing the key
 *  disengages the DWT software lock. The lock is not implemented on
 *  Cortex-M4, so the write is a harmless no-op here; it matters on
 *  Cortex-M7, where the DWT ships locked out of reset. */
#define DWT_LAR (*(__IO uint32_t *)0xE0001FB0UL)
#define DWT_LAR_UNLOCK_KEY 0xC5ACCE55UL

/**
 * @brief CoreDebug register map (partial, for DWT support).
 */
typedef struct {
  __I  uint32_t DHCSR;     /**< 0x00: Debug Halting Control and Status Register */
  __O  uint32_t DCRSR;     /**< 0x04: Debug Core Register Selector Register */
  __IO uint32_t DCRDR;     /**< 0x08: Debug Core Register Data Register */
  __IO uint32_t DEMCR;     /**< 0x0C: Debug Exception and Monitor Control Register */
} CoreDebug_Typedef;

/** CoreDebug base address */
#define CORE_DEBUG_BASE_ADDR 0xE000EDF0UL
/** CoreDebug instance pointer */
#define CoreDebug ((CoreDebug_Typedef *)CORE_DEBUG_BASE_ADDR)

/** CoreDebug DEMCR bits */
#define CORE_DEBUG_DEMCR_TRCENA_BIT (1 << 24) /**< Trace enable bit */



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_ARCH_ARMV7EM_CORE_REG_H */
