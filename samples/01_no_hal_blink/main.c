#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)
#define GPIOA_MODER    (*(volatile unsigned int*)0x40020000)
#define GPIOA_ODR      (*(volatile unsigned int*)0x40020014)

void delay(void) {
    for (volatile int i = 0; i < 500000; i++);
}

int main(void) {
    // 1. Enable GPIOA clock (bit 0)
    RCC_AHB1ENR |= (1 << 0);

    // 2. Set PA5 as output (MODER5 = 01)
    GPIOA_MODER &= ~(0x3 << (5 * 2));  // Clear mode bits for pin 5
    GPIOA_MODER |=  (0x1 << (5 * 2));  // Set mode to general purpose output

    while (1) {
        GPIOA_ODR ^= (1 << 5);  // Toggle PA5
        delay();
    }
}
