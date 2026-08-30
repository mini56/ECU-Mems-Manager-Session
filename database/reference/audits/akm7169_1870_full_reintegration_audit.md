# AKM7169ENG full MEMS/SPi reintegration - batch 1870

Date: 2026-08-30

## Exact constructor source
- Rover Group Limited `AKM7169ENG`, Mini Repair Manual, Publication Part No. AKM7169, 1993.
- Project-owner supplied scan: 43906518 bytes, 482 physical pages.
- SHA-256: `c8bbb30d7d5a52932e7f92723ba5dc70520012ac3ceac21d19ab0a39b4d4c4e0`.
- Manual-wide stated scope: Mini models manufactured from VIN `049349`.

## Rebuild policy
The prior incomplete AKM7169 integration is purged from the active database: old `SRC-AKM7169`, `RAVE-SPI93-*`, `KNOW-RAVE-SPI93-*`, `SCOPE-RAVE-MINI-SPI-AKM7169-*`, legacy `akm7169fre` source/diagnostic rows and its old fault row. The shared `otpubs_mini` catalogue row is intentionally preserved because it also documents other Rover publications.

No old AKM fact is trusted merely because it existed previously. The new exact-source identifiers are `SRC-AKM7169ENG-EXACT-482`, `AKM7169-*`, `KNOW-AKM7169-*` and `SCOPE-AKM7169ENG-*`.

## Retained constructor pages
Exactly 40 manufacturer pages are rendered from the exact binary with the project historical visual chain (150 dpi grayscale -> 1-bit -> optimized PNG):
`2, 7, 16, 24, 25, 26, 29, 36, 44, 114, 115, 116, 117, 118, 119, 120, 121, 122, 124, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 138, 140, 142, 143, 144, 145, 156, 158, 159, 459, 460`.

The retained pages cover publication/VIN scope, General Data, Engine Tuning Data 3/4/5, torque settings, underbonnet SPi locations, Microcheck/Cobest context, complete MEMS-SPi description/operation, fuel/ignition/sensor/actuator strategy, service/diagnostic procedures, fuel-delivery procedures, manifold/closed-loop operation and the original Engine Management wiring legend/diagram.

## Structured result on exact BUILD #102 database copy
- new constructor facts: **91** RAVE + **91** expert mirrors;
- new knowledge items: **91**;
- new exact applicability scopes: **5**;
- specifications/values: **81 / 81**;
- structured procedures: **13**, steps **160**, requirements/warnings/tools **11**;
- knowledge relations: **9**;
- exact visual assets: **40**, fact->page links **135**, covered new facts **91/91**;
- source-asset rows: **40**;
- legacy diagnostic lookup rows rebuilt under exact ENG source: **8**;
- corrected exact-source fuel-pressure fault entry: **1**.

Old incomplete AKM active rows after installation: RAVE **0**, knowledge **0**, legacy diagnostics **0**, old source **0**, old fault **0**.

Database validation: `PRAGMA integrity_check = ok`, `user_version = 20`, foreign-key violations = **0**. Every one of the 40 retained AKM assets is used: unlinked assets = **0**.

## Payload hashes
- SQL: 396688 bytes - SHA-256 `0efab7667ccc6fccf3075140fc794e4a759bcc211909c4e3e1e4ac547ac7149e`.
- QZ64: 40525 bytes - SHA-256 `f860cd1f3824ccfb8a91105bc2cd35d70d1d9bcd59cdaa3795dc503d3321394a`; qCompress-style round-trip = **True**.
- manifest candidate: SHA-256 `c8056ffb69c44c10409a9a4f6ccacb79656e67519955d05c60b28bdc38801384`.

## Safety
This batch is documentary/diagnostic knowledge only. It does not modify protocol, acquisition, ECU RAM, write/reset commands, Qwen/ONNX, UI or 32-bit code. The wiring page stores the raw printed ECU pin -> wire-code map only; it does not invent component functions for unlabeled/ambiguous traces.

Production rollback remains `MEMSX64` BUILD #102 commit `06eca1a478db3d32e9ae88d040e1a34e2cc98650` until this candidate is pushed, remotely re-read and validated.
