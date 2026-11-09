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
 * stage1 image - ST Nucleo-F401RE, bootloader partition layout.
 *
 * Stage-1: sectors 0-1, write-protected, no header. The CPU boots straight into this vector table, so it starts at the partition base.
 *
 * These numbers are hal_bootmap.h's, repeated because a linker script cannot
 * include a C header. They are asserted against each other there; if the two
 * ever disagree, hal_bootmap.h is the one that is right.
 *
 * The section layout is the shared one: src/arch/armv7e-m/link/cortex-m4.ld.
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

INCLUDE cortex-m4.ld
