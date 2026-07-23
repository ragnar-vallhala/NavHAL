#!/usr/bin/env bash
# Wrap a NavHAL x86-64 ELF64 kernel in a GRUB multiboot2 rescue ISO and boot it
# in QEMU with COM1 on stdio.
#
#   run.sh <kernel.elf> [--window] [--iso-only]
#
#   --window    open QEMU's graphical window (default is headless). NOTE: the
#               kernel prints to COM1, not VGA, so "Hello" still appears in the
#               terminal; the window shows the (blank) display until a VGA
#               driver lands. Needs a desktop session (DISPLAY/Wayland).
#   --iso-only  build the ISO and exit without launching QEMU.
#
# QEMU's own `-kernel` multiboot loader only accepts 32-bit ELFs, so a 64-bit
# kernel goes through GRUB (grub-mkrescue), which handles ELF64. Quit QEMU with
# Ctrl-A then X.
set -euo pipefail

ELF="${1:?usage: run.sh <kernel.elf> [--window] [--iso-only]}"
shift
ISO_ONLY=0
WINDOW=0
for arg in "$@"; do
  case "$arg" in
    --iso-only)   ISO_ONLY=1 ;;
    --window|-w)  WINDOW=1 ;;
    *) echo "run.sh: unknown option '$arg'" >&2; exit 2 ;;
  esac
done
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
[ "$ISO_ONLY" = 1 ] && exit 0

# Headless by default (serial -> this terminal); --window opens the QEMU GUI.
display=(-display none)
[ "$WINDOW" = 1 ] && display=()

exec qemu-system-x86_64 -cdrom "$ISO" -serial stdio "${display[@]}" -no-reboot
