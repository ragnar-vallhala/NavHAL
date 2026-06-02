@page roadmap_m11 M11 — HAL_API_VERSION 2

# M11 — HAL_API_VERSION 2

> Status: **planned (deferred)**
> Scope: bump the public HAL contract to v2, adding subsystem
> namespaces that v1 didn't anticipate (USB, Ethernet, BLE, AI
> accelerators, secure-element).
> Predecessor: M10 (port-as-package). The version bump only makes
> sense once vendors are publishing ports independently — otherwise
> you're flipping a flag for two MCUs.
> Unlocks: ports for higher-end SoCs (STM32H7, ESP32, nRF52 with
> BLE, etc.) whose value-add is in subsystems v1 can't model.

## Goal

After M11, NavHAL can host ports for MCUs whose primary feature is
something v1 doesn't have a namespace for. Today a chip with
hardware USB OTG can only expose it as a vendor extension
(`hal_stm32_usb_*`) — which means apps that use it aren't portable
across vendors. v2 standardises the namespace.

## Why now (and why deferred until after M10)

The v1 freeze was the right call for 2025 — surface area was small,
the discipline of "never break the contract" mattered more than
shipping new namespaces. Three things change between v1 and the
shape an Arduino-scale ecosystem needs:

1. **Chips that ship soon will have USB / Ethernet / BLE as
   first-class peripherals**, not afterthoughts. RP2040 has USB.
   ESP32 has WiFi+BT. nRF52 has BLE. STM32H7 has Ethernet MAC. A
   HAL that can't speak those won't host their ports.
2. **A v1 → v2 boundary needs a clean upgrade story for existing
   ports.** That's harder while ports are in-tree (every port
   migrates simultaneously) and easier once they're packages
   (each port migrates on its own schedule, advertising
   `hal_api_version = 1` or `hal_api_version = 2` independently).
3. **The new namespaces should be designed with real ports in
   hand**, not on speculation. RP2040 and nRF52 ports landing
   through M10 will surface the actual API needs.

## What changes

### 11.1 — New subsystem namespaces

Candidate v2 additions, in rough priority order:

| Namespace               | Why it's v2-worthy                                                                       |
|---|---|
| `hal_usb_*`             | Common on modern MCUs; v1 has zero namespace for it. |
| `hal_eth_*`             | STM32H7 / RP2040 PIO-eth / ESP32 — too divergent for vendor extensions to stay portable. |
| `hal_radio_*`           | BLE / 802.15.4 / sub-GHz radios — needs a unified packet-send/receive API. |
| `hal_secure_*`          | Hardware secure-element / TrustZone / RoT — needed for any product story. |
| `hal_dsp_*` / `hal_ai_*` | Optional. Cortex-M55 / Helium / NPU support — speculative for v2.0, probably v2.x. |
| `hal_can_*`             | CAN bus — common in automotive / industrial; v1's bus drivers don't cover it. |

Each follows the same shape as existing v1 namespaces: status-returning
functions, capability gating (`NAVHAL_HAS_USB`), opt-in via Kconfig,
mirrored backend interface (M9 vtable per subsystem).

### 11.2 — Version semantics

`HAL_API_VERSION` becomes a struct rather than a single integer:

```c
#define HAL_API_VERSION_MAJOR 2
#define HAL_API_VERSION_MINOR 0
#define HAL_API_VERSION_PATCH 0
```

A port advertises the version it targets. The cap-contract is
extended with `NAVHAL_HAS_API_<NAMESPACE>` flags so apps can
guard against v2-only namespaces:

```c
#if NAVHAL_HAS_API_USB
    hal_usb_init(...);
#endif
```

Apps continue to work against v1 unchanged. v1 ports can't fulfil
a v2 cap-requirement — `nav` rejects the install with a clear
error pointing at the missing namespace.

### 11.3 — Migration path for existing ports

Port packages stay on `hal_api_version = 1` indefinitely if they
choose. Vendors who want v2 features bump independently:

```toml
[abi]
hal_api_version = 2
```

The core repo ships v2 reference implementations for the new
namespaces (against the same Cortex-M4 / STM32F4 reference
target — the F4 supports USB, so the reference port can
demonstrate the new API).

### 11.4 — Deprecation policy

v1 stays supported. No removals — additive only. The Apache 2.0
license + the public contract mean we never break working firmware.
v1.x can continue to land bug-fix releases; v2.0 is for new code.

Deprecation language goes on individual symbols when their v2
replacement is better, but the v1 symbol stays linkable.

## Alternative considered: stay on v1 forever

The honest alternative is: **don't bump.** Add USB / Ethernet / BLE
as namespaces inside v1, calling each addition a "v1 minor". This
preserves the "v1 is the contract" story and avoids the upgrade
choreography of having two versions live at once.

The case for bumping anyway:

* v1's stability promise was made with a small surface in mind. The
  new namespaces are large enough that *some* of them will need
  shape changes once real usage stresses them — and that means
  breaking changes, which v1 forbids. v2 gives a future-proofed
  envelope.
* Vendors publishing port packages need a clear signal "this port
  speaks v1, this port speaks v2." Otherwise every port has to
  advertise an exhaustive list of namespaces and apps have to check
  each one — clumsier than a single version handshake.

Recommendation: **bump.** But also: don't rush. The v1 freeze is
serving us well; v2 should land only when there's a concrete vendor
port that needs it and a working draft of the new API has been
shaken out in their hands.

## Cost estimate

Hard to pin precisely — depends on which namespaces land in v2.0
vs v2.x. A conservative v2.0 with just `hal_usb_*` + `hal_eth_*`
and the cross-cutting versioning machinery: **~3 months** of
focused work. A more ambitious v2.0 with all six candidate
namespaces: **~6–9 months**.

## Exit criteria

* `HAL_API_VERSION_MAJOR == 2` in the core repo, with at least
  one v2-only namespace fully implemented on the reference port.
* A second port (RP2040 or similar) published to the registry as
  `hal_api_version = 2`, demonstrating the dual-version-coexist
  property is real.
* The Module ABI spec has a worked example of an app pinning
  `hal_api_version = 2` and a port satisfying it.
* Cap-contract CI runs on both v1 and v2 reference builds and
  enforces that v1 symbols haven't moved.

## Open questions

* Is `hal_radio_*` even sensible as a portable abstraction? BLE
  vs 802.15.4 vs LoRa are wildly different protocols; the unified
  shape may end up being little more than "send this byte array"
  / "receive callback" — at which point the value is questionable.
  Spike before committing.
* Should `hal_usb_*` cover host mode, device mode, or both? Most
  embedded usage is device mode; host mode adds enormous
  complexity. v2.0 = device only; host in v2.x.
* What about platform-as-code things like **device-tree-style
  pinmuxing**? Zephyr's devicetree model is heavy but proven. v1
  has the per-board `linker.ld` + manual pin assignment in source.
  v2 could introduce a manifest-driven pinmux — would significantly
  simplify board ports. Probably a separate milestone (M12?).
* Where does the v2 reference test harness live? The conformance
  harness from M8.4 needs a v2-aware mode that knows which
  namespaces a port claims to implement.
