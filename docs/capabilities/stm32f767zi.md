@page cap_stm32f767zi STM32F767ZI (Cortex-M7)

# STM32F767ZI (STM32F7 / Cortex-M7)

The third NavHAL port, in **initial bring-up**. The build system, GPIO, clock,
timer and interrupt layers are implemented and verified on real hardware (a
flashed `hal_blink` toggles the LED). The remaining peripherals are scoped
follow-ups — see [`../stm32f767zi_port_plan.md`](../stm32f767zi_port_plan.md).
The reference board used for bring-up is the ST Nucleo-F767ZI, but the port
targets the STM32F767ZI MCU; other boards built on the same MCU are supported
by adding a board layer.

## Target identity

| | |
|---|---|
| MCU                             | `STM32F767ZI` (2 MB flash, 512 KB SRAM) |
| Arch (`NAVHAL_TARGET_ARCH`)     | `cortex-m7` |
| ISA layer (`ARCH_ISA`)          | `armv7e-m` (shared with Cortex-M4) |
| Vendor (`NAVHAL_TARGET_VENDOR`) | `stm32` |
| Family (`NAVHAL_TARGET_FAMILY`) | `stm32f7` |
| Reference board (`NAVHAL_TARGET_BOARD`) | `nucleo_f767zi` |
| Toolchain                       | `arm-none-eabi-` (`gcc-arm-none-eabi`) |
| Defconfig                       | [`cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig`](../../cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig) |
| Toolchain file                  | [`cmake/toolchains/arm-none-eabi-f767-toolchain.cmake`](../../cmake/toolchains/arm-none-eabi-f767-toolchain.cmake) |
| Default sysclk                  | 16 MHz (reset-default HSI; no `hal_clock_init` in the blink path) |

## Capabilities

