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

#define CORTEX_M4
#include "navhal.h"
#include "navtest_target.h"

int putchar(int ch) {
  hal_uart_write_char(NAVTEST_UART, ch);
  return ch;
}

void abort(void) {
  while (1) {
  }
}

void *memcpy(void *dest, const void *src, unsigned int n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}
