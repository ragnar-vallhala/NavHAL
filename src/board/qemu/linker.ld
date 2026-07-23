/* NavHAL linker script — bare-metal x86-64 PC (QEMU), multiboot2.
 *
 * ELF64 loaded by GRUB at 1 MiB. The multiboot2 header must appear early in
 * the image (spec: within the first 32 KiB, 8-byte aligned), so .multiboot
 * leads. BSS carries the boot page tables and stack (startup.s zeroes it).
 */

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    . = 1M;

    .multiboot : { KEEP(*(.multiboot)) }

    .text   : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    .bss :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        . = ALIGN(4);
        __bss_end = .;
    }
}
