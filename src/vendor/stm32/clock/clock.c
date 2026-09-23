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
 * @file clock.c
 * @brief Cortex-M4 (STM32F4) Clock HAL Implementation.
 *
 * Provides functions to initialize and retrieve clock frequencies
 * including SYSCLK, AHB, APB1, and APB2 clocks.
 *
 * This implementation supports HSI, HSE, and PLL clock sources.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-21
 */

#include "internal/hal_clock_ops.h"
#include "family/flash_reg.h"
#include "family/rcc_reg.h"
#include <stdint.h>

/* Internal clock-source toggle helpers (file-local). */
/* Bound on the spin waiting for an RCC ready / status bit to settle. On real
 * silicon HSI/HSE/PLL lock and the SYSCLK switch complete in well under a
 * thousand cycles; the generous cap only stops an unbounded hang when a bit
 * never sets — e.g. PLL/HSE not modelled under QEMU, or a board with no
 * external crystal. On timeout the caller backs out and leaves the clock on the
 * reset HSI rather than spinning forever. */
#define CLOCK_READY_TIMEOUT 1000000UL

/* Spin until `cond` is false or the timeout elapses; return HAL_ERR_TIMEOUT from
 * the enclosing function on timeout. Used by helpers that return hal_status_t. */
#define WAIT_OR_TIMEOUT(cond)                                                  \
  do {                                                                         \
    uint32_t _to = CLOCK_READY_TIMEOUT;                                        \
    while (cond) {                                                             \
      if (--_to == 0u)                                                         \
        return HAL_ERR_TIMEOUT;                                                \
    }                                                                          \
  } while (0)

/* Internal clock-source toggle helpers (file-local). Return HAL_ERR_TIMEOUT if
 * the ready bit never reaches `state`. */
static hal_status_t _toggle_hse_clock(uint8_t state) {
  if (state) {
    RCC->CR |= RCC_CR_HSEON;
  } else
    RCC->CR &= ~RCC_CR_HSEON;
  WAIT_OR_TIMEOUT(((RCC->CR & RCC_CR_HSERDY) != 0) != (state));
  return HAL_OK;
}
static hal_status_t _toggle_hsi_clock(uint8_t state) {
  if (state)
    RCC->CR |= RCC_CR_HSION;
  else
    RCC->CR &= ~RCC_CR_HSION;
  WAIT_OR_TIMEOUT(((RCC->CR & RCC_CR_HSIRDY) != 0) != (state));
  return HAL_OK;
}
static hal_status_t _toggle_pll_clock(uint8_t state) {
  if (state)
    RCC->CR |= RCC_CR_PLLON;
  else
    RCC->CR &= ~RCC_CR_PLLON;
  WAIT_OR_TIMEOUT(((RCC->CR & RCC_CR_PLLRDY) != 0) != (state));
  return HAL_OK;
}

/*
 * @brief Initialize system clocks based on the provided configuration.
 * (API doc lives in common/hal_clock.h; this is an implementation note.)
 *
 * Enables and waits for the selected clock source (HSI, HSE, or PLL).
 * If PLL is selected, configures PLL parameters and switches system clock
 * source to PLL output.
 *
 * @param cfg     Clock configuration specifying the source; must not be NULL.
 * PLL parameters come from @c cfg->pll when the source is PLL.
 * @return ::HAL_OK on success, ::HAL_ERR_INVALID_ARG on a missing argument.
 */

#define STM32_HSI_FREQ_HZ 16000000U
#define STM32_HSE_FREQ_HZ 8000000U

/* The frequency SYSCLK will run at once the switch below completes.
 * stm32_clock_get_sysclk() reads the registers and so still reports the old
 * source at the point the bus prescalers are programmed -- sizing them
 * against that would leave the buses overclocked after the switch. */
static uint32_t _target_sysclk_hz(const hal_clock_config_t *cfg) {
  if (cfg->source == HAL_CLOCK_SOURCE_HSE)
    return STM32_HSE_FREQ_HZ;
  if (cfg->source != HAL_CLOCK_SOURCE_PLL)
    return STM32_HSI_FREQ_HZ;

  if (cfg->pll.pll_m == 0u || cfg->pll.pll_p == 0u)
    return 0u;
  uint32_t in = (cfg->pll.input_src == HAL_CLOCK_SOURCE_HSE)
                    ? STM32_HSE_FREQ_HZ
                    : STM32_HSI_FREQ_HZ;
  return (in / cfg->pll.pll_m) * cfg->pll.pll_n / cfg->pll.pll_p;
}

