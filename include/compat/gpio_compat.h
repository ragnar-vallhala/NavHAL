/**
 * @file gpio_compat.h
 * @brief Deprecated pre-standardization GPIO API shim.
 *
 * @details
 * Maps the pre-standardization GPIO function names onto the standardized
 * `hal_gpio_*` API so that existing drivers and samples keep building during
 * the M2-M5 migration. Included automatically by `core/cortex-m4/gpio.h`.
 *
 * This header — and all symbols it defines — are removed in M5. New code MUST
 * use the standardized names directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_GPIO_COMPAT_H
#define NAVHAL_GPIO_COMPAT_H

#define hal_gpio_setmode(pin, mode, pupd)                                      \
  hal_gpio_set_mode((pin), (mode), (pupd)) /**< @deprecated hal_gpio_set_mode */
#define hal_gpio_getmode(pin)                                                  \
  hal_gpio_get_mode(pin) /**< @deprecated hal_gpio_get_mode */
#define hal_gpio_digitalwrite(pin, state)                                      \
  hal_gpio_write((pin), (state)) /**< @deprecated hal_gpio_write */
#define hal_gpio_digitalread(pin)                                              \
  hal_gpio_read(pin) /**< @deprecated hal_gpio_read */
#define hal_gpio_enable_rcc(pin)                                               \
  hal_gpio_enable_clock(pin) /**< @deprecated hal_gpio_enable_clock */

#endif /* NAVHAL_GPIO_COMPAT_H */
