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

## Adding to it

New assertions belong here only if they hold for **every** port — if it needs
a board, a bus device or a vendor quirk, it belongs in the driver tests under
`tests/portable/` or an arch white-box tier instead. See @ref testing for
where each level runs.
