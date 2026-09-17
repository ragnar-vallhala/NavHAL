# NavHAL — Session Handoff

**Branch:** `feat/stm32f767zi-port` (all work local, **nothing pushed**)
**Working tree:** clean
**Focus of recent sessions:** STM32F767ZI / Cortex-M7 bring-up — MPU, a hardware
test rig, cache, TCM, and DMA-backed driver parity with the F4.

---

## TL;DR — what to do next

1. **Hardware-validate the remaining DMA backend that couldn't be validated yet**
   (needs extra hardware on the bench):
   - **I²C + I²C DMA** — ✓ DONE, both F767 cells flipped to `✓`. The base
     transfer FSM is HW-exercised device-free (`test_i2c_transfer_fsm_nacks_absent_device`:
     addressing reserved 0x7C returns NACK/`HAL_ERR_IO`, not a timeout). A
     successful DMA read of a device's fixed-id register was confirmed during
     bring-up (observed the id byte via DMA on I²C1); the device-wired exercise
     lives in the sample, not the suite. **Committed HIL tests stay device-free —
     nothing extra must be attached to the bench.**
   - **SDIO DMA** — attach an SD card breakout to the SDMMC1 pins and validate
     the `hal_sdio_*_async` path. A ready-made async round-trip test was
     prototyped and removed (see `git show 010efa3` discussion); re-add it under
     `#if NAVHAL_CONFIG_DRV_SDIO_DMA` + `NAVTEST_PIL_ONLY` in
     `tests/cap/sdio/test_sdio.c`. Then flip `SDIO_DMA` F767 `◐` → `✓`.
2. **Continue with additional F7 drivers** later (see "Roadmap" below).
3. When ready, `git push` the branch — it has never been pushed.

---

## Hardware bench (both boards connected via USB/ST-Link)

| Board | Core | chipid | ST-Link serial | Console port | Console USART | Baud |
|-------|------|--------|----------------|--------------|---------------|------|
| Nucleo-**F767ZI** | Cortex-M7 | `0x451` | `0672FF485570854967113446` | `/dev/ttyACM0` | USART3 (VCP) | 9600 8N1 |
| Nucleo-**F401RE** | Cortex-M4 | `0x433` | `0668FF3332504E3043232724` | `/dev/ttyACM1` | USART2 (VCP) | 9600 8N1 |

The on-target test firmware's console baud is hardcoded **9600** in
`tests/main.c:140`.

### How to run tests
- **HIL (real hardware)** — `tools/hil/run.sh --all` (or `<board>` / `--list`).
  Matches each board to its ST-Link by **chip-id** (portable — no hard-coded
  serials), builds the capped test ELF, flashes, captures the console. Board
  configs: `tools/hil/boards/*.conf`. Legacy single-board helper:
  `tools/hil/run_target_tests.sh`.
- **PIL (Renode emulator)** — `tools/pil/run.sh <board>`. Configs:
  `tools/pil/boards/*.conf`; Renode scripts: `tools/renode/`.
- **Host (SIL)** — `cmake -B build-host -S tests/host && cmake --build
  build-host && ./build-host/tests_host_drivers` (also run by the pre-commit
  hook).

**Last full HIL run: F401 159/0, F767 167/0, all green.**

---

## What shipped this session (commits, newest first)

```
ed99739 feat(i2c):  DMA-backed register read ported to F7      [◐ unvalidated]
010efa3 feat(sdio): DMA async API compiled on F7               [◐ unvalidated]
36d593a feat(uart): DMA-backed UART API ported to F7           [✓ HW-validated]
dc47317 docs: mark TCM supported, not partial
1e9ec8a feat(tcm):  explicit ITCM/DTCM placement attributes    [✓ HW-validated]
de44cfe test(fpu):  hardware-vs-soft-float comparison (not a fixed cycle cap)
dd198a4 feat(cache): hal_cache — Cortex-M7 L1 I-cache (Phase 1) [✓ HW-validated]
e4d5280 test(hil):  board-driven hardware runner (tools/hil)
55e776b fix(dwt):   unlock DWT so cycle counter runs on M7      [✓ HW-validated]
965d904 fix(navtest): unlock DWT so M7 is detected as HIL, not PIL
479926f feat(mpu):  make hal_mpu a consumable capability        [✓ HW-validated]
        (+ earlier: MPU driver, gating migration, docs, changelog)
```

