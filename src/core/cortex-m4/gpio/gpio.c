#include "core/cortex-m4/gpio.h"

uint8_t _get_port(uint16_t n)
{
    uint8_t port = n / 16;
    if (port < 0 || port > 7 || port == 6 || port == 5)
        return 0; // Wrong port numbers
    if (port == 7)
        return 5; // Correct index in the GPIO_BASE array
    return port;
}
uint8_t _get_pin(uint16_t n)
{
    return n % 16;
}
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode, hal_gpio_pullup_pulldown pupd)
{

    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);
    hal_gpio_enable_rcc(pin);

    volatile uint32_t *moder = GPIO_BASE[port_number] + (GPIO_MODER_OFFSET / sizeof(uint32_t));

    // Clear the 2 bits corresponding to the pin
    *moder &= ~(0x3 << (pin_number * 2));

    // Set the mode
    *moder |= ((mode & 0x3) << (pin_number * 2));

    // Set PUPDR
    volatile uint32_t *pupdr = GPIO_BASE[port_number] + (GPIO_PUPDR_OFFSET / sizeof(uint32_t));
    *pupdr &= ~(0x3 << (pin_number * 2));                // set to none (reset)
    *pupdr |= ((uint8_t)pupd & 0x3) << (pin_number * 2); // set pupd
}

hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);
    volatile uint32_t *moder = GPIO_BASE[port_number] + (GPIO_MODER_OFFSET / sizeof(uint32_t));
    return ((*moder) >> (pin_number * 2)) & 0x3;
}

void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state)
{

    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);

    volatile uint32_t *bsrr = GPIO_BASE[port_number] + (GPIO_BSRR_OFFSET / 4);

    if (state)
        *bsrr |= (1 << pin_number); // Set
    else
        *bsrr |= (1 << (pin_number + 16)); // Reset
}

hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);

    volatile uint32_t *idr = GPIO_BASE[port_number] + (GPIO_IDR_OFFSET / 4);
    return (*idr >> pin_number) & 0x1;
}

void hal_gpio_enable_rcc(hal_gpio_pin pin)
{
    uint8_t port_number = _get_port(pin);

    if (!(RCC_AHB1ENR & (1 << port_number)))
        RCC_AHB1ENR |= (1 << port_number);
}