MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}



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
