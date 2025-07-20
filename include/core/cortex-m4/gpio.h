#ifndef CORTEX_M4_GPIO_H
#define CORTEX_M4_GPIO_H
#include "utils/types.h"


// Ref: stm32f401re_reference.pdf pg no. 118
// nth bit is for the port A...E and H
#define RCC_AHB1ENR    (*(volatile uint32_t*)0x40023830)

// Base address of port registers
// ref: stm32f401re.pdf pg no. 52
#define GPIOA 0x40020000
#define GPIOB 0x40020400
#define GPIOC 0x40020800
#define GPIOD 0x40020C00
#define GPIOE 0x40021000
#define GPIOH 0x40021C00

#define GPIO_PORT_COUNT 6  // A to E and H
static volatile uint32_t* const GPIO_BASE[GPIO_PORT_COUNT] = {
    (uint32_t*)GPIOA,
    (uint32_t*)GPIOB,
    (uint32_t*)GPIOC,
    (uint32_t*)GPIOD,
    (uint32_t*)GPIOE,
    (uint32_t*)GPIOH
};


// Mode Register Offset
#define GPIO_MODER_OFFSET 0x00
// Output Type Register offset
#define GPIO_OTYPER_OFFSET 0x04
// Output Data Register offset
#define GPIO_ODR_OFFSET 0x14

void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode);
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state);

#endif // CORTEX_M4_GPIO_H