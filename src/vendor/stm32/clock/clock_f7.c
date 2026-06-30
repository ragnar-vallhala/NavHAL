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
 * @file clock_f7.c
 * @brief Clock HAL for STM32F7 (Cortex-M7) — HSI/HSE/PLL up to 216 MHz.
 *
 * @details
 * Same `hal_clock_*` contract as the F4 `clock.c`, but `hal_clock_init` adds
 * the three things the F7 needs to run above the F4's ~84 MHz ceiling
 * (RM0410 §3.3, §5.1.4):
 *
 * 1. **Voltage scaling + over-drive.** VOS is set to Scale 1, and for a target
 *    HCLK above 180 MHz the PWR over-drive (`ODEN`→`ODRDY`, `ODSWEN`→`ODSWRDY`)
 *    is engaged before SYSCLK is switched to the PLL.
 * 2. **Frequency-dependent flash wait states** (one per 30 MHz of HCLK, up to 7
 *    at 216 MHz), with the ART accelerator + prefetch enabled.
 * 3. **Bus-limit-aware APB prescalers** — APB1 ≤ 54 MHz, APB2 ≤ 108 MHz — chosen
 *    from the target HCLK instead of the F4's fixed /2.
 *
 * The PLLCFGR layout and the clock getters are register-identical to the F4, so
 * those are carried over unchanged. The vendor CMakeLists selects this file in
 * place of `clock.c` when `CONFIG_FAMILY_STM32F7` is set.
 */

#include "navhal_port_clock.h"
#include "family/flash_reg.h"
#include "family/rcc_reg.h"
#include <stdint.h>

/* PWR peripheral (RM0410 §4.4) — not modelled in the family reg headers, so
 * the few registers the over-drive sequence needs are declared here. */
#define PWR_BASE_ADDR 0x40007000UL
#define PWR_CR1  (*(volatile uint32_t *)(PWR_BASE_ADDR + 0x00))
#define PWR_CSR1 (*(volatile uint32_t *)(PWR_BASE_ADDR + 0x04))
#define PWR_CR1_VOS_Msk    (0x3U << 14)
#define PWR_CR1_VOS_SCALE1 (0x3U << 14) /**< Scale 1 — required for >180 MHz */
#define PWR_CR1_ODEN       (1U << 16)
#define PWR_CR1_ODSWEN     (1U << 17)
#define PWR_CSR1_ODRDY     (1U << 16)
#define PWR_CSR1_ODSWRDY   (1U << 17)
#define RCC_APB1ENR_PWREN  (1U << 28)

/* FLASH_ACR feature bits (base + latency-field position come from flash_reg.h). */
#define FLASH_ACR_LATENCY_Msk (0xFU << FLASH_ACR_LATENCY_BIT)
#define FLASH_ACR_PRFTEN      (1U << 8)
#define FLASH_ACR_ARTEN       (1U << 9)

#define HSI_FREQ_HZ 16000000U
#define HSE_FREQ_HZ 8000000U /**< Nucleo-F767ZI HSE = 8 MHz ST-LINK MCO. */

/* Internal clock-source toggle helpers (file-local). */
static void _toggle_hse_clock(uint8_t state) {
  if (state)
    RCC->CR |= RCC_CR_HSEON;
  else
    RCC->CR &= ~RCC_CR_HSEON;
  while (((RCC->CR & RCC_CR_HSERDY) != 0) != (state))
    ;
}
static void _toggle_hsi_clock(uint8_t state) {
  if (state)
    RCC->CR |= RCC_CR_HSION;
  else
    RCC->CR &= ~RCC_CR_HSION;
  while (((RCC->CR & RCC_CR_HSIRDY) != 0) != (state))
    ;
}
static void _toggle_pll_clock(uint8_t state) {
  if (state)
    RCC->CR |= RCC_CR_PLLON;
  else
    RCC->CR &= ~RCC_CR_PLLON;
  while (((RCC->CR & RCC_CR_PLLRDY) != 0) != (state))
    ;
}

