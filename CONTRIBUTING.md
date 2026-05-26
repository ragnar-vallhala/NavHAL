# Contributing to NavHAL

Thanks for your interest in NavHAL. This document covers how to file issues, set up a dev environment, and submit changes.

By participating you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## Where to file what

| Kind | Where |
|---|---|
| Reproducible bug, concrete task | [Issues](https://github.com/ragnar-vallhala/NavHAL/issues) |
| Feature idea, design discussion, "how do I…" | [Discussions](https://github.com/ragnar-vallhala/NavHAL/discussions) |
| Security report | Email the maintainer privately (see `LICENSE.md` for contact context); please do not open a public issue |
| Code change | Pull request — see below |

Search existing issues and discussions before opening a new one.

## Development environment

You'll need:

* `arm-none-eabi-gcc` toolchain (`gcc-arm-none-eabi`, `binutils-arm-none-eabi`, `libnewlib-arm-none-eabi` on Debian/Ubuntu)
* `cmake` ≥ 3.20
* Python 3 with `kconfiglib` (`pip install kconfiglib`)
* `make` or `ninja`
* For host-only checks: the system `gcc` is sufficient

Optional: `st-flash` for flashing the Nucleo-F401RE, `renode` for PIL runs.

## One-time setup

After cloning, install the repo's git hooks:

```bash
tools/install-hooks.sh
```

This points `core.hooksPath` at `.githooks/` and installs three hooks:

| Hook | Runs | Cost |
|---|---|---|
| `commit-msg` | Every commit. Rejects messages that don't match the Conventional Commits format. | instant |
| `pre-commit` | Every commit. Host tests + cmake configure (catches Kconfig syntax / `tools/kconfig.py` breakage). | ~1–2 s |
| `pre-push` | Every push. Capability contract (`tools/test_cap_contract.sh`) + sample matrix (`tools/build_all_samples.sh`). Skips docs-only pushes. | ~30–60 s |

The same checks gate PR merges in CI, so bypassing locally with `--no-verify` only delays the rejection.

## Building

### A sample

```bash
cmake -B build -DSAMPLE=hal_blink
cmake --build build -j
cmake --build build --target flash    # if st-flash is installed and the board is connected
```

Sample names come from the `SAMPLE` Kconfig string defaults; list them with `awk '/default "/{print}' Kconfig`.

### The on-target test ELF

```bash
cmake -B build-test -DTEST=ON
cmake --build build-test --target tests -j
```

### Host-runnable test subset (no cross-compiler needed)

```bash
tools/run_host_tests.sh
```

## Running the full check suite

This is what CI runs and what `pre-push` runs locally:

```bash
tools/run_host_tests.sh         # pure-logic tests under host gcc
tools/test_cap_contract.sh      # NAVHAL_HAS_* link-time gating
tools/build_all_samples.sh      # every sample declared in Kconfig
```

A green run of all three is a strong predictor that CI will be green.

## Commit message format

Conventional Commits. Subject line:

```
<type>[(<scope>)][!]: <subject>
```

| Field | Values |
|---|---|
| `type` | `feat fix docs style refactor perf test build ci chore revert` |
| `scope` | Optional. Affected area, lowercase, e.g. `(uart)`, `(caps)`, `(samples)`. Multi-scope: `(uart,dma)`. |
| `!` | Optional. Marks a breaking change. |
| subject | ≤ 72 chars total (including type/scope/`:`), imperative mood preferred, no trailing period. |

Body (optional) goes after a blank line. Wrap at ~72 chars. Explain *why*, not *what* — the diff shows the what.

Examples:

```
feat(uart): add hal_uart_write_dma_async
fix(caps): hal_fpu_enable absent when DRV_FPU=n
docs: link CoC from README
chore!: drop AVR sub-cap fallback (breaking)
test(caps): cap-contract test, sample matrix, and git hooks
```

Enforced by `.githooks/commit-msg` locally and the `Commit message lint` job in CI on every PR.

## Pull request workflow

1. **Branch off `main`** with a descriptive name: `git checkout -b feat/uart-dma-async`.
2. **Make focused commits.** One logical change per commit; rebase to clean up WIP commits before opening the PR.
3. **Run local checks**: `pre-commit` and `pre-push` will run automatically, but you can run the full suite manually first if you want.
4. **Push and open a PR**: `git push -u origin <branch>` then `gh pr create --fill`.
5. **Wait for CI.** Five required checks gate the merge:
   * Host tests
   * Build on-target ELF
   * Capability contract
   * Build all samples
   * Commit message lint
6. **Address review comments** as additional commits (don't force-push during review; squash on merge).
7. **Squash-merge** when green (`gh pr merge --squash --auto` queues the merge for when CI passes).

`main` is a protected branch — direct pushes are blocked. Every change goes through a PR.

## Code style

* C99/C11 for HAL sources; C++17 if you must (samples only).
* Public API: `hal_<subsystem>_<verb>(...)`, return `hal_status_t`, capability-gated symbols guarded with `#if NAVHAL_HAS_<CAP>`.
* No new global mutable state without justification in the commit body.
* Avoid drive-by reformatting in functional commits — separate cleanup commits.
* Follow `clang-format` if you have it; otherwise match the surrounding file.

See `docs/api_standardization.md` for the API contract.

## Adding a new sample

1. Create `samples/<tier>/<NN>_<name>/main.c` and `CMakeLists.txt`.
2. Add the matching `config SAMPLE_<NN>_<NAME>` entry to `Kconfig`. **Use `select DRV_<X>` for every driver your sample uses** — relying on Kconfig defaults breaks samples for users with stripped configs. The `Build all samples` CI job will catch this.
3. Add the `default "<sample_name>" if SAMPLE_<NN>_<NAME>` line to the `config SAMPLE` block in `Kconfig`.

## Adding a new port (e.g. new MCU family)

See `docs/api_standardization.md` and the existing AVR port under `src/arch/avr/` and `include/port/avr/` for the layered structure. New ports add directories, not build-system changes — the Kconfig schema already models per-arch / per-vendor / per-family / per-board state.

## Licensing of contributions

By submitting a contribution, you agree that your work is licensed under the [Apache License 2.0](LICENSE.md). Apache 2.0 Section 5 makes this implicit for inbound contributions; no separate CLA is required.

Per-file header for new source files (the install script doesn't add this — copy from an existing file):

```c
/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: <your name>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * ...
 */
```
