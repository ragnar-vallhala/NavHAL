# Nav Module ABI — v1 (draft)

> Mirrored in the `nav` repo at `docs/MODULE_ABI.md`. Keep both copies
> byte-identical; contributors usually only check one and assume it's
> authoritative.

The contract that any library or application built by `nav` must satisfy. The
goal is for `nav build`, `nav add`, and `nav publish` to operate on
third-party code the same way they operate on first-party code — discoverable,
versioned, capability-declared, and reproducibly buildable across targets.

Status: **draft**. Stamp `abi_version = 1` once frozen.

---

## 1. Vocabulary

| Term | Meaning |
|---|---|
| **module** | A library. Produces a static archive that an app (or another module) consumes. No `main()`. |
| **app**    | A firmware project. Owns `main()`. Produces a flashable binary. Depends on zero or more modules. |
| **package** | The on-registry artifact. Either kind (`module` or `app`) is a package. |
| **HAL contract** | The set of stable `NAVHAL_*` macros and `hal_*` symbols a module is allowed to consume. Versioned by `HAL_API_VERSION` in NavHAL. |
| **Target triple** | `<arch>/<vendor>/<family>/<board>` resolved by `nav` from `nav.toml`'s `[target]` block. |

Apps and modules share most of this spec. Differences are called out in §11.

---

## 2. Manifest — `navmod.toml`

Every package ships a single TOML manifest at its repo root. Same on-disk
format as `nav.toml`, but the `[package]` table tells `nav` which kind it is.

### Module example

```toml
[package]
kind    = "module"
name    = "bme280"
version = "1.2.0"
license = "Apache-2.0"
authors = ["Jane Doe <jane@example.com>"]
description = "Bosch BME280 temperature / humidity / pressure driver."

[abi]
abi_version       = 1   # this spec's version
hal_api_version   = 1   # which NavHAL HAL_API_VERSION the module targets

[requires]
# All of these NAVHAL_HAS_* macros must be 1 at build time. Module
# configure fails with a clear error if any is 0.
caps = ["I2C", "TIMER"]

[requires.optional]
# Module compiles either way; the source uses `#if NAVHAL_HAS_I2C_DMA`
# to pick a fast path when present.
caps = ["I2C_DMA"]

[dependencies]
# Other nav packages this module depends on, with semver ranges.
some-helper = "^1.0"

[build]
sources       = ["src/**/*.c"]
include_dirs  = ["include"]
# Optional: a Kconfig fragment merged into the project's tree (see §6).
kconfig       = "Kconfig"
# Optional: extra CMake hook called from the generated build (see §5).
cmake_extras  = "cmake/extras.cmake"
```

### App example

```toml
[package]
kind    = "app"
name    = "my-firmware"
version = "0.1.0"
license = "MIT"

[abi]
abi_version     = 1
hal_api_version = 1

[target]
# Apps pin a target triple. Modules don't — they're built per the target the
# consuming app chose.
arch   = "cortex-m4"
vendor = "stm32"
board  = "nucleo_f401re"

[build]
sources      = ["src/**/*.c"]
include_dirs = ["include"]
entry        = "src/main.c"   # informational; nav still finds main() by symbol

[dependencies]
bme280 = "^1.2"
```

### Required fields

| Field | Module | App | Notes |
|---|---|---|---|
| `[package].kind` | "module" | "app" | required |
| `[package].name` | yes | yes | `^[a-z][a-z0-9_-]{1,63}$` |
| `[package].version` | yes | yes | semver `MAJOR.MINOR.PATCH` |
| `[package].license` | yes | yes | SPDX identifier |
| `[abi].abi_version` | yes | yes | integer, this spec's version |
| `[abi].hal_api_version` | yes | yes | integer; must equal NavHAL's `HAL_API_VERSION` at build time |
| `[target].*` | no | yes | target triple |
| `[build].sources` | yes | yes | glob list |
| `[requires].caps` | optional | optional | list of `NAVHAL_HAS_*` (sans prefix) |
| `[dependencies]` | optional | optional | name → semver range |

---

## 3. File layout

Conventional layout — nav doesn't enforce paths beyond what the manifest
points at, but published packages SHOULD follow this.

```
my-module/
├── navmod.toml
├── README.md
├── LICENSE
├── include/
│   └── my_module/              ← public headers; included as <my_module/foo.h>
│       └── foo.h
├── src/
│   ├── foo.c
│   └── internal.h              ← private headers; no consumer access
├── Kconfig                     ← optional, see §6
└── cmake/
    └── extras.cmake            ← optional, see §5
