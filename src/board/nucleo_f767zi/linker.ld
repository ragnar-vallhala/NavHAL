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
    *(.rodata*)
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
