@page contributing Contributing

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

* `arm-none-eabi-gcc` toolchain (`gcc-arm-none-eabi`, `binutils-arm-none-eabi`, `libnewlib-arm-none-eabi` on Debian/Ubuntu) — for the Cortex-M4 / STM32 port. The `pre-push` hook warns and skips the cap-contract + Cortex-M sample matrix if it's missing.
* `avr-gcc` toolchain (`gcc-avr`, `avr-libc`, `binutils-avr` on Debian/Ubuntu) — for the AVR / ATmega328P port. The `pre-push` hook warns and skips the AVR sample matrix if it's missing.

Missing either toolchain only delays failure detection to PR-time — server CI gates both arches as required checks regardless.
* `cmake` ≥ 3.20
* Python 3 with `kconfiglib` (`pip install kconfiglib`)
* `make` or `ninja`
* For host-only checks: the system `gcc` is sufficient

Optional: `st-flash` for flashing the Nucleo-F401RE, `avrdude` for flashing the ATmega328P, `renode` for PIL runs.

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
| `pre-push` | Every push. Capability contract + Cortex-M4 sample matrix + AVR sample matrix (if `avr-gcc` is installed; otherwise skipped with a warning). Skips entirely for docs-only pushes. | ~60–90 s |

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
tools/test_cap_contract.sh      # NAVHAL_HAS_* link-time gating (Cortex-M4)
tools/build_all_samples.sh      # every sample declared in Kconfig (Cortex-M4)
tools/build_all_avr_samples.sh  # every portable sample under the AVR config
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
5. **Wait for CI.** Required checks for PRs to `main`:
   * Host tests
   * Build on-target ELF
   * Capability contract
   * Build all samples
   * Build all AVR samples
   * Commit message lint
   * Full suite in Renode (PIL)
6. **Address review comments** as additional commits (don't force-push during review; squash on merge).
7. **Squash-merge** when green (`gh pr merge --squash --auto` queues the merge for when CI passes).

`main` is a protected branch — direct pushes are blocked. Every change goes through a PR.

## Promoting main → stable (release gate)

`stable` is the release branch. Promoting `main` into it requires the heavier release-gate suite to pass. Open a PR from `main` to `stable`; the per-PR jobs above all fire (`pull_request.branches` includes `stable`), **plus** the heavier checks in `.github/workflows/release-gate.yml`:

* **License header on every source file** — every `.c` / `.h` / `.cpp` / `.hpp` / `.s` must carry the Apache 2.0 header.
* **Doxygen builds warning-free** (`WARN_AS_ERROR=YES`) — strict doc-quality gate; catches broken `@ref`, undocumented public symbols, malformed groups.
* **Host tests under ASan + UBSan** — catches use-after-free / signed overflow / null deref classes of bugs that plain host tests miss.
* **Reproducible build** (soft today) — builds the test ELF twice; will become a hard gate once `-fdebug-prefix-map` / `SOURCE_DATE_EPOCH` plumbing lands.
* **`Release gate complete`** — aggregator job. Add this single name to `stable`'s branch protection `contexts` along with the per-PR check names.

## Code style

* C99/C11 for HAL sources; C++17 if you must (samples only).
* Public API: `hal_<subsystem>_<verb>(...)`, return `hal_status_t`, capability-gated symbols guarded with `#if NAVHAL_HAS_<CAP>`.
* No new global mutable state without justification in the commit body.
* Avoid drive-by reformatting in functional commits — separate cleanup commits.
* Follow `clang-format` if you have it; otherwise match the surrounding file.

See `docs/api_standardization.md` for the API contract.

## Adding a new sample

1. Decide the tier — `samples/portable/` if it must run on every supported arch, `samples/cortex-m/` if it's Cortex-M-only, `samples/no_hal/` for bare-metal "without HAL" demos. Portable samples are built on **both** the Cortex-M4 and AVR matrices in CI; everything else only on Cortex-M4.
2. Create `samples/<tier>/<NN>_<name>/main.c` and `CMakeLists.txt`.
3. Add the matching `config SAMPLE_<NN>_<NAME>` entry to `Kconfig`. **Use `select DRV_<X>` for every driver your sample uses** — relying on Kconfig defaults breaks samples for users with stripped configs. The `Build all samples` CI job will catch this. Add `depends on ARCH_CORTEX_M4` for Cortex-M-only samples.
4. Add the `default "<sample_name>" if SAMPLE_<NN>_<NAME>` line to the `config SAMPLE` block in `Kconfig`.

## Adding a new port (e.g. new MCU family)

See `docs/api_standardization.md` and the existing AVR port under `src/arch/avr/` and `include/port/avr/` for the layered structure. New ports add directories, not build-system changes — the Kconfig schema already models per-arch / per-vendor / per-family / per-board state.

Concretely, a new MCU needs:

1. Source tree: `src/arch/<arch>/`, `src/vendor/<vendor>/`, `include/port/<arch>/`.
2. Kconfig entries: `ARCH_<X>`, `VENDOR_<X>`, `FAMILY_<X>`, `BOARD_<X>` cascading defaults.
3. Build glue: `cmake/toolchains/<slug>-toolchain.cmake` and `cmake/defconfigs/<slug>.defconfig` (the toolchain file points at the defconfig via `NAVHAL_DEFCONFIG`).
4. CI job: a `Build all <arch> samples` job in `.github/workflows/ci.yml` (mirror the existing `sample-matrix-avr` job).
5. Capability matrix page: copy `docs/capabilities/_template.md` → `docs/capabilities/<board_slug>.md`, fill it in, and add a column to [`docs/capabilities/README.md`](docs/capabilities/README.md). The matrix should match what `navhal_target.h` actually emits for the target — verify with `grep NAVHAL_HAS_ build-<arch>/navhal_target.h`.
6. Test-suite portability: any test under `tests/` that pokes target-specific registers must be wrapped in `#if defined(__<arch>__)` so it compiles out on other arches. Add a branch to `tests/navtest_target.h` that maps `NAVTEST_UART` to whichever HAL UART instance the board routes to its serial console. Cap-gated tests (DMA / FPU / SDIO / DWT) compile out automatically via `NAVHAL_HAS_*`.
7. PIL (Processor-in-the-Loop) integration:
   * Add `tools/pil/boards/<board>.conf` with the toolchain file, build dir, runner script, and apt deps. See `tools/pil/boards/atmega328p.conf` as the minimal example or `nucleo_f401re.conf` as the full one (with `SETUP_SCRIPT=` for non-apt installs like Renode).
   * Pick the emulator and provide a runner under `tools/<emulator>/run_tests.sh` (mirror `tools/simavr/run_tests.sh` — it's <100 lines).
   * Add the board to the matrix in `.github/workflows/renode.yml`'s `strategy.matrix.board` list. The dispatcher `tools/pil/run.sh <board>` handles everything else.

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
