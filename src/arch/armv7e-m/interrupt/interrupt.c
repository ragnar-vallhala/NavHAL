/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file interrupt.c
 * @brief Standardized HAL interrupt-controller (NVIC) driver for Cortex-M4.
 *
 * @details
 * Implements the standardized `hal_interrupt_*` API declared in
 * `port/cortex-m4/navhal_port_interrupt.h`: per-IRQ enable/disable, pending control,
 * priority configuration, callback registration/dispatch, and global
 * interrupt masking.
 */

#include "common/hal_interrupt.h"
#include "navhal_port_interrupt.h"
#include "internal/hal_interrupt_ops.h"
#include "internal/hal_interrupt_table.h"

/* Forward declaration: enable_with_priority (a Cortex-M port extension, not a
 * table entry) sets the priority before unmasking, and is defined above it. */
static hal_status_t armv7em_interrupt_set_priority(hal_irq_t irq,
                                                   uint8_t priority);
#include "common/hal_status.h"
#include <stdint.h>

/* The callback table moved to the common layer: it was a bounds check and an
 * indexed call, identical in every port. HAL_IRQ_COUNT comes from the family
 * header, so the array is the size this part actually wires -- 82 lines on the
 * F401, not a round 128. */

/* Enabling a line always sets its priority first, so it can never fire at the
 * NVIC reset default of 0 -- most urgent, and unmaskable by an RTOS BASEPRI
 * critical section, which would let a *_from_isr call from that line corrupt
 * the kernel. See HAL_IRQ_PRIORITY_DEFAULT.
 *
 * The public hal_interrupt_enable_with_priority is the common layer's, derived
 * from set_priority and enable. It used to be defined here and called by
 * .enable below, which under that layering would have called back into .enable
 * for ever. */
static hal_status_t _enable_at(hal_irq_t irq, uint8_t priority) {
  if (irq < 0)
    return HAL_ERR_INVALID_ARG; // not an NVIC interrupt

  armv7em_interrupt_set_priority(irq, priority);

  uint32_t irq_num = (uint32_t)irq;
  NVIC->ISER[irq_num / 32] |= (1U << (irq_num % 32));
  return HAL_OK;
}

static hal_status_t armv7em_interrupt_enable(hal_irq_t irq) {
  return _enable_at(irq, HAL_IRQ_PRIORITY_DEFAULT);
}

static hal_status_t armv7em_interrupt_disable(hal_irq_t irq) {
  if (irq < 0)
    return HAL_ERR_INVALID_ARG; // not an NVIC interrupt

  uint32_t irq_num = (uint32_t)irq;
  NVIC->ICER[irq_num / 32] |= (1U << (irq_num % 32));
  return HAL_OK;
}

static hal_status_t armv7em_interrupt_clear_pending(hal_irq_t irq) {
  if (irq < 0)
    return HAL_ERR_INVALID_ARG; // not an NVIC interrupt

  uint32_t irq_num = (uint32_t)irq;
  NVIC->ICPR[irq_num / 32] = (1U << (irq_num % 32));
  return HAL_OK;
}

static bool armv7em_interrupt_is_pending(hal_irq_t irq) {
  if (irq < 0)
    return false;

  uint32_t irq_num = (uint32_t)irq;
  return ((NVIC->ISPR[irq_num / 32] >> (irq_num % 32)) & 1U) != 0U;
}

static void armv7em_interrupt_dispatch(hal_irq_t irq) {
  /* An NVIC needs no acknowledgement: taking the exception clears the pending
   * bit, so there is nothing to wrap the call in. */
  (void)navhal_irq_invoke(irq);
}

// Generic vector fallback. The startup vector table routes EVERY IRQ slot that
// has no dedicated <PERIPH>_IRQHandler here (instead of the old silent
// infinite-loop trap). We read the active exception number from IPSR and
// dispatch the registered callback for that IRQ — so any peripheral whose
// driver attaches a callback and enables its line works even without a
// hand-written vector entry. This closes the "driver enables an IRQ but nobody
// wired its handler -> CPU hangs in Default_Handler on the first interrupt"
// class of bug for good (USART1/USART6 were instances of it).
//
// A genuinely unexpected exception — a system fault with no dedicated handler,
// or an enabled IRQ with no registered callback — still traps; the active
// exception number is live in IPSR for a debugger. Runs in handler mode,
// entered via a tail-branch from Default_Handler, so its epilogue performs the
// exception return.
void hal_irq_default_dispatch(void) {
  uint32_t ipsr;
  __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
  uint32_t exc = ipsr & 0x1FFu; // active exception number (0 = thread)
  if (exc >= 16u) {             // external IRQ: exc = 16 + IRQn
    uint32_t irq = exc - 16u;
    if (navhal_irq_invoke((hal_irq_t)irq))
      return;
    /* else fall through to the trap: an enabled line with nothing attached is
     * a bug, and returning would spin the exception forever */
  }
  for (;;) {
    /* unexpected exception — IPSR holds the number */
  }
}

#define SCB_SHPR1                                                              \
  (*(volatile uint32_t *)0xE000ED18UL) // MemManage, BusFault, UsageFault
