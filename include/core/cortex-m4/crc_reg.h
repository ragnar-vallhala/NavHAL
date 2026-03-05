/**
 * @file core/cortex-m4/crc_reg.h
 * @brief CRC peripheral register definitions for STM32F4.
 *
 * @details
 * Memory-mapped register structure for the STM32F4 hardware CRC unit at
 * base address 0x40023000.  The unit computes CRC-32/MPEG-2
 * (polynomial 0x04C11DB7, initial value 0xFFFFFFFF, no reflection).
 *
 * Data must be written as 32-bit words.  For byte-oriented input the
 * driver packs bytes into words before writing.
 *
 * @note Only compiled when @c _CRC_HW_ENABLED is defined.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_CRC_REG_H
#define CORTEX_M4_CRC_REG_H

#ifdef _CRC_HW_ENABLED

#include "common/hal_types.h"
#include <stdint.h>

/*---------------------------------------------------------------------------
 * CRC peripheral register block
 *---------------------------------------------------------------------------*/

/**
 * @brief CRC peripheral register map (STM32F4).
 *
 * Mapped at CRC_BASE_ADDR (0x40023000).
 */
typedef struct {
  __IO uint32_t
      DR; /**< 0x00: Data register  — write input words, read result */
  __IO uint32_t IDR; /**< 0x04: Independent data register (8-bit scratch pad) */
  __IO uint32_t CR;  /**< 0x08: Control register (bit 0 = RESET)  */
} CRC_Typedef;

/*---------------------------------------------------------------------------
 * Instance pointer
 *---------------------------------------------------------------------------*/

#define CRC_BASE_ADDR 0x40023000UL         /**< CRC unit base address */
#define CRC ((CRC_Typedef *)CRC_BASE_ADDR) /**< CRC peripheral instance */

/*---------------------------------------------------------------------------
 * RCC AHB1ENR clock-enable bit for CRC
 *---------------------------------------------------------------------------*/

#define RCC_AHB1ENR_CRCEN (1U << 12) /**< CRC clock enable bit in AHB1ENR */

/*---------------------------------------------------------------------------
 * CRC_CR — control register bits
 *---------------------------------------------------------------------------*/

#define CRC_CR_RESET                                                           \
  (1U << 0) /**< Reset CRC DR to initial value (0xFFFFFFFF) */

#endif /* _CRC_HW_ENABLED */

#endif /* CORTEX_M4_CRC_REG_H */
