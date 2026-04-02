/**
 * @file dwt_reg.h
 * @brief Cortex-M4 DWT (Data Watchpoint and Trace) register definitions.
 *
 * @details
 * This header defines the memory-mapped structure for DWT registers,
 * including cycle count, CPI count, and other performance counters.
 * It also defines the CoreDebug register for enabling the trace unit.
 *
 * @note DWT base address is 0xE0001000UL.
 * @note CoreDebug base address is 0xE000EDF0UL.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_DWT_REG_H
#define CORTEX_M4_DWT_REG_H

#include "common/hal_types.h"
#include <stdint.h>

/**
 * @brief DWT (Data Watchpoint and Trace) register map.
 */
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

#endif // CORTEX_M4_DWT_REG_H
