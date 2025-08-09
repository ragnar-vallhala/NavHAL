#include <stdint.h>

#define PERIPH_BASE 0x40000000
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000)
#define APB1PERIPH_BASE (PERIPH_BASE + 0x00000000)

#define RCC_BASE (AHB1PERIPH_BASE + 0x3800)
#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000)
#define TIM2_BASE (APB1PERIPH_BASE + 0x0000)

#define RCC_AHB1ENR (*(volatile unsigned int *)(RCC_BASE + 0x30))
#define RCC_APB1ENR (*(volatile unsigned int *)(RCC_BASE + 0x40))

#define GPIOA_MODER (*(volatile unsigned int *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL (*(volatile unsigned int *)(GPIOA_BASE + 0x20))

#define TIM2_PSC (*(volatile unsigned int *)(TIM2_BASE + 0x28))
#define TIM2_ARR (*(volatile unsigned int *)(TIM2_BASE + 0x2C))
#define TIM2_CCR1 (*(volatile unsigned int *)(TIM2_BASE + 0x34))
#define TIM2_CCMR1 (*(volatile unsigned int *)(TIM2_BASE + 0x18))
#define TIM2_CCER (*(volatile unsigned int *)(TIM2_BASE + 0x20))
#define TIM2_CR1 (*(volatile unsigned int *)(TIM2_BASE + 0x00))

void pwm_pa5_init(uint32_t frequency, float duty_cycle_percent) {
  // 1. Enable clocks
  RCC_AHB1ENR |= (1 << 0); // GPIOA clock enable
  RCC_APB1ENR |= (1 << 0); // TIM2 clock enable

  // 2. Set PA5 to AF mode
  GPIOA_MODER &= ~(0b11 << (5 * 2)); // Clear mode bits
  GPIOA_MODER |= (0b10 << (5 * 2));  // Set alternate function mode

  // 3. Set AF1 (TIM2_CH1) on PA5
  GPIOA_AFRL &= ~(0xF << (5 * 4));
  GPIOA_AFRL |= (0x1 << (5 * 4)); // AF1 for TIM2_CH1

  // 4. Set timer prescaler and ARR for desired frequency
  //    Timer clock = 16 MHz / (PSC + 1), then count to ARR for 1 PWM period

  uint32_t timer_clk = 16000000;
  uint32_t psc = 15;             // (16 MHz / 1 MHz) - 1
  uint32_t pwm_freq = frequency; // e.g., 1 kHz
  uint32_t arr = (timer_clk / (psc + 1)) / pwm_freq - 1;
  uint32_t ccr = (arr + 1) * (duty_cycle_percent / 100.0f);

  TIM2_PSC = psc;
  TIM2_ARR = arr;
  TIM2_CCR1 = ccr;

  // 5. Set PWM mode 1 on CH1 and enable preload
  TIM2_CCMR1 &= ~(0b111 << 4);
  TIM2_CCMR1 |= (0b110 << 4); // PWM mode 1
  TIM2_CCMR1 |= (1 << 3);     // Enable preload

  // 6. Enable output on CH1
  TIM2_CCER |= (1 << 0); // Enable CH1 output

  // 7. Enable auto-reload preload and start counter
  TIM2_CR1 |= (1 << 7); // ARPE
  TIM2_CR1 |= (1 << 0); // CEN
}
void delay() {
  for (volatile int i = 0; i < 10000; i++)
    __asm__("nop");
}
int main(void) {
  while (1) {
    // loop forever
    for (int i = 0; i < 100; i++) {
      pwm_pa5_init(1000, i); // 1 kHz, 50% duty cycle
      delay();
    }
  }
}
