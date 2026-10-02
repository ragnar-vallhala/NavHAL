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

`tests/host/vectors/ed25519_check` is Monocypher's copy of Google Project
Wycheproof's EdDSA corpus, which is Apache-2.0.

Upgrading one means replacing its files, keeping the prepended header, and
re-running the host vector test — which is the point of having it.
