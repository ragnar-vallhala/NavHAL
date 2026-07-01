# NavHAL changelog

Per-release notes for NavHAL, **one file per minor version** (`0.MINOR.0`).
Patch releases (`0.MINOR.PATCH`, PATCH > 0) are bug-fix / packaging-only and are
summarised inline at the bottom of each minor's page rather than given their own
file. The version is defined in [`include/navhal.h`](../../include/navhal.h)
(`VERSION`), which is the single source of truth the build checks against.

NavHAL is pre-1.0: the public `hal_*` API is frozen at `HAL_API_VERSION 1`
(since 0.2.0) and is additive-only, but SemVer minors may still carry
substantial platform changes.

| Version | Date | Theme |
|---|---|---|
| [0.3.0](0.3.0.md) | 2026-06-11 → *(dev)* | STM32F767ZI (Cortex-M7) port + unified capability gating |
| [0.2.0](0.2.0.md) | 2026-06-11 | Multi-architecture platform (Kconfig, AVR, v1 API + conformance) |
| [0.1.0](0.1.0.md) | 2025-07-18 → 2026-06 | Foundational STM32F401RE (Cortex-M4) HAL |

Read top-down for "what's the newest", bottom-up for "how did we get here".
