#include "core/cortex-m4/gpio.h"

#define GET_PORT(x) (x / 16)
#define GET_PIN(x) (x % 16)

#define GPIOA_MODER (*(volatile uint32_t *)0x40020000)
#define GPIOA_ODR (*(volatile uint32_t *)0x40020014)

void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode)
{

    uint8_t port_number = GET_PORT(pin);
    uint8_t pin_number = GET_PIN(pin);

    if (!(RCC_AHB1ENR & (1 << port_number)))
        RCC_AHB1ENR |= (1 << port_number);

    volatile uint32_t *moder = GPIO_BASE[port_number] + (GPIO_MODER_OFFSET / sizeof(uint32_t));
    // Clear the 2 bits corresponding to the pin
    *moder &= ~(0x3 << (pin_number * 2));

    // Set the mode
    *moder |= ((mode & 0x3) << (pin_number * 2));

}
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state)
{

    uint8_t port_number = GET_PORT(pin);
    uint8_t pin_number = GET_PIN(pin);

    volatile uint32_t *odr = GPIO_BASE[port_number] + (GPIO_ODR_OFFSET / 4);

    if (state)
        *odr |= (1 << pin_number);
    else
        *odr &= ~(1 << pin_number);
}
