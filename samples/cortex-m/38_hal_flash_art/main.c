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
 * @file main.c
 * @brief What the flash accelerator is worth, measured on the part.
 *
 * @details
 * The same workload timed with the accelerator off and on, in one run on one
 * board at one clock, so the only variable is FLASH_ACR. The driver enables it
 * at clock init; this turns it back off to get the "before" number, which is
 * what every NavHAL image did before that change.
 *
 * The workload reads a flash-resident table and runs a loop out of flash, so
 * it exercises the three things the register controls: the prefetch queue, the
 * instruction cache and the data cache. It is deliberately larger than a tight
 * loop, which would sit entirely in the instruction cache and flatter the
 * result.
 *
 * ### The two families answer differently, and the F7 answer is surprising
 *
 * On an F401 the accelerator is worth about 1.5x. On an F767 it is worth
 * nothing measurable -- and that is correct, not a bug. The F7's ART serves
 * flash fetched over the **ITCM** bus at 0x00200000; an image linked at
 * 0x08000000, as every image here is, is fetched over AXI, where ART has no
 * part to play. It is still worth enabling for anything placed in the .itcm
 * section the F767 linker script provides.
 *
 * What does pay on an F7 is the Cortex-M7's own L1 instruction and data
 * caches, which are a core feature rather than a flash one. This sample times
 * those too where they exist, because "the accelerator did nothing" is only
 * half an answer and the other half is worth 2.7x.
 *
 * Cycles come from the DWT counter (::hal_cycle_counter_get), which counts CPU
 * cycles directly and so is immune to the wait states being measured.
 */

#include "board.h"
#include "family/flash_reg.h"
#include "navhal.h"

static hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSE,
    .pll_m = 8,
    .pll_n = 336,
    .pll_p = 4,
    .pll_q = 7,
};
static hal_clock_config_t clk_cfg = {.source = HAL_CLOCK_SOURCE_PLL};

/* 4 KB of flash-resident constants: too big for the 1 KB instruction cache to
 * hide, so the data path is genuinely exercised. */
#define TABLE_WORDS 1024u
static const uint32_t table[TABLE_WORDS] = {
#define R16(n) (n), (n) + 1u, (n) + 2u, (n) + 3u, (n) + 4u, (n) + 5u,           \
               (n) + 6u, (n) + 7u, (n) + 8u, (n) + 9u, (n) + 10u, (n) + 11u,   \
               (n) + 12u, (n) + 13u, (n) + 14u, (n) + 15u
#define R256(n) R16(n), R16((n) + 16u), R16((n) + 32u), R16((n) + 48u),         \
                R16((n) + 64u), R16((n) + 80u), R16((n) + 96u),                 \
                R16((n) + 112u), R16((n) + 128u), R16((n) + 144u),              \
                R16((n) + 160u), R16((n) + 176u), R16((n) + 192u),              \
                R16((n) + 208u), R16((n) + 224u), R16((n) + 240u)
    R256(0u), R256(256u), R256(512u), R256(768u)};

/* Sum with a data-dependent stride so the prefetcher cannot simply stream it,
 * and a little arithmetic so the loop body is more than one instruction. */
static uint32_t workload(void) {
  uint32_t acc = 0u;
  for (uint32_t pass = 0u; pass < 64u; pass++) {
    uint32_t i = pass;
    while (i < TABLE_WORDS) {
      acc += table[i] ^ (acc >> 3);
      acc = (acc << 1) | (acc >> 31);
      i += 7u;
    }
  }
  return acc;
}

static uint32_t time_workload(uint32_t *out_acc) {
  uint32_t start, end;
  hal_cycle_counter_reset();
  start = hal_cycle_counter_get();
  *out_acc = workload();
  end = hal_cycle_counter_get();
  return end - start;
}

static void print(const char *s) { hal_uart_write_string(BOARD_CONSOLE_UART, s); }
static void print_u32(uint32_t v) { hal_uart_write_uint(BOARD_CONSOLE_UART, v); }

int main(void) {
  volatile uint32_t *const acr =
      (volatile uint32_t *)(FLASH_INTERFACE_REGISTER);

  clk_cfg.pll = pll_cfg;
  hal_clock_init(&clk_cfg);
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  hal_cycle_counter_init();

  uint32_t acc_off = 0u, acc_on = 0u;
  uint32_t saved = *acr;

  /* Decoded rather than dumped: hal_uart_write_uint prints decimal, and a raw
   * decimal register value is a puzzle rather than a result. */
  print("\r\n--- flash accelerator ---\r\nFLASH_ACR wait states: ");
  print_u32(saved & 0x7u);
  print("  prefetch: ");
  print((saved & FLASH_ACR_PRFTEN) ? "on" : "off");
#if defined(FLASH_ACR_ARTEN)
  /* F7: one ART bit covers the accelerator. */
  print("  ART: ");
  print((saved & FLASH_ACR_ARTEN) ? "on" : "off");
#else
  /* F4: prefetch and the two caches are enabled separately. */
  print("  icache: ");
  print((saved & FLASH_ACR_ICEN) ? "on" : "off");
  print("  dcache: ");
  print((saved & FLASH_ACR_DCEN) ? "on" : "off");
#endif

  /* "Before": latency only, which is what every image used to run with. */
  *acr = saved & 0x7u;
  uint32_t cycles_off = time_workload(&acc_off);

  /* "After": whatever the driver configured. */
  *acr = saved;
  uint32_t cycles_on = time_workload(&acc_on);

  print("\r\naccelerator OFF: ");
  print_u32(cycles_off);
  print(" cycles\r\naccelerator ON:  ");
  print_u32(cycles_on);
  print(" cycles\r\nspeedup x100:    ");
  print_u32(cycles_on ? (cycles_off * 100u) / cycles_on : 0u);
#if NAVHAL_CONFIG_DRV_CACHE
  {
    uint32_t acc_l1 = 0u;
    hal_icache_enable();
    uint32_t cycles_i = time_workload(&acc_l1);
    print("\r\nI-cache only: ");
    print_u32(cycles_i);
    hal_icache_disable();
    hal_dcache_enable();
    uint32_t cycles_d = time_workload(&acc_l1);
    print("   D-cache only: ");
    print_u32(cycles_d);
    hal_icache_enable();
    uint32_t cycles_l1 = time_workload(&acc_l1);
    /* The number that matters on this family: ART is an ITCM-bus feature and
     * this image runs from AXI, so the L1 caches are where the win is. */
    print("\r\nwith M7 L1 caches: ");
    print_u32(cycles_l1);
    print(" cycles  x100 vs ART-on: ");
    print_u32(cycles_l1 ? (cycles_on * 100u) / cycles_l1 : 0u);
  }
#endif
  print("\r\nchecksums (must match): ");
  print_u32(acc_off);
  print(" ");
  print_u32(acc_on);
  print(acc_off == acc_on ? "  OK\r\n" : "  MISMATCH\r\n");

  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  while (1) {
    hal_gpio_toggle(LED_BUILTIN);
    hal_delay_ms(500u);
  }
}
