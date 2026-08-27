# RAVE batch 1690 — RCL0194ENG SPi Japan 97MY wire colours

## Source

- Rover Technical Communication factory document: `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition.
- Vehicle scope: Mini 97MY SPi Japan from VIN `SAXXNNAXKBD 134455`.
- Factory page used for signal-to-wire relationships: `Engine Management System (MEMS) SPi (JAPAN) 20.4`.
- Factory colour-code legend used to expand abbreviations: RCL0194ENG colour-code page.

## Data policy

- Only relationships where connector/function and colour code are directly associated on the factory diagram are retained.
- No colour is inferred from textual ordering or from another Mini/MEMS variant.
- The facts are not generalized to MPi, European SPi, or another MEMS generation.
- Existing batch 1680 wiring relationships are preserved; batch 1690 adds explicit colour names so IA MEMS can answer wire-colour questions without guessing.
- All entries are `verifie_constructeur` and mirrored into `mems_expert_fact_external` for read-only IA MEMS retrieval.

## Added facts

| Fact key | Function | Factory relationship |
|---|---|---|
| RAVE-COLOR-SPIJ-001 | diagnostic | C549-2 ↔ C159-10: WY = white/yellow = blanc/jaune |
| RAVE-COLOR-SPIJ-002 | diagnostic | C549-3 ↔ C159-15: BG = black/green = noir/vert |
| RAVE-COLOR-SPIJ-003 | ECT | C165-2 ↔ C159-33: KG = pink/green = rose/vert |
| RAVE-COLOR-SPIJ-004 | sensor earth | common sensor earth ↔ C159-30: KB = pink/black = rose/noir; shared by ECT/TPS/IAT through SJ5 |
| RAVE-COLOR-SPIJ-005 | TPS | C175-2 ↔ C159-8: YG = yellow/green = jaune/vert |
| RAVE-COLOR-SPIJ-006 | TPS | C175-3 ↔ C159-9: YP = yellow/purple = jaune/violet |
| RAVE-COLOR-SPIJ-007 | IAT | C174-2 ↔ C159-16: GB = green/black = vert/noir |

## Validation before push

The SQL payload was round-tripped through the same Qt-compatible `qCompress` representation used by the reference batches, then applied to a copy of `ia_mems_reference_r20.sqlite` extracted from GitHub Actions #88.

- `RAVE-COLOR-SPIJ-*` in `mems_rave_fact`: 7
- mirrored `RAVE-COLOR-SPIJ-*` in `mems_expert_fact_external`: 7
- total RAVE facts after application: 67
- total expert facts after application: 79
- all new verification levels: `verifie_constructeur`
- one variant only: `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`
- database integrity: `ok`
- database schema/revision unchanged: r20
- decompressed SQL length: 8708 bytes
- decompressed SQL SHA-256: `aef8a4c9d808267188c76c25c53dac254cba57f434a16badf7791023ffcfa97a`
- qz64 file length: 1397 bytes
- qz64 SHA-256: `7288e46b99caf41e14751ab5690a7bf17fd35941e1e17527af472bf8bad3ec19`
