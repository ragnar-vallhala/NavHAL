MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
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
       triples {flash-src, ram-dst, byte-len}; zero pairs {ram-dst, byte-len}. */
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG(_sidata); LONG(_sdata); LONG(_edata - _sdata);
    __copy_table_end__ = .;
    __zero_table_start__ = .;
    LONG(_sbss); LONG(_ebss - _sbss);
    __zero_table_end__ = .;
  } > FLASH

  .data : {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT> FLASH

  .bss : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM AT > RAM

  _sidata = LOADADDR(.data);
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}

