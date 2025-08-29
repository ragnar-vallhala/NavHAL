#include "core/cortex-m4/interrupt.h"
#include "common/hal_status.h"
#include <stdint.h>
#define MAX_IRQ 128
static void (*irq_callbacks[MAX_IRQ])(void) = {0};

int8_t hal_enable_interrupt(IRQn_Type interrupt)
{
  if (interrupt < 0)
    return FAILURE; // CortexM interrupts

  uint32_t irq_num = (uint32_t)interrupt;
  uint32_t reg_idx = irq_num / 32;
  uint32_t bit_pos = irq_num % 32;

  NVIC->ISER[reg_idx] |= (1 << bit_pos);
  return SUCCESS;
}

int8_t hal_disable_interrupt(IRQn_Type interrupt)
{
  if (interrupt < 0)
    return FAILURE; // CortexM interrupts

  uint32_t irq_num = (uint32_t)interrupt;
  uint32_t reg_idx = irq_num / 32;
  uint32_t bit_pos = irq_num % 32;

  NVIC->ICER[reg_idx] |= (1 << bit_pos);
  return SUCCESS;
}

void hal_interrupt_attach_callback(IRQn_Type interrupt,
                                   void (*callback)(void))
{
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  irq_callbacks[(uint32_t)interrupt] = callback;
}

void hal_interrupt_detach_callback(IRQn_Type interrupt)
{
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  irq_callbacks[(uint32_t)interrupt] = 0;
}

void hal_handle_interrupt(IRQn_Type interrupt)
{
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  if (irq_callbacks[(uint32_t)interrupt])
    irq_callbacks[(uint32_t)interrupt]();
}

#define SCB_SHPR1 \
  (*(volatile uint32_t *)0xE000ED18UL)                 // MemManage, BusFault, UsageFault
#define SCB_SHPR2 (*(volatile uint32_t *)0xE000ED1CUL) // SVCall
#define SCB_SHPR3 (*(volatile uint32_t *)0xE000ED20UL) // PendSV, SysTick
#define __NVIC_PRIO_BITS 4
#define PRIORITY_MASK ((1UL << __NVIC_PRIO_BITS) - 1)

void hal_set_interrupt_priority(IRQn_Type interrupt, uint8_t priority)
{
  // Normalize to top 4 bits (0-15 effective priority levels)
  uint32_t prio = (priority & PRIORITY_MASK) << (8 - __NVIC_PRIO_BITS);

  if (interrupt >= 0)
  {
    // External interrupts
    NVIC->IPR[(uint32_t)interrupt] = prio;
  }
  else
  {
    // System exceptions (negative IRQn)
    switch (interrupt)
    {
    case MemoryManagement_IRQn:
      SCB_SHPR1 = (SCB_SHPR1 & ~(0xFFU << 0)) | (prio << 0);
      break;
    case BusFault_IRQn:
      SCB_SHPR1 = (SCB_SHPR1 & ~(0xFFU << 8)) | (prio << 8);
      break;
    case UsageFault_IRQn:
      SCB_SHPR1 = (SCB_SHPR1 & ~(0xFFU << 16)) | (prio << 16);
      break;
    case SVCall_IRQn:
      SCB_SHPR2 = (SCB_SHPR2 & ~(0xFFU << 24)) | (prio << 24);
      break;
    case PendSV_IRQn:
      SCB_SHPR3 = (SCB_SHPR3 & ~(0xFFU << 16)) | (prio << 16);
      break;
    case SysTick_IRQn:
      SCB_SHPR3 = (SCB_SHPR3 & ~(0xFFU << 24)) | (prio << 24);
      break;
    default:
      // HardFault, NMI and DebugMon have fixed or highest priority
      break;
    }
  }
}

uint8_t hal_get_interrupt_priority(IRQn_Type interrupt)
{
  if (interrupt >= 0)
  {
    // External interrupts
    return NVIC->IPR[(uint32_t)interrupt] >> 4; // only upper 4 bits are valid
  }
  else
  {
    // System exceptions
    uint32_t value = 0;
    switch (interrupt)
    {
    case MemoryManagement_IRQn:
      value = (SCB_SHPR1 >> 0) & 0xFF;
      break;
    case BusFault_IRQn:
      value = (SCB_SHPR1 >> 8) & 0xFF;
      break;
    case UsageFault_IRQn:
      value = (SCB_SHPR1 >> 16) & 0xFF;
      break;
    case SVCall_IRQn:
      value = (SCB_SHPR2 >> 24) & 0xFF;
      break;
    case PendSV_IRQn:
      value = (SCB_SHPR3 >> 16) & 0xFF;
      break;
    case SysTick_IRQn:
      value = (SCB_SHPR3 >> 24) & 0xFF;
      break;
    default:
      return 0xFF; // invalid / fixed priority
    }
    return value >> 4; // return normalized 0-15
  }
}
// weak symbols
#ifndef SUBMODULE
__attribute__((weak)) void PendSV_Handler(void) {}
__attribute__((weak)) void HardFault_Handler(void) {}
__attribute__((weak)) void SVCall_Handler(void) {}
#endif
