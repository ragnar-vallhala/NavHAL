/**
 * @file sdio_reg.h
 * @brief SDIO peripheral register map for STM32F4.
 *
 * @details
 * This structure provides direct access to the memory-mapped SDIO registers.
 * Based on the STM32F401x reference manual.
 */

#ifndef CORTEX_M4_SDIO_REG_H
#define CORTEX_M4_SDIO_REG_H

#include "common/hal_types.h"
#include <stdint.h>

/**
 * @brief SDIO Register Structure
 */
typedef struct {
  __IO uint32_t POWER;    /**< 0x00: Power control register */
  __IO uint32_t CLKCR;    /**< 0x04: SDI clock control register */
  __IO uint32_t ARG;      /**< 0x08: Argument register */
  __IO uint32_t CMD;      /**< 0x0C: Command register */
  __IO uint32_t RESPCMD;  /**< 0x10: Command response register */
  __IO uint32_t RESP1;    /**< 0x14: Response 1 register */
  __IO uint32_t RESP2;    /**< 0x18: Response 2 register */
  __IO uint32_t RESP3;    /**< 0x1C: Response 3 register */
  __IO uint32_t RESP4;    /**< 0x20: Response 4 register */
  __IO uint32_t DTIMER;   /**< 0x24: Data timer register */
  __IO uint32_t DLEN;     /**< 0x28: Data length register */
  __IO uint32_t DCTRL;    /**< 0x2C: Data control register */
  __IO uint32_t DCOUNT;   /**< 0x30: Data counter register */
  __IO uint32_t STA;      /**< 0x34: Status register */
  __IO uint32_t ICR;      /**< 0x38: Interrupt clear register */
  __IO uint32_t MASK;     /**< 0x3C: Mask register */
  uint32_t RESERVED0[2];  /**< 0x40-0x44: Reserved */
  __IO uint32_t FIFOCNT;  /**< 0x48: FIFO counter register */
  uint32_t RESERVED2[13]; /**< 0x4C-0x7C: Reserved */
  __IO uint32_t FIFO;     /**< 0x80: Data FIFO register */
} SDIO_Reg_Typedef;

#define SDIO_BASE 0x40012C00
#define SDIO ((volatile SDIO_Reg_Typedef *)SDIO_BASE)

/* RCC Enable Bit */
#define RCC_APB2ENR_SDIOEN (1 << 11)

/* --- Register Bit Definitions --- */

/* POWER Register */
#define SDIO_POWER_PWRCTRL_Pos (0U)
#define SDIO_POWER_PWRCTRL_Msk (0x3U << SDIO_POWER_PWRCTRL_Pos)
#define SDIO_POWER_PWRCTRL_OFF (0x0U << SDIO_POWER_PWRCTRL_Pos)
#define SDIO_POWER_PWRCTRL_ON (0x3U << SDIO_POWER_PWRCTRL_Pos)

/* CLKCR Register */
#define SDIO_CLKCR_WIDBUS_Pos (11U)
#define SDIO_CLKCR_WIDBUS_Msk (0x3U << SDIO_CLKCR_WIDBUS_Pos)
#define SDIO_CLKCR_WIDBUS_1B (0x0U << SDIO_CLKCR_WIDBUS_Pos)
#define SDIO_CLKCR_WIDBUS_4B (0x1U << SDIO_CLKCR_WIDBUS_Pos)
#define SDIO_CLKCR_WIDBUS_8B (0x2U << SDIO_CLKCR_WIDBUS_Pos)

#define SDIO_CLKCR_CLKEN (1 << 8)
#define SDIO_CLKCR_PWRSAV (1 << 9)
#define SDIO_CLKCR_BYPASS (1 << 10)
#define SDIO_CLKCR_NEGEDGE (1 << 13)
#define SDIO_CLKCR_HWFC_EN (1 << 14)

#define SDIO_CLKCR_CLKDIV 0x000000FFU

/* STA/ICR Static Flags */
#define SDIO_STATIC_FLAGS                                                      \
  (SDIO_STA_CCRCFAIL | SDIO_STA_DCRCFAIL | SDIO_STA_CTIMEOUT |                 \
   SDIO_STA_DTIMEOUT | SDIO_STA_TXUNDERR | SDIO_STA_RXOVERR |                  \
   SDIO_STA_CMDREND | SDIO_STA_CMDSENT | SDIO_STA_DATAEND | SDIO_STA_DBCKEND)

/* CMD Register */
#define SDIO_CMD_CMDINDEX_Pos (0U)
#define SDIO_CMD_CMDINDEX_Msk (0x3FU << SDIO_CMD_CMDINDEX_Pos)
#define SDIO_CMD_WAITRESP_Pos (6U)
#define SDIO_CMD_WAITRESP_Msk (0x3U << SDIO_CMD_WAITRESP_Pos)
#define SDIO_CMD_WAITRESP_NO (0x0U << SDIO_CMD_WAITRESP_Pos)
#define SDIO_CMD_WAITRESP_SHORT (0x1U << SDIO_CMD_WAITRESP_Pos)
#define SDIO_CMD_WAITRESP_LONG (0x3U << SDIO_CMD_WAITRESP_Pos)