#define SCB_SHPR2 (*(volatile uint32_t *)0xE000ED1CUL) // SVCall
#define SCB_SHPR3 (*(volatile uint32_t *)0xE000ED20UL) // PendSV, SysTick
#define __NVIC_PRIO_BITS 4
#define PRIORITY_MASK ((1UL << __NVIC_PRIO_BITS) - 1)

static hal_status_t armv7em_interrupt_set_priority(hal_irq_t irq, uint8_t priority) {
  // A LEVEL, 0..15, not a raw register value. Masking instead of refusing turns
  // 0xE0 -- a caller meaning level 14 -- into level 0, the most urgent and the
  // one that cannot be masked by a BASEPRI critical section. Silently.
  if (priority > PRIORITY_MASK)
    return HAL_ERR_INVALID_ARG;
  uint32_t prio = priority << (8 - __NVIC_PRIO_BITS);

  if (irq >= 0) {
    // External interrupts. Bounded: IPR has one byte per wired line, and
    // writing past it lands in whatever register follows.
    if ((unsigned long)irq >= (unsigned long)HAL_IRQ_COUNT)
      return HAL_ERR_INVALID_ARG;
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
    case NonMaskableInt_IRQn:
    case HardFault_IRQn:
    case DebugMonitor_IRQn:
      // Fixed or highest priority in the architecture; nothing to write.
      return HAL_ERR_NOT_SUPPORTED;
    default:
      // Not a system exception this core has -- reporting HAL_OK for it would
      // be claiming a priority was set on a line that does not exist.
      return HAL_ERR_INVALID_ARG;
    }
  }
  return HAL_OK;
}

static uint8_t armv7em_interrupt_get_priority(hal_irq_t irq) {
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

static void armv7em_interrupt_enable_global(uint32_t state) {
  __asm volatile("msr primask, %0" : : "r"(state) : "memory");
}

static uint32_t armv7em_interrupt_disable_global(void) {
  uint32_t state;
  __asm volatile("mrs %0, primask" : "=r"(state));
  __asm volatile("cpsid i" : : : "memory");
  return state;
}

static void armv7em_cpu_idle(void) {
  /* DSB before WFI so prior memory writes (e.g. clearing a wake flag) retire
   * first; WFI sleeps the core until a wakeup event. A pending wakeup event at
   * entry returns immediately, so this cannot deadlock. */
  __asm volatile("dsb 0xf" : : : "memory");
  __asm volatile("wfi" : : : "memory");
}

static void armv7em_interrupt_clear_all_pending(void) {
  /* STM32F401RE wires IRQs 0..81 (NVIC ICPR words 0..2). Writing past
   * that range is a no-op on real silicon but produces "unhandled
   * write" warnings in Renode's NVIC model. Keep the loop tight to the
   * chip's actual IRQ count. */
  for (int i = 0; i < 3; i++) {
    NVIC->ICPR[i] = 0xFFFFFFFF;
  }
}

/* System (internal) exceptions get their own named weak vectors — override any
 * with a strong definition. No Default_Handler routing for these.
 *
 * Unconditional, and weak, which is the whole mechanism: an embedding RTOS that
 * owns PendSV/SVCall/SysTick simply defines them strongly and its versions win.
 * These were once wrapped in `#ifndef SUBMODULE`, which did the same job twice
 * and broke the build the second time: the startup vector table still names all
 * nine, so -DSUBMODULE (what nav passes when a dependency owns the vectors) left
 * the link with nine undefined references to its own vectors.
 *
 * Default_Handler is deliberately NOT here. The startup file defines it, weakly,
 * as a tail-branch into hal_irq_default_dispatch -- the IPSR-based fallback that
 * dispatches any IRQ with an attached callback. A second definition in this file
 * either raced it (two weak definitions: whichever the link order reached first
 * won) or, under -DSUBMODULE where this one was strong, silently replaced the
 * fallback with an empty stub. Attach and enable would still report success and
 * no interrupt would ever reach its callback. */
__attribute__((weak)) void PendSV_Handler(void) {}
__attribute__((weak)) void HardFault_Handler(void) {}
__attribute__((weak)) void SVCall_Handler(void) {}
__attribute__((weak)) void NMI_Handler(void) {}
__attribute__((weak)) void MemManage_Handler(void) {}
__attribute__((weak)) void BusFault_Handler(void) {}
__attribute__((weak)) void UsageFault_Handler(void) {}
__attribute__((weak)) void DebugMon_Handler(void) {}

/* USART vectors are this MCU's, not the core's, so they are defined by the
 * vendor's UART backend rather than here. */

/** @brief The ARMv7E-M NVIC interrupt backend. */
const hal_interrupt_ops_t _hal_interrupt_ops = {
    .enable = armv7em_interrupt_enable,
    .disable = armv7em_interrupt_disable,
    .dispatch = armv7em_interrupt_dispatch,
    .cpu_idle = armv7em_cpu_idle,
    .disable_global = armv7em_interrupt_disable_global,
    .enable_global = armv7em_interrupt_enable_global,
    .set_priority = armv7em_interrupt_set_priority,
    .get_priority = armv7em_interrupt_get_priority,
    .is_pending = armv7em_interrupt_is_pending,
    .clear_pending = armv7em_interrupt_clear_pending,
    .clear_all_pending = armv7em_interrupt_clear_all_pending,
};
