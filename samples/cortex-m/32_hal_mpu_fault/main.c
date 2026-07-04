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
 * @brief MPU enforcement demo: provoke a MemManage fault on a protected region.
 *
 * @details
 * Reads a buffer normally, then covers it with a no-access MPU region and reads
 * it again. The second read traps into the MemManage fault, caught by a strong
 * ::MemManage_Handler (overriding the named weak vector). The handler records
 * the fault, drops MPU enforcement so the retried access completes, and the
 * program reports that the violation fired.
 *
 * The MemManage fault must be enabled (@c SCB_SHCSR.MEMFAULTENA) or an MPU
 * violation escalates to HardFault instead.
 *
 * Status is printed on USART3 (ST-LINK VCP, 9600 8N1).
 */

#include "navhal.h"

#define CONSOLE HAL_UART_2

/* Cortex-M7 System Control Block fault registers (not wrapped by the HAL). */
#define SCB_SHCSR (*(volatile uint32_t *)0xE000ED24u)
#define SCB_CFSR (*(volatile uint32_t *)0xE000ED28u)
#define SCB_MMFAR (*(volatile uint32_t *)0xE000ED34u)
#define SHCSR_MEMFAULTENA (1u << 16) /* enable the MemManage fault. */
#define CFSR_MMARVALID (1u << 7)     /* MMFAR holds a valid faulting address. */

static volatile uint32_t s_fault_count;
static volatile uint32_t s_fault_addr;

/* The region to protect: 32 bytes, 32-byte aligned (MPU regions are power-of-two
 * and size-aligned). volatile so the reads below are actually emitted. */
static volatile uint8_t protected_buf[32] __attribute__((aligned(32)));

static void print_hex8(uint8_t v) {
  static const char d[] = "0123456789ABCDEF";
  char s[3] = {d[(v >> 4) & 0xF], d[v & 0xF], '\0'};
  hal_uart_print(CONSOLE, s);
}

static void print_hex32(uint32_t v) {
  static const char d[] = "0123456789ABCDEF";
  char s[9];
  for (int i = 0; i < 8; i++)
    s[i] = d[(v >> ((7 - i) * 4)) & 0xF];
  s[8] = '\0';
  hal_uart_print(CONSOLE, s);
}

/* Runs in MemManage fault context (via Default_Handler dispatch). */
/* Strong override of the named weak MemManage vector. Runs in fault context. */
void MemManage_Handler(void) {
  s_fault_count++;
  if (SCB_CFSR & CFSR_MMARVALID)
    s_fault_addr = SCB_MMFAR;
  SCB_CFSR = SCB_CFSR; /* clear the sticky MemManage status bits (write-1). */
  /* Remove the protection so the faulting access succeeds when it re-executes
   * on return, letting the program continue past the demonstration. */
  hal_mpu_disable();
}

int main(void) {
  hal_uart_init(CONSOLE, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_print(CONSOLE, "\r\n[mpu] enforcement demo\r\n");

  if (!hal_mpu_present()) {
    hal_uart_print(CONSOLE, "[mpu] no MPU on this target\r\n");
    for (;;)
      ;
  }

  protected_buf[0] = 0xAB;
  hal_uart_print(CONSOLE, "[mpu] normal read (no MPU): 0x");
  print_hex8(protected_buf[0]);
  hal_uart_print(CONSOLE, "\r\n");

  /* Enable the MemManage fault so a violation traps into MemManage_Handler
   * instead of escalating to HardFault. */
  SCB_SHCSR |= SHCSR_MEMFAULTENA;

  /* Cover the buffer with a no-access region; the privileged background region
   * (bg_priv=true) keeps code/stack/peripherals reachable. */
  hal_mpu_region_t r = {
      .base = (uint32_t)(uintptr_t)protected_buf,
      .size = HAL_MPU_SIZE_32B,
      .ap = HAL_MPU_AP_NONE,
      .mem = HAL_MPU_MEM_NORMAL_WB,
      .executable = false,
      .shareable = false,
      .srd_mask = 0,
  };
  hal_mpu_configure_region(0, &r);
  hal_mpu_enable(true);

  hal_uart_print(CONSOLE, "[mpu] reading protected buffer (MPU active)...\r\n");
  uint8_t v = protected_buf[0]; /* -> MemManage fault -> on_memmanage. */

  hal_uart_print(CONSOLE, "[mpu] read returned 0x");
  print_hex8(v);
  hal_uart_print(CONSOLE, ", faults=");
  print_hex32(s_fault_count);
  hal_uart_print(CONSOLE, " addr=0x");
  print_hex32(s_fault_addr);
  hal_uart_print(CONSOLE, "\r\n");

  if (s_fault_count > 0 && s_fault_addr == (uint32_t)(uintptr_t)protected_buf)
    hal_uart_print(CONSOLE, "[mpu] ENFORCEMENT CONFIRMED\r\n");
  else
    hal_uart_print(CONSOLE, "[mpu] NO FAULT - enforcement FAILED\r\n");

  for (;;)
    ;
}
