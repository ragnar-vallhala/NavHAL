#ifndef CORTEX_M4_TIMER_REG_H
#define CORTEX_M4_TIMER_REG_H

#include "common/hal_types.h"
#include <stdint.h>

typedef struct {
  __IO uint32_t CR1; /*!< 0x00: Control register 1 (All timers) */
  __IO uint32_t CR2; /*!< 0x04: Control register 2 (All timers; basic timers use
                        only some bits) */
  __IO uint32_t SMCR;  /*!< 0x08: Slave mode control register (All except TIM6,
                          TIM7 — reserved) */
  __IO uint32_t DIER;  /*!< 0x0C: DMA/Interrupt enable register (All timers) */
  __IO uint32_t SR;    /*!< 0x10: Status register (All timers) */
  __IO uint32_t EGR;   /*!< 0x14: Event generation register (All timers) */
  __IO uint32_t CCMR1; /*!< 0x18: Capture/Compare mode register 1 (All except
                          TIM6, TIM7 — reserved) */
  __IO uint32_t CCMR2; /*!< 0x1C: Capture/Compare mode register 2 (All except
                          TIM6, TIM7 — reserved) */
  __IO uint32_t CCER;  /*!< 0x20: Capture/Compare enable register (All except
                          TIM6, TIM7 — reserved) */
  __IO uint32_t CNT;   /*!< 0x24: Counter (All timers; width depends on timer:
                          TIM2/TIM5=32-bit, others=16-bit) */
  __IO uint32_t PSC;   /*!< 0x28: Prescaler (All timers) */
  __IO uint32_t ARR;  /*!< 0x2C: Auto-reload register (All timers; width depends
                         on timer) */
  __IO uint32_t RCR;  /*!< 0x30: Repetition counter register (Only TIM1, TIM8,
                         TIM9–TIM11; reserved otherwise) */
  __IO uint32_t CCR1; /*!< 0x34: Capture/Compare register 1 (All except TIM6,
                         TIM7 — reserved) */
  __IO uint32_t CCR2; /*!< 0x38: Capture/Compare register 2 (All except TIM6,
                         TIM7 — reserved) */
  __IO uint32_t CCR3; /*!< 0x3C: Capture/Compare register 3 (Only TIM1–TIM5,
                         TIM8; reserved otherwise) */
  __IO uint32_t CCR4; /*!< 0x40: Capture/Compare register 4 (Only TIM1–TIM5,
                         TIM8; reserved otherwise) */
  __IO uint32_t BDTR; /*!< 0x44: Break and dead-time register (Only TIM1, TIM8 —
                         reserved otherwise) */
  __IO uint32_t
      DCR; /*!< 0x48: DMA control register (All timers except TIM6, TIM7) */
  __IO uint32_t DMAR; /*!< 0x4C: DMA address for full transfer (All timers
                         except TIM6, TIM7) */
  __IO uint32_t OR;   /*!< 0x50: Option register (Only TIM2, TIM5, TIM9–TIM11;
                         reserved otherwise) */
} TIMx_Reg_Typedef;

#define TIM1_BASE_ADDR 0x40010000
#define GPTIMx_BASE_ADDR 0x40000000
#define TIM9_BASE_ADDR 0x40014000

static inline TIMx_Reg_Typedef *GET_TIMx_BASE(uint8_t n) {
  switch (n) {
  case 1:
    return (TIMx_Reg_Typedef *)TIM1_BASE_ADDR;
  case 2:
    return (TIMx_Reg_Typedef *)(GPTIMx_BASE_ADDR + 0x000);
  case 3:
    return (TIMx_Reg_Typedef *)(GPTIMx_BASE_ADDR + 0x400);
  case 4:
    return (TIMx_Reg_Typedef *)(GPTIMx_BASE_ADDR + 0x800);
  case 5:
    return (TIMx_Reg_Typedef *)(GPTIMx_BASE_ADDR + 0xC00);
  case 9:
    return (TIMx_Reg_Typedef *)TIM9_BASE_ADDR;
  case 10:
    return (TIMx_Reg_Typedef *)(TIM9_BASE_ADDR + 0x400);
  case 11:
    return (TIMx_Reg_Typedef *)(TIM9_BASE_ADDR + 0x800);
  default:
    return NULL; // unsupported timer
  }
}

// CR1
#define TIMx_CR1_CEN 0x1

// PSC
#define TIMx_PSC_MASK 0xFFFF

// ARR
#define TIMx_ARR_MASK 0xFFFF
#define TIM2_5_ARR_MASK 0xFFFFFFFF

// EGR
#define TIMx_EGR_UG 0x1

// CNT
#define TIMx_CNT_MASK 0xFFFF
#define TIM2_5_CNT_MASK 0xFFFFFFFF

// DIER
#define TIMx_DIER_UIE 0x1

// SR
#define TIMx_SR_UIF 0x1

// CCR
#define TIMx_CCRy_MASK 0xFFFF
#define TIM2_5_CCRy_MASK 0xFFFFFFFF

// CCER
#define TIMx_CCER_CCxE_MASK(n) (0x1 << (((n % 4) - 1) * 4))

// CCMRx
// Use channel 1,2 with CCMR1 and 3,4 with CCMR2
#define TIMx_CCMRy_OCzPE_Pos(ch) ((ch % 2 == 1) ? 0x3 : 0x11)
#define TIMx_CCMRy_OCzM_Pos(ch) ((ch % 2 == 1) ? 0x4 : 0x12)

#define TIMx_CCMRy_OCzM_MASK(ch) (0x7 << (TIMx_CCMRy_OCzM_Pos(ch)))
#define TIMx_CCMRy_OCzM_PWM_MODE1_MASK(ch)                                     \
  (0x6 << TIMx_CCMRy_OCzM_Pos(ch)) // In upcounting, channel 1 is active as
                                   // long as TIMx_CNT<TIMx_CCR1 else
// inactive. In downcounting, channel 1 is inactive (OC1REF=‘0) as long as
// TIMx_CNT>TIMx_CCR1 else active (OC1REF=1)

#define TIMx_CCMRy_OCxM_PWM_MODE2_MASK(ch)                                     \
  (0x7 << TIMx_CCMRy_OCzM_Pos(ch)) // In upcounting, channel 1 is inactive as
                                   // long as TIMx_CNT<TIMx_CCR1
// else active. In downcounting, channel 1 is active as long as
// TIMx_CNT>TIMx_CCR1 else inactive.
#define TIMx_CCMRy_OCxPE(ch) (0x1 << TIMx_CCMRy_OCzPE_Pos(ch))

#endif // !CORTEX_M4_TIMER_REG_H