/* Divide-by-N to the RCC field encodings. HPRE skips 32; PPRE tops out at 16.
 * Returns false for a divider the field cannot express. */
/* Wait states for a given HCLK at 2.7-3.6 V (RM0368 Table 6: one more per
 * 30 MHz). Derived rather than hardcoded: the old code wrote 2 for every PLL
 * config, which is right at 84 MHz by coincidence and wrong for any other
 * target hal_clock_init_hz will happily accept. */
static uint32_t _flash_ws_for(uint32_t hclk) {
  uint32_t ws = (hclk == 0u) ? 0u : (hclk - 1u) / 30000000U;
  return (ws > 7u) ? 7u : ws;
}

/* Latency plus the ART Accelerator, written as one value.
 *
 * The whole register resets to 0, so prefetch and both caches are off until
 * something turns them on, and nothing did: every image ran at 84 MHz and 2
 * wait states with no prefetch, no instruction cache and no data cache.
 *
 * The caches are reset while still disabled, which is the only time RM0368
 * permits it -- a cache re-enabled with stale lines would serve whatever the
 * previous clock configuration left behind. Write, reset, then enable. */
static void _flash_set_acr(volatile uint32_t *acr, uint32_t hclk) {
  uint32_t ws = _flash_ws_for(hclk) << FLASH_ACR_LATENCY_BIT;

  *acr = ws;                                            /* caches off */
  *acr = ws | FLASH_ACR_ICRST | FLASH_ACR_DCRST;        /* flush both */
  *acr = ws;                                            /* release reset */
  *acr = ws | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
}

static bool _hpre_encode(uint16_t div, uint32_t *out) {
  switch (div) {
  case 1: *out = RCC_CFGR_HPRE_DIV1; return true;
  case 2: *out = RCC_CFGR_HPRE_DIV2; return true;
  case 4: *out = RCC_CFGR_HPRE_DIV4; return true;
  case 8: *out = RCC_CFGR_HPRE_DIV8; return true;
  case 16: *out = RCC_CFGR_HPRE_DIV16; return true;
  case 64: *out = RCC_CFGR_HPRE_DIV64; return true;
  case 128: *out = RCC_CFGR_HPRE_DIV128; return true;
  case 256: *out = RCC_CFGR_HPRE_DIV256; return true;
  case 512: *out = RCC_CFGR_HPRE_DIV512; return true;
  default: return false;
  }
}

static bool _ppre_encode(uint16_t div, uint32_t *out) {
  switch (div) {
  case 1: *out = RCC_CFGR_PPRE_DIV1; return true;
  case 2: *out = RCC_CFGR_PPRE_DIV2; return true;
  case 4: *out = RCC_CFGR_PPRE_DIV4; return true;
  case 8: *out = RCC_CFGR_PPRE_DIV8; return true;
  case 16: *out = RCC_CFGR_PPRE_DIV16; return true;
  default: return false;
  }
}

/* Smallest divider keeping a bus at or under its ceiling. */
static uint16_t _min_div_for(uint32_t hclk, uint32_t limit_hz) {
  uint16_t d = 1u;
  while (d < 16u && (hclk / d) > limit_hz)
    d = (uint16_t)(d * 2u);
  return d;
}

