/**
 * @file uart.h
 * @brief UART driver interface for STM32F4 (Cortex-M4).
 *
 * This file provides the definitions, macros, and function declarations
 * for UART communication, including initialization, data transmission,
 * reception, and availability checks for UART1, UART2, and UART6.
 */

#ifndef CORTEX_M4_UART_H
#define CORTEX_M4_UART_H

#include "utils/types.h"

/**
 * @enum hal_uart_t
 * @brief Enumeration of available UART peripherals.
 */
typedef enum
{
    UART1, /**< UART1 peripheral */
    UART2, /**< UART2 peripheral */
    UART6  /**< UART6 peripheral */
} hal_uart_t;

/* ============================================================
 *                  UART Write Macros
 * ============================================================ */

/**
 * @brief Macro for writing various data types to UART2.
 */
#define uart2_write(val) _Generic((val), \
    char: uart2_write_char,              \
    signed char: uart2_write_int,        \
    unsigned char: uart2_write_int,      \
    short: uart2_write_int,              \
    unsigned short: uart2_write_int,     \
    int: uart2_write_int,                \
    unsigned int: uart2_write_int,       \
    long: uart2_write_int,               \
    unsigned long: uart2_write_int,      \
    long long: uart2_write_int,          \
    unsigned long long: uart2_write_int, \
    float: uart2_write_float,            \
    double: uart2_write_float,           \
    const char *: uart2_write_string,    \
    char *: uart2_write_string)(val)

/**
 * @brief Macro for writing various data types to UART1.
 */
#define uart1_write(val) _Generic((val), \
    char: uart1_write_char,              \
    signed char: uart1_write_int,        \
    unsigned char: uart1_write_int,      \
    short: uart1_write_int,              \
    unsigned short: uart1_write_int,     \
    int: uart1_write_int,                \
    unsigned int: uart1_write_int,       \
    long: uart1_write_int,               \
    unsigned long: uart1_write_int,      \
    long long: uart1_write_int,          \
    unsigned long long: uart1_write_int, \
    float: uart1_write_float,            \
    double: uart1_write_float,           \
    const char *: uart1_write_string,    \
    char *: uart1_write_string)(val)

/**
 * @brief Macro for writing various data types to UART6.
 */
#define uart6_write(val) _Generic((val), \
    char: uart6_write_char,              \
    signed char: uart6_write_int,        \
    unsigned char: uart6_write_int,      \
    short: uart6_write_int,              \
    unsigned short: uart6_write_int,     \
    int: uart6_write_int,                \
    unsigned int: uart6_write_int,       \
    long: uart6_write_int,               \
    unsigned long: uart6_write_int,      \
    long long: uart6_write_int,          \
    unsigned long long: uart6_write_int, \
    float: uart6_write_float,            \
    double: uart6_write_float,           \
    const char *: uart6_write_string,    \
    char *: uart6_write_string)(val)

/* ============================================================
 *                  Peripheral Base Addresses
 * ============================================================ */

#define PERIPH_BASE        0x40000000UL
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE    (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x7400)

#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800)

#define USART2_BASE        (APB1PERIPH_BASE + 0x4400)
#define USART1_BASE        0x40011000
#define USART6_BASE        0x40011400

/* ============================================================
 *                  Register Definitions
 * ============================================================ */

#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0x44))

#define USART2_SR          (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR          (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR         (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1         (*(volatile uint32_t *)(USART2_BASE + 0x0C))

#define USART1_SR          (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR          (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR         (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1         (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define USART6_SR          (*(volatile uint32_t *)(USART6_BASE + 0x00))
#define USART6_DR          (*(volatile uint32_t *)(USART6_BASE + 0x04))
#define USART6_BRR         (*(volatile uint32_t *)(USART6_BASE + 0x08))
#define USART6_CR1         (*(volatile uint32_t *)(USART6_BASE + 0x0C))

/* ============================================================
 *                  Bit Definitions
 * ============================================================ */

#define RCC_APB1ENR_USART2EN (1 << 17)
#define RCC_APB2ENR_USART1EN (1 << 4)
#define RCC_APB2ENR_USART6EN (1 << 5)

#define USART_CR1_UE   (1 << 13)
#define USART_CR1_TE   (1 << 3)
#define USART_CR1_RE   (1 << 2)

#define USART_SR_TXE   (1 << 7)
#define USART_SR_RXNE  (1 << 5)

/* ============================================================
 *                  Function Declarations
 * ============================================================ */

/**
 * @brief Initialize the specified UART peripheral.
 * @param baudrate UART baud rate.
 * @param uart UART peripheral to initialize.
 */
void uart_init(uint32_t baudrate, hal_uart_t uart);

/**
 * @brief Write a character to the specified UART.
 * @param c Character to transmit.
 * @param uart UART peripheral.
 */
void uart_write_char(char c, hal_uart_t uart);

/**
 * @brief Write an integer to the specified UART.
 * @param num Integer to transmit.
 * @param uart UART peripheral.
 */
void uart_write_int(int32_t num, hal_uart_t uart);

/**
 * @brief Write a floating-point number to the specified UART.
 * @param num Float to transmit.
 * @param uart UART peripheral.
 */
void uart_write_float(float num, hal_uart_t uart);

/**
 * @brief Write a string to the specified UART.
 * @param s Null-terminated string to transmit.
 * @param uart UART peripheral.
 */
void uart_write_string(const char *s, hal_uart_t uart);

/* ---------- UART1 Specific ---------- */
void uart1_init(uint32_t baudrate);
void uart1_write_char(char c);
void uart1_write_int(int32_t num);
void uart1_write_float(float num);
void uart1_write_string(const char *s);

/* ---------- UART2 Specific ---------- */
void uart2_init(uint32_t baudrate);
void uart2_write_char(char c);
void uart2_write_int(int32_t num);
void uart2_write_float(float num);
void uart2_write_string(const char *s);

/* ---------- UART6 Specific ---------- */
void uart6_init(uint32_t baudrate);
void uart6_write_char(char c);
void uart6_write_int(int32_t num);
void uart6_write_float(float num);
void uart6_write_string(const char *s);

/**
 * @brief Read a character from the specified UART.
 * @param uart UART peripheral.
 * @return Character read.
 */
char uart_read_char(hal_uart_t uart);

/**
 * @brief Check if data is available in the specified UART's receive buffer.
 * @param uart UART peripheral.
 * @return Number of bytes available.
 */
int uart_available(hal_uart_t uart);

int uart1_available(void);
char uart1_read_char(void);

int uart2_available(void);
char uart2_read_char(void);

int uart6_available(void);
char uart6_read_char(void);

/**
 * @brief Read characters from UART2 until a delimiter or maximum length is reached.
 * @param buffer Destination buffer.
 * @param maxlen Maximum number of bytes to read.
 * @param delimiter Character to stop reading at.
 * @return Number of bytes read.
 */
uint32_t uart2_read_until(char *buffer, uint32_t maxlen, char delimiter);

#endif // !CORTEX_M4_UART_H
