/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief x86-64 clock + timebase demo: calibrate the TSC, then time a delay.
 *
 * Prints the calibrated TSC frequency, then measures a 500 ms busy-wait via the
 * millisecond timebase — the reported elapsed time should read ~500.
 */

#include "board.h"
#include "common/hal_clock.h"
#include "common/hal_timer.h"
#include "common/hal_uart.h"

int main(void) {
  hal_uart_config_t ucfg = {.baudrate = 115200};
  hal_uart_init(CONSOLE_UART, &ucfg);

  hal_clock_config_t ccfg = {.source = HAL_CLOCK_SOURCE_TSC};
  hal_clock_init(&ccfg, 0);
  hal_timebase_init(1000); /* 1 ms tick */

  hal_uart_write_string(CONSOLE_UART, "TSC Hz: ");
  hal_uart_write_uint(CONSOLE_UART, hal_clock_get_sysclk());
  hal_uart_write_string(CONSOLE_UART, "\r\n");

  uint32_t t0 = hal_timebase_get_millis();
  hal_delay_ms(500);
  uint32_t t1 = hal_timebase_get_millis();

  hal_uart_write_string(CONSOLE_UART, "elapsed ms over delay(500): ");
  hal_uart_write_uint(CONSOLE_UART, t1 - t0);
  hal_uart_write_string(CONSOLE_UART, "\r\n");

  for (;;)
    __asm__ volatile("hlt");
}