| `NAVHAL_HAS_*` | Status | Driver | Notes |
|---|---|---|---|
| GPIO              | ✓ | `src/vendor/stm32/gpio/gpio.c`            | Reuses the F4 driver; F7 `gpio_reg.h` uses contiguous port indexing (A–G + H). Verified on LD1 (PB0). |
| TIMER             | ✓ | `src/vendor/stm32/timer/timer.c`         | TIM2–5 / TIM1 / TIM9–11; same register layout as F4. |
| CLOCK             | ✓ | `src/vendor/stm32/clock/clock_f7.c`      | HSI / HSE / PLL up to **216 MHz**, verified on hardware. VOS Scale 1, PWR over-drive (>180 MHz), HCLK-scaled flash wait states + ART/prefetch, APB1 ≤54 / APB2 ≤108 MHz prescalers. |
| INTERRUPT         | ✓ | `src/arch/armv7e-m/interrupt/interrupt.c`| NVIC; shared ARMv7E-M arch code. |
| UART              | ✓ | `src/vendor/stm32/uart/uart_f7.c`        | USART1/2/3/6: polling, interrupt RX, and the **DMA-backed** API (`hal_uart_write_dma` / `init_dma_rx`, `DRV_UART_DMA`). F7-specific IP (ISR/RDR/TDR, DMA peripheral address = TDR/RDR not DR). USART3 (ST-LINK VCP, PD8/PD9) verified at 115200; DMA TX validated on hardware (`test_uart_dma` — the DMA-written marker reaches the VCP). |
| I2C               | ✓ | `src/vendor/stm32/i2c/i2c_f7.c`         | Master; full rewrite for the F7 timing-register IP (`TIMINGR` / `ISR`-`ICR` / CR2-framed / `RXDR`-`TXDR`). Opt-in via `CONFIG_DRV_I2C`; `test_i2c` passes. The transfer FSM is hardware-exercised device-free: addressing a reserved address returns a NACK (`HAL_ERR_IO`), not a timeout, proving START/addressing/ACK-sampling run on silicon (`test_i2c_transfer_fsm_nacks_absent_device`). The data phase is validated in PIL against a Renode-modelled BMP180 (`write_read` of the chip-id register), and a successful device read was confirmed during bring-up. |
| SPI               | ◐ | `src/vendor/stm32/spi/spi_f7.c`         | Master, 8/16-bit. F7-specific (`CR2.DS` frame size + `FRXTH`, byte-`DR` FIFO access) — the F4 `CR1.DFF` is gone. Opt-in via `CONFIG_DRV_SPI`; `test_spi` (8) passes; init is register-verified on HIL and a JEDEC-ID read against a Renode `GenericSpiFlash` validates the transmit/receive FIFO path in PIL. |
| PWM               | ✓ | `src/vendor/stm32/pwm/pwm.c`             | Reuses the shared timer-based driver. Opt-in via `CONFIG_DRV_PWM`; `test_pwm` (11) passes on hardware. |
| FLASH             | ✓ | `src/vendor/stm32/flash/flash.c`        | Key/value store on sectors 6/7 (256 KB each) of the real F767 12-sector 2 MB map. Opt-in via `CONFIG_DRV_FLASH`; `test_flash_raw` (6) passes on hardware. Bring-up fixed two `flash.c` bugs (M7 write-buffer `DSB`; NULL guard). |
| CRC_HW            | ✓ | `src/vendor/stm32/crc/crc.c`            | Hardware CRC-32; default polynomial is register-compatible with F4. Opt-in via `CONFIG_DRV_CRC`; the CRC suite (7) passes via the hardware unit on F767. |
| CYCLE_COUNTER     | ✓ | `src/arch/armv7e-m/dwt/dwt.c`            | DWT-backed; shared ARMv7E-M arch code. Opt-in via `CONFIG_DRV_DWT`; `test_dwt` (6) passes on hardware. |
| MPU               | ✓ | `src/arch/armv7e-m/mpu/mpu.c`            | PMSAv7 MPU, **16 regions** (read at runtime from `MPU_TYPE.DREGION`); shared ARMv7-M driver. Opt-in via `CONFIG_DRV_MPU`; `test_mpu` (4) passes on hardware — presence/region-count, bit-exact `RBAR`/`RASR` encoding, configure/disable, bulk apply. Fault-on-violation *enforcement* is demonstrated on hardware by the `32_hal_mpu_fault` sample: a read of a no-access region traps into `MemManage_Handler` (faulting address in `MMFAR`), which recovers by disabling the MPU. |
| CACHE             | ✓ | `src/arch/armv7e-m/cache/cache.c`        | L1 **instruction + data cache**; M7-only, opt-in via `CONFIG_DRV_CACHE`. I-cache via `hal_icache_enable`; D-cache via `hal_dcache_enable` (full set/way invalidate then `CCR.DC`) plus by-MVA `hal_dcache_clean/invalidate/clean_invalidate`. Coherency is kept by clean/invalidate around every DMA hand-off: ETH descriptors/buffers, and the general-DMA drivers (UART/I2C/SDIO) via the `navhal_dma_*` helpers, which skip uncached DTCM and reject unreachable ITCM (see `navhal_port_dma.h`). `test_cache` (5) passes; the full suite runs **157/0 in PIL with the D-cache enabled in boot**. HIL hardware coherency sign-off pending. |
| FPU               | ✓ | `src/arch/armv7e-m/fpu/fpu.c`            | Hardware **double-precision** FPU (`-mfpu=fpv5-d16`, hard float) via `CONFIG_USE_FPU` + `CONFIG_DRV_FPU`. `test_fpu_accel` (3) passes on hardware. |
| DMA               | ✓ | `src/vendor/stm32/dma/dma.c`            | DMA1/DMA2 stream controller (register-compatible with F4). Opt-in via `CONFIG_DRV_DMA`; `test_dma` (17) passes on hardware. Coherent while the L1 D-cache stays off (see caveats); a DMA UART backend is still pending. |
| SDIO              | ◐ | `src/vendor/stm32/sdio/sdio.c`          | **Polled** SD-card block I/O. The F7 SDMMC1 IP is register-identical to the F4 SDIO (same base `0x40012C00`, same APB2ENR bit, same AF12 pinmux, same vector slot 49), so the shared driver runs unchanged. Opt-in via `CONFIG_DRV_SDIO`; `test_sdio` (6) passes, and a card-init + 512-byte block write/read round-trip is validated in PIL against a Renode `SD.STM32FSDMMC` + attached card (`NAVTEST_PIL_ONLY`). The DMA-backed async API (`DRV_SDIO_DMA`) is now compiled on the F7 too — same shared driver, and coherent with the L1 D-cache off — but is **not yet hardware-validated**: the Nucleo has no card slot, and Renode's SD model does not service the SDMMC→DMA request path (the async round-trip times out there while the polled path works). |
| UART_DMA          | ✓ | `uart_f7.c`                             | `hal_uart_write_dma` / `init_dma_rx`. TX hardware-validated (`test_uart_dma`); RX implemented, not bench-exercised. |
| I2C_DMA           | ✓ | `i2c_f7.c`                              | `hal_i2c_read_regs_dma` — write reg pointer, then a DMA (RXDMAEN + AUTOEND) read on DMA1 Stream 0 Ch 1. `test_i2c_dma` checks the argument contract; a completed DMA read needs a bus device, so it lives in the wired sample `samples/cortex-m/19_hal_dma_i2c`, not the device-free suite. Validated during bring-up: a DMA read of a device's fixed-id register returned the expected byte on silicon. |
| SDIO_DMA          | ◐ | `sdio.c`                               | `hal_sdio_*_async`, shared driver. Compiled on F7, coherent with the D-cache off, but **not hardware-validated** (no card; Renode doesn't service SDMMC→DMA). |
| ETH               | ✓ | `src/vendor/stm32/eth/eth_f7.c`         | Frame-level MAC + its dedicated DMA driving the on-board LAN8742 over RMII (`hal_eth_*`, `DRV_ETH`). RMII pins PA1/PA2/PA7, PC1/PC4/PC5, PG11/PG13, **TXD1 on PB13** (AF11). Descriptors/buffers live in a dedicated ETHRAM linker region (SRAM2) — the ETH DMA can't reach DTCM. Interrupts are enabled only when a callback is registered (else polling), avoiding a receive-buffer-unavailable storm. **Hardware-validated, both directions**: device-free `test_eth` reads the PHY ID `0x0007C131` over MDIO; the `29_hal_eth` sample links at 100M full and its broadcast frames reach the host NIC; the `30_hal_eth_bridge` UART↔Ethernet chat (with `chat.py`) round-trips text host↔board, confirming RX and TX end-to-end. |

`✗` here means the silicon has the peripheral but the NavHAL driver isn't
validated for F7 yet — treated like `—` at link time.

### Cortex-M7-only silicon (delta from the F4 / Cortex-M4 ports)

Every capability above is shared ARMv7E-M arch code or a reused/ported F4
driver — there is **no NavHAL driver module exclusive to M7**. The M7 delta
is at the *silicon* level: features the Cortex-M4 / STM32F4 port has no
equivalent for. NavHAL either folds these into an existing module or leaves
them unwrapped for now.

| M7-only feature | NavHAL status | Where | Notes |
|---|---|---|---|
| Double-precision FPU (`fpv5-d16`) | ✓ (in the FPU module) | `cmake/arch/armv7e-m.cmake` | M4 has only the single-precision `fpv4-sp-d16`. Same `hal_fpu` API; the `-mfpu` is picked from `CMAKE_SYSTEM_PROCESSOR` so M7 gets hardware `double`. |
| L1 caches — 16 KB I-cache + 16 KB D-cache | ✓ (both driven) | `src/arch/armv7e-m/cache/cache.c` | I-cache via `hal_icache_enable`; D-cache via `hal_dcache_enable` + the clean/invalidate maintenance API. DMA buffers stay coherent via clean-before-TX / invalidate-after-RX at each driver hand-off (ETH internally; UART/I2C/SDIO via the `navhal_dma_*` helpers). PIL-green with the D-cache on; HIL sign-off pending. M4 has no cache at all. |
| DTCM / ITCM tightly-coupled memory (128 KB / 16 KB) | ✓ (static placement) | `common/hal_tcm.h` + `linker.ld` + `startup.s` | Pin code/data into the 0-wait TCMs with `NAVHAL_ITCM` / `NAVHAL_DTCM` / `NAVHAL_DTCM_NOINIT` (opt-in `CONFIG_USE_TCM`); the board startup copies `.itcm`/`.dtcm` from flash at reset and zeroes `.dtcm_bss`. `test_tcm` (3) passes on hardware (ITCM code executes from `0x0`, DTCM init copied, NOINIT zeroed). No TCM allocator / heap-in-TCM yet. Since DTCM is not cached, it doubles as a coherency-free home for DMA buffers. |

`◐` here flags an M7 feature NavHAL is *aware* of but does not yet expose as a
standalone driver; `✓` means it's already covered inside the listed module.

(The flash ART accelerator + prefetch, enabled in `clock_f7.c`, is *not* listed
here — it is an STM32F4 feature too, so it isn't part of the M7 delta.)

## Default Kconfig state

What `navhal_target.h` contains after
`cmake -B b -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake -DSAMPLE=hal_blink`:

```
NAVHAL_HAS_GPIO          1
NAVHAL_HAS_TIMER         1
NAVHAL_HAS_CLOCK         1
NAVHAL_HAS_INTERRUPT     1
NAVHAL_HAS_UART          1   (uart_f7.c — polling + IRQ + DMA backend)
NAVHAL_HAS_DMA           0   (opt-in via CONFIG_DRV_DMA — verified working)
NAVHAL_HAS_FPU           0   (opt-in via CONFIG_USE_FPU+DRV_FPU — verified working)
NAVHAL_HAS_CYCLE_COUNTER 0   (opt-in via CONFIG_DRV_DWT — verified working)
NAVHAL_HAS_FLASH         0   (opt-in via CONFIG_DRV_FLASH — verified working)
NAVHAL_HAS_I2C/SPI/PWM/CRC_HW/SDIO  0
```

DMA / FPU / DWT are off in the *default* config (opt-in), but all three are
implemented and pass their on-target test suites — see the bring-up record.

## Hardware bring-up record

* Board probed: `st-info --probe` → `STM32F76x_F77x`, chipid `0x451`,
  2048 KiB flash / 512 KiB SRAM (matches the linker script).
* `hal_blink` (LD1 / PB0) built (`text 9892 / data 4 / bss 524`), flashed to
  `0x08000000` and verified by `st-flash`. Erase reported sector 0 = 0x8000
  (32 KB), confirming the F7 sector map differs from the F4's 16 KB sector 0.
* `hal_uart_tx` flashed; USART3 output captured on `/dev/ttyACM0` at 115200 —
  100×`Hello World` + `UART TX NO DMA Test finished: 115 ticks`, correctly
  framed (confirms the F7 ISR/TDR path and the BRR baud calc from APB1=16 MHz).
* On-target test ELF (`-DTEST=ON`) flashed; results captured over USART3 @9600:
  **30 tests, 0 failures** (conformance 15, timebase 8, CRC 7). M4 white-box
  and raw-flash suites intentionally skipped on M7.
* Vector table: the F767 ships its own `src/board/nucleo_f767zi/startup.s`
  (STM32F767xx layout). Verified that vector slot 39 (USART3) resolves to the
  dispatch handler `0x...2254` rather than the literal `0` the F401-based arch
  table left there — so interrupt-driven USART3 dispatches instead of faulting.
* Clock brought to **216 MHz** (HSI→PLL, over-drive engaged): captured
  `sysclk=216000000 ahb=216000000 apb1=54000000 apb2=108000000` over USART3 while
  the UART kept running — confirming VOS/over-drive/WS=7 and the APB prescalers.
* Test ELF rebuilt with `CONFIG_USE_FPU` + `DRV_FPU` + `DRV_DWT` + `DRV_DMA`
  (hard-float `fpv5-d16`); on-target run reported **56 tests, 0 failures** —
  adds DMA (17), CYCLE_COUNTER/DWT (6) and FPU (3) to the 30 above.
* Flash KV store (F7-4): after correcting the sector map and fixing the M7
  write-buffer `DSB` + NULL-guard bugs, the `test_flash_raw` suite (6) passes —
  a clean-boot run reports **36 tests, 0 failures**. A probe confirmed
  erase→save→read round-trips real data on sector 6 (0x08080000).

## Caveats and known limitations

* Reset default is HSI 16 MHz; call `hal_clock_init` with a PLL config to scale
  up (up to 216 MHz — `clock_f7.c` does VOS/over-drive/wait-states for you).
* UART DMA TX is hardware-validated; the DMA **RX** path (`hal_uart_init_dma_rx`)
  is implemented but not yet exercised on the bench (no serial input source
  wired). It is a *circular* RX buffer read live, so with the D-cache on it must
  sit in DTCM (uncached, DMA1/2-reachable) or be invalidated before each read —
  the driver only guards reachability.
* The **L1 D-cache is enabled via `hal_dcache_enable`** and kept coherent by
  clean/invalidate around every DMA hand-off — ETH descriptors/buffers directly,
  and the general-DMA drivers (UART/I2C/SDIO) through the `navhal_dma_*` helpers,
  which clean before a TX DMA, invalidate after an RX DMA, skip uncached DTCM,
  and reject DMA-unreachable ITCM. Caller DMA buffers must be `NAVHAL_DMA_ALIGN`
  (32-byte, size-padded). Full suite is **157/0 in PIL with the D-cache on**;
  on-hardware coherency sign-off (HIL) is the remaining step before enabling it
  in the shipped defconfig.
* Wired into CI: `sample-matrix-f767` (portable samples build under the F767
  toolchain) and `build-on-target-f767` (test-ELF compile) in `ci.yml`, plus a
  `nucleo_f767zi` job in the per-arch PIL matrix (`renode.yml`) that runs the
  on-target suite under Renode — with `DRV_SDIO` enabled (board-conf
  `TEST_EXTRA_CONFIG`) so the SDIO block round-trip runs there too.
