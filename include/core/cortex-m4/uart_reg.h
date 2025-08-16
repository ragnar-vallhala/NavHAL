/**
 * @brief UART peripheral register map for STM32F4.
 *
 * @details
 * This structure provides direct access to the memory-mapped UART registers.
 * Each member corresponds to a specific UART register and its offset.
 * All registers are marked as `__IO` for volatile read/write access.
 */
#ifndef CORTEX_M4_UART_REG_H
#define CORTEX_M4_UART_REG_H

#include "common/hal_types.h"
#include <stdint.h>

typedef struct {
  __IO uint32_t SR;  /*!< 0x00: Status Register
                      *  Contains flags like TXE, RXNE, TC, etc.
                      */
  __IO uint32_t DR;  /*!< 0x04: Data Register
                      *  Write to transmit data, read to receive data.
                      */
  __IO uint32_t BRR; /*!< 0x08: Baud Rate Register
                      *  Configures UART baud rate based on peripheral clock.
                      */
  __IO uint32_t CR1; /*!< 0x0C: Control Register 1
                      *  Enables UART, configures word length, parity, etc.
                      */
  __IO uint32_t
      CR2;            /*!< 0x10: Control Register 2
                       *  Configures stop bits, clock enable for synchronous mode.
                       */
  __IO uint32_t CR3;  /*!< 0x14: Control Register 3
                       *  Enables DMA, error interrupts, smart card mode, etc.
                       */
  __IO uint32_t GTPR; /*!< 0x18: Guard Time and Prescaler Register
                       *  Used for smart card mode timing and prescaler.
                       */
} UARTx_Reg_Typedef;

#define USART1_BASE 0x40011000
#define USART2_BASE 0x40004400
#define USART6_BASE 0x40011400

#define GET_USARTx_BASE(n)                                                      \
  ((UARTx_Reg_Typedef *)(n == 1                                                \
                             ? (USART1_BASE)                                   \
                             : (n == 2 ? (USART2_BASE)                         \
                                       : (n == 6 ? (USART6_BASE) : (NULL)))))


/* Clock enable bits */
#define RCC_APB1ENR_USART2EN (1 << 17)
#define RCC_APB2ENR_USART1EN (1 << 4)
#define RCC_APB2ENR_USART6EN (1 << 5)


/* Control register bits */
#define USART_CR1_UE (1 << 13) ///< USART Enable
#define USART_CR1_TE (1 << 3)  ///< Transmitter Enable
#define USART_CR1_RE (1 << 2)  ///< Receiver Enable

/* Status register bits */
#define USART_SR_TXE (1 << 7)  ///< Transmit Data Register Empty
#define USART_SR_RXNE (1 << 5) ///< Read Data Register Not Empty

#endif // !CORTEX_M4_UART_REG_H
