#include <stdint.h>
#include <stdio.h>
#define CORTEX_M4
#include "navhal.h"

// Peripheral base addresses
#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE (PERIPH_BASE + 0x00000000UL)

#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000)
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800)
#define USART2_BASE (APB1PERIPH_BASE + 0x4400)

// Register definitions
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

#define USART2_SR (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1 (*(volatile uint32_t *)(USART2_BASE + 0x0C))

// Bit definitions
#define RCC_AHB1ENR_GPIOAEN (1 << 0)
#define RCC_APB1ENR_USART2EN (1 << 17)

#define USART_CR1_UE (1 << 13)
#define USART_CR1_TE (1 << 3)
#define USART_SR_TXE (1 << 7)

// Delay function
void delay(volatile uint32_t count)
{
    while (count--)
        __asm__("nop");
}

void uart2_init(void)
{
    // 1. Enable clocks
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Enable GPIOA clock
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN; // Enable USART2 clock

    // 2. Set PA2 to alternate function (AF7 for USART2 TX)
    GPIOA_MODER &= ~(0x3 << (2 * 2)); // Clear mode bits for PA2
    GPIOA_MODER |= (0x2 << (2 * 2));  // Set alternate function mode
    GPIOA_AFRL &= ~(0xF << (4 * 2));  // Clear AFRL bits for PA2
    GPIOA_AFRL |= (0x7 << (4 * 2));   // Set AF7 (USART2)
    
    int baudrate = 115200;
    USART2_BRR = (hal_clock_get_apb1clk() + (baudrate / 2)) / baudrate;
    // 3. Configure USART2
    // USART2_BRR = 0x0683;        // 9600 baud @ 16 MHz (see earlier calculation)
    USART2_CR1 = USART_CR1_TE;  // Enable transmitter
    USART2_CR1 |= USART_CR1_UE; // Enable USART
}

void uart2_write_char(char c)
{
    while (!(USART2_SR & USART_SR_TXE))
        ;          // Wait until TXE = 1
    USART2_DR = c; // Write data
}

void uart2_write_int(int num)
{
    char buf[12]; // Enough for 32-bit int
    int i = 0;

    if (num == 0)
    {
        uart2_write_char('0');
        return;
    }

    if (num < 0)
    {
        uart2_write_char('-');
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i--)
    {
        uart2_write_char(buf[i]);
    }
}

void uart2_write_string(const char *s)
{
    while (*s)
    {
        uart2_write_char(*s++);
    }
}

hal_clock_config_t clk_cfg = {
    HAL_CLOCK_SOURCE_PLL};

hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSI,
    .pll_m = 16,  // 16 MHz / 16 = 1 MHz
    .pll_n = 336, // 1 MHz * 336 = 336 MHz VCO
    .pll_p = 4,   // 336 / 4 = 84 MHz SYSCLK
    .pll_q = 7    // 336 / 7 ≈ 48 MHz USB
};
int main(void)
{

    hal_clock_init(&clk_cfg, &pll_cfg);

    uart2_init();
    hal_gpio_setmode(GPIO_PA06, GPIO_INPUT, GPIO_PULLUP);
    hal_gpio_setmode(GPIO_PA05, GPIO_OUTPUT, 0);

    while (1)
    {
        uart2_write_string("SYSCLK: ");
        uart2_write_int(hal_clock_get_sysclk());
        uart2_write_string(", AHBCLK: ");
        uart2_write_int(hal_clock_get_ahbclk());
        uart2_write_string(", ABP1CLK: ");
        uart2_write_int(hal_clock_get_apb1clk());
        uart2_write_string(", ABP2CLK: ");
        uart2_write_int(hal_clock_get_apb2clk());
        uart2_write_string("\n");

        // if (hal_gpio_digitalread(GPIO_PA06))
        // {
        //     hal_gpio_digitalwrite(GPIO_PA05, GPIO_HIGH);
        //     uart2_write_string("PA06 is HIGH\n");
        // }
        // else
        // {
        //     hal_gpio_digitalwrite(GPIO_PA05, GPIO_LOW);
        //     uart2_write_string("PA06 is LOW\n");
        // }
        delay(100000);
    }
}
