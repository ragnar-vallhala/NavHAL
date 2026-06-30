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

#include "host_mmio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* APB1/APB2 + AHB1-GPIO live in 0x40000000..0x40023FFF on the F7 (RCC at
 * 0x40023800, flash interface at 0x40023C00, GPIO A.. at 0x40020000, USART/
 * SPI/I2C/TIM below). One mapping covers everything the drivers touch. */
#define PERIPH_BASE 0x40000000UL
#define PERIPH_SIZE 0x00024000UL

/* On-chip flash — the flash driver programs/erases the KV-store sectors. */
#define FLASH_BASE 0x08000000UL
#define FLASH_SIZE 0x00200000UL

static void map_fixed(uintptr_t addr, size_t size) {
  void *p = mmap((void *)addr, size, PROT_READ | PROT_WRITE,
                 MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED || (uintptr_t)p != addr) {
    fprintf(stderr, "host_mmio: mmap @ 0x%lx failed\n", (unsigned long)addr);
    perror("mmap");
    abort();
  }
}

void host_mmio_setup(void) {
  map_fixed(PERIPH_BASE, PERIPH_SIZE);
  map_fixed(FLASH_BASE, FLASH_SIZE);
  host_mmio_reset();
}

void host_mmio_reset(void) {
  memset((void *)PERIPH_BASE, 0, PERIPH_SIZE);
  memset((void *)FLASH_BASE, 0xFF, FLASH_SIZE); /* erased flash reads as 0xFF */
}

void host_reg_set(uintptr_t addr, uint32_t bits) {
  *(volatile uint32_t *)addr |= bits;
}

void host_reg_clear(uintptr_t addr, uint32_t bits) {
  *(volatile uint32_t *)addr &= ~bits;
}
