/*
 * Test-ELF linker script — Cortex-M7 / STM32F767ZI.
 *
 * Mirrors tests/arch/cortex-m4/linker.ld but with the F767 memory map
 * (2 MB flash, 512 KB contiguous SRAM @ 0x20000000, 16 KB ITCM @ 0x0). Used by
 * the TEST build via NAVHAL_TEST_LINKER_FLAGS in cmake/arch/armv7e-m.cmake.
 * Carries the same ITCM/DTCM sections as the board linker so the shared F767
 * startup (which copies them) resolves its symbols in the test build too.
 */
MEMORY
{
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  ITCM   (rwx) : ORIGIN = 0x00000000, LENGTH = 16K
  /* SRAM2 (top 16 KB) carved off as ETHRAM: the Ethernet DMA reaches SRAM but
   * not DTCM, so its descriptors/buffers live here (see the board linker). */
  RAM    (rwx) : ORIGIN = 0x20000000, LENGTH = 496K
  ETHRAM (rw)  : ORIGIN = 0x2007C000, LENGTH = 16K
}

SECTIONS {
  .text : {
    KEEP(*(.isr_vector))   /* interrupt vector table */
    *(.text*)
    *(.rodata*)

    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;

    . = ALIGN(4);
    *(.ARM.extab*)
    . = ALIGN(4);

    . = ALIGN(4);
    *(.eh_frame*)

    /* Reset copy/zero tables (const) walked by Reset_Handler (boot.c). Kept in
       the read-only .text load segment so no spurious RW segment appears. Copy
       triples {flash-src, ram-dst, byte-len}; zero pairs {ram-dst, byte-len}.
       F7 TCM regions are extra rows. Symbols are resolved after layout. */
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG(_siitcm); LONG(_sitcm); LONG(_eitcm - _sitcm);
    LONG(_sidtcm); LONG(_sdtcm); LONG(_edtcm - _sdtcm);
    LONG(_sidata); LONG(_sdata); LONG(_edata - _sdata);
    __copy_table_end__ = .;
    __zero_table_start__ = .;
    LONG(_sdtcm_bss); LONG(_edtcm_bss - _sdtcm_bss);
    LONG(_sbss); LONG(_ebss - _sbss);
    __zero_table_end__ = .;
  } > FLASH

  /* Instruction TCM — hot code (NAVHAL_ITCM), copied flash -> ITCM at reset. */
  .itcm : {
    . = ALIGN(4);
    _sitcm = .;
    *(.itcm*)
    . = ALIGN(4);
    _eitcm = .;
  } > ITCM AT> FLASH
  _siitcm = LOADADDR(.itcm);

  /* Data TCM at the base of RAM (0x20000000 = DTCM): initialized (.dtcm,
   * copied) + zero-init (.dtcm_bss). */
  .dtcm : {
    . = ALIGN(4);
    _sdtcm = .;
    *(.dtcm*)
    . = ALIGN(4);
    _edtcm = .;
  } > RAM AT> FLASH
  _sidtcm = LOADADDR(.dtcm);

  .dtcm_bss (NOLOAD) : {
    . = ALIGN(4);
    _sdtcm_bss = .;
    *(.dtcm_bss*)
    . = ALIGN(4);
    _edtcm_bss = .;
  } > RAM

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
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT> FLASH

  .bss : {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  /* Ethernet DMA descriptors + buffers (NAVHAL_ETH_RAM), in DMA-reachable SRAM
   * (see the board linker). Uninitialized: the driver populates them at init. */
  .eth_ram (NOLOAD) : {
    . = ALIGN(32);
    *(.eth_ram*)
    . = ALIGN(32);
  } > ETHRAM

  _sidata = LOADADDR(.data);
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
