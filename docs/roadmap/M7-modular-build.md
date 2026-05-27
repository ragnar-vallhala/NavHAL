@page roadmap_m7 M7 — Modular build system

# M7 — Modular build system

> Status: **partial** — §7.1 (CMake fragments) and §7.3 (sample Kconfig)
> landed; §7.2 (per-port Kconfig fragments) deferred. See "What landed"
> at the bottom.
> Scope: split monolithic `CMakeLists.txt` + `Kconfig` into per-port fragments.
> Predecessor: M6 (AVR port). No external dependencies.
> Unlocks: 5–15 MCUs without the build files becoming unmaintainable.

## Goal

After M7, adding a new MCU is **purely additive** at the build-system
layer — no edits to the root `CMakeLists.txt` or the root `Kconfig`.
Each port owns its arch-specific compile flags, linker plumbing, and
Kconfig schema in files under its own subtree.

## Why now

The current root `CMakeLists.txt` already has visible strain at two arches:

```cmake
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m4")
    # 12 lines of -mcpu/-mthumb/FPU flags + tests/linker.ld
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "avr")
    # 7 lines of -mmcu/F_CPU
else()
    message(WARNING "No architecture flags predefined for core '...'")
endif()
```

This pattern doesn't scale. At 5 arches it's a 60-line ladder, at 15
it's 200 lines, and every refactor risks breaking an arch you don't
build locally. The same shape repeats inside the `if(TEST)` block.

The Kconfig has the same issue: `ARCH_AVR8`, `VENDOR_MICROCHIP`,
`FAMILY_ATMEGA328P`, `BOARD_ATMEGA328P`, and every `SAMPLE_NN_*` live
in one root file. With 15 boards × 5 vendors that's hundreds of
entries in one file — merge-conflict heaven, hard to review.

## What changes

### 7.1 — Per-arch CMake fragments

Create `cmake/arch/<arch>.cmake`. Each holds the arch's compile
flags, linker flags, startup file slot, and any other arch-specific
build glue. The root `CMakeLists.txt` becomes:

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/arch/${ARCH_ISA}.cmake)
```

That single line replaces the current 60-line `if/elseif` ladder.

`cmake/arch/armv7e-m.cmake` (extracted from today's `cortex-m4` branch):
```cmake
# Cortex-M family compile + link flags
set(ARCH_C_FLAGS    "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb ${FPU_FLAGS} -O0 -g")
set(ARCH_ASM_FLAGS  "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb ${FPU_FLAGS}")
set(ARCH_LINK_FLAGS "-T ${SRC_BOARD}/linker.ld -nostdlib ${FPU_FLAGS}")
set(NAVHAL_STARTUP_FILE "${SRC_ARCH}/startup/startup.s")
```

`cmake/arch/avr.cmake`:
```cmake
set(AVR_F_CPU "16000000UL")
set(ARCH_C_FLAGS    "-mmcu=${FAMILY} -DF_CPU=${AVR_F_CPU} -Os -g")
set(ARCH_ASM_FLAGS  "-mmcu=${FAMILY}")
set(ARCH_LINK_FLAGS "-mmcu=${FAMILY}")
set(NAVHAL_STARTUP_FILE "")   # avr-libc supplies crt0
```

Same treatment for the test-build linker selection inside `if(TEST)`.

### 7.2 — Per-port Kconfig fragments

Split the root `Kconfig` so each port ships its own `Kconfig`
fragment under its source tree. The root becomes:

```kconfig
mainmenu "NavHAL Configuration System"
source "src/arch/$(ARCH_ISA)/Kconfig"
source "src/vendor/$(VENDOR)/Kconfig"
# common driver caps stay here
source "samples/Kconfig"
```

Each port's `Kconfig` fragment declares its arch / vendor / family /
board choice entries and any port-specific options (e.g. AVR's
`F_CPU` could move there).

### 7.3 — Sample tier Kconfig fragment

Pull every `config SAMPLE_NN_*` into `samples/Kconfig` (with
subdirectories for `portable/`, `cortex-m/`, `no_hal/` each `source`d
in). Adding a sample then touches one file, not the root.

## Cost estimate

Engineering: **~2 days** of careful refactoring. Mostly mechanical
extraction; the risk is breaking the implicit ordering between
Kconfig and CMake variable resolution.

Validation: full cap-contract + sample-matrix + PIL on both arches
runs unchanged.

## Exit criteria

* Root `CMakeLists.txt` has no `if(CMAKE_SYSTEM_PROCESSOR STREQUAL …)`
  ladders. Arch-specific lines all live under `cmake/arch/`.
* Root `Kconfig` is < 50 lines. Per-port options live under their
  port's tree.
* `cmake -B build -DSAMPLE=hal_blink` produces a byte-identical ELF
  to today's build (reproducibility from the release gate confirms).
* CI green on both arches (cap contract, sample matrix, PIL).

## Open questions

* Should `samples/Kconfig` be flat or sub-`source`d per tier? Flat
  is simpler; sub-sourced isolates contributors who add a sample to
  one file. Today: flat (the §7.3 implementation ships one file).
* Where does the `linker.ld` for a board live — `src/board/<board>/`
  (current) or part of a board package that the port supplies?
  Tabled — answered in M10 when "port" becomes a package.

## What landed (M7 v1)

§7.1 (CMake) and §7.3 (sample Kconfig) shipped — the high-leverage
work. §7.2 (per-port Kconfig fragments for arch / vendor / family /
board choices) was deferred because:

* The choice blocks in root Kconfig have interdependent defaults
  (`default FAMILY_STM32F4 if VENDOR_STM32`) that don't cleanly factor
  into per-port files without each port knowing about every other
  port's choices.
* Per-port fragments are most valuable when there are many ports
  pulling at the schema — at two MCUs the root Kconfig is still
  readable.

§7.2 picks back up under **M7 v2** when either:
1. A third MCU is being added and the choice-block clutter
   actually starts to hurt review, or
2. M10 (port-as-package) is in progress and ports need to ship
   their Kconfig fragment in their own repo.

Until then, the M7 v1 wins:
* Adding an arch is one new `cmake/arch/<ARCH_ISA>.cmake` file —
  no edits to root `CMakeLists.txt`.
* Adding a sample is one edit to `samples/Kconfig` — no edits to
  root `Kconfig`.
