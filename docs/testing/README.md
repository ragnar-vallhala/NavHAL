# NavHAL — Testing Report

**Status:** Current as of 2026-05-21
**Companion to:** `docs/api_standardization.md`, `docs/execution_plan.md`,
`docs/m2_plus_plan.md`.

This folder is the canonical write-up of how NavHAL is tested today. It
follows the verification ladder commonly used in
safety-critical / control-systems work — **SIL → PIL → HIL** — adapted
for an embedded HAL where the firmware *is* the model.

| File | What it covers |
|------|----------------|
| [`model.md`](model.md) | The SIL / PIL / HIL test model and how NavHAL implements each level. |
| [`coverage.md`](coverage.md) | What is tested where: per-suite coverage matrix mapped onto SIL/PIL/HIL. |
| [`findings.md`](findings.md) | What the first real-hardware run exposed — one driver bug, five test-strictness fixes, two latent driver gaps. |
| [`howto.md`](howto.md) | How to build and run the suites locally + CI matrix description. |

## At a glance

- **142 tests** across **15 suites** covering the 13 standardized M2 drivers.
- **3 verification levels** in active use: SIL (host gcc), PIL (Renode in
  CI), HIL (Nucleo-F401RE).
- **0 failures** on the latest on-target run (full log in `findings.md`).
- **1 real driver bug** found by HIL: `hal_gpio_set_output_speed` shifted
  OSPEEDR by 1 bit per pin instead of 2 — fixed before this report.
- **2 latent driver gaps** flagged (NULL guards in `hal_flash_*`,
  pre-CMD17/24 NULL check in `hal_sdio_*_block`) — tests carry `TODO(driver)`
  pointers.

## Why the three levels

A single test environment cannot catch every class of bug:

| Level | Catches | Misses |
|---|---|---|
| **SIL** (host) | Algorithmic math, contract invariants, enum stability | Anything register-touching, ISA-level math, real timing |
| **PIL** (Renode) | Register-write logic, ISA integration, build-time wiring | Real-peripheral edge cases, electrical, true timing |
| **HIL** (Nucleo) | Real-silicon peripheral semantics, true timing, ISR delivery | Costly to run frequently; needs a board attached |

Together they form a pyramid: SIL runs on every commit in seconds, PIL
runs on every PR in CI, HIL runs on demand against a board. Each layer
keeps the layer below it honest.
