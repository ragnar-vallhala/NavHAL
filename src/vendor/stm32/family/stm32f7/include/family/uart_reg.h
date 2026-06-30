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
 * @file family/uart_reg.h
 * @brief USART peripheral register map for STM32F7 (RM0410 §34).
 *
 * @details
 * The STM32F7 USART is the modern ST IP (shared with F0/F3/F7/L4), which is
 * **not** register-compatible with the F4's USART. The differences this header
 * encodes versus the F4 model:
 *
 * - Status lives in a read-only `ISR`; flags are cleared by writing 1 to the
 *   matching bit of `ICR` (the F4 cleared by a read of SR then DR).
 * - Data is split into separate `RDR` (receive) and `TDR` (transmit) registers
 *   instead of the single F4 `DR`.
 * - `CR1` enable bits moved: `UE` is bit 0 (was bit 13 on F4); `TE`/`RE` stay
 *   at bits 3/2.
 *
 * Consumed by `src/vendor/stm32/uart/uart_f7.c` (the F7-specific driver the
 * vendor CMakeLists selects in place of the F4 `uart.c`).
 */
#ifndef CORTEX_M7_UART_REG_H
#define CORTEX_M7_UART_REG_H

#include "common/hal_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  __IO uint32_t CR1;  /*!< 0x00: Control register 1 */
  __IO uint32_t CR2;  /*!< 0x04: Control register 2 */
  __IO uint32_t CR3;  /*!< 0x08: Control register 3 */
  __IO uint32_t BRR;  /*!< 0x0C: Baud rate register */
  __IO uint32_t GTPR; /*!< 0x10: Guard time and prescaler register */
  __IO uint32_t RTOR; /*!< 0x14: Receiver timeout register */
  __IO uint32_t RQR;  /*!< 0x18: Request register */
  __IO uint32_t ISR;  /*!< 0x1C: Interrupt and status register (read-only) */
  __IO uint32_t ICR;  /*!< 0x20: Interrupt flag clear register */
  __IO uint32_t RDR;  /*!< 0x24: Receive data register (bits 8:0) */
  __IO uint32_t TDR;  /*!< 0x28: Transmit data register (bits 8:0) */
} UARTx_Reg_Typedef;

#define USART1_BASE 0x40011000
#define USART2_BASE 0x40004400
#define USART3_BASE 0x40004800
#define USART6_BASE 0x40011400

#define GET_USARTx_BASE(n)                                                     \
  ((volatile UARTx_Reg_Typedef *)((n) == 1   ? (USART1_BASE)                   \
                                  : (n) == 2 ? (USART2_BASE)                   \
                                  : (n) == 3 ? (USART3_BASE)                   \
                                  : (n) == 6 ? (USART6_BASE)                   \
                                             : (0)))

/* Clock enable bits (same RCC bit positions as F4 for these instances). */
#define RCC_APB1ENR_USART2EN (1 << 17)
#define RCC_APB1ENR_USART3EN (1 << 18)
#define RCC_APB2ENR_USART1EN (1 << 4)
#define RCC_APB2ENR_USART6EN (1 << 5)

/* CR1 control bits (note UE moved to bit 0 vs F4's bit 13). */
#define USART_CR1_UE     (1 << 0)  ///< USART enable
#define USART_CR1_RE     (1 << 2)  ///< Receiver enable
#define USART_CR1_TE     (1 << 3)  ///< Transmitter enable
#define USART_CR1_RXNEIE (1 << 5)  ///< RXNE interrupt enable
#define USART_CR1_TCIE   (1 << 6)  ///< Transmission-complete interrupt enable
#define USART_CR1_TXEIE  (1 << 7)  ///< TXE interrupt enable

/* ISR status bits (read-only). */
#define USART_ISR_PE   (1 << 0) ///< Parity error
#define USART_ISR_FE   (1 << 1) ///< Framing error
#define USART_ISR_NE   (1 << 2) ///< Noise detected
#define USART_ISR_ORE  (1 << 3) ///< Overrun error
#define USART_ISR_IDLE (1 << 4) ///< Idle line detected
#define USART_ISR_RXNE (1 << 5) ///< Read data register not empty
#define USART_ISR_TC   (1 << 6) ///< Transmission complete
#define USART_ISR_TXE  (1 << 7) ///< Transmit data register empty

/* ICR clear bits (write 1 to clear the matching ISR flag). */
#define USART_ICR_PECF   (1 << 0) ///< Clear parity error
#define USART_ICR_FECF   (1 << 1) ///< Clear framing error
#define USART_ICR_NCF    (1 << 2) ///< Clear noise flag
#define USART_ICR_ORECF  (1 << 3) ///< Clear overrun error
#define USART_ICR_IDLECF (1 << 4) ///< Clear idle line
#define USART_ICR_TCCF   (1 << 6) ///< Clear transmission complete

/* CR3 DMA enable bits (for the future F7 DMA backend). */
#define USART_CR3_DMAT (1 << 7) ///< DMA enable for transmitter
#define USART_CR3_DMAR (1 << 6) ///< DMA enable for receiver


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !CORTEX_M7_UART_REG_H
