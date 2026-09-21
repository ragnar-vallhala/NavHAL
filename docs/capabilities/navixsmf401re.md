@page cap_navixsmf401re NAVIXSM-F401RE (STM32F4)

# NAVIXSM-F401RE v0.1.0 (STM32F4 / Cortex-M4)

A custom flight-controller board on the same STM32F401RE the Nucleo carries, so
the silicon capabilities are [that page](stm32f401re.md)'s. What differs is the
*board*: this one fits the parts the Nucleo leaves off, which is why two drivers
that are merely buildable there are usable here.

| Difference from the Nucleo | Consequence |
|---|---|
| 32.768 kHz crystal on OSC32 | `hal_rtc` runs from LSE instead of falling back to LSI. `hal_rtc_get_clock()` reports `HAL_RTC_CLOCK_LSE`. |
| USB routed to a USB-C connector, PA11/PA12 | `hal_usb_cdc` enumerates as a real `/dev/ttyACM*`. |
| 8 MHz HSE crystal on PH0/PH1 | The 48 MHz USB clock comes from the HSE PLL (M=8 N=336 P=4 Q=7 → 84 MHz sysclk). |
| Debug via a bare ST-LINK/V2 | No virtual COM port, so no serial console — see *Running tests* below. |
| `LED_BUILTIN` is active **low** | `LED_ON` / `LED_OFF` in `board.h` carry the polarity so samples do not have to. |

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)   | `cortex-m4` |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `stm32` |
| Family (`NAVHAL_TARGET_FAMILY`) | `stm32f4` |
| Board (`NAVHAL_TARGET_BOARD`)   | `navixsmf401re` |
| Toolchain                     | `arm-none-eabi-` (`gcc-arm-none-eabi`) |
| Defconfig                     | [`cmake/defconfigs/cortex-m4_stm32f4_navixsmf401re.defconfig`](../../cmake/defconfigs/cortex-m4_stm32f4_navixsmf401re.defconfig) |
| Toolchain file                | [`cmake/toolchains/arm-none-eabi-toolchain.cmake`](../../cmake/toolchains/arm-none-eabi-toolchain.cmake) — shared with the Nucleo, so it seeds *that* board's defconfig; pass this board's defconfig explicitly. |
| Default sysclk                | 84 MHz via PLL. Samples needing USB drive the PLL from the 8 MHz HSE so PLLQ lands on exactly 48 MHz. |

## Board map

| Alias | Pins | Notes |
|---|---|---|
| `LED_BUILTIN` | PC0 | Active low (3V3 → 1k → LED → PC0). `LED_RGB` on PC1. |
| `BOARD_CONSOLE_UART` | USART2 | Goes to the flight-controller header, not to a debug probe. |
| `BOARD_GPS_UART` | USART6, PC6/PC7 | |
| `BOARD_PWM_TIMER` | TIM3 → PB4/PB5/PB0/PB1 | Four motor outputs. |
| `BOARD_SPI_BUS` | SPI1, CS PA4, INT1 PC13 | IMU. |
| `BOARD_LORA_SPI` | SPI2, NSS PB12, RST PB2, BUSY PA8, DIO1 PA15 | |
| `BOARD_USB_DM` / `_DP` / `_VBUS` | PA11 / PA12 / PA9 | OTG_FS device. |
| `BOARD_HSE_FREQ_HZ` | PH0/PH1 | 8 MHz crystal. |

## Capabilities

Identical to the [Nucleo-F401RE](stm32f401re.md) — same die, same drivers. The
two opt-in drivers whose hardware only this board carries:

| `NAVHAL_HAS_*` | Status | Notes |
|---|---|---|
| RTC     | ✓ | LSE-backed. `hal_rtc_init()` waits up to `lse_timeout_ms` (default 5 s) for the crystal; this one has been measured starting in 7–10 s from cold, so pass a longer timeout on a cold boot or it silently falls back to LSI. |
| USB_CDC | ✓ | Enumerates as `0483:5740`. Needs a PLL configuration whose Q output is exactly 48 MHz. |

## Running tests

There is no console UART and the ST-LINK/V2 has no virtual COM port, so the
usual "flash and read the serial log" path does not exist here. The suite is
driven over SWD instead: OpenOCD's gdb server, a breakpoint on the console
write, and the argument printed out through the debug port. That is what
`CONSOLE=swd` in [`tools/hil/boards/navixsmf401re.conf`](../../tools/hil/boards/navixsmf401re.conf)
selects.

```sh
tools/ntest hil navixsmf401re      # whole on-target suite, over SWD
```

Every console string costs a breakpoint round-trip, so this takes minutes where
a UART capture takes seconds.

The USB data path needs a host on the other end and so is not part of that
suite. Flash `34_hal_usb_cdc`, plug the USB-C port into a machine, and run:

```sh
tools/hil/usb_cdc_check.py
```

## Caveats and known limitations

* The board shares chip id `0x433` with the Nucleo-F401RE, so `st-info --probe`
  cannot tell them apart. `tools/ntest hil` disambiguates by whether the probe
  brings a VCP along, which works for these two but is not a general answer.
* LSE start-up was measured at 7–10 s from cold, which is slow for a 32.768 kHz
  crystal — worth checking the load capacitors (22 pF against a crystal wanting
  ~15 pF would explain it) before trusting the default timeout.
* CI builds this board's test ELF but nothing runs it; the hardware results in
  this page come from a local `ntest hil` run.