### New capabilities and where they live
- **MPU** — `hal_mpu` (`src/arch/armv7e-m/mpu/mpu.c`, `include/common/hal_mpu.h`),
  `DRV_MPU`. HW-validated on M4 (8 regions) and M7 (16 regions).
- **Cache** — `hal_cache` (`src/arch/armv7e-m/cache/cache.c`,
  `include/common/hal_cache.h`), `DRV_CACHE` (M7-only). **I-cache only** —
  `hal_icache_enable()`. Call it first in `main()` (see `tests/main.c`).
- **TCM** — placement attributes in `include/common/hal_tcm.h`: `NAVHAL_ITCM`
  (code), `NAVHAL_DTCM` (init data), `NAVHAL_DTCM_NOINIT` (zeroed buffers).
  `USE_TCM` (M7-only). Linker regions + startup copy in
  `src/board/nucleo_f767zi/linker.ld`, `tests/arch/cortex-m7/linker.ld`,
  `src/board/nucleo_f767zi/startup.s`. **Both linkers must stay in sync** —
  they share the F767 startup that copies `.itcm`/`.dtcm`.
- **DMA backends (F7)** — `DRV_UART_DMA` (default-on M7 now), `DRV_I2C_DMA`,
  `DRV_SDIO_DMA` all un-gated from M4-only.

---

## Validation status of the DMA work (important)

| Backend | Code | Validated? | Notes |
|---|---|---|---|
| **UART DMA** | `uart_f7.c` | **✓ hardware** | DMA-written marker `[uart-dma-tx-ok]` reaches the USART3 VCP. `tests/cap/uart_dma`. TX proven; **RX implemented but not bench-exercised** (no serial input source). |
| **I²C DMA** | `i2c_f7.c` `hal_i2c_read_regs_dma` | **✓ hardware** | Successful DMA read of a device's fixed-id register confirmed during bring-up. Committed suite is device-free (arg contract); the wired exercise is the sample. **Renode does not model I²C→DMA**. |
| **SDIO DMA** | shared `sdio.c` async | **◐ not validated** | Un-gate only (proven F4 code). No SD card; **Renode does not model SDMMC→DMA**. |

**Key finding:** Renode emulates the peripherals but **not their DMA request
paths** — the polled path passes in PIL, the DMA path times out. So I²C/SDIO DMA
are genuinely unvalidatable without real devices. They are marked `◐ ¶` in the
capability matrix (`docs/capabilities/README.md`), not a false `✓`.

---

## Gotchas / hard-won facts (don't re-learn these)

- **DWT needs unlocking on Cortex-M7.** The M7 DWT ships with its CoreSight
  software lock engaged; you must write `0xC5ACCE55` to the LAR at `0xE0001FB0`
  before `CYCCNTENA` or `CYCCNT` never advances. Fixed in both `dwt.c` and the
  PIL-detection probe (`navtest_pil.h`). The M4 has no such lock (write is a
  no-op).
- **`navtest_in_pil()`** distinguishes HIL vs PIL by whether `CYCCNT` advances;
  it depends on the DWT unlock above. If it misfires, `NAVTEST_SKIP_ON_PIL`
  tests silently skip on real hardware and `NAVTEST_PIL_ONLY` tests wrongly run.
- **ITCM lives at `0x00000000`** on the F767 and works with default boot — the
  startup copies `.itcm` there and code executes 0-wait. Validated.
- **L1 D-cache is OFF** (bring-up default). That's why all DMA buffers are
  coherent today. **When D-cache is enabled (Phase 2), every DMA/shared buffer
  needs clean/invalidate** — or place it in DTCM (`NAVHAL_DTCM_NOINIT`), which
  is coherency-free. The UART/I²C/SDIO DMA code has no cache maintenance yet.
