# NavHAL — Testing Guide

**Companion to:** `docs/api_standardization.md`, `docs/execution_plan.md`,
`docs/m2_plus_plan.md`.

NavHAL has three test entry points, each used for a different purpose:

| Entry point | Purpose | Speed | Hardware |
|---|---|---|---|
| **Host subset** | Pure-logic CI feedback | < 30 s | None |
| **Renode (CI)** | On-target ELF in an STM32F4 emulator | ~1 min | None |
| **On-target flash** | Real silicon, real peripherals | ~1 min + bring-up | Nucleo-F401RE |

All three speak to the same `navtest` harness — same suite tables, same
assertion macros — so writing a new test is a single act regardless of
where you intend to run it.

---

## 1. Host subset

A standalone build that compiles the pure-logic tests with the system
`gcc` and runs them as a native binary.

```sh
tools/run_host_tests.sh
```

That's a thin wrapper over:

```sh
cmake -B build-host -S tests/host
cmake --build build-host -j
./build-host/tests_host
```

What runs:
- `HAL_STATUS (host)` — `HAL_OK == 0`, `HAL_ERR == 1`, distinct codes,
  `HAL_OK_OR_RETURN` semantics.
- `CONVERSION (host)` — `str_to_int`, `str_to_float` edge cases.
- `CRC SOFTWARE (host)` — the table-driven CRC-32/MPEG-2 path of
  `src/core/cortex-m4/crc/crc.c` compiled without `_CRC_HW_ENABLED`.
- `GPIO ENCODING (host)` — locks the `GPIO_Pxnn` enum layout and the
  `GPIO_GET_PIN` / `GPIO_GET_PORT_NUMBER` macros that every driver uses.

Exit code is the number of failures. CI fails the `host-tests` job if
non-zero.

---

## 2. On-target build (Renode-friendly)

The same build is used for both Renode and physical flashing. The
crucial flag is `-DTEST=ON` — it excludes `samples/` from the configure
so the test build's `flash_tests` target doesn't collide with each
sample's `flash` target.

```sh
cmake -B build -DTEST=ON
cmake --build build --target tests -j
```

Output: `build/tests` (the ELF). The on-target main lives in
`tests/main.c`; it boots UART2 at 9600 baud and walks
`all_suites[]` — every suite registered there runs in order.

### 2a. Run inside Renode

A scripted invocation that boots an emulated F401RE machine, loads the
ELF, captures USART2 to a log, and exits on the navtest summary:

```sh
tools/renode/run_tests.sh build/tests
```

Exit code is the number of failures (0 = green, non-zero = red). The
captured UART log is echoed to stdout for context.

### 2b. Flash to a Nucleo-F401RE

Single-command flow — build, flash, capture USART2, and exit on the
navtest failure count:

```sh
tools/run_target_tests.sh                # /dev/ttyACM0 @ 9600, 120 s timeout
tools/run_target_tests.sh /dev/ttyACM1   # override port
```

Or the unwrapped pieces (useful if you already have a serial console
attached):

```sh
cmake --build build --target flash_tests
# then watch USART2 (PA2/PA3 on the Arduino-style header) at 9600 8N1.
```

The on-target binary returns from `main` — it does not loop
indefinitely, so `tools/uart_capture.py` exits cleanly with the
failure count (0 = green) the moment it sees the summary.

---

## 3. CI matrix

`.github/workflows/ci.yml` runs four jobs on every push to `main` and
every pull request:

| Job | What it does |
|---|---|
| `host-tests` | Configures + builds + runs the host subset. Should be < 30 s. |
| `build-on-target` | Cross-compiles the test ELF with arm-none-eabi-gcc; uploads it as a workflow artifact. |
| `renode-on-target` | Downloads the ELF, installs Renode, runs `tools/renode/run_tests.sh`, exits on failure count. |
| `build-no-cap` | Strips `CONFIG_DRV_{DMA,FPU,SDIO,DWT}` from `.config`, builds, and verifies via `arm-none-eabi-nm` that the capability-gated symbols did **not** link in. Proves the `NAVHAL_HAS_*` contract. |

---

## 4. Adding a test for a new driver

A new driver test file `tests/test_<x>.{c,h}` follows this pattern:

```c
/* tests/test_<x>.h */
#ifndef TEST_X_H
#define TEST_X_H
#include "navtest/navtest.h"

void test_x_init_returns_ok(void);
void test_x_init_rejects_null(void);

extern const navtest_suite_t test_x_suite;
#endif
```

```c
/* tests/test_<x>.c */
#define CORTEX_M4
#include "test_x.h"
#include "core/cortex-m4/x.h"
#include "navtest/navtest.h"

void test_x_init_returns_ok(void) {
  hal_x_config_t cfg = { /* … */ };
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_x_init(&cfg));
}

void test_x_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_x_init(NULL));
}

static const navtest_case_t x_cases[] = {
    NAVTEST_CASE(test_x_init_returns_ok),
    NAVTEST_CASE(test_x_init_rejects_null),
};

const navtest_suite_t test_x_suite = {
    .name = "X",
    .cases = x_cases,
    .count = sizeof(x_cases) / sizeof(x_cases[0]),
    .between = NULL, /* or e.g. wait_uart_empty for clock-tweaking suites */
};
```

Then register the suite in `tests/main.c`:

```c
#include "test_x.h"
/* … */
static const navtest_suite_t *const all_suites[] = {
    /* …existing suites… */
    &test_x_suite,
};
```

### If the driver is capability-gated

Wrap both files in the matching `#if NAVHAL_HAS_<FEATURE>` — see
`tests/test_dma.{c,h}`, `tests/test_dwt.{c,h}`, `tests/test_fpu_accel.{c,h}`,
or `tests/test_sdio.{c,h}` for working examples. The `#include` in
`tests/main.c` stays unconditional, but the suite-array entry is
gated:

```c
#include "test_x.h"
/* … */
static const navtest_suite_t *const all_suites[] = {
    /* … */
#if NAVHAL_HAS_X
    &test_x_suite,
#endif
};
```

### Coverage bar

Per `docs/m2_plus_plan.md`:

- Every public function in `include/core/cortex-m4/<driver>.h` has at
  least one success-path test.
- Every `hal_status_t`-returning function has at least one error-path
  test (NULL pointer, invalid id, pre-init call as relevant).
- The case order in the suite table is the order they run on-target —
  prefer dependency-light cases first.

### Host vs on-target

If the test exercises a register, it stays on-target. If it's pure
logic (table-driven math, enum invariants, helper arithmetic), put it
under `tests/host/` — see `tests/host/test_crc_sw.c` or
`tests/host/test_gpio_encoding.c` for the pattern and the standalone
`tests/host/CMakeLists.txt` for how host suites are listed.

---

## 5. Troubleshooting

**"Total failures not found in UART log"** — Renode's emulation window
(currently 30 s in `tools/renode/navhal_f401re.resc`) expired before
the suite finished. Either the suite genuinely hung, or it's grown
past the budget — bump `emulation RunFor` and retry.

**Sample build fails with "target `flash` already defined"** — you
configured the same build dir with both `-DTEST=ON` and a `SAMPLE=…`
in `.config`. They are mutually exclusive (PR1 of M2+). Use separate
build dirs: `build-test/` for tests, `build-sample/` for samples.

**Host build's CRC test fails after the implementation changes** — the
canonical "123456789" → `0x0376E6E7` vector is fixed by the CRC-32/MPEG-2
standard, not by our implementation. A mismatch is a real regression in
the lookup table; fix the table, not the test.
