@page cap_x86_64_qemu x86-64 PC (QEMU)

# x86-64 PC (QEMU)

A bare-metal x86-64 port, booted by GRUB through multiboot2 and normally run under
QEMU. It exists to prove the HAL's seams are not Cortex-shaped: the interrupt
controller is an 8259 PIC rather than an NVIC, there is no memory-mapped peripheral
bus to speak of, and the timing surface is a programmable interval timer instead of
SysTick.

Its capability set is therefore small on purpose, and the small set is the point.
Only five port contracts exist here, and Kconfig now refuses the rest with the
reason printed rather than failing later on a missing header.

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)   | `x86_64` |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `pc` |
| Family (`NAVHAL_TARGET_FAMILY`) | `pc` |
| Board (`NAVHAL_TARGET_BOARD`)   | `qemu` |
| Toolchain                     | the host compiler, no cross prefix |
| Defconfig                     | [`cmake/defconfigs/x86_64_pc_qemu.defconfig`](../../cmake/defconfigs/x86_64_pc_qemu.defconfig) |
| Toolchain file                | [`cmake/toolchains/x86_64-qemu-toolchain.cmake`](../../cmake/toolchains/x86_64-qemu-toolchain.cmake) |
| Default sysclk                | the TSC, calibrated at runtime against PIT channel 2 over a ~40 ms one-shot (the 8254 input clock is 1,193,182 Hz). There is no fixed figure to quote: the port measures it. |

## Capabilities

| `NAVHAL_HAS_*` | Status | Driver | Notes |
|---|---|---|---|
| INTERRUPT         | ✓ | `src/arch/x86_64/interrupt/`, `src/vendor/pc/interrupt/` | 8259 PIC, not an NVIC. The port owns the EOI, which is why `dispatch` is a per-port op and not a loop in the common layer. |
| CLOCK             | ✓ | `src/vendor/pc/clock/clock.c`     | Reports a measured TSC frequency; nothing to reconfigure. |
| TIMEBASE          | ✓ | `src/vendor/pc/timer/timebase.c`  | PIT-backed millisecond tick. This is the port's whole timing surface. |
| UART              | ✓ | `src/vendor/pc/uart/uart.c`       | 16550 on the legacy COM ports. |
| TIMER             | ✗ | — | `navhal_port_timer.h` exists but is intentionally empty, and no `_hal_timer_ops` is defined. `CONFIG_DRV_TIMER=y` is refused here; use `hal_timebase_*`. |
| CRC               | ✗ | — | No `navhal_port_crc.h`, so even the software CRC-32 fallback cannot be reached. Refused at configure time. |
| GPIO, SPI, I2C, ADC, PWM, FLASH | — | — | No such peripheral to drive on a PC, and no port header. Refused at configure time. |
| DMA, DWT, FPU, MPU, CACHE, TCM, SDIO, RTC, USB_CDC, ETH, WATCHDOG, RESET | — | — | Cortex-M or STM32 blocks; gated off this arch in Kconfig. |

## Extras this port carries that no other does

`src/vendor/pc/vga` and `src/vendor/pc/ps2` back the samples' console and keyboard.
Neither is a `hal_*` contract -- they are board-support code for a PC, in the same
sense that a Nucleo's LED pin is.

## Samples

[`samples/x86/`](../../samples/x86): hello, timing, interrupt and echo. These are the
tier the `x86-64 QEMU smoke` CI job runs.
