/*
 * Test-ELF linker script — Cortex-M7 / STM32F767ZI.
 *
 * Mirrors tests/arch/cortex-m4/linker.ld but with the F767 memory map
 * (2 MB flash, 512 KB contiguous SRAM @ 0x20000000). Used by the TEST build
 * via NAVHAL_TEST_LINKER_FLAGS in cmake/arch/armv7e-m.cmake.
 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
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

  .data : AT (ADDR(.text) + SIZEOF(.text)) {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM

  .bss : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  _sidata = LOADADDR(.data);
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
