/**
 * @file core/cortex-m4/spi_reg.h
 * @brief Cortex-M4 SPI peripheral register definitions and bit masks.
 *
 * @details
 * This header defines the memory-mapped structure of SPI registers
 * and related macros for accessing and configuring the SPI peripheral
 * on Cortex-M4 microcontrollers.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_SPI_REG_H
#define CORTEX_M4_SPI_REG_H

#include "common/hal_types.h"
#include <stdint.h>

/**
 * @brief SPI peripheral register map.
 */
typedef struct {
  __IO uint32_t CR1;     /**< Control register 1 (0x00) */
  __IO uint32_t CR2;     /**< Control register 2 (0x04) */
  __IO uint32_t SR;      /**< Status register (0x08) */
  __IO uint32_t DR;      /**< Data register (0x0C) */
  __IO uint32_t CRCPR;   /**< CRC polynomial register (0x10) */
  __IO uint32_t RXCRCR;  /**< RX CRC register (0x14) */
  __IO uint32_t TXCRCR;  /**< TX CRC register (0x18) */
  __IO uint32_t I2SCFGR; /**< I2S configuration register (0x1C) */
  __IO uint32_t I2SPR;   /**< I2S prescaler register (0x20) */
} SPI_Reg_Typedef;

/** Base addresses for SPI peripherals on STM32F401 */
#define SPI1_BASE 0x40013000
#define SPI2_BASE 0x40003800

/** Get pointer to SPI peripheral instance */
#define GET_SPIx_BASE(n)                                                       \
  ((volatile SPI_Reg_Typedef *)(n == 1 ? (SPI1_BASE)                           \
                                       : (n == 2 ? (SPI2_BASE) : 0)))

/** Clock enable bits in RCC */
#define RCC_APB2ENR_SPI1EN (1 << 12)
#define RCC_APB1ENR_SPI2EN (1 << 14)

/** CR1 register bit positions and masks */
#define SPI_CR1_CPHA (1U << 0)
#define SPI_CR1_CPOL (1U << 1)
#define SPI_CR1_MSTR (1U << 2)
#define SPI_CR1_BR_Pos 3
#define SPI_CR1_BR_Msk (0x7U << SPI_CR1_BR_Pos)
#define SPI_CR1_SPE (1U << 6)
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR1_SSI (1U << 8)
#define SPI_CR1_SSM (1U << 9)
#define SPI_CR1_RXONLY (1U << 10)
#define SPI_CR1_DFF (1U << 11)
#define SPI_CR1_BIDIOE (1U << 14)
#define SPI_CR1_BIDIMODE (1U << 15)

/** SR register bit positions and masks */
#define SPI_SR_RXNE (1U << 0)
#define SPI_SR_TXE (1U << 1)
#define SPI_SR_BSY (1U << 7)

#endif // CORTEX_M4_SPI_REG_H
