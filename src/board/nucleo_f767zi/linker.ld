/*
 * Linker script — ST Nucleo-F767ZI (STM32F767ZI).
 *
 * Flash: 2 MB single bank @ 0x08000000.
 * ITCM:  16 KB  @ 0x00000000 — instruction TCM, 0-wait code (see .itcm).
 * RAM:   512 KB @ 0x20000000 — one contiguous block: DTCM 128 KB + SRAM1 368 KB
 *        + SRAM2 16 KB, address-contiguous to 0x20080000. Explicit DTCM data
 *        (.dtcm / .dtcm_bss) is placed first so it lands in the 0-wait DTCM at
 *        the base of the block; ordinary .data/.bss follow. The L1 D-cache is
 *        left disabled, so no cache-coherency carve-out is needed yet.
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

SECTIONS {

  .isr_vector : {
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .text : {
    *(.text*)

    /* Reset copy/zero tables (const) walked by Reset_Handler (boot.c). Kept in
       the read-only .text load segment so no spurious RW segment appears. Copy
       triples {flash-src, ram-dst, byte-len}; zero pairs {ram-dst, byte-len}.
       F7 TCM regions are extra rows. */
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

  /* Instruction TCM — hot code (NAVHAL_ITCM), copied flash -> ITCM at reset. */
  .itcm : {
    . = ALIGN(4);
    _sitcm = .;
    *(.itcm*)
    . = ALIGN(4);
    _eitcm = .;
  } > ITCM AT> FLASH
  _siitcm = LOADADDR(.itcm);

  /* Data TCM — placed at the base of RAM (0x20000000 = DTCM). Initialized
   * DTCM data (NAVHAL_DTCM) is copied at reset; .dtcm_bss (NAVHAL_DTCM_NOINIT)
   * is zeroed. */
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
    *(.init*)
    *(.fini*)
  } > RAM AT> FLASH
  _sidata = LOADADDR(.data);

  .bss : {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(8);
    _ebss = .;
    _heap_start = .;
  } > RAM
  _heap_end   = ORIGIN(RAM) + LENGTH(RAM);
  _estack = ORIGIN(RAM) + LENGTH(RAM);

  /* Ethernet DMA descriptors + buffers (NAVHAL_ETH_RAM), in DMA-reachable SRAM.
   * Uninitialized: the driver populates the rings at init. */
  .eth_ram (NOLOAD) : {
    . = ALIGN(32);
    *(.eth_ram*)
    . = ALIGN(32);
  } > ETHRAM
}
