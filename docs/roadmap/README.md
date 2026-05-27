@page roadmap NavHAL Roadmap

# NavHAL — roadmap to Arduino-scale

> Scope: this folder is the engineering roadmap for taking NavHAL from
> a 2-MCU bundled monorepo to a port-package ecosystem supporting tens
> to hundreds of MCUs. Each milestone is its own page below.

## Vision

Arduino's reach comes from two things NavHAL doesn't have yet:

1. A **per-core distribution model** — vendors publish their MCU support
   independently; the platform doesn't have to merge every vendor PR.
2. A **library catalog** with tens of thousands of pre-vetted modules.

NavHAL's structural answer to (1) is the `nav` ecosystem (CLI +
registry + quarantine worker) — most of the *infrastructure* exists
already; what's missing is the data model for "a port is a package."
The answer to (2) is the [Module ABI](../MODULE_ABI.md) spec, which
already covers `module` and `app` kinds; a third `port` kind closes
the loop.

This roadmap is the work to get there, factored into five milestones.

## Where we are today (post-M6)

| | |
|---|---|
| Supported MCUs                       | 2 — STM32F401RE (Cortex-M4) · ATmega328P (AVR) |
| Built-in CI gates                    | Host tests, cap-contract, sample matrices (both archs), PIL (Renode for Cortex, simavr for AVR), Conventional Commits, release gate on `main → stable` |
| Per-MCU PIL dispatcher               | `tools/pil/run.sh <board>` + `tools/pil/boards/<board>.conf` |
| Distribution model                   | bundled monorepo |
| HAL API version                      | `HAL_API_VERSION 1` — frozen |
| Module ABI                           | v1 draft (`module` + `app` kinds) |

## The plan

| Milestone        | Status      | Scope                                       | Unlocks                                              |
|---|---|---|---|
| @ref roadmap_m7  | partial — v1 done, v2 deferred | Modular build system | 5–15 MCUs without CMakeLists/Kconfig becoming a swamp |
| @ref roadmap_m8  | **done**    | CI tiering + portable test framework        | Per-arch CI scaling; HAL-only tests run on every arch |
| @ref roadmap_m9  | planned     | Driver vtable / vendor-backend abstraction  | ~80 % less per-vendor boilerplate; conformance enforced by interface |
| @ref roadmap_m10 | planned     | Port as a registry package                  | Strategic shift away from monorepo. Vendors publish ports independently. |
| @ref roadmap_m11 | planned     | `HAL_API_VERSION 2`                         | Subsystem namespaces v1 couldn't anticipate — USB, Ethernet, BLE, AI accelerators |

The first three (M7–M9) are pure engineering on the current monorepo.
The last two (M10–M11) are the strategic shifts that take NavHAL
from "thoughtful HAL for a handful of MCUs" to "Arduino-scale".

## Dependency graph

```
   M6 (done)
    │
    ▼
   M7 ── M8
    │     │
    ▼     ▼
   M9 ──┐
        │
        ▼
       M10 ── M11
```

* **M7 first** — fragmenting the build is a no-regret refactor that
  every subsequent milestone benefits from.
* **M8 in parallel** with M7 — independent concerns; can interleave.
* **M9 needs M7** — the vtable refactor will touch a lot of
  per-vendor code; cleaner if the build is already modular.
* **M10 needs M9** — a "port package" is well-defined only when the
  port-vendor interface is clean. Otherwise every port package has to
  ship its own snowflake build glue.
* **M11 needs M10** — bumping the API to v2 means promising stability
  on the new subsystems; better to do that once vendors are publishing
  independently and the contract is exercised across many ports.

## Strategic shape — bundled vs port-package

The single biggest question this roadmap answers: **should NavHAL
remain a monorepo (Zephyr model) or split into core + per-port packages
(Arduino-cores / PlatformIO-platforms model)?**

Both are viable engineering shapes. Trade-offs:

| | Bundled monorepo (Zephyr-style) | Port packages (Arduino-cores-style) |
|---|---|---|
| Maintainer effort per new MCU                | Reviews + merges every vendor PR | Optional approval at registry submission |
| Repo size after 50 MCUs                      | Several GB; slow clones | Core stays small; ports clone on demand |
| Cross-port refactoring                       | One atomic PR | Coordinated PRs across N repos (painful) |
| Conformance testing                          | One test matrix in one CI | Per-port CI + a central conformance suite |
| Vendor velocity                              | Bottlenecked on you | Independent |
| User onboarding                              | `git clone navhal` is everything | `nav board install rp2040` extends the install |

M10 is where the answer gets committed. Before M10, NavHAL stays
monorepo. After M10, the AVR (or RP2040, ESP32, …) ports could move
to standalone repos under the `nav` registry — and the core repo
becomes the reference Cortex-M4 port + the HAL contract + the test
framework. **The architecture allows either; M7–M9 don't foreclose
M10.** That's the property to preserve.

## Open strategic questions

* Should the v1 → v2 boundary on `HAL_API_VERSION` ever happen, or is
  v1 going to live forever and new capability namespaces become
  `NAVHAL_HAS_USB` extensions inside v1? Answer affects M11's shape
  (whether it's "promote a flag" or "ship a new HAL version").
* When ports become packages (M10), where does the **conformance
  suite** live? Per-port repos would each carry the relevant subset;
  the core repo could ship a "conformance harness" that pulls them
  all and runs them together.
* `HAL_API_VERSION 1` was frozen on the assumption of small surface.
  At ten ports, every accidental contract violation in a port
  becomes a real-world bug — needs stricter conformance gating
  before M10.
* Where does the **manifest data** for "this port supports caps X,
  Y, Z" come from? Today it's `navhal_target.h` generated by
  Kconfig. In the port-package world, that needs to be in
  `navmod.toml` so `nav add port-x` can answer "does this port have
  USB?" without building it first.

## Non-goals

* RTOS adoption (FreeRTOS / Zephyr-kernel integration) — orthogonal
  to scaling MCU count; revisit only if a downstream needs it.
* Replacing `nav` with PlatformIO or another existing ecosystem —
  the comparison was made deliberately; NavHAL's discipline only
  pays off if `nav` is the loader.
* Browser-based IDE / cloud build — Arduino has it; NavHAL doesn't
  intend to compete on that axis.

## How this file evolves

Each milestone page tracks its own status (planned / in-progress /
done) at the top. When a milestone lands, its `[ ]` in the table
above gets ticked and its status flips. New milestones append; we
don't renumber.
