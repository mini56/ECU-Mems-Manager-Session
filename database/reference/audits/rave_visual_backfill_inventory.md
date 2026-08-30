# RAVE visual backfill — inventory before asset capture

Date: 2026-08-29

## Immutable base / scope

- Production base inspected: `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Production is **not modified** by this chantier.
- Working branch: `tmp-rave-visual-backfill`.
- Official packaged SQLite inspected from BUILD #101: `database/expert/ia_mems_reference_r20.sqlite`.
- Current RAVE facts: **93**.
- Current historical RAVE facts remain untouched by this inventory/audit step.
- The validated RAVE 1680 pilot mechanism is the architectural reference: original local asset + additive fact↔asset link + source/page/hash + manifest resolver + user-openable resource.

## Exact provenance of the 93 existing RAVE facts

| Batch | Source | Facts | Current visual-reference state | Visual backfill target |
|---|---|---:|---|---|
| 1660 | `RCL0193ENG` Mini Workshop Manual 1997–2000 | 31 | `image_ref` empty; `source_section` identifies PDF pages/sections | Capture the 26 distinct cited PDF pages and link every fact to all pages required by its source section |
| 1670 | `RCL0194ENG` MPi 97MY | 15 | factory page labels present, but 3 legacy facts incorrectly cite `39.1` for Cooling Fan MPi | Capture `15.1`, `20.1`, `20.2`, **`39.2`**; preserve an explicit audit of legacy `39.1` before correction |
| 1680 | `RCL0194ENG` SPi Japan 97MY wiring | 14 | `20.3`, `20.4`, `39.3` | Reuse the three validated pilot assets |
| 1690 | `RCL0194ENG` SPi Japan wire colours | 7 | `20.4`; colour legend used but not separately linked | Reuse `20.4`; capture/link Rover colour-code legend |
| 1700 | `RCL0194ENG` injector/purge/IAC colours | 8 | `20.4`; colour legend used but not separately linked | Reuse `20.4`; reuse colour-code legend |
| 1710 | `RCL0194ENG` 20.3 wire colours | 11 | `20.3`; colour legend used but not separately linked | Reuse `20.3`; reuse colour-code legend |
| 1720 | `AKM7169ENG` Mini SPi classic 1993+ | 7 | `image_ref` empty; logical sections recorded | Capture Introduction/publication scope and `Engine Tuning Data 3`, `4`, `5` after exact PDF-index verification |

Total: **31 + 15 + 14 + 7 + 8 + 11 + 7 = 93 facts**.

## 1. RCL0193ENG — 31 facts / batch 1660

None of the 31 facts currently carries an `image_ref`.

Distinct PDF pages cited by the existing `source_section` fields:

`38, 39, 40, 98, 101, 107, 108, 109, 112, 113, 114, 117, 118, 120, 121, 122, 123, 125, 126, 127, 128, 129, 130, 131, 135, 170`

That is **26 distinct PDF pages** for the existing knowledge.

Topics include tuning data, fuel pressure, TPS, EVAP, CKP, MAP, ECT, IAT, HO2S, IACV, injectors, throttle cable/pedal/housing, ignition coil, ECM and removal/refit procedures.

Some facts cite multiple pages: the final relation must support **one fact → several assets**.

Before capture, the historical `PDF p.xxx` references must be mapped against the exact retained RCL0193 factory scan; no page offset may be guessed.

## 2. RCL0194ENG — 55 facts / batches 1670–1710

Factory PDF retained by the validated pilot:
- `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition;
- 41 PDF pages;
- source PDF SHA-256: `64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051`.

Verified physical mapping:

| Source page | PDF index | Meaning | State |
|---|---:|---|---|
| colour legend | 5 | `COLOUR CODES` / Rover `RCL 0145` | to add once |
| `15.1` | 13 | Charging and Starting — MPi | to add |
| `20.1` | 15 | Engine Management System (MEMS) — MPi | to add |
| `20.2` | 16 | Engine Management System (MEMS) — MPi | to add |
| `20.3` | 17 | Engine Management System (MEMS) — SPi Japan | pilot asset validated |
| `20.4` | 18 | Engine Management System (MEMS) — SPi Japan | pilot asset available |
| `39.1` | 24 | **Heater Blower** | not a Cooling Fan MPi source |
| `39.2` | 25 | **Cooling Fan — MPi** | to add; correct target for 3 MPi facts |
| `39.3` | 26 | Cooling Fan — SPi Japan | pilot asset available |

### Legacy source-reference divergence

Exactly three historical MPi facts currently cite `39.1` even though the correct factory page is `39.2`:

- `RAVE-WIR-MPI-006` — cooling fan wiring;
- `RAVE-WIR-MPI-008` — coolant sensor wiring;
- `RAVE-WIR-MPI-012` — sensor ground.

Their technical content matches the real `39.2` page (including C159-28 / C019 / C005 and C169 / C159-15 / C159-13).

**Do not silently rewrite them.** The wrong legacy ref and the verified replacement must remain auditable.

## 3. AKM7169ENG — 7 facts / batch 1720

Current logical source sections:
- `Introduction / publication scope`;
- `Engine Tuning Data 3`;
- `Engine Tuning Data 4`;
- `Engine Tuning Data 5`;
- common `Tuning Data 3–5`.

Exact PDF indexes must be verified against the selected factory scan before any visual extraction.

## Current production visual coverage

Official BUILD #101 includes only the six generic local SVG diagrams.

The validated pilot V2 adds original RAVE pages:
- `RCL0194ENG 20.3`;
- `RCL0194ENG 20.4`;
- `RCL0194ENG 39.3`.

`20.3` has been selected from a natural user question, proposed, clicked and opened correctly on the real PC.

## Architecture / data rule

1. One physical asset per unique document/page/figure; no duplicate copies.
2. Preserve source document, page/figure label, physical PDF index and SHA-256.
3. Multiple facts may share one asset.
4. One fact may link to multiple assets.
5. Keep legacy source fields for audit; corrections must be explicit and traceable.
6. Visual source and structured fact complement each other.
7. If visual inspection contradicts existing data, record the divergence before correcting anything.
8. Resolver/UI may only expose physically present, declared assets.
9. Non-diagram visual types (table, procedure page, exploded view, photo, chart, warning panel, cover/scope page) must be representable; extend schema additively if needed.
10. No useful source information may be discarded because the current schema lacks a slot.

## Execution order

1. Complete RCL0194: add `15.1`, `20.1`, `20.2`, `39.2` and colour legend; reuse `20.3`, `20.4`, `39.3`.
2. Establish a traceable correction path for the three legacy `39.1` references.
3. Validate hashes, links and user resolution for RCL0194.
4. Backfill RCL0193 26 pages after exact scan/index mapping.
5. Backfill AKM7169 after exact scan/index mapping.
6. Build a temporary test artifact only after internal consistency checks.
7. Validate with natural/free user wording, not only prompts designed around manifest keys.

## Production guard

`MEMSX64` remains exactly BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`. No #102 is authorised by this inventory/audit step.
