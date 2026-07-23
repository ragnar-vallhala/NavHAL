/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief "Hello" over COM1 on bare-metal x86-64 under QEMU.
 *
 * Build + run:  cmake -B build-x86 \
 *                 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-qemu-toolchain.cmake \
 *                 -DSAMPLE=hal_x86_hello -G Ninja
 *               cmake --build build-x86 --target run
 */

#include "board.h"
#include "common/hal_uart.h"

int main(void) {
  hal_uart_config_t cfg = {.baudrate = 115200};
  hal_uart_init(CONSOLE_UART, &cfg);
  hal_uart_write_string(CONSOLE_UART,
                        "Hello from NavHAL on x86-64 (QEMU)!\r\n");
  for (;;)
    __asm__ volatile("hlt");
}
