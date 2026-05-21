/**
 * @file port/cortex-m4/navhal_port_uart.h
 * @brief Cortex-M4 / STM32F4 UART port header.
 *
 * @details
 * The public UART API lives in @c common/hal_uart.h, which includes this
 * header. This file carries the DMA-backed UART prototypes (available only
 * when the DMA backend is enabled) and the deprecated-name shim.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_UART_H
#define NAVHAL_PORT_UART_H

#include "common/hal_uart.h"
#include "navhal_port_config.h"
#include "family/uart_reg.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Selects the DMA transmit/receive backend when DMA is available. */
#ifdef _DMA_ENABLED
#define _UART_BACKEND_DMA
#endif

/* -------------------------------------------------------------------------- *
 * DMA-backed UART API — available only when the DMA backend is enabled.
 * -------------------------------------------------------------------------- */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

/** @brief Transmit a byte buffer using DMA (buffer must stay valid). */
hal_status_t hal_uart_write_dma(hal_uart_t uart, const uint8_t *data,
                                uint16_t length);
/** @brief Set up a UART for DMA-based circular reception. */
hal_status_t hal_uart_init_dma_rx(hal_uart_t uart, uint8_t *buffer,
                                  uint16_t length);
/** @brief Transmit a null-terminated string using DMA. */
hal_status_t hal_uart_write_string_dma(hal_uart_t uart, const char *s);

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Deprecated pre-standardization UART names — retained as a backward-compat alias. */
#include "compat/uart_compat.h"

#endif /* NAVHAL_PORT_UART_H */
