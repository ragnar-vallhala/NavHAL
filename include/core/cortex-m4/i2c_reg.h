/**
 * @file core/cortex-m4/i2c_reg.h
 * @brief Cortex-M4 I²C peripheral register definitions and bit masks.
 *
 * @details
 * This header defines the memory-mapped structure of I²C registers
 * and related macros for accessing and configuring the I²C peripheral
 * on Cortex-M4 microcontrollers.
 *
 * Structures:
 * - `I2C_Reg_Typedef` : Represents the layout of I²C registers.
 *
 * Macros:
 * - Base addresses and port access macros.
 * - Control (CR1, CR2), status (SR1, SR2), clock (CCR, TRISE), and filter (FLTR) bits.
 *
 * These definitions allow the HAL and driver code to manipulate
 * I²C hardware registers in a readable and maintainable way.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_I2C_REG_H
#define CORTEX_M4_I2C_REG_H

#include "utils/types.h" /**< For __IO macro */
#include <stdint.h>

/**
 * @brief I²C peripheral register map.
 *
 * @details
 * Represents all the I²C registers used to control and monitor the peripheral.
 */
typedef struct {
    __IO uint32_t CR1;   /**< Control register 1 (0x00) */
    __IO uint32_t CR2;   /**< Control register 2 (0x04) */
    __IO uint32_t OAR1;  /**< Own address register 1 (0x08) */
    __IO uint32_t OAR2;  /**< Own address register 2 (0x0C) */
    __IO uint32_t DR;    /**< Data register (0x10) */
    __IO uint32_t SR1;   /**< Status register 1 (0x14) */
    __IO uint32_t SR2;   /**< Status register 2 (0x18) */
    __IO uint32_t CCR;   /**< Clock control register (0x1C) */
    __IO uint32_t TRISE; /**< TRISE register (0x20) */
    __IO uint32_t FLTR;  /**< Filter register (0x24) */
} I2C_Reg_Typedef;

/** Base address for I²C peripheral */
#define I2C_BASE_ADDR 0x40005400 /**< I²C base address */

/** Get pointer to I²C peripheral instance */
#define I2C_GET_BASE(n) ((I2C_Reg_Typedef *)(I2C_BASE_ADDR + (0x400 * n)))

/** Enable mask for APB1 peripheral clock */
#define I2C_APB1ENR_MASK(n) (1 << (21 + n))

/** CR1 register bit positions and masks */
#define I2C_CR1_PE_Pos     0
#define I2C_CR1_PE_MASK    (1U << I2C_CR1_PE_Pos)
#define I2C_CR1_START_Pos  8
#define I2C_CR1_START_MASK (1U << I2C_CR1_START_Pos)
#define I2C_CR1_STOP_Pos   9
#define I2C_CR1_STOP_MASK  (1U << I2C_CR1_STOP_Pos)
#define I2C_CR1_ACK_Pos    10
#define I2C_CR1_ACK_MASK   (1U << I2C_CR1_ACK_Pos)
#define I2C_CR1_POS_Pos    11
#define I2C_CR1_POS_MASK   (1U << I2C_CR1_POS_Pos)
#define I2C_CR1_SWRST_Pos  15
#define I2C_CR1_SWRST_MASK (1U << I2C_CR1_SWRST_Pos)

/** CR2 register bit positions and masks */
#define I2C_CR2_FREQ_Pos    0
#define I2C_CR2_FREQ_MASK   (0x3FU << I2C_CR2_FREQ_Pos)
#define I2C_CR2_ITERREN_Pos 8
#define I2C_CR2_ITERREN     (1U << I2C_CR2_ITERREN_Pos)
#define I2C_CR2_ITEVTEN_Pos 9
#define I2C_CR2_ITEVTEN     (1U << I2C_CR2_ITEVTEN_Pos)
#define I2C_CR2_ITBUFEN_Pos 10
#define I2C_CR2_ITBUFEN     (1U << I2C_CR2_ITBUFEN_Pos)

/** SR1 register bit positions and masks */
#define I2C_SR1_SB_Pos      0
#define I2C_SR1_SB_MASK     (1U << I2C_SR1_SB_Pos)
#define I2C_SR1_ADDR_Pos    1
#define I2C_SR1_ADDR_MASK   (1U << I2C_SR1_ADDR_Pos)
#define I2C_SR1_BTF_Pos     2
#define I2C_SR1_BTF_MASK    (1U << I2C_SR1_BTF_Pos)
#define I2C_SR1_STOPF_Pos   4
#define I2C_SR1_STOPF       (1U << I2C_SR1_STOPF_Pos)
#define I2C_SR1_RXNE_Pos    6
#define I2C_SR1_RXNE_MASK   (1U << I2C_SR1_RXNE_Pos)
#define I2C_SR1_TXE_Pos     7
#define I2C_SR1_TXE_MASK    (1U << I2C_SR1_TXE_Pos)
#define I2C_SR1_BERR_Pos    8
#define I2C_SR1_BERR        (1U << I2C_SR1_BERR_Pos)
#define I2C_SR1_ARLO_Pos    9
#define I2C_SR1_ARLO        (1U << I2C_SR1_ARLO_Pos)
#define I2C_SR1_AF_Pos      10
#define I2C_SR1_AF          (1U << I2C_SR1_AF_Pos)
#define I2C_SR1_OVR_Pos     11
#define I2C_SR1_OVR         (1U << I2C_SR1_OVR_Pos)
#define I2C_SR1_TIMEOUT_Pos 14
#define I2C_SR1_TIMEOUT     (1U << I2C_SR1_TIMEOUT_Pos)

/** SR2 register bit positions */
#define I2C_SR2_MSL_Pos 0
#define I2C_SR2_MSL     (1U << I2C_SR2_MSL_Pos)
#define I2C_SR2_BUSY_Pos 1
#define I2C_SR2_BUSY     (1U << I2C_SR2_BUSY_Pos)
#define I2C_SR2_TRA_Pos 2
#define I2C_SR2_TRA      (1U << I2C_SR2_TRA_Pos)

/** CCR register bit positions and masks */
#define I2C_CCR_CCR_Pos  0
#define I2C_CCR_CCR_MASK (0xFFFU << I2C_CCR_CCR_Pos)
#define I2C_CCR_DUTY_Pos 14
#define I2C_CCR_DUTY     (1U << I2C_CCR_DUTY_Pos)
#define I2C_CCR_FS_Pos   15
#define I2C_CCR_FS_MASK  (1U << I2C_CCR_FS_Pos)

/** TRISE register bit positions and masks */
#define I2C_TRISE_TRISE_Pos 0
#define I2C_TRISE_TRISE_Msk (0x3FU << I2C_TRISE_TRISE_Pos)

#endif // CORTEX_M4_I2C_REG_H
