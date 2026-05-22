/**
 * @file main.c
 * @brief Example application: Generate PWM on PB10 using TIM2 and print duty cycle over HAL_UART_2.
 *
 * @details
 * - Initializes SysTick timer with 40 µs tick.
 * - Initializes HAL_UART_2 at 9600 baud.
 * - Configures PB10 as alternate function (AF1) for TIM2_CH3.
 * - Initializes TIM2 for PWM output using HAL PWM driver.
 * - Continuously ramps the duty cycle from 0% to 100% and prints it over HAL_UART_2.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void)
{
    hal_timebase_init(40);                                /**< Initialize SysTick with 40 µs tick */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});                                /**< Initialize HAL_UART_2 at 9600 baud */
    hal_gpio_set_mode(GPIO_PB10, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE); /**< Set PB10 as alternate function */
    hal_gpio_set_alternate_function(GPIO_PB10, HAL_GPIO_AF1); /**< AF1 for TIM2_CH3 */

    hal_pwm_handle_t pwm = {TIM2, 3};                      /**< Initialize PWM handle on TIM2, channel 3 */
    hal_pwm_init(&pwm, 15000, 0.10f);                /**< Init PWM at 15 kHz, 10% duty cycle */
    hal_pwm_start(&pwm);                             /**< Start PWM output */

    float value = 0.0f;                              /**< Variable for duty cycle update */

    while (1)
    {
        hal_pwm_set_duty_cycle(&pwm, value);         /**< Update PWM duty cycle */
        value += 0.01f;                              /**< Increment duty cycle by 1% */
        if (value >= 1.0f)                           /**< Reset to 0% after 100% */
            value = 0.0f;

        hal_uart_print(HAL_UART_2, value);                           /**< Print current duty cycle over HAL_UART_2 */
        hal_uart_print(HAL_UART_2, "\n\r");                          /**< Newline and carriage return */
        hal_delay_ms(10);                                 /**< Delay 10 ms */
    }
}