/** @brief Resulting PLL output (= HCLK with AHB /1) for the given config, Hz. */
static uint32_t _pll_output_hz(const hal_pll_config_t *p) {
  if (p->pll_m == 0 || p->pll_p == 0)
    return 0;
  uint32_t in = (p->input_src == HAL_CLOCK_SOURCE_HSE) ? HSE_FREQ_HZ : HSI_FREQ_HZ;
  return (in / p->pll_m) * p->pll_n / p->pll_p;
}

/** @brief Flash wait states for an HCLK at Vdd 2.7–3.6 V (RM0410 Table 6):
 *  one WS per 30 MHz band, clamped to the 0..7 the field allows. */
static uint32_t _flash_ws_for(uint32_t hclk) {
  uint32_t ws = (hclk == 0) ? 0 : (hclk - 1) / 30000000U;
  return (ws > 7) ? 7 : ws;
}

hal_status_t hal_clock_init(const hal_clock_config_t *cfg,
                            const hal_pll_config_t *pll_cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  if (cfg->source == HAL_CLOCK_SOURCE_PLL && pll_cfg == NULL)
    return HAL_ERR_INVALID_ARG;

  /* Power up the PWR controller and select voltage Scale 1 (needed before
   * over-drive / high frequency). */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  (void)RCC->APB1ENR; /* ensure the enable has taken effect */
  PWR_CR1 = (PWR_CR1 & ~PWR_CR1_VOS_Msk) | PWR_CR1_VOS_SCALE1;

  /* Enable and wait for the selected source. */
  if (cfg->source == HAL_CLOCK_SOURCE_HSE) {
    _toggle_hse_clock(RCC_ON);
  } else if (cfg->source == HAL_CLOCK_SOURCE_HSI) {
    _toggle_hsi_clock(RCC_ON);
  } else if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
    if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSE)
      _toggle_hse_clock(RCC_ON);
    else
      _toggle_hsi_clock(RCC_ON);

    _toggle_pll_clock(RCC_OFF);
    RCC->PLLCFGR = 0;
    if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSI)
      RCC->PLLCFGR &= ~RCC_PLLCFGR_SRC;
    else
      RCC->PLLCFGR |= RCC_PLLCFGR_SRC;
    RCC->PLLCFGR |=
        RCC_PLLCFGR_PLLM(pll_cfg->pll_m) | RCC_PLLCFGR_PLLN(pll_cfg->pll_n) |
        RCC_PLLCFGR_PLLP(pll_cfg->pll_p) | RCC_PLLCFGR_PLLQ(pll_cfg->pll_q);
    _toggle_pll_clock(RCC_ON);
  }

  /* Target HCLK (AHB prescaler is /1 below). */
  uint32_t hclk = (cfg->source == HAL_CLOCK_SOURCE_PLL) ? _pll_output_hz(pll_cfg)
                  : (cfg->source == HAL_CLOCK_SOURCE_HSE) ? HSE_FREQ_HZ
                                                          : HSI_FREQ_HZ;

  /* Engage over-drive before raising the frequency past 180 MHz. */
  if (cfg->source == HAL_CLOCK_SOURCE_PLL && hclk > 180000000U) {
    PWR_CR1 |= PWR_CR1_ODEN;
    while (!(PWR_CSR1 & PWR_CSR1_ODRDY))
      ;
    PWR_CR1 |= PWR_CR1_ODSWEN;
    while (!(PWR_CSR1 & PWR_CSR1_ODSWRDY))
      ;
  }

  volatile uint32_t *const FLASH_ACR =
      (volatile uint32_t *)(FLASH_INTERFACE_REGISTER);

  /* Raise flash wait states BEFORE switching to a faster clock. */
  if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
    *FLASH_ACR = (*FLASH_ACR & ~FLASH_ACR_LATENCY_Msk) | FLASH_ACR_PRFTEN |
                 FLASH_ACR_ARTEN | (_flash_ws_for(hclk) << FLASH_ACR_LATENCY_BIT);
  }

  /* Bus prescalers: AHB /1; APB1 ≤ 54 MHz, APB2 ≤ 108 MHz. */
  uint32_t ppre1 = (hclk <= 54000000U)    ? RCC_CFGR_PPRE_DIV1
                   : (hclk <= 108000000U) ? RCC_CFGR_PPRE_DIV2
                                          : RCC_CFGR_PPRE_DIV4;
  uint32_t ppre2 = (hclk <= 108000000U) ? RCC_CFGR_PPRE_DIV1 : RCC_CFGR_PPRE_DIV2;

  RCC->CFGR &= ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK);
  RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 << RCC_CFGR_HPRE_BIT) |
               (ppre1 << RCC_CFGR_PPRE1_BIT) | (ppre2 << RCC_CFGR_PPRE2_BIT);

  /* Switch the system clock source. */
  if (cfg->source == HAL_CLOCK_SOURCE_HSI) {
    RCC->CFGR &= ~(0x3 << RCC_CFGR_SW_BIT);
    while ((((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 0)
      ;
  } else if (cfg->source == HAL_CLOCK_SOURCE_HSE) {
    RCC->CFGR &= ~(0x3 << RCC_CFGR_SW_BIT);
    RCC->CFGR |= (1 << RCC_CFGR_SW_BIT);
    while ((((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 1)
      ;
  } else { /* PLL */
    RCC->CFGR &= ~(0x3 << RCC_CFGR_SW_BIT);
    RCC->CFGR |= (2 << RCC_CFGR_SW_BIT);
    while ((((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 2)
      ;
  }

  /* Lower flash wait states AFTER dropping to a slower non-PLL clock. */
  if (cfg->source != HAL_CLOCK_SOURCE_PLL) {
    *FLASH_ACR = (*FLASH_ACR & ~FLASH_ACR_LATENCY_Msk) |
                 (_flash_ws_for(hclk) << FLASH_ACR_LATENCY_BIT);
  }

  return HAL_OK;
}

uint32_t hal_clock_get_sysclk(void) {
  uint8_t sws = ((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3;
  switch (sws) {
  case 0:
    return HSI_FREQ_HZ;
  case 1:
    return HSE_FREQ_HZ;
  case 2: {
    uint32_t pll_m = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLM_BIT) & 0x3F;
    uint32_t pll_n = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLN_BIT) & 0x1FF;
    uint32_t pll_p = (((RCC->PLLCFGR >> RCC_PLLCFGR_PLLP_BIT) & 0x3) + 1) * 2;
    uint32_t pll_src = (RCC->PLLCFGR >> RCC_PLLCFGR_SRC_BIT) & 0x1;
    uint32_t vco_in = pll_src ? HSE_FREQ_HZ : HSI_FREQ_HZ;
    if (pll_m == 0 || pll_p == 0)
      return 0;
    return (vco_in / pll_m) * pll_n / pll_p;
  }
  default:
    return 0;
  }
}

static uint32_t _decode_prescaler(uint32_t val) {
  switch (val) {
  case 0x8: return 2;
  case 0x9: return 4;
  case 0xA: return 8;
  case 0xB: return 16;
  case 0xC: return 64;
  case 0xD: return 128;
  case 0xE: return 256;
  case 0xF: return 512;
  default:  return 1;
  }
}

static uint32_t _decode_apb_prescaler(uint32_t val) {
  switch (val) {
  case 0x4: return 2;
  case 0x5: return 4;
  case 0x6: return 8;
  case 0x7: return 16;
  default:  return 1;
  }
}

uint32_t hal_clock_get_ahbclk(void) {
  uint32_t prescaler = ((RCC->CFGR) >> RCC_CFGR_HPRE_BIT) & 0xF;
  return hal_clock_get_sysclk() / _decode_prescaler(prescaler);
}

uint32_t hal_clock_get_apb1clk(void) {
  uint32_t prescaler = ((RCC->CFGR) >> RCC_CFGR_PPRE1_BIT) & 0x7;
  return hal_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}

uint32_t hal_clock_get_apb2clk(void) {
  uint32_t prescaler = ((RCC->CFGR) >> RCC_CFGR_PPRE2_BIT) & 0x7;
  return hal_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}
