@page roadmap_bootloader Secure bootloader

# Secure bootloader

> Status: **planned**
> Scope: a two-stage, signature-verified bootloader for STM32F401RE, with the
> production part locked at RDP Level 2.
> Predecessor: none — additive, but it re-partitions flash, so it must land
> before any board ships with an app larger than 256 KiB.
> Unlocks: authenticated field updates over UART and USB CDC; crashloop
> recovery without physical access.

## Goal

Firmware that only runs images signed by NAVRobotec, updatable over the links
the drone already has, and recoverable when the application crashes. The
production part runs at RDP Level 2 — no SWD, option bytes frozen — so the
design is shaped by one fact: **whatever is immutable is immutable forever.**

SWD stays open during development. That is a bench convenience, not part of the
threat model; the code is identical at every protection level and only the
embedded public key differs.

## Design decisions (load-bearing)

| Decision | Choice | Why |
|---|---|---|
| Stage count | **two** | RDP2 is irreversible. A USB CDC stack frozen for the life of the product is not an acceptable risk; only the trust anchor is frozen. |
| Slots | **single, no A/B** | A second slot needs the app under 192 KiB on this part. The crashloop counter plus the RX sniffer cover the realistic failure modes at a fraction of the flash. |
| Signature | **Ed25519 over a SHA-256 digest** | Asymmetric is mandatory: a shared HMAC key is extractable through SWD on a dev board and would then sign anything. SHA-256 rather than SHA-512 roughly halves the per-boot hash of a 384 KiB app. |
| Integrity | **SHA-256 in the image, CRC-32 as a cheap pre-check** | `hal_crc` is hardware-backed and rejects a truncated transfer in microseconds before the expensive hash runs. |
| Stage-1 transport | **UART only** | Every byte in stage-1 is permanent. USB enumeration and the CDC class belong in the replaceable stage. |
| Boot-mode signalling | **`.noinit` block in SRAM** | Survives reset for free — `Reset_Handler` only zeroes what the linker's zero table lists (`boot.c:90`). |
| Rollback floor | **KV store key** | Already exists; no new persistence mechanism. Meaningful only once RDP2 stops an attacker erasing it. |

## What is deliberately NOT in this bootloader

* **A/B slots and rollback-on-failure** — see above. Revisiting means
  re-partitioning, so the decision is recorded rather than left open.
* **Firmware encryption** — buys nothing while SWD is open in dev, and at RDP2
  the readout it would protect against is already blocked.
* **Delta or partial updates** — the app spans 128 KiB sectors; that is the
  smallest erasable unit up there.
* **ROM DFU / BOOT0 fallback** — unreachable at RDP2.

## Flash partition (STM32F401RE, 512 KiB)

| Sec | Start | End | Size | Partition | Erase unit |
|---|---|---|---|---|---|
| 0 | `0x08000000` | `0x08003FFF` | 16K | Stage-1 (WRP) | never erased |
| 1 | `0x08004000` | `0x08007FFF` | 16K | Stage-1 (WRP) | never erased |
| 2 | `0x08008000` | `0x0800BFFF` | 16K | KV primary | 1 sector |
| 3 | `0x0800C000` | `0x0800FFFF` | 16K | KV secondary | 1 sector |
| 4 | `0x08010000` | `0x0801FFFF` | 64K | Stage-2 | 1 sector |
| 5 | `0x08020000` | `0x0803FFFF` | 128K | App | 3 sectors |
| 6 | `0x08040000` | `0x0805FFFF` | 128K | App | 3 sectors |
| 7 | `0x08060000` | `0x0807FFFF` | 128K | App | 3 sectors |

| Partition | Base | Total | Header | Usable |
|---|---|---|---|---|
| Stage-1 | `0x08000000` | 32,768 | — | 32,768 |
| KV | `0x08008000` | 32,768 | — | 32,768 |
| Stage-2 | `0x08010000` | 65,536 | 512 | 65,024 |
| App | `0x08020000` | 393,216 | 512 | 392,704 |

The KV store moves from sectors 6–7 to 2–3 — two `#define` edits at
`flash_reg.h:78-79`; the address and size macros are already parameterised by
sector number. This also closes a latent overlap: `nucleo_f401re/linker.ld`
declares the full 512 KiB while the KV store squats at `0x08040000`, so an app
over 256 KiB silently collides with it today, and a KV compaction erasing
sector 6 would take app code with it.

