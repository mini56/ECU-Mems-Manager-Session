# RAVE batch 1720 — AKM7169ENG Mini SPi classique, portée 1993+ / marché non précisé

## Source

- Rover Group Limited factory manual: `AKM7169ENG`, Mini Repair Manual, publication part no. `AKM7169`, 1993.
- Scope stated by the manual introduction: all Mini models manufactured from VIN `049349`.
- SPi pages used: `Engine Tuning Data 3` (manual gearbox), `Engine Tuning Data 4` (automatic gearbox), `Engine Tuning Data 5` (high compression engine).
- Publication identity is independently listed by Original Technical Publications as `Service Manual - Mini All Models 1993 on (AKM7169)`.
- Primary manual text consulted through an indexed scan of AKM7169ENG. The relevant SPi tuning pages do not state UK, Europe or Japan market scope.

## Data policy

- This batch is intentionally aimed at the classic/non-Japan-underrepresented Mini SPi coverage.
- No market is inferred: `market_non_precise` is retained whenever the AKM7169 page does not specify UK, Europe, Japan or export.
- The manual-wide VIN `049349` start is kept as document scope; it is not converted into an ECU-specific VIN breakpoint when the individual tuning-data page does not state one.
- `High compression engine` is kept verbatim and is **not** automatically renamed `Cooper`.
- OCR-ambiguous tuning values are excluded.
- The fuel-pressure tolerance from AKM7169 is kept separate from the later RCL0193 1997+ fact; the two tolerances are not merged.
- All retained facts are direct constructor statements and are stored as `verifie_constructeur`.
- ECU-specific mirror facts use the exact ECU code where the factory page gives it; generic SPi facts keep `firmware_code='*'`.

## Added facts

| Fact key | Scope | Factory statement retained |
|---|---|---|
| `RAVE-SPI93-SCOPE-001` | Mini SPi, AKM7169, 1993+, market non précisé | AKM7169ENG is Rover Group Limited 1993, applies from VIN 049349, and contains MEMS-SPi material; no market is inferred from the used pages. |
| `RAVE-SPI93-ECU-MANUAL-001` | 1.3 Mini SPi manual | `MNE101040` |
| `RAVE-SPI93-ECU-AUTO-001` | 1.3 Mini SPi automatic | `MNE101060` |
| `RAVE-SPI93-ECU-HICOMP-001` | 1.3 Mini SPi high compression | `MNE101070` |
| `RAVE-SPI93-SPEEDDENSITY-001` | Tuning Data 3–5 | Indirect single-point injection with ECU control using speed/density air-flow measurement. |
| `RAVE-SPI93-TPS-001` | Tuning Data 3–5 | TPS voltage: 0–1 V closed; 4–5 V open. |
| `RAVE-SPI93-FUELPRESS-001` | Tuning Data 3–5 | Fuel pressure: 1.0 bar, ±4.0%, constant. |

## Validation before repository integration

The payload was round-tripped through the Qt-compatible `qCompress` representation and applied to a copy of the real `ia_mems_reference_r20.sqlite` extracted from GitHub Actions #94.

- before: **86 RAVE / 98 expert**
- after: **93 RAVE / 105 expert**
- new `SRC-AKM7169` facts in `mems_rave_fact`: **7**
- mirrored `SRC-AKM7169` facts in `mems_expert_fact_external`: **7**
- pre-existing `RAVE-SPI93-*` keys before application: **0**
- all new verification levels: `verifie_constructeur`
- ECU mirror mappings: `MNE101040`, `MNE101060`, `MNE101070`; generic facts remain `*`
- database integrity: `ok`
- database schema/revision unchanged: `user_version = 20`
- decompressed SQL length: **10086 bytes**
- decompressed SQL SHA-256: `2b68af4bd031908a5a46f4f9acae65e673edb2ee17772e6afdc2c9d4cfe43bb9`
- qz64 file length: **2161 bytes**
- qz64 file SHA-256: `e9d91ad14edf8340131050df933459b05b889927a4310db5235c657ba4862e77`

## Separation from the postponed Japan lot

The previously identified RCL0194 SPi Japan candidates (`C159-5 GU`, `C159-25 WB` / `C161-18 WS`, `C159-19 RW`) are not included here. They remain postponed for a later, separate Japan-specific batch so that this batch can improve classic Mini SPi coverage without mixing markets.
