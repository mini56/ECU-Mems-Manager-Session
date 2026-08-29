# RAVE visual backfill — inventory before asset capture

Date: 2026-08-29

## Immutable base / scope

- Production base inspected: `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Production is **not modified** by this chantier.
- Working branch: `tmp-rave-visual-backfill`.
- Official packaged SQLite inspected from BUILD #101: `database/expert/ia_mems_reference_r20.sqlite`.
- Current RAVE facts: **93**.
- Current historical RAVE facts must remain unchanged; visual backfill is additive only.
- The validated RAVE 1680 pilot mechanism is the architectural reference: local original asset + additive fact↔asset link + source/page/hash + manifest resolver + user-openable resource.

## Exact provenance of the 93 existing RAVE facts

| Batch | Source | Facts | Current visual-reference state | Visual backfill target |
|---|---|---:|---|---|
| 1660 | `RCL0193ENG` Mini Workshop Manual 1997–2000 | 31 | `image_ref` empty on all 31 facts; `source_section` identifies PDF pages/sections | Capture the 26 distinct cited PDF pages and link each fact to all pages needed by its source section |
| 1670 | `RCL0194ENG` MPi 97MY | 15 | `image_ref` already records factory page labels | Capture `15.1`, `20.1`, `20.2`, `39.1` and resolve combined refs without duplicating assets |
| 1680 | `RCL0194ENG` SPi Japan 97MY wiring | 14 | `image_ref` covers `20.3`, `20.4`, `39.3` | Already proven by pilot; reuse the same three original assets and links |
| 1690 | `RCL0194ENG` SPi Japan wire colours | 7 | all facts point to `20.4`; colour-code legend is cited by audit but not separately linked | Reuse `20.4`; also capture/link the exact Rover colour-code legend page once identified |
| 1700 | `RCL0194ENG` injector/purge/IAC colours | 8 | all facts point to `20.4`; colour-code legend cited by audit | Reuse `20.4`; reuse the single colour-code legend asset |
| 1710 | `RCL0194ENG` SPi Japan 20.3 colours | 11 | all facts point to `20.3`; colour-code legend cited by audit | Reuse `20.3`; reuse the single colour-code legend asset |
| 1720 | `AKM7169ENG` Mini SPi classic 1993+ | 7 | `image_ref` empty; logical source sections recorded | Capture Introduction/publication scope and `Engine Tuning Data 3`, `4`, `5`; exact PDF page indexes to be verified against the source scan before extraction |

Total: **31 + 15 + 14 + 7 + 8 + 11 + 7 = 93 facts**.

## Document-level inventory

### 1. RCL0193ENG — 31 facts / batch 1660

The BUILD #101 SQLite records 31 facts from `Mini Workshop Manual — RCL0193ENG`. None currently carries an `image_ref`.

Distinct cited PDF pages extracted from the 31 `source_section` values:

`38, 39, 40, 98, 101, 107, 108, 109, 112, 113, 114, 117, 118, 120, 121, 122, 123, 125, 126, 127, 128, 129, 130, 131, 135, 170`

That is **26 distinct PDF pages** for the existing knowledge only.

Topics covered by those pages include:
- Engine Tuning Data (SPi/MPi fuel pressure, TPS, lost-motion gap);
- EVAP purge;
- engine-management strategy;
- CKP, MAP, ECT, IAT, TPS, HO2S, IACV and injectors;
- throttle cable adjustment/removal;
- throttle pedal/housing/TP sensor;
- ignition coil;
- ECM;
- IACV/IAT/ECT/CKP/MAP removal-refit procedures;
- injector service.

Important: some facts cite two pages or page ranges. Backfill must link the fact to **all necessary source pages**, not arbitrarily choose only the first page.

Source status before capture:
- publication identity `RCL0193ENG` 5th Edition is verified;
- a public 372-page RAVE PDF source has been located and identifies itself as RCL0193ENG / Rover Technical Communication;
- mapping between the historical `PDF p.xxx` references and the chosen source file must be checked before committing any image asset/hash.

### 2. RCL0194ENG — 55 facts / batches 1670–1710

All 55 RCL0194 facts already contain textual `image_ref` values. The **seven unique factory page labels** required by existing facts are:

`15.1`, `20.1`, `20.2`, `20.3`, `20.4`, `39.1`, `39.3`

In addition, batches 1690–1710 explicitly use the Rover factory **wire-colour legend** to expand abbreviations. Its exact page/figure label is not yet represented by `image_ref`; it must be identified and stored as an additional supporting asset rather than inferred.

Current state:
- `20.3`, `20.4`, `39.3` were extracted as original page images in the validated RAVE 1680 pilot;
- `20.3` was selected automatically from a natural user query, proposed as an exact RAVE resource, clicked and opened successfully on the real PC;
- the backfill must **reuse** these assets rather than store duplicate copies;
- remaining direct fact pages to add: `15.1`, `20.1`, `20.2`, `39.1`;
- supporting legend page remains to be identified/captured once.

### 3. AKM7169ENG — 7 facts / batch 1720

The BUILD #101 SQLite records seven facts from `Rover Mini Repair Manual — AKM7169ENG`. Their current `image_ref` values are empty.

Logical sections already recorded:
- `Introduction / publication scope`;
- `Engine Tuning Data 3` — SPi manual / MNE101040;
- `Engine Tuning Data 4` — SPi automatic / MNE101060;
- `Engine Tuning Data 5` — SPi high-compression / MNE101070;
- facts common to Tuning Data 3–5: speed/density, TPS range, fuel pressure.

Before capture, the exact PDF page indexes of these four logical visual targets must be verified against the selected factory scan. No page number is to be invented from the section name.

## Current packaged visual coverage before backfill

BUILD #101 contains six generic local SVG diagrams only:
- MEMS 1.2 ECU;
- MEMS 1.3 ECU;
- MEMS 1.6 ECU;
- MEMS 1.9 ECU;
- ROSCO 3-pin;
- MEMS 1.9 OBD 16-pin.

The original RAVE pages are not present in official #101. The validated pilot added only:
- `RCL0194ENG 20.3`;
- `RCL0194ENG 20.4`;
- `RCL0194ENG 39.3`.

## Backfill architecture to reuse

The chantier must keep the pilot principles and generalise them, not create a second parallel system:

1. one physical source asset per unique document/page/figure;
2. SHA-256 and source-document identity retained;
3. additive relation allowing multiple facts/knowledge items to point to the same asset;
4. one fact may point to multiple assets when its source section spans several pages;
5. exact source page/figure label kept separately from the rendered filename;
6. no historical `mems_rave_fact` statement is silently rewritten during visual backfill;
7. if visual inspection contradicts an existing fact, record a divergence/audit item first — do not silently alter the fact;
8. original source visual and structured fact remain complementary;
9. resolver/UI must only propose assets physically present and declared;
10. future source types (tables, figures, exploded views, procedure illustrations) must be representable without forcing them all into a generic `diagram` meaning.

## Schema check triggered by the fundamental rule

The pilot tables are sufficient for a simple page illustration, but the full chantier must audit whether an additional generic asset layer is needed for non-diagram material such as:
- table;
- procedure illustration;
- exploded view;
- photograph;
- chart/graph;
- warning/caution panel;
- multi-page procedure;
- source-document cover/scope page.

If `mems_rave_illustration` cannot represent one of these cleanly, the schema must be extended **additively** before data is lost. No type of useful source material may be discarded only because the present schema has no slot for it.

## First execution order

1. Preserve/reuse the three validated RCL0194 pilot assets.
2. Complete RCL0194 because its mapping is already explicit: `15.1`, `20.1`, `20.2`, `39.1`, plus exact colour-code legend.
3. Backfill RCL0193 using the 26 verified historical PDF page references, after page-index/source validation.
4. Backfill AKM7169 after exact source scan/page mapping is established.
5. Run integrity/hash/link/manifest/resolver checks after each document group.
6. Build a temporary test artifact only after the visual backfill candidate is internally consistent.
7. Use natural/free user questions for real-PC validation; do not validate only with prompts designed around known manifest keys.

## Production guard

`MEMSX64` must remain exactly on BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48` until the backfill candidate has been audited, built on a temporary branch and validated. No #102 is authorised by this inventory step.
