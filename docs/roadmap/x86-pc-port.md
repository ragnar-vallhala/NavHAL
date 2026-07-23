@page roadmap_x86 x86-64 PC port

# x86-64 PC port

> Status: **in-progress** — Slice 1 (boot + UART) landed on `feat/x86-qemu-port`.
> Scope: a bare-metal x86-64 port of NavHAL that boots on a PC, QEMU first,
> real hardware later.
> Predecessor: M7 (modular build) — a new port is purely additive there.
> Unlocks: running NavHAL-based code on commodity x86 (SITL, companion
> computers, CI on real silicon-class targets).

## Goal

A fourth NavHAL port — peer to STM32 (Cortex-M4/M7) and ATmega328P (AVR) —
that implements the frozen `hal_*` contract on bare-metal x86-64. Application
code that includes `navhal.h` and drives the portable API should run unchanged
on a PC, exactly as it does on an MCU.

The port is validated in QEMU (`qemu-system-x86_64`) but is **not** a
simulation shim: it boots the machine, runs in long mode with no OS, and
drives real PC hardware (16550 UART, 8254 PIT, 8259 PIC, TSC). The same image
is intended to boot on physical hardware.

## Design decisions (load-bearing)

| Decision | Choice | Why |
|---|---|---|
| Word size          | **x86-64 (long mode)** | The real target is 64-bit. i386 is simpler to boot but a dead end. |
| Toolchain          | **native host `gcc`, freestanding** | No cross-compiler needed on an x86-64 host. `-m32` is *not* usable — it can't produce long-mode code. |
| Boot / load        | **multiboot2 via GRUB rescue ISO** | QEMU's `-kernel` multiboot1 loader rejects ELF64; GRUB multiboot2 loads it cleanly. `tools/qemu/run.sh` builds the ISO. |
| Layer names        | `arch=x86_64`, `vendor=pc`, `family=pc`, `board=qemu` | x86 peripherals are ISA-standard (16550/8254/8259), not chipset-specific, so one `pc` family covers them. `board=generic_pc` can join later for real hardware. |
| Interrupt controller | **legacy 8259 PIC + 8254 PIT** (not APIC/HPET) | Works on the QEMU `pc` machine and any PC with legacy support; far less code than APIC. Revisit APIC/HPET only if SMP or high-res timing is needed. |
| Peripheral scope   | **only drivers that map to real PC hardware** | A PC has no GPIO/SPI/I2C/ADC/PWM. Those stay disabled; the `NAVHAL_CONFIG_DRV_*`-gated port includes mean disabled drivers pull in nothing. |

## What is deliberately NOT in this port

- **GPIO, SPI, I2C, ADC, PWM** — a commodity PC has none of these. They stay
  Kconfig-disabled; no port drivers, no port headers.
- **VGA / framebuffer text console** — tempting (it would put text in the QEMU
  window) but **off-mission**: it fits no portable `hal_*` contract (no MCU has
  VGA), and it duplicates output the UART console already provides on serial.
  If an in-window console is ever wanted it lives as an x86-only extra, not in
  the HAL API, and only after the timing/interrupt core is done.
- **DMA, MPU, FPU-as-driver, cache, DWT, SDIO, ETH, flash** — no PC equivalent
  or no near-term need. Disabled.

## Slices

Each slice is the smallest increment that adds one portable HAL contract and is
verified end-to-end in QEMU before the next starts.

| # | Slice | HAL contract | Needs IRQs? | Status |
|---|---|---|---|---|
| 1 | Boot + UART           | `hal_uart_*` (TX, polled)                    | no  | **done** |
| 2 | Clock + timebase (polled) | `hal_clock_init`, `hal_timebase_get_micros/millis`, `hal_delay_ms/us` | no | **done** |
| 3 | Interrupts            | `hal_interrupt_*` (IDT + 8259 PIC)           | —   | next |
| 4 | Periodic timebase + timer | `hal_timebase_tick`/callbacks, `hal_timer_*` | yes | planned |
| 5 | UART RX               | `hal_uart_read_char/available/read_until`    | opt | planned |

### Slice 1 — Boot + UART  (done)

Multiboot2 `startup.s`: GRUB enters in 32-bit protected mode; the stub
identity-maps the first 1 GiB with 2 MiB pages, enters long mode, enables SSE
(the SysV float ABI uses xmm), and calls `main()`. `vendor/pc/uart/uart.c`
drives the 16550 (COM1..COM4) polled TX. Verified: prints "Hello…" over COM1.