```

Consumers `#include "my_module/foo.h"` — namespaced by the module slug.
Direct includes of files from `src/` are forbidden and may be rejected by
nav's validator.

---

## 4. Capability requirements

A module declares which `NAVHAL_HAS_*` macros it consumes:

* **`[requires].caps`** — hard requirements. Build fails at configure time if
  any cap is `0` for the resolved target. Error message names the missing cap
  and which manifest line declared it.
* **`[requires.optional].caps`** — soft. Module source uses
  `#if NAVHAL_HAS_<X>` to branch.

Module source **MUST NOT** redefine any `NAVHAL_*` macro. nav validates this
by inspecting the preprocessor output.

Apps can additionally pin a Kconfig fragment via `[build].kconfig` to flip
optional caps on for the build.

---

## 5. Build interface

Modules don't ship `CMakeLists.txt`. nav generates one from the manifest at
build time. The generated rule is roughly:

```cmake
add_library(<name> STATIC ${SOURCES})
target_include_directories(<name> PUBLIC ${INCLUDE_DIRS})
target_link_libraries(<name> PUBLIC navhal)
target_compile_definitions(<name> PRIVATE
    NAV_MODULE_NAME="<name>"
    NAV_MODULE_VERSION="<version>"
)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/extras.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/extras.cmake")
endif()
```

`cmake/extras.cmake` is the escape hatch for modules that need something
unusual (linker scripts, asm flags, custom commands). It runs in the module's
scope; it MUST NOT modify global state (set parent-scope variables, add
top-level commands, etc.). Validated by nav.

Apps follow the same pattern but produce an `add_executable(<name>)` target
and inherit the project's linker/startup setup from the active toolchain
file (`cmake/toolchains/<arch>-toolchain.cmake`).

---

## 6. Optional Kconfig fragment

If a module ships a `Kconfig` file, nav appends it under a generated
`menu "<module-name>"` block inside the project's Kconfig tree before
running `tools/kconfig.py`. Module Kconfig symbols MUST be prefixed with
the module slug uppercased — e.g. `CONFIG_BME280_OVERSAMPLING_X16`.

Validation rules:

* No `mainmenu` in module Kconfig.
* No `source` statements pointing outside the module's own tree.
* All defined symbols start with `<MODULE_SLUG>_`.
* No `select` of a NavHAL `DRV_*` cap (use `[requires].caps` in the manifest
  for that; this keeps the cap requirement visible in the manifest, not
  buried in a Kconfig cascade).

The generated `navhal_target.h` exposes `NAVHAL_CONFIG_<X>` macros for every
Kconfig symbol — module sources read those, not the raw `CONFIG_*` names.

---

## 7. Lifecycle

NavHAL targets are bare-metal. There are no global constructors with side
effects (the AVR / Cortex-M startup paths don't run `__attribute__((constructor))`
or C++ static init). Lifecycle is **explicit**.

A module MAY expose:

```c
hal_status_t <module>_init(void);
hal_status_t <module>_deinit(void);
```

* `_init` MUST be idempotent (safe to call twice).
* `_init` MUST NOT touch hardware before `hal_clock_init` has returned `HAL_OK`
  — modules that depend on a clock must say so in their docs, but
  there's no automatic ordering. Apps drive init order.
* `_init` MAY register IRQ callbacks via `hal_interrupt_attach_callback`. It
  MUST NOT install handlers by raw vector-table patching.
* `_deinit` is optional; if present, MUST release IRQs and put peripherals
  in a safe state.

A module that doesn't own state can omit both.

---

## 8. Naming conventions

* All public symbols (functions, types, macros) prefixed with `<module_slug>_`
  or `<MODULE_SLUG>_`. The slug is the manifest `name` with `-` → `_`.
