/**
 * @file port/avr/navhal_port_uart.h
 * @brief AVR / ATmega328P UART port header.
 *
 * The public UART API lives in @c common/hal_uart.h, which includes this
 * header. The ATmega328P USART0 has no DMA backend, so unlike the Cortex-M4
 * port there are no DMA-backed UART prototypes here.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_UART_H
#define NAVHAL_PORT_UART_H

#include "common/hal_uart.h"

#endif /* NAVHAL_PORT_UART_H */
