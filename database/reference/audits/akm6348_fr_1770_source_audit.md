# AKM 6348 FR / batch 1770 — source registration audit

Date: 2026-08-29

## Purpose

Register the French Mini repair manual `AKM 6348 - FRENCH` in the additive knowledge foundation before page-level extraction. This batch does not rewrite historical tables and does not add unverified technical values from individual pages.

## Source state

- Public indexed representation: `Rover Mini Manuel Atelier FR 1991`, Scribd document 935795924.
- Indexed page count: 328 pages.
- Indexed scope: Mini produced from March 1976.
- Direct public PDF URL identified separately: `Rover-Mini-Manuel-atelier-fr.pdf`; HTTP metadata observed a content length of 13,083,374 bytes.
- Exact PDF SHA-256 and physical-page mapping are not yet pinned in the repository at this stage.

Because the exact retained binary has not yet been hashed, this first batch uses `verification_level=source_externe`. No page-specific specification, procedure, torque, wiring value or illustration is promoted to `verifie_constructeur` by batch 1770.

## Rows added by 1770

- 1 `mems_applicability_scope`: Mini, year_from 1976, all unspecified axes kept NULL.
- 1 `mems_scope_constraint`: production date >= `1976-03`.
- 1 `mems_knowledge_item`: documentary identity/source record.
- 1 `mems_knowledge_scope` link.
- 5 French `mems_term_alias` rows for deterministic retrieval.

## Payload fingerprints

- decoded SQL: 2715 bytes — SHA-256 `4222b2c99d7030350a06043fee9c80e927f2db6b3131cb7545fed6f0d81e3336`;
- final qz64: 1497 bytes — SHA-256 `0de7f667adda6e4b4770f3c9426ec3cbc7299a9606eff533e2f015690d6b5ca0`.

## Safety

Documentary/database only. No ECU communication, protocol, acquisition, RAM, write/reset, Qwen/ONNX, UI or 32-bit behavior is modified. Production `MEMSX64` remains BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Next extraction action

Acquire/hash the exact French PDF binary, verify its page count/identity, then map and capture useful original pages. Page-level facts and visuals must be linked to exact physical pages and retain their own evidence level. Do not infer a page index from another edition.
