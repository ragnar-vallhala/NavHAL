@page cap_navixdev NAVIXDEV — flight controller v0.0.3 (STM32F4)

# NAVIXDEV (STM32F4 / Cortex-M4)

The development revision that preceded the @ref cap_navixsmf401re production
board. Same STM32F401RE, so the silicon capabilities are
[that page](stm32f401re.md)'s — but it is a **breakout board, not an earlier cut
of the same flight controller**. Sensors sit on pin headers rather than on the
PCB, there is no USB and no LoRa, and twelve PWM outputs are brought out.
Firmware built for the production board does not run here unchanged.

It exists as its own board because the hardware exists: v0.0.3 units are still
on the bench, and a board a HIL run can be pointed at needs an identity the
build system understands.

## Difference from the production board

| | v0.0.3 (this board) | v0.1.0 |
|---|---|---|
| IMU | BMX160 module on a 7-pin header, on **I2C1** | ICM-42688-P on **SPI1** (`PA4`..`PA7`) |
| IMU interrupts | `PC0`, `PC1` | `PC13` |
| Barometer | BMP180 module header, on I2C1 | DPS368 on I2C1 |
| I2C1 pins | `PB8` / `PB9` only | `PB6` / `PB7` *and* `PB8` / `PB9` |
| Indicator | RGB LED on `PB12`/`PB13`/`PB14`, **active high** | LED on `PC0` active low, WS2812 on `PC1` |
| Buzzer | `PB15` | `PA0` |
| LoRa | absent | SPI2 on `PB12`..`PB15` |
| USB | **absent** — `PA12` unrouted, `PA11` is a PWM pin | OTG-FS on `PA11`/`PA12` |
| PWM outputs | 12, on three headers | 4 motor outputs |
| Magnetometer / power monitor | absent | IST8310, INA226 on I2C1 |

Unchanged between the two: console on USART2 (`PA2`/`PA3`), GPS on USART6
(`PC6`/`PC7`), micro-SD on SDIO (`PC8`..`PC12`, `PD2`, CD on `PC5`), SWD, the
8 MHz HSE crystal, and the four TIM3 motor pins.

### PWM headers

Three 4-pin headers, one whole timer each:

| Header | Timer | Pins |
|---|---|---|
| PWM1 | TIM1 CH1..CH4 | `PA8` `PA9` `PA10` `PA11` |
| PWM2 | TIM5 CH1..CH4 | `PA0` `PA1` `PA2` `PA3` |
| PWM3 | TIM3 CH1..CH4 | `PB4` `PB5` `PB0` `PB1` |

`BOARD_MOTOR1`..`4` are PWM3 (`BOARD_MOTOR_TIMER`), the same pins the production
board drives its motors on. PWM2 channels 3 and 4 are `PA2`/`PA3`, which are also
the console UART, so those two cannot both be in use. `BOARD_GP_TIMER` is TIM4
here rather than the production board's TIM5, which PWM2 occupies.

`BOARD_PWM_TIMER` / `BOARD_PWM_CHANNEL` / `BOARD_PWM_PIN` — the trio the portable
PWM sample drives — point at PWM1 channel 1 (`PA8`, TIM1, AF1) so a sample never
drives a motor line.

@warning The RGB LED's anodes are driven straight from the GPIOs with no series
         resistors on this revision. Push-pull output at 3V3 exceeds the LED's
         forward-current rating — drive it briefly, use open-drain, or PWM it at
         a low duty.

## Pins routed nowhere

Ten pins carry a net label on the v0.0.3 schematic but reach no connector:
`PA12`, `PA15`, `PB2`, `PB6`, `PB7`, `PB10`, `PC2`, `PC3`, `PC4`, `PC13`. They
are not aliased in `board.h`.

The six pins v0.1.0 leaves unconnected and v0.0.3 does not are `PA1`, `PA10`,
`PB10`, `PC2`, `PC3`, `PC4`. Of those only two are genuinely usable here: `PA1`
(PWM2 CH2) and `PA10` (PWM1 CH3). The other four are in the unrouted list above.

### How this map was derived, and what that is worth

From the **netlist** in `v0.0.3.kicad_pcb` — every MCU pad resolved to the other
component pads on its net — not from the schematic's net *labels*.

That distinction matters, because net labels are wrong in both directions here.
v0.0.3 labels every port pin `PA0`, `PB10`, … whether or not it reaches a
connector, so a label is not evidence of a connection. And v0.1.0 leaves the
user LED's net unnamed: `PC0` connects to `Net-(LED1-K)` and appears in no label
list at all, so absence from the labels is not evidence of no connection either.
Comparing the two revisions' label sets suggests the boards are near-identical.
Comparing their netlists shows they are not. Use the netlist.

## Target identity

| | |
|---|---|
| Arch (`NAVHAL_TARGET_ARCH`)   | `cortex-m4` |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `stm32` |
| Family (`NAVHAL_TARGET_FAMILY`) | `stm32f4` |
| Board (`NAVHAL_TARGET_BOARD`)   | `navixdev` |
| Toolchain                     | `arm-none-eabi-` (`gcc-arm-none-eabi`) |
| Defconfig                     | [`cmake/defconfigs/cortex-m4_stm32f4_navixdev.defconfig`](../../cmake/defconfigs/cortex-m4_stm32f4_navixdev.defconfig) |

## Running tests

```
bash tools/hil/run.sh navixdev
```

Debug is a bare ST-LINK/V2, which carries no virtual COM port, and this board
has no USB device port of its own, so the suite is driven over SWD
(`CONSOLE=swd`). Every console string costs a breakpoint round-trip, which is
why the timeout is 600 s rather than the Nucleo's seconds.

A USB-serial adapter on the UART1 header (`3V3`, `GND`, `PA2` TX, `PA3` RX) is
the faster console if one is to hand. The board config is
`tools/hil/boards/navixdev.conf`.
