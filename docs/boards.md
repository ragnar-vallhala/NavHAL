@page boards Describing a board

# Describing a board

A board description says how a PCB is wired. It lives in Kconfig, and
`tools/kconfig.py` turns it into the `board.h` that code includes, so there is one
place a pin is stated and one place to correct it.

There are three ways to get the board you need, in increasing order of how much you
take on.

## 1. Use an in-tree board

`src/board/<slug>/` holds the boards this repo describes, each with a
`Kconfig.choice` (declaring its `BOARD_*` symbol), a `Kconfig` (the description), a
linker script, and sometimes `startup.s` or a `boot/` directory. Select one with its
defconfig:

```sh
cmake -B build -DNAVHAL_DEFCONFIG=cmake/defconfigs/cortex-m4_stm32f4_navixdev.defconfig \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake
```

**The values are not overridable from your `.config`.** Every `PIN_`/`NUM_`/`VAL_`
symbol is declared with a bare type and a default and no prompt, and a promptless
Kconfig symbol is not user-assignable: the assignment is read and discarded. That
used to be documented as the opposite. If a pin here is wrong for the hardware in
front of you, the board description is wrong -- fix it, rather than working around
it. `tools/kconfig.py` now prints a line naming any assignment that did not take, so
this fails loudly instead of quietly.

## 2. Bring your own board, out of tree

Your PCB does not have to live in this repo. Point the build at a directory holding
the same things an in-tree board holds:

```sh
cmake -B build -DNAVHAL_BOARD_DIR=../my-boards/widget \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake
```

The directory needs at least:

| file | what it does |
|---|---|
| `Kconfig.choice` | declares `config BOARD_<NAME>`, which is what selects the board |
| `Kconfig` | the description: `PIN_*`, `NUM_*`, `VAL_*` symbols, each `default ... if BOARD_<NAME>` |
| `linker.ld` | the memory map, unless the arch supplies one |
| `startup.s`, `boot/` | optional, same role as in-tree |

Naming the directory *offers* the board; it does not select it. Put
`CONFIG_BOARD_<NAME>=y` in your `.config` (or a defconfig) as well. A missing or
malformed directory is refused at configure time rather than silently falling back
to an in-tree board.

## 3. Adapt a board that already exists

```sh
tools/derive_board.py --from navixdev --name navixdev_r4 --out ../my-boards/r4
```

That copies the base description, renames its symbol throughout, and brings the
linker script with it. Edit what differs on your hardware and build it as in (2).

**It is a copy, not an overlay, and that is deliberate.** Kconfig gates every default
on `if BOARD_<NAME>`, and within a `choice` exactly one board is selected, so there is
no condition under which a derived board would pick up its base's values. Real
inheritance would mean rewriting every condition in every board onto an alias symbol
that a derived board could select -- a thin override file bought at the cost of a far
less readable description everywhere. The trade a copy makes is the opposite one:
your description is complete and readable, and improvements to the base do not reach
you. For a board revision that moves two pins, copy. For a board that is really the
same board, fix the original.

## What belongs in a description, and what does not

Describe what is wired. An undescribed peripheral is absent from the driver's pin
table -- the SPI driver guards each instance on `#if defined(BOARD_SPI2_SCK)` -- and
that absence is how a description says "not wired". Declaring a peripheral the PCB
does not route is worse than leaving it out, because the numbers look authoritative:
navixdev once declared SPI2 on the three pins carrying its RGB LED and buzzer, and
PWM2 on the pins carrying its own console UART.

If you have not traced it, do not write it down.

## See also

- @ref capabilities -- what each supported MCU and board actually provides.
- @ref consuming -- building an application against NavHAL from your own tree.
