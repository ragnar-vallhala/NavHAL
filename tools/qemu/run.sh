#!/usr/bin/env bash
# Wrap a NavHAL x86-64 ELF64 kernel in a GRUB multiboot2 rescue ISO and boot it
# in QEMU with COM1 on stdio.
#
#   run.sh <kernel.elf> [--iso-only]
#
# QEMU's own `-kernel` multiboot loader only accepts 32-bit ELFs, so a 64-bit
# kernel goes through GRUB (grub-mkrescue), which handles ELF64. Quit QEMU with
# Ctrl-A then X.
set -euo pipefail

ELF="${1:?usage: run.sh <kernel.elf> [--iso-only]}"
MODE="${2:-}"
ISO="${ELF%.elf}.iso"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

mkdir -p "$WORK/iso/boot/grub"
cp "$ELF" "$WORK/iso/boot/navhal.elf"
cat > "$WORK/iso/boot/grub/grub.cfg" <<'CFG'
set timeout=0
menuentry "NavHAL" {
    multiboot2 /boot/navhal.elf
    boot
}
CFG

grub-mkrescue -o "$ISO" "$WORK/iso" >/dev/null 2>&1
echo "ISO: $ISO"
[ "$MODE" = "--iso-only" ] && exit 0

exec qemu-system-x86_64 -cdrom "$ISO" -serial stdio -display none -no-reboot
