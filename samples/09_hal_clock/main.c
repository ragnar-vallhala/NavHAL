
#define CORTEX_M4
#include "navhal.h"

hal_pll_config_t pll_cfg = {.input_src =
                                HAL_CLOCK_SOURCE_HSE, // external crystal 8 MHz
                            .pll_m = 8,
                            .pll_n = 168,
                            .pll_p = 2,
                            .pll_q = 7};

hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL};

int main() {
  hal_clock_init(&cfg, &pll_cfg);
  systick_init(40);
  uart2_init(9600);
 
  while (1) {
   
     uart2_write("sysclk=" );
     uart2_write(hal_clock_get_sysclk());
     uart2_write(", apb1clk=" );
     uart2_write(hal_clock_get_apb1clk());
     uart2_write(", apb2clk=" );
     uart2_write(hal_clock_get_apb2clk());
     uart2_write(", ahbclk=" );
     uart2_write(hal_clock_get_ahbclk());
     uart2_write("\n");
     delay_ms(1000);
     
     

    
  }
}