static hal_status_t stm32_clock_init(const hal_clock_config_t *cfg) {
  /* A PLL source with no usable PLL parameters is rejected rather than being
   * programmed: m/n/p are divisors, and a zeroed config would either divide by
   * zero or wait forever for a lock that cannot happen. This replaces the old
   * pll_cfg == NULL check, and also catches a present-but-empty config, which
   * that check let through. */
  if (cfg->source == HAL_CLOCK_SOURCE_PLL &&
      (cfg->pll.pll_m == 0u || cfg->pll.pll_n == 0u || cfg->pll.pll_p == 0u))
    return HAL_ERR_INVALID_ARG;

  /* cfg is non-NULL: the public layer validated it before dispatching. */
  // Enable and wait for selected clock source. On a ready-bit timeout, bail out
  // immediately: the system clock is left on the reset HSI rather than being
  // switched onto a source that never came up (which would be a dead clock).
  hal_status_t st;
  if (cfg->source == HAL_CLOCK_SOURCE_HSE) {
    if ((st = _toggle_hse_clock(RCC_ON)) != HAL_OK)
      return st;
  } else if (cfg->source == HAL_CLOCK_SOURCE_HSI) {
    if ((st = _toggle_hsi_clock(RCC_ON)) != HAL_OK)
      return st;
  }

  // PLL configuration and enabling (if PLL is selected)
  else if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
    // Enable PLL input source clock and wait for readiness
    if (cfg->pll.input_src == HAL_CLOCK_SOURCE_HSE) {
      if ((st = _toggle_hse_clock(RCC_ON)) != HAL_OK)
        return st;
    } else if (cfg->pll.input_src == HAL_CLOCK_SOURCE_HSI) {
      if ((st = _toggle_hsi_clock(RCC_ON)) != HAL_OK)
        return st;
    }

    if ((st = _toggle_pll_clock(RCC_OFF)) != HAL_OK)
      return st;
    RCC->PLLCFGR = 0;
    // Set PLL source (HSI=0, HSE=1)
    if (cfg->pll.input_src == HAL_CLOCK_SOURCE_HSI) {
      RCC->PLLCFGR &= ~RCC_PLLCFGR_SRC;
    } else {
      RCC->PLLCFGR |= RCC_PLLCFGR_SRC;
    }

    // Set PLL dividers and multipliers
    RCC->PLLCFGR |=
        RCC_PLLCFGR_PLLM(cfg->pll.pll_m) | RCC_PLLCFGR_PLLN(cfg->pll.pll_n) |
        RCC_PLLCFGR_PLLP(cfg->pll.pll_p) | RCC_PLLCFGR_PLLQ(cfg->pll.pll_q);

    if ((st = _toggle_pll_clock(RCC_ON)) != HAL_OK)
      return st; // PLL never locked — stay on HSI instead of hanging.
  }

  // Configure flash latency based on target clock
  volatile uint32_t *const FLASH_ACR =
      (volatile uint32_t *)(FLASH_INTERFACE_REGISTER);

  // When increasing frequency (switching to PLL), increase wait states FIRST
  if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
    _flash_set_acr(FLASH_ACR, _target_sysclk_hz(cfg));
  }

  /* Bus prescalers. The config's dividers used to be accepted and discarded;
   * they are honoured now, with 0 meaning "pick one". Each is clamped to the
   * bus ceiling from RM0368: AHB and APB2 are 84 MHz, APB1 is 42 MHz, and
   * overclocking a bus is not something a caller should be able to ask for by
   * getting the arithmetic wrong. */
  uint32_t sysclk_hz = _target_sysclk_hz(cfg);
  if (sysclk_hz == 0u)
    return HAL_ERR_INVALID_ARG;

  uint16_t hpre_div = (cfg->hpre_div != 0u) ? cfg->hpre_div : 1u;
  uint32_t hpre;
  if (!_hpre_encode(hpre_div, &hpre))
    return HAL_ERR_INVALID_ARG;

  uint32_t hclk = sysclk_hz / hpre_div;

  /* 0 means "leave it to the driver", and that keeps the values this backend
   * has always programmed rather than the fastest legal ones. Making a field
   * work should not change the clock tree under firmware that never set it. */
  uint16_t ppre1_div = (cfg->ppre1_div != 0u) ? cfg->ppre1_div : 2u;
  uint16_t ppre2_div = (cfg->ppre2_div != 0u) ? cfg->ppre2_div : 2u;

  /* Clamp an explicit request that would exceed the bus limit. */
  uint16_t ppre1_min = _min_div_for(hclk, 42000000u);
  uint16_t ppre2_min = _min_div_for(hclk, 84000000u);
  if (ppre1_div < ppre1_min)
    ppre1_div = ppre1_min;
  if (ppre2_div < ppre2_min)
    ppre2_div = ppre2_min;

  uint32_t ppre1;
  uint32_t ppre2;
  if (!_ppre_encode(ppre1_div, &ppre1) || !_ppre_encode(ppre2_div, &ppre2))
    return HAL_ERR_INVALID_ARG;

  (RCC->CFGR) &=
      ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK);
  (RCC->CFGR) |= (hpre << RCC_CFGR_HPRE_BIT) |
                 (ppre1 << RCC_CFGR_PPRE1_BIT) |
                 (ppre2 << RCC_CFGR_PPRE2_BIT);

  // Switch system clock source (each switch-status wait is bounded too).
  if (cfg->source == HAL_CLOCK_SOURCE_HSI) {
    (RCC->CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT); // Select HSI
    WAIT_OR_TIMEOUT((((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 0);
  } else if (cfg->source == HAL_CLOCK_SOURCE_HSE) {
    (RCC->CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
    (RCC->CFGR) |= (1 << RCC_CFGR_SW_BIT); // Select HSE
    WAIT_OR_TIMEOUT((((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 1);
  } else if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
    (RCC->CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
    (RCC->CFGR) |= (2 << RCC_CFGR_SW_BIT); // Select PLL
    WAIT_OR_TIMEOUT((((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 2);
  }

  // When decreasing frequency (switching from PLL to HSI/HSE), decrease wait
  // states AFTER
  if (cfg->source != HAL_CLOCK_SOURCE_PLL) {
    /* Also the path a board takes when it never uses the PLL at all, which is
     * why the accelerator is enabled here too rather than only on the way up. */
    _flash_set_acr(FLASH_ACR, _target_sysclk_hz(cfg));
  }

  return HAL_OK;
}

/**
 * @brief Get the current system clock frequency in Hz.
 *
 * Detects the current SYSCLK source and calculates frequency based on
 * configured clock source and PLL parameters.
 *
 * @return SYSCLK frequency in Hertz.
 */
static uint32_t stm32_clock_get_sysclk(void) {
  uint32_t sysclk;
  uint8_t sws = ((RCC->CFGR) >> RCC_CFGR_SWS_BIT) & 0x3;

  switch (sws) {
  case 0:              // HSI selected
    sysclk = 16000000; // Internal HSI clock frequency
    break;
  case 1:             // HSE selected
    sysclk = 8000000; // External crystal frequency (should be configurable)
    break;
  case 2: // PLL selected
  {
    uint32_t pll_m = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLM_BIT) & 0x3F;
    uint32_t pll_n = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLN_BIT) & 0x1FF;
    uint32_t pll_p = (((RCC->PLLCFGR >> RCC_PLLCFGR_PLLP_BIT) & 0x3) + 1) * 2;

    uint32_t pll_src = (RCC->PLLCFGR >> RCC_PLLCFGR_SRC_BIT) & 0x1;
    uint32_t vco_in =
        pll_src ? 8000000 : 16000000; // 8MHz for HSE, 16MHz for HSI

    sysclk = (vco_in / pll_m) * pll_n / pll_p;
    break;
  }
  default:
    sysclk = 0; // Unknown clock source/error
    break;
  }

  return sysclk;
}

/**
 * @brief Decode AHB prescaler register value to division factor.
 *
 * @param val 4-bit prescaler value from RCC_CFGR register.
 * @return Division factor (1, 2, 4, 8, 16, 64, 128, 256, 512).
 */
static uint32_t _decode_prescaler(uint32_t val) {
  switch (val) {
  case 0x0:
    return 1;
  case 0x8:
    return 2;
  case 0x9:
    return 4;
  case 0xA:
    return 8;
  case 0xB:
    return 16;
  case 0xC:
    return 64;
  case 0xD:
    return 128;
  case 0xE:
    return 256;
  case 0xF:
    return 512;
  default:
    return 1;
  }
}

/**
 * @brief Decode APB prescaler register value to division factor.
 *
 * @param val 3-bit prescaler value from RCC_CFGR register.
 * @return Division factor (1, 2, 4, 8, 16).
 */
static uint32_t _decode_apb_prescaler(uint32_t val) {
  switch (val) {
  case 0x0:
    return 1;
  case 0x4:
    return 2;
  case 0x5:
    return 4;
  case 0x6:
    return 8;
  case 0x7:
    return 16;
  default:
    return 1;
  }
}

/**
 * @brief Get the current AHB clock frequency.
 *
 * Calculated as SYSCLK divided by AHB prescaler.
 *
 * @return AHB bus clock frequency in Hertz.
 */
static uint32_t stm32_clock_get_ahbclk(void) {
  uint32_t prescaler = ((RCC->CFGR) >> RCC_CFGR_HPRE_BIT) & 0xF;
  return stm32_clock_get_sysclk() / _decode_prescaler(prescaler);
}

/**
 * @brief Get the current APB1 clock frequency.
 *
 * Calculated as SYSCLK divided by APB1 prescaler.
 *
 * @return APB1 bus clock frequency in Hertz.
 */
static uint32_t stm32_clock_get_apb1clk(void) {
  uint32_t prescaler = ((RCC->CFGR) >> RCC_CFGR_PPRE1_BIT) & 0x7;
  return stm32_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}

/**
 * @brief Get the current APB2 clock frequency.
 *
 * Calculated as SYSCLK divided by APB2 prescaler.
 *
 * @return APB2 bus clock frequency in Hertz.
 */
static uint32_t stm32_clock_get_apb2clk(void) {
  uint32_t prescaler = ((RCC->CFGR) >> RCC_CFGR_PPRE2_BIT) & 0x7;
  return stm32_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}

static uint8_t stm32_clock_get_bus_count(void) {
  return (uint8_t)HAL_CLOCK_BUS_COUNT;
}

static uint32_t stm32_clock_get_bus_clock(uint8_t bus) {
  switch ((hal_clock_bus_t)bus) {
  case HAL_CLOCK_BUS_AHB:
    return stm32_clock_get_ahbclk();
  case HAL_CLOCK_BUS_APB1:
    return stm32_clock_get_apb1clk();
  case HAL_CLOCK_BUS_APB2:
    return stm32_clock_get_apb2clk();
  default:
    return 0u;
  }
}


/* PLL solving, RM0368 §6.3.2: VCO input must land in 1..2 MHz, VCO output in
 * 192000000..432000000 MHz, and PLLP is one of 2/4/6/8. Picking these by hand is
 * where a clock config goes quietly wrong -- 09_hal_clock shipped an N that
 * put the VCO below its minimum while still producing the right SYSCLK. */
#define PLL_VCO_IN_HZ 1000000U
#define PLL_VCO_MIN_HZ 192000000U
#define PLL_VCO_MAX_HZ 432000000U
#define PLL_SYSCLK_MAX_HZ 84000000U

static hal_status_t _solve_pll(hal_clock_source_t input_src, uint32_t target_hz,
                               hal_pll_config_t *out) {
  if (target_hz == 0u || target_hz > PLL_SYSCLK_MAX_HZ)
    return HAL_ERR_INVALID_ARG;

  uint32_t in = (input_src == HAL_CLOCK_SOURCE_HSE) ? STM32_HSE_FREQ_HZ
                                                    : STM32_HSI_FREQ_HZ;

  /* A 1 MHz VCO input gives the finest N granularity the part allows, and
   * divides exactly for both the 8 MHz HSE and the 16 MHz HSI. */
  if ((in % PLL_VCO_IN_HZ) != 0u)
    return HAL_ERR_INVALID_ARG;
  uint32_t m = in / PLL_VCO_IN_HZ;
  if (m < 2u || m > 63u)
    return HAL_ERR_INVALID_ARG;

  for (uint32_t p = 2u; p <= 8u; p += 2u) {
    uint64_t vco = (uint64_t)target_hz * p;
    if (vco < PLL_VCO_MIN_HZ || vco > PLL_VCO_MAX_HZ)
      continue;

    uint32_t n = (uint32_t)(vco / PLL_VCO_IN_HZ);
    if (n < 50u || n > 432u)
      continue;
    /* Reject a target the integer N cannot hit exactly. */
    if ((uint64_t)n * PLL_VCO_IN_HZ != vco)
      continue;

    out->input_src = input_src;
    out->pll_m = (uint8_t)m;
    out->pll_n = (uint16_t)n;
    out->pll_p = (uint8_t)p;
    /* 48 MHz for USB where the VCO allows it, else the nearest legal Q. */
    uint32_t q = (uint32_t)(vco / 48000000u);
    out->pll_q = (uint8_t)((q >= 2u && q <= 15u) ? q : 7u);
    return HAL_OK;
  }
  return HAL_ERR_INVALID_ARG;
}

hal_status_t hal_clock_init_hz(hal_clock_source_t pll_input,
                               uint32_t target_hz) {
  hal_clock_config_t cfg = {0};
  cfg.source = HAL_CLOCK_SOURCE_PLL;

  hal_status_t st = _solve_pll(pll_input, target_hz, &cfg.pll);
  if (st != HAL_OK)
    return st;

  return hal_clock_init(&cfg);
}

const hal_clock_ops_t _hal_clock_ops = {
    .init = stm32_clock_init,
    .get_sysclk = stm32_clock_get_sysclk,
    .get_bus_count = stm32_clock_get_bus_count,
    .get_bus_clock = stm32_clock_get_bus_clock,
};
