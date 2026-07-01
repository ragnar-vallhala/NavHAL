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
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  ITCM  (rwx) : ORIGIN = 0x00000000, LENGTH = 16K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 512K
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

  _sidata = LOADADDR(.data);
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
