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
- **DMA, MPU, FPU-as-driver, cache, DWT, SDIO, ETH, flash** — no PC equivalent
  or no near-term need. Disabled.

## Screen console (VGA)

Added by request so the QEMU/PC *screen* shows console output. It is
deliberately **not** a portable `hal_*` driver (VGA has no MCU equivalent):
`src/vendor/pc/vga/vga.c` is an x86-only text-mode console (0xB8000, 80x25), and
the UART driver mirrors every transmitted character to it. So the on-screen
terminal and the serial line always show the same output, with no changes to any
sample. RX is not echoed to VGA by the driver (the echo sample writes it back
through the UART, which mirrors).

## Slices

Each slice is the smallest increment that adds one portable HAL contract and is
verified end-to-end in QEMU before the next starts.

| # | Slice | HAL contract | Needs IRQs? | Status |
|---|---|---|---|---|
| 1 | Boot + UART           | `hal_uart_*` (TX, polled)                    | no  | **done** |
| 2 | Clock + timebase (polled) | `hal_clock_init`, `hal_timebase_get_micros/millis`, `hal_delay_ms/us` | no | **done** |
| 3 | Interrupts + periodic tick | `hal_interrupt_*` (IDT + 8259 PIC); PIT IRQ0 -> `hal_timebase_tick` | — | **done** |
| 4 | General-purpose timer | `hal_timer_*` (PIT channels)                 | yes | optional |
| 5 | UART RX               | `hal_uart_read_char/available/read_until`    | opt | **done** |

Slice 3 folded in the periodic-timebase half of the original Slice 4: the tick
callback and `hal_timebase_get_tick()` are now driven by the PIT IRQ. The
remainder of Slice 4 is the general-purpose `hal_timer_*` API, which maps
awkwardly onto the 3-channel PIT (only channel 0 has an IRQ line, and the
STM32-style prescaler/auto-reload model doesn't fit) — deprioritized as
**optional** until a sample needs it. Slice 5 (UART RX) is the more useful next
step.

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

### Slice 3 — Interrupts (IDT + 8259 PIC)  (done)

- 256-entry IDT, every vector defaulting to a halt-on-fault stub (so a stray
  CPU exception freezes instead of triple-faulting into a reboot loop),
  overridden for the 16 PIC lines. ISR stubs save volatiles, 16-align the stack,
  and call the C dispatcher.
- Remap the master/slave 8259 PIC (IRQ0–15 → vectors 32–47) so hardware IRQs
  don't collide with CPU exceptions; all lines masked, `hal_interrupt_enable`
  unmasks per line; IDT load + remap + `sti` happen lazily on first enable.
- `hal_interrupt_*` (enable/disable/attach/detach-callback). The PIC has fixed
  hardware priority, so there is no settable-priority entry point.
- `hal_timebase_init` programs PIT channel 0 for a periodic IRQ0 wired to
  `hal_timebase_tick()`, so the tick counter and callback advance from a real
  interrupt.

### Slice 4 — General-purpose timer  (optional)

Expose the remaining PIT channels through `hal_timer_*`
(init/start/stop/attach_callback). Deprioritized: the STM32 prescaler/ARR shape
maps poorly onto the PIT, and the periodic tick that most code needs already
landed in Slice 3.

### Slice 5 — UART RX  (done)

`hal_uart_available` / `read_char` / `read_until` on the 16550 RX path (LSR Data
Ready + RBR), polled. `read_until` consumes the delimiter without storing it.
Sample `x86/03_hal_x86_echo` echoes a line. RX-interrupt-driven input
(`HAL_IRQ_COM1`, IRQ4) is a future extra; polled satisfies the contract.

Note: `hal_uart_init` flushes the RX FIFO (a clean receiver on init), so bytes
arriving before init are dropped — visible only when input is piped before the
guest is ready, not in interactive use.

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
