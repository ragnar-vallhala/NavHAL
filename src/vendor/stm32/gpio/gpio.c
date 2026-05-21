/**
 * @file gpio.c
 * @brief Standardized HAL GPIO driver implementation for STM32F4 (Cortex-M4).
 *
 * @details
 * Implements the standardized `hal_gpio_*` API declared in
 * `port/cortex-m4/navhal_port_gpio.h`: pin configuration, mode/pull/alternate-function
 * setup, output type/speed, and port clock enabling. The hot-path
 * write/read/toggle helpers are defined `static inline` in the header.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_gpio.h"
#include "family/gpio_reg.h"
#include "family/rcc_reg.h"

hal_status_t hal_gpio_enable_clock(hal_gpio_pin_t pin) {
  uint32_t port = GPIO_GET_PORT_NUMBER(pin);
  if (!(RCC->AHB1ENR & (1U << port)))
    RCC->AHB1ENR |= (1U << port);
  return HAL_OK;
}

hal_status_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                               hal_gpio_pull_t pull) {
  hal_gpio_enable_clock(pin); // Ensure GPIO port clock enabled

  uint32_t shift = GPIO_GET_PIN(pin) * 2;

  // Set the mode bits for the pin
  GPIO_GET_PORT(pin)->MODER &= ~(0x3U << shift);
  GPIO_GET_PORT(pin)->MODER |= (((uint32_t)mode & 0x3U) << shift);

  // Configure pull-up/pull-down resistor
  GPIO_GET_PORT(pin)->PUPDR &= ~(0x3U << shift);
  GPIO_GET_PORT(pin)->PUPDR |= (((uint32_t)pull & 0x3U) << shift);

  return HAL_OK;
}

hal_gpio_mode_t hal_gpio_get_mode(hal_gpio_pin_t pin) {
  return (hal_gpio_mode_t)((GPIO_GET_PORT(pin)->MODER >>
                            (GPIO_GET_PIN(pin) * 2)) &
                           0x3U);
}

hal_status_t hal_gpio_set_alternate_function(hal_gpio_pin_t pin,
                                             hal_gpio_af_t af) {
  hal_gpio_set_mode(pin, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);

  uint8_t pin_num = GPIO_GET_PIN(pin);
  uint32_t mask = 0xFU << (4 * (pin_num % 8));
  if (pin_num < 8) {
    GPIO_GET_PORT(pin)->AFRL &= ~mask;
    GPIO_GET_PORT(pin)->AFRL |= ((uint32_t)af << (4 * pin_num));
  } else {
    GPIO_GET_PORT(pin)->AFRH &= ~mask;
    GPIO_GET_PORT(pin)->AFRH |= ((uint32_t)af << (4 * (pin_num % 8)));
  }
  return HAL_OK;
}

hal_status_t hal_gpio_set_output_type(hal_gpio_pin_t pin,
                                      hal_gpio_output_type_t type) {
  GPIO_GET_PORT(pin)->OTYPER &= ~(0x1U << GPIO_GET_PIN(pin));
  GPIO_GET_PORT(pin)->OTYPER |= (((uint32_t)type & 0x1U) << GPIO_GET_PIN(pin));
  return HAL_OK;
}

hal_status_t hal_gpio_set_output_speed(hal_gpio_pin_t pin,
                                       hal_gpio_output_speed_t speed) {
  /* OSPEEDR is 2 bits per pin (M4 RM0383 §8.4.3); the shift must scale. */
  uint32_t shift = GPIO_GET_PIN(pin) * 2;
  GPIO_GET_PORT(pin)->OSPEEDR &= ~(0x3U << shift);
  GPIO_GET_PORT(pin)->OSPEEDR |= (((uint32_t)speed & 0x3U) << shift);
  return HAL_OK;
}

hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;

  hal_gpio_set_mode(pin, cfg->mode, cfg->pull);

  if (cfg->mode == HAL_GPIO_MODE_OUTPUT || cfg->mode == HAL_GPIO_MODE_AF) {
    hal_gpio_set_output_type(pin, cfg->output_type);
    hal_gpio_set_output_speed(pin, cfg->output_speed);
  }
  if (cfg->mode == HAL_GPIO_MODE_AF)
    hal_gpio_set_alternate_function(pin, cfg->alternate);

  return HAL_OK;
}
