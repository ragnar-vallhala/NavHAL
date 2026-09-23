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
 * Test-ELF linker script - Cortex-M7 / STM32F767ZI.
 *
 * Same memory map as the F767 board, including the ETHRAM carve-out, so the
 * shared F767 startup resolves its TCM symbols in the test build too.
 *
 * Only the memory map lives here. The section layout is arch-level and shared:
 * src/arch/armv7e-m/link/cortex-m7.ld
 */

MEMORY
{
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  ITCM   (rwx) : ORIGIN = 0x00000000, LENGTH = 16K
  /* SRAM2 (top 16 KB) is carved off as ETHRAM: the Ethernet DMA master reaches
   * SRAM but not DTCM, so its descriptors/buffers must live here, not in the
   * DTCM-based RAM region below. */
  RAM    (rwx) : ORIGIN = 0x20000000, LENGTH = 496K
  ETHRAM (rw)  : ORIGIN = 0x2007C000, LENGTH = 16K
}

INCLUDE cortex-m7.ld
