## 2026-09-03 — RCL0213ENG generalized 212-page retest: single remaining title-source defect

Exact isolated run `33800358993` tested generic SHA `ba96a4795cba4b8cb44bd13cf7b6c1637009cede` against frozen source `rave/xn/elxn970e.pdf` (`RCL 0213ENG`, 212 pages). The run is RED only at the final strict identity assertion `title_source_identifies_electrical_reference_library`.

Verified successful results before that final assertion:
- document key: `DOC_RCL0213ENG`;
- revision key: `REV_RCL0213ENG_SOURCE`;
- document kind: `electrical_reference_library`;
- 212/212 page-grounded `ravemems_section` entries;
- 212/212 section provenance rows;
- 146 entries with detected manufacturer identifiers;
- 212 entries with source-grounded titles;
- 0 fabricated workshop operations, 0 phases, 0 steps;
- 0 numeric defects;
- 0 open review flags;
- exact visual replay: 304 DB visuals, 304 replayed, 304/304 fidelity verified, 0 failed, 0 missing, 0 unexpected;
- 0 visual-link failures (there are 0 operation-linked visual links for this non-procedure document);
- audit issue count after replay: 0;
- SQLite integrity: `ok`; foreign-key issues: 0;
- replay evidence correctly reports page range `1..212`;
- protected `MEMSX64` remains BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.

Artifact: `RAVEMEMS-V2-RCL0213ENG-212PAGE-GENERALIZED-TEST`, ID `9910816415`, size 28,315,506 bytes, ZIP SHA256 `7ced4eb27bc15d4e0f0b6fa5fbb42a428849e1934b1331e6613748e72a646009`.

Correction of an earlier reported number: the exact first and current test artifacts/logs contain **304 visuals**, not 395. The previously stated 395/395 figure was incorrect and must not be reused.

Single remaining defect: document row is `('DOC_RCL0213ENG', 'RCL 0213ENG', 'electrical_reference_library', 'Workshop Manual')`. Root cause is generic title selection scanning source lines first and returning the first title-family phrase encountered, while document-family classification correctly prioritizes `Electrical Reference Library` evidence across the source. The title selector must use the already-selected document family and return only the matching source title phrase.

Next action: make `_document_title_from_evidence` family-aware, without page/publication hardcoding or any other semantic change, then rerun the exact isolated 212-page test and regression-check RCL0193ENG.
