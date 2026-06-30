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
 * @file family/i2c_reg.h
 * @brief STM32F7 I²C peripheral register map (RM0410 §33).
 *
 * @details
 * The F7 I²C is the **modern** ST IP (shared with F0/F3/F7/L4), unrelated to
 * the F4's legacy block. Timing comes from a single `TIMINGR` (not CCR/TRISE),
 * status is a read-only `ISR` cleared via `ICR`, transfers are framed by
 * `CR2` (SADD / NBYTES / RD_WRN / AUTOEND / START / STOP), and data uses split
 * `RXDR` / `TXDR`. Consumed by `src/vendor/stm32/i2c/i2c_f7.c`.
 */

#ifndef CORTEX_M7_I2C_REG_H
#define CORTEX_M7_I2C_REG_H

#include "common/hal_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  __IO uint32_t CR1;      /**< 0x00 Control register 1 */
  __IO uint32_t CR2;      /**< 0x04 Control register 2 */
  __IO uint32_t OAR1;     /**< 0x08 Own address 1 */
  __IO uint32_t OAR2;     /**< 0x0C Own address 2 */
  __IO uint32_t TIMINGR;  /**< 0x10 Timing register */
  __IO uint32_t TIMEOUTR; /**< 0x14 Timeout register */
  __IO uint32_t ISR;      /**< 0x18 Interrupt and status (read-only flags) */
  __IO uint32_t ICR;      /**< 0x1C Interrupt clear */
  __IO uint32_t PECR;     /**< 0x20 PEC register */
  __IO uint32_t RXDR;     /**< 0x24 Receive data */
  __IO uint32_t TXDR;     /**< 0x28 Transmit data */
} I2C_Reg_Typedef;

/* I²C1 0x40005400, I²C2 0x40005800, I²C3 0x40005C00 (same bases as F4). */
#define I2C_BASE_ADDR 0x40005400UL
#define I2C_GET_BASE(n) ((I2C_Reg_Typedef *)(I2C_BASE_ADDR + (0x400UL * (n))))
/* APB1ENR: I2C1EN bit21, I2C2EN bit22, I2C3EN bit23. */
#define I2C_APB1ENR_MASK(n) (1U << (21 + (n)))

/* CR1 */
#define I2C_CR1_PE (1U << 0)

/* CR2 */
#define I2C_CR2_SADD_Pos 0
#define I2C_CR2_RD_WRN (1U << 10) /**< 1 = read transfer */
#define I2C_CR2_START (1U << 13)
#define I2C_CR2_STOP (1U << 14)
#define I2C_CR2_NBYTES_Pos 16
#define I2C_CR2_NBYTES_Msk (0xFFU << I2C_CR2_NBYTES_Pos)
#define I2C_CR2_AUTOEND (1U << 25)
/** @brief Build CR2 for a 7-bit-address transfer of @p n bytes. */
#define I2C_CR2_SADD7(addr) (((uint32_t)(addr) << 1) & 0xFEU)
#define I2C_CR2_NBYTES(n) (((uint32_t)(n) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES_Msk)

/* ISR (read-only status) */
#define I2C_ISR_TXE (1U << 0)
#define I2C_ISR_TXIS (1U << 1)
#define I2C_ISR_RXNE (1U << 2)
#define I2C_ISR_ADDR (1U << 3)
#define I2C_ISR_NACKF (1U << 4)
#define I2C_ISR_STOPF (1U << 5)
#define I2C_ISR_TC (1U << 6)
#define I2C_ISR_TCR (1U << 7)
#define I2C_ISR_BERR (1U << 8)
#define I2C_ISR_ARLO (1U << 9)
#define I2C_ISR_BUSY (1U << 15)

/* ICR (write 1 to clear) */
#define I2C_ICR_NACKCF (1U << 4)
#define I2C_ICR_STOPCF (1U << 5)

/* TIMINGR presets for I2CCLK = 16 MHz (reset HSI / APB1 default on F767).
 * From the ST reference timing tables. At other I²C clocks these must be
 * recomputed (CubeMX / the RM algorithm); i2c_f7.c documents this limit. */
#define I2C_TIMINGR_SM_16MHZ 0x00303D5BUL /**< 100 kHz standard mode */
#define I2C_TIMINGR_FM_16MHZ 0x0010061AUL /**< 400 kHz fast mode */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !CORTEX_M7_I2C_REG_H
