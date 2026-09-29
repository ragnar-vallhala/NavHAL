@page conformance Conformance contract

# Conformance contract

> The definition of "this port implements NavHAL correctly."
> Tests: `tests/portable/conformance/`. Checklist: @ref api_standardization
> §14. Consumers: @ref roadmap_m9, @ref roadmap_m10, @ref roadmap_m11.

## What it is

A driver test asks "does SPI move bytes on this board?" A conformance test
asks "does this port honour the contract every `hal_*` caller is written
against?" The second question is the one that has to be answerable without a
board on the desk and without knowing which vendor wrote the port.

`tests/portable/conformance/test_conformance.c` holds those assertions. They
are mechanical and contract-level, which is what makes them portable by
construction: they gate on `NAVHAL_CONFIG_DRV_*` and compile against the
public headers only, so the same file runs on every port with no per-target
variants.

## What it asserts

The tests are the executable half of the per-driver checklist in
@ref api_standardization §14. The checklist is the prose statement; this tier
is the part a machine can enforce:

* `hal_<p>_init` accepts `const hal_<p>_config_t *` and returns `HAL_OK` on a
  first call, an `HAL_ERR_*` on a second call with a conflicting config.
* Every fallible entry point returns `hal_status_t`; data leaves through an
  out-pointer, never a sentinel return value.
* Instances are addressed by a typed id enum, not a bare `uint8_t`.
* Buffers cross the boundary as `(ptr, len)`; timeouts are milliseconds.
* The `NAVHAL_CONFIG_DRV_*` gates a port claims match the symbols it actually
  exports — a port cannot advertise a capability it does not implement.

Items in §14 that no test can reach — "the public header is declarations-only
with no target `#ifdef`s" — stay checklist-only and are enforced at review.

## Why it is load-bearing

This tier is the gate, not a nicety. @ref roadmap_m10 turns ports into
independently published packages, at which point nobody reviews a vendor's
port before users install it; passing this suite becomes the difference
between a tier-1 port and an unvetted one. @ref roadmap_m9 enforces the same
contract structurally through the driver vtable, and @ref roadmap_m11 will
need a v2-aware mode that knows which namespaces a port is claiming.

A port that compiles, links and blinks an LED but fails this tier is not a
port. That is the whole point of writing the contract down.

## How much of the surface it covers

The number is measured rather than remembered:

```sh
tools/conformance_coverage.py          # the counts, and what is missing
tools/conformance_coverage.py --list   # every entry point and its state
tools/conformance_coverage.py --gate   # what CI runs
```

A **public entry point** is a `hal_*` function declared in `include/common/`:
the surface a port has to implement. The inline hot paths under `include/port/`
are out — those are a vendor's own accessors, not a shared contract. **Covered**
means this suite calls it; another tier calling it does not count, because the
question is whether a *new port* would be caught getting it wrong, and only this
suite runs everywhere.

Everything else is declared in
[`tests/portable/conformance/uncovered.txt`](../../tests/portable/conformance/uncovered.txt),
one line per entry point, in two kinds:

* **cannot be covered** — the call ends the run (`hal_system_reset`), destroys
  what the run needs (`hal_flash_erase`), or waits on hardware a bare board does
  not have (`hal_sdio_wait_flag`). Six of these.
* **debt** — reachable from a bare board, nobody has written the case yet.
  Delete the line when the case lands. Most of this debt *is* exercised by its
  `DRV_*` cap suite under `tests/cap/`; what it lacks is the portable check that
  a different port implements it the same way.

The gate deliberately enforces no threshold — a number invites picking one that
passes. It fails on three things: an entry point that is neither covered nor
declared, a declared line the suite has since started calling (a stale excuse),
and an ops table under `include/internal/` with no completeness case in
`test_vtable.c`. That last one is the vendor half: `test_vtable.c` walks a
table's bytes, so a table that *grows* an entry is covered the day it grows, but
a table that is newly *added* is covered only when someone writes its case — and
before this, nothing noticed if they did not.

## Adding to it

New assertions belong here only if they hold for **every** port — if it needs
a board, a bus device or a vendor quirk, it belongs in the driver tests under
`tests/portable/` or an arch white-box tier instead. See @ref testing for
where each level runs.
