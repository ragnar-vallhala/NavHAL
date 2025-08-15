#include "core/cortex-m4/uart.h"
#define CORTEX_M4
#include "navhal.h"

int main() {
  uart2_init(9600);
  uart2_write("Version: ");
  uart2_write(VERSION);
  uart2_write("\n");
  while (1)
    ;
}
