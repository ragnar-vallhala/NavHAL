#define CORTEX_M4
#include "navhal.h"



int main() {
  systick_init(1000);
  uart2_init(9600); // Initialized uart communication
  timer_init(TIM5,0,1000000000); // Initialized the timer
  while (1) {
    uart2_write("Timer: "); // Printing timer to the screen
    uart2_write(timer_get_count(TIM5)); // Printing the count of value stored in the TIMER 5
    uart2_write("\n\r"); // break the line 
    // delay_ms(10);
  }
}
