# NavHAL Naming Conventions — Quick Reference

A one-page cheat-sheet for contributors. The full rationale lives in
`docs/api_standardization.md` (Sections 5–9); this page is the lookup table.

## Symbols

| Symbol kind | Rule | Example |
|-------------|------|---------|
| Public function | `hal_<peripheral>_<verb>[_<qualifier>]`, lower snake_case | `hal_uart_write`, `hal_gpio_set_mode` |
| Async qualifier | `_it` (interrupt-driven) / `_dma` (DMA-backed) suffix | `hal_uart_write_dma`, `hal_i2c_read_it` |
| Public type | `hal_<peripheral>_<noun>_t` | `hal_uart_config_t`, `hal_gpio_pin_t` |
| Enum constant | `HAL_<PERIPH>_<NAME>`, UPPER snake_case | `HAL_UART_1`, `HAL_GPIO_MODE_OUTPUT` |
| Public macro / constant | `HAL_<PERIPH>_<NAME>` | `HAL_TIMEOUT_FOREVER` |
| Capability macro | `NAVHAL_HAS_<FEATURE>` (always defined; test with `#if`) | `NAVHAL_HAS_DMA` |
| Generated config macro | `NAVHAL_CONFIG_<NAME>` | `NAVHAL_CONFIG_BOARD` |
| Internal symbol | `navhal_` prefix, or `static`; lives under `src/` | `navhal_uart_isr_dispatch` |
| Vendor / arch / family symbol | `<layer>_` prefix — never `hal_` | `stm32_usart_*`, `armv7em_nvic_*` |

## Rules

- Verbs are always `snake_case`: `set_mode`, never `setmode`.
- Every fallible function returns `hal_status_t` (`HAL_OK` == 0 == success);
  output data goes through a caller-supplied pointer parameter.
- Buffers are passed as `(pointer, size_t length)` pairs; timeouts in
  milliseconds.
- Peripheral instances are referenced by a typed `hal_<p>_id_t` enum — never a
  bare `uint8_t` or an opaque handle.

## Banned from the public API

- Bare `uart_*`, `timer_*`, `delay_ms` (no `hal_` prefix).
- Per-instance macro families (`uart1_write_*`, `uart2_write_*`).
- ISR symbols — `*_IRQHandler`, `*_Handler`. Applications register a
  `hal_<p>_callback_t` instead.
- Register-map headers (`*_reg.h`) and raw register macros — these are
  family-private.

## Compiler shims (use these, not raw `__attribute__`)

`NAVHAL_INLINE`, `NAVHAL_ALWAYS_INLINE`, `NAVHAL_UNUSED`, `NAVHAL_USED`,
`NAVHAL_WEAK`, `NAVHAL_PACKED`, `NAVHAL_NORETURN`, `NAVHAL_DEPRECATED(msg)` —
all from `common/navhal_compiler.h`.

## Deprecated (do not use in new code; retained as a backward-compat alias)

`SUCCESS` / `FAILURE` (use `HAL_OK` / `HAL_ERR`), `byte` (use `uint8_t`),
`__UNUSED` (use `NAVHAL_UNUSED`).
