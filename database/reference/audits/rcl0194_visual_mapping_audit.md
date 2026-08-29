# RCL0194ENG visual source mapping audit

Date: 2026-08-29  
Working branch: `tmp-rave-visual-backfill`  
Production reference: `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`

## Purpose

Verify the physical PDF mapping of every RCL0194 source page required by the 55 existing RAVE facts **before** adding more original page images, and use the visual source to detect provenance errors in legacy facts.

No historical fact or qz64 is modified by this audit.

## Factory source

Document: `RCL0194ENG` — MINI Electrical Circuit Diagrams, 3rd Edition.  
PDF page count: 41.  
Source URL used by the validated RAVE 1680 pilot:

`https://www.goclassic.eu/upload/content/e68ce7_9c0fd6a5b3a14a3585ecddb02b3ceb9d.pdf`

Source PDF SHA-256 already recorded by the validated pilot:

`64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051`

## Verified physical mapping

| PDF index (zero-based) | Factory page/role | Verified content |
|---:|---|---|
| 5 | COLOUR CODES / `RCL 0145` | B=black/noir, G=green/vert, K=pink/rose, LG=light green/vert clair, N=brown/brun, O=orange, P=purple/violet, R=red/rouge, S=slate/grey, U=blue/bleu, W=white/blanc, Y=yellow/jaune |
| 13 | `15.1` | Charging and Starting — MPi — 97MY from VIN SAXXNNAZEBD 134455 |
| 15 | `20.1` | Engine Management System (MEMS) — MPi — 97MY from VIN SAXXNNAZEBD 134455 |
| 16 | `20.2` | Engine Management System (MEMS) — MPi — 97MY from VIN SAXXNNAZEBD 134455 |
| 17 | `20.3` | Engine Management System (MEMS) — SPi Japan — already packaged by pilot |
| 18 | `20.4` | Engine Management System (MEMS) — SPi Japan — already packaged by pilot |
| 24 | `39.1` | **HEATER BLOWER** |
| 25 | `39.2` | **COOLING FAN — MPi** |
| 26 | `39.3` | COOLING FAN — SPi Japan — already packaged by pilot |

The contents page itself also lists `HEATER BLOWER ... 39.1`, `COOLING FAN MPi ... 39.2`, and `SPi (JAPAN) ... 39.3`.

## Proven legacy divergence: `39.1` must not be used as Cooling Fan MPi

The BUILD #101 SQLite contains exactly three RCL0194 MPi facts with a current legacy reference to `39.1`:

### `RAVE-WIR-MPI-006`
- topic: `cooling_fan_wiring`
- current source section: `Engine Management System (MEMS) MPI, 20.1; Cooling Fan MPI 39.1`
- current image ref: `RCL0194ENG:20.1,39.1`
- statement: command circuit LGB through C448-9/C162-9 to ECU C159-28; fan relay C019; fan C005.

### `RAVE-WIR-MPI-008`
- topic: `coolant_sensor_wiring`
- current source section: `Engine Management System (MEMS) MPI, 20.2; Cooling Fan MPI 39.1`
- current image ref: `RCL0194ENG:20.2,39.1`
- statement: coolant sensor C169; ECU C159-15; sensor earth C159-13.

### `RAVE-WIR-MPI-012`
- topic: `sensor_ground`
- current source section: `Engine Management System (MEMS) MPI, 20.2; Cooling Fan MPI 39.1`
- current image ref: `RCL0194ENG:20.2,39.1`
- statement: C159-13 SENSOR EARTH, shared sensor return network.

## Cross-check against the actual page `39.2`

The factory `39.2` page contains:
- `COOLING FAN` / `MPI`;
- fan `C005`;
- fan relay `C019`;
- ECU route including `C159-28`;
- coolant temperature sensor `C169`;
- sensor/ECU connections `C159-15` and `C159-13`.

Therefore the technical content of the three legacy facts is consistent with `39.2`; the **source-page label is the erroneous element**.

## Consequence for visual backfill

- Do **not** package `39.1` as evidence for these Cooling Fan MPi facts.
- Package `39.2` as the correct original source page.
- Preserve `39.1` in the audit as the old legacy value.
- Any later correction visible to IA/user must be explicit and traceable rather than silently editing old provenance.
- The validated pilot assets `20.3`, `20.4`, `39.3` must be reused.

## Additional confirmation relevant to MAP scope

The actual MPi page `20.2` contains a MAP sensor connector `C186` and the associated ECU/sensor-earth wiring. This is distinct from the SPi architecture where MAP is integrated in the ECU. This audit therefore supports keeping the SPi/MPi distinction rather than applying the SPi integrated-MAP rule to MPi.

This note is source audit only; no MAP fact is rewritten by this commit.

## Next data step after this audit

Prepare an additive RCL0194 visual backfill candidate containing the original source assets for:
- colour legend (PDF index 5);
- `15.1` (13);
- `20.1` (15);
- `20.2` (16);
- `39.2` (25);

and reusing the pilot assets:
- `20.3` (17);
- `20.4` (18);
- `39.3` (26).

Validate source PDF hash, image hashes, page labels, fact↔asset links and resolver behavior before any temporary real-PC build.
