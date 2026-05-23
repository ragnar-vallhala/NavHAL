/**
 * @file src/vendor/microchip/gpio/gpio.c
 * @brief ATmega328P GPIO HAL driver.
 *
 * @details
 * Implements the portable GPIO API (@c common/hal_gpio.h) for the
 * ATmega328P ports B, C and D — the configuration calls (init, set_mode,
 * …). The hot-path accessors (write / read / toggle) are @c static
 * @c inline in @c navhal_port_gpio.h so a constant pin folds to `sbi`/`cbi`.
 * A ::hal_gpio_pin_t is encoded 8 per port: @c port = pin >> 3
 * (0 = B, 1 = C, 2 = D), @c bit = pin & 7.
 *
 * ATmega328P GPIO has no clock gate, no slew-rate control, no open-drain
 * mode and no alternate-function multiplexer in the STM32 sense, so the
 * corresponding config knobs are accepted as no-ops (or, for alternate
 * function, reported unsupported).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_gpio.h"

#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

/* port index: 0 = B, 1 = C, 2 = D */
static volatile uint8_t *ddr_reg(uint8_t port) {
  switch (port) {
  case 0:  return &DDRB;
  case 1:  return &DDRC;
  default: return &DDRD;
  }
}

static volatile uint8_t *port_out_reg(uint8_t port) {
  switch (port) {
  case 0:  return &PORTB;
  case 1:  return &PORTC;
  default: return &PORTD;
  }
}

hal_status_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                               hal_gpio_pull_t pull) {
  uint8_t p = (uint8_t)pin >> 3;
  uint8_t m = (uint8_t)(1u << ((uint8_t)pin & 7u));
  volatile uint8_t *ddr = ddr_reg(p);
  volatile uint8_t *out = port_out_reg(p);

  if (mode == HAL_GPIO_MODE_OUTPUT) {
    *ddr |= m;
  } else {
    /* INPUT — AF / ANALOG have no ATmega328P meaning, treated as input. */
    *ddr &= (uint8_t)~m;
    /* On the AVR the PORT bit is the input pull-up enable; there is no
     * pull-down, so anything other than PULL_UP leaves the pin floating. */
    if (pull == HAL_GPIO_PULL_UP)
      *out |= m;
    else
      *out &= (uint8_t)~m;
  }
  return HAL_OK;
}

hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  /* output_type, output_speed and alternate have no ATmega328P effect. */
  return hal_gpio_set_mode(pin, cfg->mode, cfg->pull);
}

hal_gpio_mode_t hal_gpio_get_mode(hal_gpio_pin_t pin) {
  uint8_t p = (uint8_t)pin >> 3;
  uint8_t m = (uint8_t)(1u << ((uint8_t)pin & 7u));
  return (*ddr_reg(p) & m) ? HAL_GPIO_MODE_OUTPUT : HAL_GPIO_MODE_INPUT;
}

hal_status_t hal_gpio_enable_clock(hal_gpio_pin_t pin) {
  /* ATmega328P GPIO ports are always clocked — nothing to enable. */
  (void)pin;
  return HAL_OK;
}

hal_status_t hal_gpio_set_alternate_function(hal_gpio_pin_t pin,
                                             hal_gpio_af_t af) {
  /* The ATmega328P has no GPIO alternate-function multiplexer; peripheral
   * pin functions are fixed in silicon. */
  (void)pin;
  (void)af;
  return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_gpio_set_output_type(hal_gpio_pin_t pin,
                                      hal_gpio_output_type_t type) {
  /* ATmega328P outputs are push-pull only; accepted as a no-op. */
  (void)pin;
  (void)type;
  return HAL_OK;
}

hal_status_t hal_gpio_set_output_speed(hal_gpio_pin_t pin,
                                       hal_gpio_output_speed_t speed) {
  /* No slew-rate control on the ATmega328P; accepted as a no-op. */
  (void)pin;
  (void)speed;
  return HAL_OK;
}
