#ifndef CORTEX_M4_I2C_REG_H
#define CORTEX_M4_I2C_REG_H

#include "utils/types.h" // For __IO macro if you already define it
#include <stdint.h>

// ----------------------
// I²C Register Map
// ----------------------
typedef struct {
  __IO uint32_t CR1;   // 0x00 Control register 1
  __IO uint32_t CR2;   // 0x04 Control register 2
  __IO uint32_t OAR1;  // 0x08 Own address register 1
  __IO uint32_t OAR2;  // 0x0C Own address register 2
  __IO uint32_t DR;    // 0x10 Data register
  __IO uint32_t SR1;   // 0x14 Status register 1
  __IO uint32_t SR2;   // 0x18 Status register 2
  __IO uint32_t CCR;   // 0x1C Clock control register
  __IO uint32_t TRISE; // 0x20 TRISE register
  __IO uint32_t FLTR;  // 0x24 Filter register
} I2C_Reg_Typedef;

// ----------------------
// Base addresses
// ----------------------
#define I2C_BASE_ADDR 0x40005400
#define I2C_GET_BASE(n) ((I2C_Reg_Typedef *)(I2C_BASE_ADDR + (0x400 * n)))

// Enable Mask
#define I2C_APB1ENR_MASK(n) (1 << (21 + n))

// ----------------------
// CR1 bits
// ----------------------
#define I2C_CR1_PE_Pos 0
#define I2C_CR1_PE_MASK (1U << I2C_CR1_PE_Pos)

#define I2C_CR1_START_Pos 8
#define I2C_CR1_START_MASK (1U << I2C_CR1_START_Pos)

#define I2C_CR1_STOP_Pos 9
#define I2C_CR1_STOP_MASK (1U << I2C_CR1_STOP_Pos)

#define I2C_CR1_ACK_Pos 10
#define I2C_CR1_ACK_MASK (1U << I2C_CR1_ACK_Pos)

#define I2C_CR1_POS_Pos 11
#define I2C_CR1_POS_MASK (1U << I2C_CR1_POS_Pos)

#define I2C_CR1_SWRST_Pos 15
#define I2C_CR1_SWRST_MASK (1U << I2C_CR1_SWRST_Pos)

// ----------------------
// CR2 bits
// ----------------------
#define I2C_CR2_FREQ_Pos 0
#define I2C_CR2_FREQ_MASK (0x3FU << I2C_CR2_FREQ_Pos) // Max 42 MHz

#define I2C_CR2_ITERREN_Pos 8
#define I2C_CR2_ITERREN (1U << I2C_CR2_ITERREN_Pos)

#define I2C_CR2_ITEVTEN_Pos 9
#define I2C_CR2_ITEVTEN (1U << I2C_CR2_ITEVTEN_Pos)

#define I2C_CR2_ITBUFEN_Pos 10
#define I2C_CR2_ITBUFEN (1U << I2C_CR2_ITBUFEN_Pos)

// ----------------------
// SR1 bits
// ----------------------
#define I2C_SR1_SB_Pos 0
#define I2C_SR1_SB_MASK (1U << I2C_SR1_SB_Pos) // Start bit sent

#define I2C_SR1_ADDR_Pos 1
#define I2C_SR1_ADDR_MASK (1U << I2C_SR1_ADDR_Pos) // Address sent/matched

#define I2C_SR1_BTF_Pos 2
#define I2C_SR1_BTF_MASK (1U << I2C_SR1_BTF_Pos) // Byte transfer finished

#define I2C_SR1_STOPF_Pos 4
#define I2C_SR1_STOPF (1U << I2C_SR1_STOPF_Pos)

#define I2C_SR1_RXNE_Pos 6
#define I2C_SR1_RXNE_MASK (1U << I2C_SR1_RXNE_Pos) // Data register not empty

#define I2C_SR1_TXE_Pos 7
#define I2C_SR1_TXE_MASK (1U << I2C_SR1_TXE_Pos) // Data register empty

#define I2C_SR1_BERR_Pos 8
#define I2C_SR1_BERR (1U << I2C_SR1_BERR_Pos) // Bus error

#define I2C_SR1_ARLO_Pos 9
#define I2C_SR1_ARLO (1U << I2C_SR1_ARLO_Pos) // Arbitration lost

#define I2C_SR1_AF_Pos 10
#define I2C_SR1_AF (1U << I2C_SR1_AF_Pos) // Acknowledge failure

#define I2C_SR1_OVR_Pos 11
#define I2C_SR1_OVR (1U << I2C_SR1_OVR_Pos) // Overrun/Underrun

#define I2C_SR1_TIMEOUT_Pos 14
#define I2C_SR1_TIMEOUT (1U << I2C_SR1_TIMEOUT_Pos)

// ----------------------
// SR2 bits
// ----------------------
#define I2C_SR2_MSL_Pos 0
#define I2C_SR2_MSL (1U << I2C_SR2_MSL_Pos) // Master mode

#define I2C_SR2_BUSY_Pos 1
#define I2C_SR2_BUSY (1U << I2C_SR2_BUSY_Pos) // Bus busy

#define I2C_SR2_TRA_Pos 2
#define I2C_SR2_TRA (1U << I2C_SR2_TRA_Pos) // Transmitter mode

// ----------------------
// CCR bits
// ----------------------
#define I2C_CCR_CCR_Pos 0
#define I2C_CCR_CCR_MASK (0xFFFU << I2C_CCR_CCR_Pos)

#define I2C_CCR_DUTY_Pos 14
#define I2C_CCR_DUTY (1U << I2C_CCR_DUTY_Pos)

#define I2C_CCR_FS_Pos 15
#define I2C_CCR_FS_MASK (1U << I2C_CCR_FS_Pos) // Fast mode

// ----------------------
// TRISE bits
// ----------------------
#define I2C_TRISE_TRISE_Pos 0
#define I2C_TRISE_TRISE_Msk (0x3FU << I2C_TRISE_TRISE_Pos)

#endif // CORTEX_M4_I2C_REG_H
