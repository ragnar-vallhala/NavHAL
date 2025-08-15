/**
 * @file main.c
 * @brief Example application: Generate PWM on PB10 using TIM2 and print duty cycle over UART2.
 *
 * @details
 * - Initializes SysTick timer with 40 µs tick.
 * - Initializes UART2 at 9600 baud.
 * - Configures PB10 as alternate function (AF1) for TIM2_CH3.
 * - Initializes TIM2 for PWM output using HAL PWM driver.
 * - Continuously ramps the duty cycle from 0% to 100% and prints it over UART2.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void)
{
    systick_init(40);                                /**< Initialize SysTick with 40 µs tick */
    uart2_init(9600);                                /**< Initialize UART2 at 9600 baud */
    hal_gpio_setmode(GPIO_PB10, GPIO_AF, GPIO_PUPD_NONE); /**< Set PB10 as alternate function */
    hal_gpio_set_alternate_function(GPIO_PB10, GPIO_AF01); /**< AF1 for TIM2_CH3 */

    PWM_Handle pwm = {TIM2, 3};                      /**< Initialize PWM handle on TIM2, channel 3 */
    hal_pwm_init(&pwm, 15000, 0.10f);                /**< Init PWM at 15 kHz, 10% duty cycle */
    hal_pwm_start(&pwm);                             /**< Start PWM output */

    float value = 0.0f;                              /**< Variable for duty cycle update */

    while (1)
    {
        hal_pwm_set_duty_cycle(&pwm, value);         /**< Update PWM duty cycle */
        value += 0.01f;                              /**< Increment duty cycle by 1% */
        if (value >= 1.0f)                           /**< Reset to 0% after 100% */
            value = 0.0f;

        uart2_write(value);                           /**< Print current duty cycle over UART2 */
        uart2_write("\n\r");                          /**< Newline and carriage return */
        delay_ms(10);                                 /**< Delay 10 ms */
    }
}
