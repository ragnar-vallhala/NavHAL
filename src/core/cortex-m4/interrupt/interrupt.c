#include "core/cortex-m4/interrupt.h"
#include "common/hal_status.h"
#include <stdint.h>
#define MAX_IRQ 128
static void (*irq_callbacks[MAX_IRQ])(void) = {0};

int8_t hal_enable_interrupt(IRQn_Type interrupt) {
  if (interrupt < 0)
    return FAILURE; // CortexM interrupts

  uint32_t irq_num = (uint32_t)interrupt;
  uint32_t reg_idx = irq_num / 32;
  uint32_t bit_pos = irq_num % 32;

  NVIC->ISER[reg_idx] |= (1 << bit_pos);
  return SUCCESS;
}
int8_t hal_disable_interrupt(IRQn_Type interrupt) {
  if (interrupt < 0)
    return FAILURE; // CortexM interrupts

  uint32_t irq_num = (uint32_t)interrupt;
  uint32_t reg_idx = irq_num / 32;
  uint32_t bit_pos = irq_num % 32;

  NVIC->ICER[reg_idx] |= (1 << bit_pos);
  return SUCCESS;
}

void hal_interrupt_attach_callback(IRQn_Type interrupt,
                                   void (*callback)(void)) {
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  irq_callbacks[(uint32_t)interrupt] = callback;
}

void hal_interrupt_detach_callback(IRQn_Type interrupt) {
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  irq_callbacks[(uint32_t)interrupt] = 0;
}

void hal_handle_interrupt(IRQn_Type interrupt) {
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  if (irq_callbacks[(uint32_t)interrupt])
    irq_callbacks[(uint32_t)interrupt]();
}
