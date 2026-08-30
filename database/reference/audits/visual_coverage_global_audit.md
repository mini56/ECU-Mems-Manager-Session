# Global visual coverage audit — ECU MEMS Manager reference base

Date: 2026-08-29

Working branch: `tmp-rave-visual-backfill`

Audited HEAD before this audit file: `a4c1dea0b4bae048a1deabd676de54ac12dd978a`

Production remains strictly `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

This audit is documentary only. It does not change any qz64, image, historical fact, ECU communication, protocol, acquisition, RAM, write/reset, UI, Qwen/ONNX or 32-bit code.

## 1. Current visual-resource inventory

The reference package currently contains **40 physical image/diagram resources** under `database/reference/images`:

| Group | Physical resources | Format | Origin/status |
|---|---:|---|---|
| Generic MEMS/diagnostic diagrams | 6 | SVG | pre-existing before the RAVE visual-backfill project |
| RCL0194ENG | 8 | PNG | RAVE visual backfill / pilot + batch 1750 |
| RCL0193ENG | 26 | PNG | RAVE visual backfill / batch 1760 |
| AKM7169ENG | 0 | — | blocked on exact reproducible source binary |
| **Total** | **40** | 6 SVG + 34 PNG | |

Therefore the current RAVE visual-backfill work added/consolidated **34 original RAVE page/visual assets**. The 6 generic SVG diagrams are not counted as newly extracted RAVE material.

### Six pre-existing generic resources

1. `mems_1_2_ecu_connector.svg`
2. `mems_1_3_ecu_connectors.svg`
3. `mems_1_6_ecu_connector.svg`
4. `mems_1_9_ecu_connector.svg`
5. `mems_1_9_obd_16pin.svg`
6. `rover_rosco_3pin_black.svg`

### Eight RCL0194ENG assets

- `COLOUR CODES` legend;
- `15.1`;
- `20.1`;
- `20.2`;
- `20.3`;
- `20.4`;
- `39.2`;
- `39.3`.

All eight are present physically, registered in `manifest.visual_assets`, registered in `manifest.diagrams`, hashed, and linked to historical RAVE knowledge through batch 1750.

### Twenty-six RCL0193ENG assets

PDF source pages:

`38, 39, 40, 98, 101, 107, 108, 109, 112, 113, 114, 117, 118, 120, 121, 122, 123, 125, 126, 127, 128, 129, 130, 131, 135, 170`.

All 26 are present physically, registered in `manifest.visual_assets`, registered in `manifest.diagrams`, hashed, and linked to historical RAVE knowledge through batch 1760.

## 2. Manifest/catalog state

`manifest.json` contains:

- **40 `diagrams` entries** = 6 generic + 34 RAVE;
- **34 `visual_assets` entries** = the 34 RAVE PNGs only.

The `IaMemsDiagramCatalog` implementation is data-driven: it reads `manifest.diagrams`, rejects unsafe/non-existent paths, checks the local file exists, scores the user's diagram/cabling/pinout query against the declared key/path, and returns the matching local asset only with sufficient evidence. Adding a correctly described future manifest entry does not require a dedicated C++ case.

This means the **40 declared diagram files are physically available to the catalog** on this branch.

## 3. Historical RAVE fact coverage

The historical base contains **93 RAVE facts**.

| Source document | Historical facts | Facts with visual coverage | Coverage |
|---|---:|---:|---:|
| RCL0194ENG | 55 | 55 | 100% |
| RCL0193ENG | 31 | 31 | 100% |
| AKM7169ENG | 7 | 0 | 0% |
| **Total** | **93** | **86** | **92.47%** |

Therefore **86/93 historical RAVE facts now have at least one original visual source attached**.

### Link-row counts

- batch 1750 / RCL0194ENG: **88 fact→illustration links**;
- batch 1760 / RCL0193ENG: **43 fact→page links**;
- total additive visual links: **131**.

There are more links than facts because one fact can legitimately use more than one source page, and one page can support many facts. This avoids copying the same image for every fact.

RCL0194 batch 1750 additionally records exactly **3 provenance corrections** from historical `39.1` to real `39.2` for the Cooling Fan MPi facts without rewriting the historical rows.

## 4. Accessibility levels

The word “accessible” must be separated into levels so the audit does not overclaim.

### A — Physically present in the package

- generic resources: **6/6**;
- new RAVE resources: **34/34**;
- total: **40/40**.

### B — Registered in the diagram catalog

- generic resources: **6/6**;
- RAVE resources: **34/34**;
- total: **40/40** entries exist in `manifest.diagrams`.

The catalog can resolve and open a declared file when the request reaches it and has enough diagram/cabling/brochage evidence.

### C — Structured RAVE knowledge linked to a visual source

- RCL0194: **55/55 facts**;
- RCL0193: **31/31 facts**;
- AKM7169: **0/7 facts**;
- total: **86/93 facts**.

All **34/34 RAVE assets** are part of this structured visual coverage; no unused/fictitious RAVE asset is counted.

### D — End-to-end validation on the user's real PC

New RAVE asset explicitly validated end-to-end:

- **`RCL0194ENG:20.3` — ✅ VALIDATED**: natural user wording → correct RAVE facts → exact RAVE diagram button → click → correct original RAVE image opens.

The pre-existing generic `MEMS 1.9 ECU` diagram was also confirmed to open during the pilot, but it is not one of the newly extracted RAVE PNGs.

The remaining **33 new RAVE assets must not be labelled individually real-PC validated merely because they are physically present and catalogued**. Their packaging/hash/database integration is validated; their individual user click-path has not been tested one by one.

## 5. Visual information currently not accessible

### AKM7169ENG

The 7 structured historical AKM7169 facts currently have **no visual asset**.

This is intentional and safe, not a forgotten extraction:

- the original batch used text from an indexed AKM7169ENG factory scan;
- the exact PDF/scan binary was not retained;
- public representations currently found advertise inconsistent physical page counts (482 versus 463);
- no physical index is therefore inferred;
- no fake PNG is generated.

Status: `STRUCTURED_DATA_PRESENT / VISUAL_BACKFILL_BLOCKED_ON_EXACT_SOURCE_BINARY`.

Once the exact factory scan is acquired: hash it first, verify edition/page count, physically locate Introduction + Engine Tuning Data 3/4/5, then create the additive assets and links.

## 6. Missing-file / orphan conclusion for the new RAVE assets

For the completed RCL0194 and RCL0193 backfills:

- RAVE visual files expected: **34**;
- RAVE visual files physically present: **34**;
- RAVE visual assets declared in `manifest.visual_assets`: **34**;
- RAVE visual entries declared in `manifest.diagrams`: **34**;
- RAVE assets used by the structured coverage: **34**.

Therefore this audit finds **no missing physical RCL0193/RCL0194 asset and no deliberately created orphan visual** in the completed backfills.

The only known historical visual gap is the **7 AKM7169 facts / 0 AKM asset** case, already explicitly blocked and documented.

## 7. Important program-evolution finding: catalog accessibility versus natural-language routing

The current committed code on `tmp-rave-visual-backfill` is still based on BUILD #101 routing. In `IaMemsTab::sendQuestion`, the order is currently:

1. enrich known generation;
2. run `clarificationPrompt(effectiveQuestion)`;
3. return immediately if clarification is requested;
4. only then call `updateDiagramSuggestion(effectiveQuestion)`.

Therefore an image can be **physically present + manifest-registered + database-linked** yet still fail to be proposed for some natural user formulations because an unnecessary generation clarification can intercept the question first.

The separate RAVE 1680 V2 pilot demonstrated on the real PC that allowing a confidently resolved local diagram to bypass that unnecessary clarification enabled the successful natural-language path that produced and opened `RCL0194ENG:20.3`.

This is a genuine **future program-evolution opportunity**, but it is separate from the database/visual backfill. It must not be silently merged into production. It needs its own report gate, scope, build/test and explicit validation before `MEMSX64` changes.

## 8. Audit summary

- **34 new RAVE visual assets** added/consolidated by this project;
- **40 total diagram/image resources** currently in the reference package including 6 older generic SVGs;
- **34/34 new RAVE assets physically present and catalog-registered**;
- **86/93 historical RAVE facts visually covered = 92.47%**;
- **131 structured fact→visual links**;
- **RCL0194 = 100% visual coverage**;
- **RCL0193 = 100% visual coverage**;
- **AKM7169 = 0/7 visual coverage**, explicitly blocked on exact source binary;
- **1 new RAVE image (`20.3`) individually validated end-to-end on the user's real PC**;
- **33 other new RAVE images packaged/linked/catalogued but not individually real-PC clicked/validated**;
- no automatic control of ECU communication is introduced by any visual/database information.

## Next safe actions

1. Keep searching for an exact reproducible AKM7169ENG source binary; do not invent pagination.
2. When desired, create a separate functional pilot to generalize the already-proven V2 diagram-routing behavior so the 34 RAVE visuals are reliably proposed from natural wording.
3. Continue future RAVE enrichment under the permanent rule: structured information + original tables/views/illustrations + source/hash + user accessibility whenever the source permits it.
