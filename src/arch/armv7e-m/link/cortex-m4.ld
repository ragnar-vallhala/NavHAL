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
 * ARMv7E-M section layout - flat memory map (Cortex-M4).
 *
 * The section layout for a Cortex-M4 part with one flash bank and one RAM block. A board supplies its MEMORY block and
 * INCLUDEs this; the layout itself is not something a board gets to have an
 * opinion about, and when six copies existed they drifted -- the test scripts
 * kept .ARM.exidx the board scripts discarded, and dropped the heap and bss
 * bound symbols entirely.
 *
 * What a board owns is its MEMORY block: origins, lengths, and any carve-out
 * its silicon forces. Everything below is arch-level and shared.
 *
 * Resolved via -L on the link line; see cmake/arch/armv7e-m.cmake.
 */

SECTIONS {
  
  .isr_vector : {
    KEEP(*(.isr_vector))
    . = ALIGN(4);   
  } > FLASH
  
  .text : {

    *(.text*)

    /* Reset copy/zero tables (const) walked by Reset_Handler (boot.c). Kept in
       the read-only .text load segment so no spurious RW segment appears. Copy
       triples {flash-src, ram-dst, byte-len}; zero pairs {ram-dst, byte-len}. */
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG(_sidata); LONG(_sdata); LONG(_edata - _sdata);
    __copy_table_end__ = .;
    __zero_table_start__ = .;
    LONG(_sbss); LONG(_ebss - _sbss);
    __zero_table_end__ = .;

  } > FLASH

  /* Read-only data in its own output section rather than folded into .text.
     Both still land in FLASH and stay contiguous, so nothing about the image
     or its hash changes -- what changes is that they can be told apart.

     Two things that buys. `size` starts reporting code as code: 16 KiB of the
     F401 test image's "text" was actually rodata, which matters most on the
     ATmega, where the flash budget is tight and growth needs attributing.
     And an MPU region can cover data without covering code, so a jump into a
     lookup table faults instead of executing constants -- _srodata is aligned
     to 32, the ARMv7-M minimum region size, so that step does not have to come
     back and move things.

     The copy/zero tables above stay in .text deliberately: they are read by
     Reset_Handler before anything else runs, and keeping them there is what
     avoids a spurious RW segment. Splitting rodata out does not add one
     either -- the linker folds it into the same read+execute LOAD segment,
     since the two are adjacent in the same region. That is only the ELF
     view, which nothing on a bare-metal part consults; the MPU works on
     addresses, and _srodata/_erodata are what it would be given. */
  .rodata : {
    . = ALIGN(32);
    _srodata = .;
    *(.rodata*)
    . = ALIGN(32);
    _erodata = .;
  } > FLASH


  /DISCARD/ :
  {
      *(.ARM.extab* .gnu.linkonce.armextab.*)
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  }

  /* Survives a warm reset. The startup code zeroes only what the zero table
     names (_sbss.._ebss) and loads only what the copy table names, and this
     section is in neither, so its contents carry across a reset and are
     garbage after a cold boot. hal_boot.c's _sboot lives here; that is the
     whole mechanism by which one boot leaves a request for the next. */
  .noinit (NOLOAD) : {
    . = ALIGN(8);
    _snoinit = .;
    KEEP(*(.noinit*))
    . = ALIGN(8);
    _enoinit = .;
  } > RAM

  .data : {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
    *(.init*)
    *(.fini*)
  } > RAM AT> FLASH

  .bss : {
    _sbss = .;
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(8);
    _ebss = .;
    __bss_end__ = .;
    _heap_start = .;
  } > RAM AT > RAM
  _heap_end   = ORIGIN(RAM) + LENGTH(RAM);
  _sidata = LOADADDR(.data);
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