Stage-1 carries no header. It is the root of trust, protected by WRP and RDP2
rather than by a signature, and the CPU boots straight into its vector table.

F767ZI has a different sector map and needs its own table before that port
adopts this.

## Image format

Stage-2 and the app share one header; the signature covers it.

```
+0x00  magic    u32
+0x04  version  u32      monotonic, checked against the KV rollback floor
+0x08  length   u32      body bytes following the header
+0x0C  sha256   u8[32]   over header[0..0x0C) || body
+0x2C  sig      u8[64]   Ed25519 over sha256
+0x200 body              vector table starts here
```

The header pads to `0x200` because the body begins with the vector table and
VTOR needs power-of-two alignment at least as large as the table — 101 entries
× 4 = 404 bytes on this part, so 512.

`length` is clamped to the partition maximum *before* it bounds the hash. An
unvalidated length is the standard way a verifier is walked off the end of
flash into attacker-chosen bytes.

## Shared boot block

`.noinit` at the base of RAM, identically placed in all three linker scripts.

```c
/* include/common/hal_boot.h */
#define BOOT_MAGIC        0xB007C0DEu
#define BOOT_REQ_LOADER   0x10ADED00u
#define BOOT_MAX_ATTEMPTS 3u

typedef struct { uint32_t magic, request, attempts, check; } boot_shared_t;
extern volatile boot_shared_t _sboot;
```

`check` is `magic ^ request ^ attempts ^ 0xA5A5A5A5`. Uninitialised SRAM could
plausibly hold `BOOT_MAGIC` by chance; it will not hold that value together
with a matching checksum, which is what makes a cold boot safe.

## Boot flow

```
reset
 ├ stage-1  seed _sboot if invalid            (cold boot)
 │          kick IWDG if running
 │          verify stage-2: length, sha256, Ed25519
 │             fail -> uart_recovery()        (never returns)
 │          request == BOOT_REQ_LOADER  -> uart_recovery()
 │          attempts >= BOOT_MAX_ATTEMPTS -> uart_recovery()
 │          account attempts, jump 0x08010200
 │
 ├ stage-2  verify app the same way
 │             fail -> update_mode()          (UART + CDC)
 │          jump 0x08020200
 │
 └ app      sniff UART and CDC for the magic sequence
            clear attempts after proven liveness
```

Attempt accounting runs before the jump: a watchdog or window-watchdog reset
increments, a power-on or NRST reset clears — a human intervened, so the strike
count starts clean. The application clears it only after a liveness threshold,
never at startup, or a crash that happens after `main()` resets its own strike
count forever.

The application-side half of this — how bytes reach the matcher on each
transport, and when a match is allowed to act — is the boot sniffer below.

## Boot sniffer

The application watches its own console for a magic sequence and reboots into
the loader when it sees one. This is the entry path that does not need a
debugger, a button, or a working update protocol in the application.

### What it has to survive

The sniffer exists to recover a board whose application is misbehaving, so the
requirement that shapes every choice below is that **it keeps working when the
main loop is wedged**. A sniffer fed from the application's own drain loop dies
with the application, which is precisely the case it was built for. Every feed
below is therefore driven from an ISR or from DMA.

### UART — circular DMA plus the IDLE line

The only fully main-loop-independent receive path the HAL has, and both halves
already exist:

```c
hal_uart_init_dma_rx(uart, ring, sizeof ring);   /* hardware fills the ring */
hal_uart_attach_idle_callback(uart, on_idle);    /* ISR on each frame gap */

static void on_idle(void) {                      /* ISR context */
  uint16_t head;
  hal_uart_dma_rx_index(uart, &head);
  for (; tail != head; tail = (uint16_t)((tail + 1u) % sizeof ring))
    hal_boot_match_byte(ring[tail]);
}
```

DMA fills the ring with no CPU involvement and the IDLE interrupt wakes the
walker on each burst. Neither depends on the application scheduling anything.

Where RX DMA is not configured, the application's existing RX interrupt
callback feeds `hal_boot_match_byte` one byte at a time — still ISR-driven, so
still independent of the main loop. Feeding from a polled drain loop works and
is the wrong choice for anything but development, because it stops sniffing at
exactly the moment it is needed.

### CDC — registering the callback is a takeover, not an addition

