@page consuming Consuming NavHAL from another project

# Consuming NavHAL

How to build an application against NavHAL from your own tree. This describes
what NavHAL guarantees **today**, as a bundled source dependency. The packaged
future — `nav board install`, a registry, `find_package` against an installed
prefix — is @ref roadmap_m10 and does not exist yet: there are no `install()`
rules in this tree.

## The contract

Add NavHAL as a subdirectory and link the `hal` target:

```cmake
cmake_minimum_required(VERSION 3.20)
project(myapp C ASM)

add_subdirectory(extern/NavHAL navhal)

add_executable(myapp src/main.c ${STARTUP_FILE})

target_link_options(myapp PRIVATE
  "-T" "${CMAKE_CURRENT_SOURCE_DIR}/extern/NavHAL/src/board/${BOARD}/linker.ld"
  "-nostdlib" "--specs=nosys.specs")

target_link_libraries(myapp PRIVATE hal)
```

That is the whole of it. You supply only what is genuinely your own choice:

| You supply | Why it is yours |
|---|---|
| `-T <board>/linker.ld` | Which board's memory map to link against. |
| `-nostdlib --specs=nosys.specs` | The bare-metal link mode. |
| `${STARTUP_FILE}` | Exported to your scope by NavHAL; it is the vector table for the selected family. |

Everything NavHAL knows about its own build arrives with the `hal` target as a
usage requirement, and you should not repeat any of it:

| NavHAL supplies | What it is |
|---|---|
| Include paths | `include/`, the port, family and board directories. |
| `-include navhal_target.h` | The generated header defining every `NAVHAL_CONFIG_*`. The public headers are written against these, so without it `#include "navhal.h"` does not compile. |
| `-L <arch>/link` | Resolves the shared section layout that each board's `linker.ld` pulls in by name. |

## Two things that are easy to get wrong

**Kconfig needs `srctree`.** NavHAL's `Kconfig` sources its fragments through
globs (`src/arch/*/Kconfig.choice`), which resolve relative to `$srctree`. A
build rooted in your project, not in NavHAL, has to say where NavHAL is:

```sh
srctree=/path/to/extern/NavHAL cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/extern/NavHAL/cmake/toolchains/arm-none-eabi-toolchain.cmake
```

Without it the configure fails with `'src/arch/*/Kconfig.choice' not found`.

**NavHAL needs a `.config`.** It is generated from a defconfig on the first
configure and is what selects the arch, vendor, family and board. Copy the
defconfig for your target into NavHAL's root before configuring, or pass
`-DNAVHAL_DEFCONFIG=<path>`, which seeds it when no `.config` exists yet.

## Why the target carries all this

It is not decoration. NavHAL's own builds get these flags from `CMAKE_C_FLAGS`
and `CMAKE_EXE_LINKER_FLAGS`, which are **directory-scoped variables**: a parent
project calling `add_subdirectory()` does not inherit them. Anything NavHAL
needs and does not attach to the `hal` target is therefore invisible to you.

That is not hypothetical. It shipped twice:

- **0.3.3** moved each board's section layout into a shared arch script that the
  board `INCLUDE`s by name, without carrying the `-L` that resolves it. `ld`
  searches the current directory and `-L` paths only — never the directory of
  the script doing the including — so a consumer's link died on
  `cannot open linker script file cortex-m4.ld`.
- **0.3.5** fixed that and left its twin standing: the force-included
  `navhal_target.h` was stranded the same way, so `#include "navhal.h"` failed
  to compile with `unknown type name 'hal_irq_t'`.

Both were invisible to CI, because every tier built NavHAL from inside the repo.
**Use 0.3.6 or later** — 0.3.3, 0.3.4 and 0.3.5 are each unusable from outside
in one way or the other.

## The check

`tools/check_consumer.sh` builds a throwaway project against the working tree
exactly as described above, for Cortex-M4 and Cortex-M7, and asserts it compiles
and links. It runs in CI as **Consumer build (out-of-tree)**, and it is the tier
that would have caught both bugs above on the pull request that introduced them.

Run it locally the same way:

```sh
tools/check_consumer.sh          # m4 and m7
tools/check_consumer.sh m4       # one arch
```

It leaves your `.config` untouched.

AVR and x86-64 consumers are not covered: they are a different link shape, with
no `-T` of a board script, so covering them means a second fixture rather than
another row in the table. The compile half of the contract is identical, so
they are worth adding as soon as anyone consumes NavHAL there.

## Using `nav`

[`nav`](https://github.com/ragnar-vallhala/nav) scaffolds all of the above.
`nav create <project>` writes the CMakeLists, resolves NavHAL into a per-user
cache at `~/.nav/navhal/<ref>` (default ref: `stable`) and generates the board
context; `nav build` sets `srctree` and seeds the `.config` for you. If you are
starting a new NavHAL application, start there rather than from this page.
