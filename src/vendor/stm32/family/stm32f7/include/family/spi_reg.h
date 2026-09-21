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
 * @file family/spi_reg.h
 * @brief Cortex-M4 SPI peripheral register definitions and bit masks.
 *
 * @details
 * This header defines the memory-mapped structure of SPI registers
 * and related macros for accessing and configuring the SPI peripheral
 * on Cortex-M4 microcontrollers.
 */

#ifndef CORTEX_M4_SPI_REG_H
#define CORTEX_M4_SPI_REG_H

#include "common/hal_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
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
/* NOTE: CR1 bit 11 is CRCL on the F7, not DFF — data size moved to CR2.DS
 * below. SPI_CR1_DFF is intentionally absent for the F7; spi_f7.c uses CR2. */
#define SPI_CR1_BIDIOE (1U << 14)
#define SPI_CR1_BIDIMODE (1U << 15)

/** CR2: F7 data size (DS) + RX-FIFO threshold (FRXTH). The F4's CR1.DFF is
 *  replaced by these — DS selects the frame width and FRXTH lets RXNE assert on
 *  an 8-bit boundary (so byte-at-a-time RX works). */
#define SPI_CR2_DS_Pos 8
#define SPI_CR2_DS_Msk (0xFU << SPI_CR2_DS_Pos)
#define SPI_CR2_DS_8BIT (0x7U << SPI_CR2_DS_Pos)  /* 0111 = 8-bit frames */
#define SPI_CR2_DS_16BIT (0xFU << SPI_CR2_DS_Pos) /* 1111 = 16-bit frames */
#define SPI_CR2_FRXTH (1U << 12)                  /* RXNE on 8-bit (1/4 FIFO) */

/** SR register bit positions and masks */
#define SPI_SR_RXNE (1U << 0)
#define SPI_SR_TXE (1U << 1)
#define SPI_SR_BSY (1U << 7)


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // CORTEX_M4_SPI_REG_H
