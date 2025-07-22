#ifndef CORTEX_M4_UART_H
#define CORTEX_M4_UART_H

#include "utils/types.h"

// // Peripheral base addresses
#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE (PERIPH_BASE + 0x00000000UL)

#define RCC_BASE (AHB1PERIPH_BASE + 0x3800)
#define USART2_BASE (APB1PERIPH_BASE + 0x4400)

// // Register definitions
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define USART2_SR (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1 (*(volatile uint32_t *)(USART2_BASE + 0x0C))

// // Bit definitions
#define RCC_APB1ENR_USART2EN (1 << 17)

// // Bit definitions
#define RCC_APB1ENR_USART2EN (1 << 17)

#define USART_CR1_UE (1 << 13)
#define USART_CR1_TE (1 << 3)
#define USART_SR_TXE (1 << 7)

void uart2_init(uint32_t baudrate);
void uart2_write_char(char c);
void uart2_write_int(int32_t num);
void uart2_write_float(float num);
void uart2_write_string(const char *s);
#endif // !CORTEX_M4_UART_H