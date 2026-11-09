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

/*
 * Test-ELF linker script - Cortex-M4 / STM32F401RE.
 *
 * Same memory map as the F401 boards. Sharing their section layout is what
 * keeps the test image and the shipped image linked the same way, which a
 * separate copy had already stopped doing.
 *
 * Only the memory map lives here. The section layout is arch-level and shared:
 * src/arch/armv7e-m/link/cortex-m4.ld
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

/* Same overlap check as the arch script: the KV store erases whole sectors, and
 * this image is the one big enough to reach into the low ones. CMake passes the
 * store's extent; without it the symbols are zero and the check stands down. */
PROVIDE(__kv_start = 0);
PROVIDE(__kv_end = 0);
ASSERT(__kv_end == 0 || (LOADADDR(.data) + SIZEOF(.data)) <= __kv_start ||
           ORIGIN(FLASH) >= __kv_end,
       "test image overlaps the flash KV store -- CONFIG_FLASH_KV_*_SECTOR must be outside this image")


INCLUDE cortex-m4.ld
