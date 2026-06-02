@page roadmap_m10 M10 — Port as a registry package

# M10 — Port as a registry package

> Status: **planned**
> Scope: promote "port" to a first-class kind in the Module ABI.
> Vendors publish their MCU support as `nav` registry packages
> instead of PRs to the NavHAL monorepo.
> Predecessor: M9 (driver vtable). A port is only well-defined when
> the vendor-facing interface is stable.
> Unlocks: the strategic shift to Arduino-scale. Vendor velocity
> decouples from the maintainer; the core repo stays small.

## Goal

After M10:

* `nav board install rp2040` fetches a port package from the
  registry, extracts its `src/arch/`, `src/vendor/`, `include/port/`
  contents into a sandboxed location, registers its toolchain file
  and defconfig, and sources its Kconfig fragment.
* Existing `cmake -B build -DSAMPLE=hal_blink
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rp2040-toolchain.cmake`
  flow continues to work.
* The NavHAL core repo ships **one** reference port (Cortex-M4 /
  STM32F401RE). Every other port — including the existing AVR /
  ATmega328P — lives in its own repo, published to the registry,
  owned by whoever cares enough to maintain it.

## Why now

Once M9 lands, the vendor-facing interface (`hal_<peripheral>_ops_t`)
is stable. A port is then a well-defined deliverable:
*"a struct populated with vendor functions plus a CMake toolchain
file plus a defconfig plus a Kconfig fragment."*

That deliverable shape **is** a package. Continuing to merge every
vendor's port into the main repo at that point is friction without
upside — review bandwidth concentrates on you, the repo grows
forever, and contributors can't iterate on their port without
gating on your reviews.

This is also where NavHAL's strategic differentiation from Arduino
becomes real: Arduino's per-core repos are vendor-maintained but
mostly *unverified*. NavHAL's port packages flow through the
quarantine worker (`nav publish`) which can build them against the
conformance harness from M8.4 before marking them safe. **"Verified
ports" is a defensible market position.**

## What changes

### 10.1 — `port` kind in `navmod.toml`

Extends [Module ABI v1](../MODULE_ABI.md) with a third package kind:

```toml
[package]
kind        = "port"
name        = "rp2040"
version     = "1.0.0"
license     = "Apache-2.0"
maintainers = ["…@raspberrypi.com"]

[abi]
abi_version     = 1
hal_api_version = 1
conformance     = "tier-1"   # NEW; see §10.3

[port]
arch              = "cortex-m0"
vendor            = "rpi"
families          = ["rp2040"]
boards            = ["pico", "pico_w"]
toolchain_file    = "cmake/toolchains/arm-none-eabi-rp2040.cmake"
defconfig         = "cmake/defconfigs/rp2040_pico.defconfig"
kconfig_fragment  = "Kconfig.rp2040"

# Optional: capabilities the port claims to implement. Used by the
# registry UI to filter ports without building them.
[port.caps]
provides = ["GPIO", "UART", "I2C", "SPI", "TIMER", "PWM", "CLOCK",
            "INTERRUPT", "FLASH", "DMA", "CYCLE_COUNTER"]
absent   = ["FPU", "SDIO", "CRC_HW"]
```

The `[port]` table is new; everything else mirrors the existing
`module` / `app` shape from the ABI spec.

### 10.2 — `nav board install` flow

```
nav board install rp2040
  ↓
resolve rp2040 in registry (latest stable matching hal_api_version)
  ↓
download tarball, verify quarantine-worker signature
  ↓
extract under $NAVHAL_BOARDS_DIR/rp2040/   (~/.nav/boards/rp2040/)
  ↓
register toolchain file path in nav's config
  ↓
nav build ... -DCMAKE_TOOLCHAIN_FILE=$NAVHAL_BOARDS_DIR/rp2040/.../toolchain.cmake
```

The board files live OUTSIDE the NavHAL source tree. `cmake -B build`
sources Kconfig fragments from `$NAVHAL_BOARDS_DIR` automatically
via a new `BOARDS_ROOT` cmake variable.

### 10.3 — Conformance tiers

Not every port supports every cap; not every port has been HIL-tested.
Three tiers, descending rigour:

| Tier | Means |
|---|---|
| **tier-1: conformant**  | Passes the full conformance harness (M8.4) on a real board. HIL CI run weekly by the port's maintainer. The reference / officially-vetted level. |
| **tier-2: validated**   | Passes the conformance harness in an emulator only (no HIL). Most community ports start here. |
| **tier-3: experimental**| Builds against `HAL_API_VERSION 1` and runs Cortex-M-style smoke tests. No claim of behaviour parity with the reference port. |

The tier appears in registry listings and in `nav board show <name>`.
Apps can require a minimum tier:

```toml
# my-firmware/nav.toml
[dependencies]
ports = { rp2040 = { tier = ">= tier-2" } }
```

### 10.4 — Reference Cortex-M4 port stays in core

The STM32F401RE port stays bundled in NavHAL because it's the
reference *and* the conformance test ELF needs *something* to run
against. Every other port — including the AVR port that's currently
in-tree — migrates to its own repo (`navhal-port-avr`,
`navhal-port-rp2040`, etc.).

The migration of AVR specifically is the proof-of-concept for the
whole M10 mechanism: if AVR-as-package works end-to-end, the
remaining ports are mechanical.

## Cost estimate

* `port` kind in Module ABI + spec write-up: **~3 days**.
* `nav board install` / `show` / `list` / `remove` commands: **~2 weeks**
  (need solid UX, registry-API integration, signature verification,
  rollback on failure).
* Sandbox/extraction layout + cmake `BOARDS_ROOT` integration: **~1 week**.
* AVR migration to standalone repo (proof of concept): **~1 week**.
* Quarantine-worker integration for ports (auto-build against the
  conformance harness on publish): **~1 week**.
* Total: **~6–8 weeks** of focused work.

## Exit criteria

* `navhal-port-avr` repo exists, hosted on GitHub, published to the
  `nav` registry.
* `nav board install avr` works end-to-end on a clean machine.
* `cmake -B build -DSAMPLE=hal_blink` after `nav board install avr`
  produces the same ELF as today's in-tree AVR build.
* The Module ABI v1 spec gains a `[port]` section, mirror-committed
  to both repos.
* The quarantine worker rejects a port that doesn't pass the
  conformance harness on every supported target triple.

## Open questions

* **Lifecycle of the in-tree AVR port**: do we delete it the day
  `navhal-port-avr` is published, or keep it as a deprecated
  fallback for one release cycle? Keeping it is friendlier;
  deleting it is the honest commitment to the new model.
* **Versioning across the core ↔ port boundary**: a port pins
  `hal_api_version = 1`. If the core ships a v1.1 minor that
  introduces a new optional cap, can old ports still work? (Yes,
  since the cap is optional. But the registry needs to know the
  port's exact target HAL version.)
* **Conformance tier upgrades over time**: a community port might
  start tier-3 and reach tier-1 after months. Does the registry
  re-run conformance automatically when a new core ships? (Should.)
* **Trademark / branding**: "official NavHAL port" — who decides?
  Probably tier-1 + maintainer sign-off; needs a written policy.
* **Single-vendor lock-in risk**: if an MCU vendor publishes
  `navhal-port-x` and then stops maintaining it, can someone else
  fork? (License is Apache 2.0, so yes; registry needs a
  "successor" mechanism.)