#define SDIO_CMD_WAITINT (1 << 8)
#define SDIO_CMD_WAITPEND (1 << 9)
#define SDIO_CMD_CPSMEN (1 << 10)

/* STA Register */
#define SDIO_STA_CCRCFAIL (1 << 0)
#define SDIO_STA_DCRCFAIL (1 << 1)
#define SDIO_STA_CTIMEOUT (1 << 2)
#define SDIO_STA_DTIMEOUT (1 << 3)
#define SDIO_STA_TXUNDERR (1 << 4)
#define SDIO_STA_RXOVERR (1 << 5)
#define SDIO_STA_CMDREND (1 << 6)
#define SDIO_STA_CMDSENT (1 << 7)
#define SDIO_STA_DATAEND (1 << 8)
#define SDIO_STA_STBITERR (1 << 9)
#define SDIO_STA_DBCKEND (1 << 10)
#define SDIO_STA_CMDACT (1 << 11)
#define SDIO_STA_TXACT (1 << 12)
#define SDIO_STA_RXACT (1 << 13)
#define SDIO_STA_TXFIFOHE (1 << 14)
#define SDIO_STA_RXFIFOHF (1 << 15)
#define SDIO_STA_TXFIFOF (1 << 16)
#define SDIO_STA_RXFIFOF (1 << 17)
#define SDIO_STA_TXFIFOE (1 << 18)
#define SDIO_STA_RXFIFOE (1 << 19)
#define SDIO_STA_TXDAVL (1 << 20)
#define SDIO_STA_RXDAVL (1 << 21)
#define SDIO_STA_SDIOIT (1 << 22)

/* ICR Register */
#define SDIO_ICR_CCRCFAILC (1 << 0)
#define SDIO_ICR_DCRCFAILC (1 << 1)
#define SDIO_ICR_CTIMEOUTC (1 << 2)
#define SDIO_ICR_DTIMEOUTC (1 << 3)
#define SDIO_ICR_TXUNDERRC (1 << 4)
#define SDIO_ICR_RXOVERRC (1 << 5)
#define SDIO_ICR_CMDRENDC (1 << 6)
#define SDIO_ICR_CMDSENTC (1 << 7)
#define SDIO_ICR_DATAENDC (1 << 8)
#define SDIO_ICR_STBITERRC (1 << 9)
#define SDIO_ICR_DBCKENDC (1 << 10)
#define SDIO_ICR_SDIOITC (1 << 22)

/* MASK Register */
#define SDIO_MASK_CCRCFAILIE (1 << 0)
#define SDIO_MASK_DCRCFAILIE (1 << 1)
#define SDIO_MASK_CTIMEOUTIE (1 << 2)
#define SDIO_MASK_DTIMEOUTIE (1 << 3)
#define SDIO_MASK_TXUNDERRIE (1 << 4)
#define SDIO_MASK_RXOVERRIE (1 << 5)
#define SDIO_MASK_CMDRENDIE (1 << 6)
#define SDIO_MASK_CMDSENTIE (1 << 7)
#define SDIO_MASK_DATAENDIE (1 << 8)
#define SDIO_MASK_STBITERRIE (1 << 9)
#define SDIO_MASK_DBCKENDIE (1 << 10)
#define SDIO_MASK_CMDACTIE (1 << 11)
#define SDIO_MASK_TXACTIE (1 << 12)
#define SDIO_MASK_RXACTIE (1 << 13)
#define SDIO_MASK_TXFIFOHEIE (1 << 14)
#define SDIO_MASK_RXFIFOHFIE (1 << 15)
#define SDIO_MASK_TXFIFOFIE (1 << 16)
#define SDIO_MASK_RXFIFOFIE (1 << 17)
#define SDIO_MASK_TXFIFOEIE (1 << 18)
#define SDIO_MASK_RXFIFOEIE (1 << 19)
#define SDIO_MASK_TXDAVLIE (1 << 20)
#define SDIO_MASK_RXDAVLIE (1 << 21)
#define SDIO_MASK_SDIOITIE (1 << 22)

/* DCTRL Register */
#define SDIO_DCTRL_DTEN (1 << 0)
#define SDIO_DCTRL_DTDIR (1 << 1)
#define SDIO_DCTRL_DTMODE (1 << 2)
#define SDIO_DCTRL_DMAEN (1 << 3)
#define SDIO_DCTRL_DBLOCKSIZE_Pos 4

#endif // !CORTEX_M4_SDIO_REG_H
