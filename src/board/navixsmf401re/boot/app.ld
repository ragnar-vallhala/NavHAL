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
 * app image - NAVIXSM-F401RE (STM32F401RE), bootloader partition layout.
 *
 * Application: sectors 5-7, behind its own 512-byte header. This is the script
 * an application links with once the bootloader owns the low sectors; with it,
 * the KV store at sectors 2-3 is outside the image and
 * CONFIG_FLASH_KV_PRIMARY_SECTOR=2 / CONFIG_FLASH_KV_SECONDARY_SECTOR=3 links.
 * The board's flat linker.ld (0x08000000, 512K) is unchanged and stays the
 * default; this one is selected with -DBOOT_IMAGE=app.
 *
 * The part is the same STM32F401RE as the Nucleo, so the flash map is the same
 * as src/board/nucleo_f401re/boot/app.ld. These numbers are hal_bootmap.h's,
 * repeated because a linker script cannot include a C header. They are asserted
 * against each other there; if the two ever disagree, hal_bootmap.h is the one
 * that is right.
 *
 * The section layout is the shared one: src/arch/armv7e-m/link/cortex-m4.ld.
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08020200, LENGTH = 392704
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

INCLUDE cortex-m4.ld