The matcher's signature is already `hal_usb_cdc_rx_callback_t`
(`hal_usb_cdc.h:114`), so CDC can drive it directly. What the API does not do
is tee the stream: `hal_usb_cdc_set_rx_callback` delivers bytes to the callback
**instead of** queueing them for `hal_usb_cdc_read`. An application that
registers the sniffer naively and also calls `hal_usb_cdc_read` will find its
own console silently empty.

So the sniffer owns the callback and forwards:

```c
static void cdc_rx(const uint8_t *d, uint16_t n) {  /* ISR context */
  for (uint16_t i = 0; i < n; i++)
    hal_boot_match_byte(d[i]);
  if (app_rx != NULL)
    app_rx(d, n);                                   /* chain, never swallow */
}
```

### The matcher

One state machine fed a byte at a time, which makes fragmentation irrelevant:
a DMA chunk boundary, a 64-byte CDC packet and a single interrupt byte all
behave identically, and a sequence split across two transfers still matches.

```c
static uint8_t pos;

void hal_boot_match_byte(uint8_t b) {
  if (b == BOOT_SEQ[pos]) {
    if (++pos == sizeof BOOT_SEQ)
      hal_boot_request();
  } else {
    pos = (b == BOOT_SEQ[0]) ? 1u : 0u;
  }
}
```

The single-byte retry on mismatch is correct **only if no proper prefix of
`BOOT_SEQ` is also a suffix of it**; anything else needs a real KMP failure
table to resync. Eight all-distinct bytes satisfy that by construction, so
that is the constraint on the constant, and a host test asserts it — otherwise
someone improves the magic value one day and quietly breaks resync for every
sequence that arrives mid-stream.

Eight bytes puts a false positive at 2^-64 per stream position, which is why
no idle-gap framing is required around the sequence.

### Entry is an availability boundary, not a security one

Signature verification is what makes the bootloader safe; the sniffer only
decides *when* to reboot into it. Entering the loader therefore grants an
attacker nothing they could not already do by pulling power — but on a vehicle,
anyone able to write to the console can now reboot it mid-flight, and that is a
fall out of the sky. The application holds the policy:

```c
void hal_boot_entry_disable(void);   /* refuse entry; matching continues */
void hal_boot_entry_enable(void);
bool hal_boot_entry_is_disabled(void);
```

The name says entry, because what is disabled is the entry path and not
booting: `hal_boot_disable` would read as "brick the board". A vehicle calls
`hal_boot_entry_disable()` when the airframe arms and `hal_boot_entry_enable()`
when it disarms. The default is enabled, so a board that never calls either
stays recoverable, which is the right default for the bricked-application case.

This is deliberately not spelled "armed": on an airframe that word already
means the opposite polarity — the sniffer is disarmed exactly when the vehicle
is armed — and one identifier carrying two opposite senses is how the check
eventually gets inverted.

### Acting on a match

`hal_boot_request()` sets `request = BOOT_REQ_LOADER` in `_sboot`, recomputes
`check` last so a power cut mid-write cannot forge a valid block, issues a
barrier, and calls `hal_system_reset`. Stage-1 sees the request on the next
boot and hands over to recovery.

Resetting instantly from an ISR is the wrong default while motors are turning,
so the application may install a hook that runs first — cut throttle, flush a
log — after which the reset proceeds. If the hook does not return, the reset
happens anyway: a wedged application is the case this feature exists for, and
waiting politely for it would defeat the point.

## Flash driver work

The raw primitives already exist as file-statics in
`src/vendor/stm32/flash/flash.c`: `_flash_erase_sector_`,
`_flash_program_word_` (currently `NAVHAL_UNUSED`), `_flash_unlock_`,
`_flash_wait_`. The work is promoting them to a gated public API returning
`hal_status_t`, not writing them.

```c
hal_status_t hal_flash_raw_erase_sector(uint8_t sector);
hal_status_t hal_flash_raw_program(uint32_t addr, const void *data, uint32_t len);
```

Two changes inside. The API rejects any address outside the partition the
caller owns — belt and braces behind WRP, so a loader-overwrite bug surfaces as
a clean error in development rather than a WRP fault in the field. And
`_flash_wait_` kicks the watchdog inside its `BSY` poll: the IWDG keeps running
across a system reset and is stopped only by a power-on reset, so both loader
stages inherit whatever timeout the application set — possibly 100 ms — while a
128 KiB sector erase can approach 2 s. A full app erase is three such sectors.
Without that kick, every update on a watchdog-enabled board resets mid-erase.

