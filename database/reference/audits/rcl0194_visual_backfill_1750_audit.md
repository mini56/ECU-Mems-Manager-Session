# RCL0194 visual backfill — batch 1750 audit

Date: 2026-08-29

Production remains `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.
This batch is documentary only. It does **not** modify ECU communication, protocol,
acquisition, RAM reads, ECU writes/resets, safety routing, Qwen/ONNX runtime, or
the working 32-bit path. Database/IA/RAVE knowledge remains consultative and can
never take control of ECU communication.

Primary source: Rover `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition,
41 PDF pages, SHA-256 `64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051`.

Eight source visuals are retained once: COLOUR CODES, 15.1, 20.1, 20.2, 20.3,
20.4, 39.2 and 39.3. They supplement structured database facts; they do not
replace structured knowledge.

Important source findings:
- `20.2` directly confirms an external MAP SENSOR (204) on MPi, distinct from
  the integrated MAP arrangement established for SPi.
- Factory `39.1` is HEATER BLOWER.
- Factory `39.2` is COOLING FAN — MPi.
- Historical facts `RAVE-WIR-MPI-006`, `RAVE-WIR-MPI-008`,
  `RAVE-WIR-MPI-012` are preserved unchanged and receive an additive effective
  source correction to `RCL0194ENG:39.2`.

Batch `research_enrichment_1750.qz64` is generated deterministically from the
historical RCL0194 facts reconstructed from the BUILD #101 seed/enrichment set.
It adds only visual-evidence/link/correction structures.

Validation:
- `PRAGMA integrity_check`: `ok`
- `PRAGMA user_version`: `20`
- historical RAVE facts: `93`
- historical expert facts: `105`
- RCL0194 visual assets: `8`
- RCL0194 direct/effective visual coverage: `55/55`
- SPi Japan colour facts with Rover colour-code legend: `26/26`
- explicit historical source corrections: `3`

Generated qz64:
- bytes: `2485`
- SHA-256: `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`
- decompressed SQL bytes: `25941`
- SQL SHA-256: `e9b10b33b67165dc14effc81fb8a62308ab5340ed8a5d38b0d7f6aec43191090`