### Slice 2 — Clock + timebase, polled  (done)

The timing core the nav stack actually needs, with **no interrupt dependency**:

- `hal_clock_init` — calibrate the TSC frequency against the 8254 PIT
  (count PIT ticks over a known interval, derive TSC Hz). Store it; `hal_clock`
  queries report it. A PC has no PLL tree to configure, so init is calibration,
  not clock-tree setup. Leave the measured value overridable (real TSC rate
  drifts from nominal — a calibration knob, not a hardcoded constant).
- `hal_timebase_get_micros` / `get_millis` — `rdtsc` scaled by the calibrated
  frequency.
- `hal_delay_ms` / `hal_delay_us` — busy-wait on the TSC.

`hal_timebase_init(tick_us)` records the tick period; the *periodic* tick and
`hal_timebase_tick()` callback need the PIT IRQ (Slice 4), so they stay stubbed
until then. The query + delay paths are fully live after Slice 2.

Verify: a sample that prints, `hal_delay_ms(500)`, prints again — measure the
wall-clock gap under QEMU.

### Slice 3 — Interrupts (IDT + 8259 PIC)

- Build and load a 256-entry IDT; ISR stubs save/restore state and dispatch.
- Remap the 8259 PIC (IRQ0–15 → vectors 32–47) so hardware IRQs don't collide
  with CPU exceptions.
- Wire the `hal_interrupt_*` contract (enable/disable/attach-callback), matching
  how the other ports expose external IRQs.

This is the prerequisite for anything periodic or event-driven.

### Slice 4 — Periodic timebase + timer

On top of Slice 3: program PIT channel 0 for a periodic IRQ0 that calls
`hal_timebase_tick()`, making `hal_timebase_get_millis` (tick-counted) and
timebase callbacks live. Expose PIT channels through `hal_timer_*`
(init/start/stop/attach_callback).

### Slice 5 — UART RX

`hal_uart_available` / `read_char` / `read_until` on the 16550 RX path — polled
first; RX-interrupt-driven once Slice 3 is in.

## Build & run

```sh
tools/qemu/build_run.sh              # configure + build + boot hal_x86_hello
tools/qemu/build_run.sh --window     # same, in a QEMU GUI window
tools/qemu/run.sh <kernel.elf>       # wrap an existing ELF in a GRUB ISO + boot
```

Kconfig target: `CONFIG_ARCH_X86_64` / `VENDOR_PC` / `FAMILY_PC` / `BOARD_QEMU`
(seeded by `cmake/toolchains/x86_64-qemu-toolchain.cmake`).

## Testing

- **Smoke, per slice:** boot in QEMU under a timeout, assert expected serial
  output on COM1 (`-serial stdio`). Slice 1 asserts the "Hello…" line.
- **Host/PIL later:** an x86 entry for the PIL harness (boot the ISO headless,
  scrape serial) mirrors the Renode/simavr dispatchers under `tools/`. Wire
  once Slice 2+ gives it something worth asserting beyond boot.
- On-target `tests/` (the Unity suite) is not wired for x86 yet; the arch
  fragment leaves the `NAVHAL_TEST_*` slots empty so a non-TEST configure is
  unaffected.

## File layout

```
cmake/arch/x86_64.cmake                     freestanding flags
cmake/toolchains/x86_64-qemu-toolchain.cmake
cmake/defconfigs/x86_64_pc_qemu.defconfig
src/arch/x86_64/                            startup.s (+ interrupt/, timebase/ to come)
src/vendor/pc/                              uart/ (+ clock/, timer/, interrupt/ to come)
src/board/qemu/                             linker.ld, board.h
include/port/x86_64/                        utils/*_types.h, navhal_port_*.h
samples/x86/00_hal_x86_hello/
tools/qemu/{run.sh,build_run.sh}
```

## Open questions

- **Real-hardware boot beyond QEMU** — same ISO boots on a USB stick via GRUB;
  a `board=generic_pc` may want UEFI rather than legacy BIOS/multiboot. Defer
  until a physical target is chosen.
- **APIC/HPET** — only if legacy PIC/PIT proves insufficient (SMP, sub-ms
  timing). Not planned.
- **Where the port lands long-term** — under M10 (port-as-package) this becomes
  a standalone port package like any other; nothing here forecloses that.
