# Vendored crypto

Three Ed25519 backends, selectable with `CONFIG_BOOT_ED25519_*`. Each is
upstream source with the repo's Apache header prepended — the licence gate wants
one on every file under `src/` — and each upstream copyright and licence notice
left intact directly below it. Prepending our header does not relicense anything;
the notice that governs each file is the one it carries.

| Backend | Version | Source | Upstream licence |
|---|---|---|---|
| monocypher | 4.0.2 | https://github.com/LoupVaillant/Monocypher | CC0-1.0 or BSD-2-Clause |
| tweetnacl | 20140427 | https://tweetnacl.cr.yp.to/ | public domain |
| compact25519 | main (c25519 submodule) | https://github.com/DavyLandman/compact25519 | MIT (c25519: public domain) |
| sha256 | crypto-algorithms, master | https://github.com/B-Con/crypto-algorithms | public domain |

The SHA-256 is the image digest, shared by all three backends -- it is not part
of Ed25519, which carries its own SHA-512 internally. It was checked against the
FIPS 180-4 known answers (empty, "abc", the 56-byte case and 1,000,000 'a')
before being vendored, and `tests/host/test_boot_crypto.c` keeps those vectors.

`tests/host/vectors/ed25519_check` is Monocypher's copy of Google Project
Wycheproof's EdDSA corpus, which is Apache-2.0.

Upgrading one means replacing its files, keeping the prepended header, and
re-running the host vector test — which is the point of having it.
