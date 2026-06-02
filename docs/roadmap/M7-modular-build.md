@page roadmap_m7 M7 — Modular build system

# M7 — Modular build system

> Status: **done** — §7.1 (CMake fragments), §7.2 (per-port Kconfig
> fragments) and §7.3 (sample Kconfig) all landed. See "What landed"
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
work. §7.2 (per-port Kconfig fragments) was initially deferred over
one concern:

* The choice blocks in root Kconfig appeared to have interdependent
  defaults (`default FAMILY_STM32F4 if VENDOR_STM32`) that didn't
  cleanly factor into per-port files without each port knowing about
  every other port's choices.

## What landed (M7 v2 — §7.2)

§7.2 shipped, and the "interdependent defaults" blocker dissolved on
closer inspection: **those conditional defaults are redundant with the
`depends on` chain.** Each vendor `depends on` its arch, each family on
its vendor, each board on its family. So under any parent selection
exactly one child member is *visible*, and a Kconfig `choice`
auto-selects its single visible member — the `default X if PARENT`
lines never actually decided anything. Dropping them removes the
cross-port coupling entirely.

Mechanism:

* Each port axis owns fragments under its own source tree:
  * `src/arch/<isa>/Kconfig.choice` — the arch's entry in the
    "Processor Architecture" choice; `…/Kconfig` — its identity
    strings (`ARCH_ISA`, `CMAKE_SYSTEM_PROCESSOR`) and per-arch build
    defaults (toolchain prefix, flasher, flash address);
    `…/Kconfig.features` — arch-only peripheral toggles (FPU, DMA,
    DWT — Cortex-M only).
  * `src/vendor/<vendor>/Kconfig{,.choice}` and
    `src/vendor/<vendor>/family/<family>/Kconfig{,.choice}` —
    vendor / family choice entries and their `VENDOR` / `FAMILY`
    identity strings.
  * `src/board/<board>/Kconfig{,.choice}` — board choice entry and
    its `BOARD` identity string.
* The root `Kconfig` glob-sources these: each `choice` block does
  `source "src/.../Kconfig.choice"` (kconfiglib supports `source`
  inside a choice and glob patterns), and the identity/feature
  fragments are sourced outside the choices. The prompted build knobs
  (`TOOLCHAIN_PREFIX` etc.) keep their single prompt in root; each arch
  only contributes a `default … if ARCH_*`.

Verification (all four target × sample combinations):

* `tools/kconfig.py` produces the **identical** symbol set and values
  (`.config`, `config.cmake`, `autoconf.h`, `navhal_target.h`) before
  and after — the only delta is emission order, which is irrelevant to
  object-like `#define`s and `set(... CACHE)` lines.
* `hal_blink` links to a **byte-identical ELF** on both arches
  (Cortex-M4 and AVR) when built from the same path.
* Dropping a throwaway `src/arch/rv32/Kconfig.choice` makes `ARCH_RV32`
  appear in the arch choice with **zero edits to root `Kconfig`** —
  the "purely additive" goal.

Note on the original `< 50 lines` exit target: it conflicted with the
same section's "common driver caps stay here," and the latter wins. The
*platform-selection ladder* (arch / vendor / family / board, ~90 lines)
is fully externalized and no per-port option remains in root; the
common driver / feature / build menus legitimately stay inline, so the
root is ~190 lines. The scaling pain the milestone targeted — the
per-port choice ladder — is gone.

The M7 wins:
* Adding an arch is one new `cmake/arch/<ARCH_ISA>.cmake` file plus a
  fragment under `src/arch/<isa>/` — no edits to root `CMakeLists.txt`
  or root `Kconfig`.
* Adding a vendor / family / board is a fragment under its source
  tree — no edits to root `Kconfig`.
* Adding a sample is one edit to `samples/Kconfig` — no edits to
  root `Kconfig`.