- **Gating model:** single macro family `NAVHAL_CONFIG_DRV_*` (1:1 Kconfig
  mirror, force-included via `navhal_target.h`), tested with `#if` (never
  `#ifdef`). `NAVHAL_HAS_*` are deprecated aliases (kept via `NAVHAL_HAS_MAP` in
  `tools/kconfig.py`). Drivers gated into the build at CMake level per vendor.
- **Two linkers for F7:** samples use `src/board/nucleo_f767zi/linker.ld`; the
  test ELF uses `tests/arch/cortex-m7/linker.ld`. Both use the F767 board
  startup, so both carry the TCM sections.
- **Adding a cap suite:** drop `tests/cap/<x>/test_<x>.c` (auto-globbed), gate it
  `#if NAVHAL_CONFIG_<CAP>`, register it in `tests/main.c`, and add the cap to a
  board's `TEST_EXTRA_CONFIG` in `tools/hil/boards/*.conf` (and/or
  `tools/pil/boards/*.conf`) so it actually runs.
- **Commit rules (enforced by hooks):** Conventional Commits, subject ≤72 chars,
  pre-commit runs host tests + kconfig configure. **No Claude/attribution
  trailer** in commit messages.

---

## Capability matrix truth (`docs/capabilities/`)

- `◐` should mean *a real part of the capability is unimplemented* (e.g. CACHE:
  D-cache off) OR *implemented-but-unvalidated* (the `¶` footnote: I²C/SDIO DMA).
- `✓` = implemented + validated (MPU, TCM, UART/UART_DMA, cache I-cache path).
- Per-MCU detail belongs in `docs/capabilities/stm32f767zi.md`, not the matrix
  footnotes (README footnotes state only what a symbol means + point to the MCU
  page).

---

## Roadmap / open items

1. **Validate I²C DMA + SDIO DMA on hardware** (tomorrow — needs sensor + card).
2. **D-cache (Phase 2)** — the big one. Enable L1 D-cache + a clean/invalidate
   maintenance API + retrofit every DMA/SDIO/UART buffer path. Hazardous;
   separate, carefully-validated change. `hal_cache.h` already documents it as
   the next phase.
3. **UART DMA RX** — implemented, validate with a TX↔RX loopback jumper.
4. **Additional F7 drivers** — continue per the port plan
   (`docs/roadmap/stm32f767zi-port.md`) / capability matrix `✗` rows.
   - **Ethernet — ✓ DONE & hardware-validated.** Frame-level MAC + dedicated DMA
     over RMII/LAN8742 (`hal_eth_*`, `src/vendor/stm32/eth/eth_f7.c`, `DRV_ETH`).
     Device-free `test_eth` reads PHY ID `0x0007C131`; bring-up sample
     `29_hal_eth` links 100M-full to a host and its broadcast frames reach the
     host NIC; `30_hal_eth_bridge` (+ `chat.py`) round-trips a UART↔Ethernet chat
     both ways, so **RX and TX are both hardware-confirmed**. Gotchas captured in
     memory `eth-f7-bringup-gotchas`: ETH DMA can't
     reach DTCM (buffers in the new ETHRAM/SRAM2 linker region), **TXD1 is PB13**
     not PG14, IRQs storm without a callback (gated on one), HCLK ≥ 25 MHz.
     **RX not yet physically confirmed** (needs the host to transmit to it).
5. **Version + release** — `VERSION` in `include/navhal.h` is the source of
   truth (`0.3.x-dev`). Path to ship: push → merge to `main` → cut to `stable`
   with a version bump.

---

## Standing constraints (from the user)

- Conventional Commits; **no attribution trailer** in commit messages.
- Keep work on `feat/stm32f767zi-port`; don't push without being asked.
- Hardware validation is fair game now (both boards connected) — but only what
  the bench supports (no I²C device / SD card wired yet).