* Headers under `include/<module_slug>/`.
* Status-returning functions return `hal_status_t`.
* No symbol shadowing of NavHAL's `hal_*` prefix.
* C++ public APIs are wrapped in `extern "C"` unless the module manifest
  declares `[abi].language = "cpp"`.

nav's validator runs `nm -gC` on the produced archive and rejects any
external symbol that doesn't start with the module's prefix.

---

## 9. Versioning

* `[package].version` is **semver**: `MAJOR.MINOR.PATCH`.
* `MAJOR` bumps on breaking changes to the module's public API.
* `[abi].abi_version` is this spec; a different value MAY make the module
  unbuildable.
* `[abi].hal_api_version` MUST equal `HAL_API_VERSION` at build time; nav
  rejects the module if NavHAL ships a different version.
* Pre-release: `MAJOR.MINOR.PATCH-rc.N` and `-alpha.N`; treated as
  lower-precedence than the released version (semver §11).

---

## 10. Dependencies

* `[dependencies]` is a flat map of `name = semver-range`.
* Ranges: `^1.2` (>=1.2, <2.0), `~1.2.3` (>=1.2.3, <1.3.0), `>=1.2.3`,
  `=1.2.3`, `1.2.x`. Standard semver-range grammar.
* nav resolves the dependency graph with a topological sort, fails on
  cycles with a path-trace.
* Diamond conflicts (two paths request incompatible ranges) are an error.
  No "version unification" magic.
* Optional dependencies (compile only if available) are declared under
  `[dependencies.optional]`.

---

## 11. Differences between modules and apps

| | Module | App |
|---|---|---|
| `[package].kind` | "module" | "app" |
| Pins target triple | no | yes (in `[target]`) |
| Produces | static archive | flashable binary |
| Owns `main()` | no | yes |
| Drives init order | no | yes |
| Can ship its own Kconfig fragment | yes | yes |
| Can depend on apps | no | no (apps depend on modules only) |
| Linker script | from toolchain file | from toolchain file (overridable in `cmake/extras.cmake`) |

A module that defines `main()` is rejected by nav's validator.

---

## 12. Distribution

* **Source only.** No precompiled binaries are accepted on the registry.
  An embedded target is the cross-product of arch × vendor × family × board ×
  toolchain × cap-set; shipping binaries doesn't scale.
* On publish, nav uploads the source tree + manifest. The quarantine worker
  re-builds the module against each of a set of canonical target triples
  (the supported NavHAL targets) before marking the publish "safe".
* Tarballs are deterministic: file modes normalised, timestamps zeroed, sorted
  entries. The registry stores `(sha256, version)` per package.
* A `nav publish` records the source-tree hash in the manifest as
  `[abi].source_hash = "sha256:..."` so consumers can verify integrity.

---

## 13. Validation rules nav enforces

At `nav add` time:

1. Manifest parses; required fields present.
2. semver well-formed.
3. `hal_api_version` matches NavHAL at this nav install.
4. No `caps` reference an unknown `NAVHAL_HAS_*` name.

At `nav build` time:

1. All hard `[requires].caps` are 1 for the resolved target.
2. Generated static archive's external symbols all match the module prefix.
3. Module didn't redefine any `NAVHAL_*` macro.
4. `cmake/extras.cmake` (if present) only touches its own target.
5. No module defines `main()`; exactly one app does.

At `nav publish` time:

1. All of the above plus successful build against every canonical target.
2. Source tarball hash matches what's about to be uploaded.

---

## 14. Open questions

Things deliberately left out of v1 — flag for v2:

* **C++ modules**: ABI-level rules around exceptions, RTTI, template
  instantiation. Today `extern "C"` is required.
* **Trait / interface protocol**: a module that says "I implement
  hal_sensor v1" so apps can swap drivers (BME280 vs SHT30). Useful but
  needs a separate interface registry.
* **Runtime plugin loading**: not in scope. Embedded targets don't have
  dynamic linkers.
* **Cross-module IRQ priority arbitration**: today apps own IRQ priorities.
  A future "module declares max latency, nav assigns priorities" pass is
  out of scope.
* **License compatibility checks**: nav records SPDX ids but doesn't refuse
  GPL-into-Apache today. Could be a quarantine-worker rule.
* **Reproducible binary builds across machines**: today reproducibility is
  best-effort; we don't pin compiler version per module.
