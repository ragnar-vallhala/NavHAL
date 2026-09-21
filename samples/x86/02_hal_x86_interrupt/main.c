/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief x86-64 interrupt demo: a PIT IRQ drives a periodic tick callback.
 *
 * hal_timebase_init() programs 8254 PIT channel 0 for a 1 kHz IRQ0 through the
 * IDT + 8259 PIC. Over a 500 ms TSC busy-wait the interrupt should fire ~500
 * times, so both the app callback counter and hal_timebase_get_tick() read ~500
 * — proof the IDT, PIC remap, ISR stubs, and dispatch all work.
 */

#include "board.h"
#include "common/hal_clock.h"
#include "common/hal_timer.h"
#include "common/hal_uart.h"

static volatile uint32_t app_ticks;

static void on_tick(void) { app_ticks++; }

int main(void) {
  hal_uart_config_t ucfg = {.baudrate = 115200};
  hal_uart_init(CONSOLE_UART, &ucfg);

  hal_clock_config_t ccfg = {.source = HAL_CLOCK_SOURCE_TSC};
  hal_clock_init(&ccfg);

  hal_timebase_set_callback(on_tick);
  hal_timebase_init(1000); /* 1 kHz PIT IRQ */

  hal_delay_ms(500); /* PIT IRQ fires ~500 times during this busy-wait */

  hal_uart_write_string(CONSOLE_UART, "app callback ticks: ");
  hal_uart_write_uint(CONSOLE_UART, app_ticks);
  hal_uart_write_string(CONSOLE_UART, "\r\n");
  hal_uart_write_string(CONSOLE_UART, "hal_timebase_get_tick: ");
  hal_uart_write_uint(CONSOLE_UART, hal_timebase_get_tick());
  hal_uart_write_string(CONSOLE_UART, "\r\n");

  for (;;)
    __asm__ volatile("hlt");
}
