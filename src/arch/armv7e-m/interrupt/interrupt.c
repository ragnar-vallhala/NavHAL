/**
 * @file interrupt.c
 * @brief Standardized HAL interrupt-controller (NVIC) driver for Cortex-M4.
 *
 * @details
 * Implements the standardized `hal_interrupt_*` API declared in
 * `port/cortex-m4/navhal_port_interrupt.h`: per-IRQ enable/disable, pending control,
 * priority configuration, callback registration/dispatch, and global
 * interrupt masking.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_interrupt.h"
#include "common/hal_status.h"
#include <stdint.h>

#define MAX_IRQ 128
static hal_interrupt_callback_t irq_callbacks[MAX_IRQ] = {0};

hal_status_t hal_interrupt_enable(hal_irq_t irq) {
  if (irq < 0)
    return HAL_ERR_INVALID_ARG; // not an NVIC interrupt

  uint32_t irq_num = (uint32_t)irq;
  NVIC->ISER[irq_num / 32] |= (1U << (irq_num % 32));
  return HAL_OK;
}

hal_status_t hal_interrupt_disable(hal_irq_t irq) {
  if (irq < 0)
    return HAL_ERR_INVALID_ARG; // not an NVIC interrupt

  uint32_t irq_num = (uint32_t)irq;
  NVIC->ICER[irq_num / 32] |= (1U << (irq_num % 32));
  return HAL_OK;
}

hal_status_t hal_interrupt_clear_pending(hal_irq_t irq) {
  if (irq < 0)
    return HAL_ERR_INVALID_ARG; // not an NVIC interrupt

  uint32_t irq_num = (uint32_t)irq;
  NVIC->ICPR[irq_num / 32] = (1U << (irq_num % 32));
  return HAL_OK;
}

bool hal_interrupt_is_pending(hal_irq_t irq) {
  if (irq < 0)
    return false;

  uint32_t irq_num = (uint32_t)irq;
  return ((NVIC->ISPR[irq_num / 32] >> (irq_num % 32)) & 1U) != 0U;
}

hal_status_t hal_interrupt_attach_callback(hal_irq_t irq,
                                           hal_interrupt_callback_t callback) {
  if (irq < 0 || irq >= MAX_IRQ)
    return HAL_ERR_INVALID_ARG;
  irq_callbacks[(uint32_t)irq] = callback;
  return HAL_OK;
}

hal_status_t hal_interrupt_detach_callback(hal_irq_t irq) {
  if (irq < 0 || irq >= MAX_IRQ)
    return HAL_ERR_INVALID_ARG;
  irq_callbacks[(uint32_t)irq] = 0;
  return HAL_OK;
}

void hal_interrupt_dispatch(hal_irq_t irq) {
  if (irq < 0 || irq >= MAX_IRQ)
    return;
  if (irq_callbacks[(uint32_t)irq])
    irq_callbacks[(uint32_t)irq]();
}

#define SCB_SHPR1                                                              \
  (*(volatile uint32_t *)0xE000ED18UL) // MemManage, BusFault, UsageFault
#define SCB_SHPR2 (*(volatile uint32_t *)0xE000ED1CUL) // SVCall
#define SCB_SHPR3 (*(volatile uint32_t *)0xE000ED20UL) // PendSV, SysTick
#define __NVIC_PRIO_BITS 4
#define PRIORITY_MASK ((1UL << __NVIC_PRIO_BITS) - 1)

hal_status_t hal_interrupt_set_priority(hal_irq_t irq, uint8_t priority) {
  // Normalize to top 4 bits (0-15 effective priority levels)
  uint32_t prio = (priority & PRIORITY_MASK) << (8 - __NVIC_PRIO_BITS);

  if (irq >= 0) {
    // External interrupts
    NVIC->IPR[(uint32_t)irq] = prio;
  } else {
    // System exceptions (negative IRQn)
    switch (irq) {
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
  return HAL_OK;
}

uint8_t hal_interrupt_get_priority(hal_irq_t irq) {
  if (irq >= 0) {
    // External interrupts
    return NVIC->IPR[(uint32_t)irq] >> 4; // only upper 4 bits are valid
  } else {
    // System exceptions
    uint32_t value = 0;
    switch (irq) {
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

void hal_interrupt_enable_global(uint32_t state) {
  __asm volatile("msr primask, %0" : : "r"(state) : "memory");
}

uint32_t hal_interrupt_disable_global(void) {
  uint32_t state;
  __asm volatile("mrs %0, primask" : "=r"(state));
  __asm volatile("cpsid i" : : : "memory");
  return state;
}

void hal_interrupt_clear_all_pending(void) {
  /* STM32F401RE wires IRQs 0..81 (NVIC ICPR words 0..2). Writing past
   * that range is a no-op on real silicon but produces "unhandled
   * write" warnings in Renode's NVIC model. Keep the loop tight to the
   * chip's actual IRQ count. */
  for (int i = 0; i < 3; i++) {
    NVIC->ICPR[i] = 0xFFFFFFFF;
  }
}

#ifndef SUBMODULE
__attribute__((weak)) void PendSV_Handler(void) {}
__attribute__((weak)) void HardFault_Handler(void) {}
__attribute__((weak)) void SVCall_Handler(void) {}
__attribute__((weak)) void Default_Handler(void) {}
__attribute__((weak)) void DMA1_Stream6_IRQHandler(void) {}

#else
void Default_Handler(void) {}
__attribute__((weak)) void DMA1_Stream6_IRQHandler(void) {}
#endif

void USART2_IRQHandler(void) { hal_interrupt_dispatch(USART2_IRQn); }