Programming uses the word path rather than the half-word path the KV store
uses: four times fewer program cycles at 3.3 V.

## Provisioning

RDP2 freezes the option bytes, WRP and BOR included, so the order is one-way:

1. Flash stage-1, stage-2 and app over SWD
2. Set WRP on sectors 0–1
3. Set BOR level — a browned-out core executing garbage bypasses every check
   above, and this is the cheapest mitigation available
4. Set RDP2 **last**

Steps 2 and 3 are permanent once step 4 runs.

Three tiers: development at RDP0 with SWD, validation at RDP1, production at
RDP2. RDP1 blocks readout but downgrading mass-erases, so those boards stay
recoverable and are where the full matrix runs before any unit is locked.

## Slices

### Slice 1 — Partition
KV store to sectors 2–3; linker scripts for all three images, each declaring
only the region it owns; `hal_bootmap.h` with the partition constants. No
crypto, no new behaviour — this is the re-partition on its own.

### Slice 2 — Raw flash API
Promote the statics, add the bounds check and the watchdog kick in the `BSY`
poll.

### Slice 3 — Boot block
`hal_boot.h`, `.noinit` in all three scripts, the attempt counter and reset-cause
accounting, the shared magic-sequence matcher. Testable with no crypto present.

### Slice 4 — Crypto and signing tool
SHA-256 and Ed25519 verify, validated on host against published test vectors
first; `tools/` signing tool that prepends the header, hashes and signs. Sizing
of the Ed25519 implementation is confirmed here.

### Slice 5 — Stage-1
Verify, jump, UART recovery. Fits 32 KiB.

### Slice 6 — Stage-2
App verification, UART and CDC update mode, rollback floor in the KV store.
`hal_usb_cdc_init()` is called only on entry to update mode — enumeration costs
about a second and has no business on the fast path.

### Slice 7 — Application integration
Sniffer wired into both transports — UART on circular DMA plus the IDLE
callback, CDC on a forwarding RX callback — the `hal_boot_entry_disable`
policy gate, and the liveness clear of the attempt counter.

### Slice 8 — Provisioning and lockdown
Option-byte tool, full RDP1 validation, then RDP2 on production units.

Slices 1–3 are independently useful and carry no cryptographic risk. The chain
of trust does not exist until slice 5.

## Testing

Per the device-free rule, committed on-target tests must pass on a bare board:
image verification against fixtures compiled into the test binary, the `_sboot`
state machine, rollback comparison, and the partition bounds checks all
qualify. The matcher is pure logic and belongs on the host tier: feed the
sequence whole, then split at every possible boundary, preceded by near-misses
and by partial prefixes, and assert exactly one match — plus the ring walker
across a wraparound, and the all-distinct-bytes property of `BOOT_SEQ`.
Anything that needs a host feeding an image over a wire is a sample.

The matrix that must pass at RDP1 before any unit is locked: good boot; corrupt
app; corrupt stage-2; bad signature on each; rollback rejection; crashloop to
stage-2; UART recovery of a bad stage-2; CDC update; power cut mid-erase and
mid-program.

## Exit criteria

* An unsigned or tampered image is refused at both stages.
* Three consecutive watchdog resets land in stage-2 update mode.
* A bad stage-2 is recoverable over UART with no debugger attached.
* A power cut at any point during an update leaves the board updatable.
* An RDP2 unit completes a signed update over CDC.

## Open questions

* Which Ed25519 implementation — stage-1 fitting in 32 KiB depends on it, and a
  miss changes the partition table. Resolve in slice 4, before slice 1 is hard
  to undo.
* Measured boot latency. The estimate is ~170 ms for the app hash plus verify at
  84 MHz; if it lands materially higher, the app hash may need to move behind a
  "verified once, recorded in KV" scheme, which weakens the guarantee.
* Whether stage-2 should be able to update stage-2, or only stage-1. Self-update
  is convenient and is also the classic way to brick a fleet.
* F767ZI partition table and whether that port wants the same two-stage shape.
* Whether a sequence arriving while entry is disabled should be remembered and
  acted on at the next `hal_boot_entry_enable`. Convenient for "reboot it as
  soon as it lands"; also a way to arm a reboot the operator has forgotten
  about.
