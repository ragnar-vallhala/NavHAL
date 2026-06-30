/*
 * Linker script — ST Nucleo-F767ZI (STM32F767ZI).
 *
 * Flash: 2 MB single bank @ 0x08000000.
 * RAM:   512 KB viewed as one contiguous block @ 0x20000000
 *        (DTCM 128 KB + SRAM1 368 KB + SRAM2 16 KB are address-contiguous
 *        from 0x20000000 to 0x20080000). The L1 D-cache is left disabled for
 *        bring-up, so no cache-coherency carve-out is needed yet.
 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 512K
}



SECTIONS {

  .isr_vector : {
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .text : {

    *(.text*)

    *(.rodata*)

  } > FLASH

  /DISCARD/ :
  {
      *(.ARM.extab* .gnu.linkonce.armextab.*)
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  }

  .data : AT (ADDR(.text) + SIZEOF(.text)) {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
    *(.init*)
    *(.fini*)
  } > RAM

  .bss : {
    _sbss = .;
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(8);
    _ebss = .;
    _heap_start = .;
  } > RAM
  _heap_end   = ORIGIN(RAM) + LENGTH(RAM);
  _sidata = LOADADDR(.data);
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
