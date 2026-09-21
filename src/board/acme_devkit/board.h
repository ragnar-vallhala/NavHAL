/**
 * @file board.h
 * @brief ACME devkit board description.
 *
 * @details
 * A reference port carries no real hardware, so this names only what a GPIO
 * sample needs. Real boards additionally describe their console UART, clock
 * source and pin map.
 */

#ifndef BOARD_H
#define BOARD_H

#include "common/hal_gpio.h"

/* The board-level names a portable sample expects. A real board also
 * describes its console UART, clock source and header pin map. */
#define LED_BUILTIN GPIO_PA05    /**< User LED, port A pin 5. */
#define LED_ON      HAL_GPIO_HIGH /**< Level that lights ::LED_BUILTIN. */

#endif /* BOARD_H */
