## 2026-09-03 — RCL0213ENG retest reveals multiline source-title boundary

Exact isolated run `33800789001` tested SHA `97d2e7b0a90825e2c2efbf928f2002623d16d81d` and remained RED only on `title_source_identifies_electrical_reference_library`.

All other strict checks passed again:
- 212 pages;
- `DOC_RCL0213ENG` / `REV_RCL0213ENG_SOURCE`;
- document kind `electrical_reference_library`;
- 212 reference sections and 212 section provenance rows;
- 0 workshop operations/phases/steps;
- 304/304 exact visual replay verified, 0 failures, 0 missing, 0 unexpected;
- visual replay audit issues 0;
- SQLite integrity `ok`, 0 FK issues;
- 0 numeric defects and 0 open review flags;
- protected `MEMSX64` BUILD #103 unchanged.

The corrected row is now `('DOC_RCL0213ENG', 'RCL 0213ENG', 'electrical_reference_library', 'RCL 0213ENG')`. This proves the family-aware selection no longer picks the wrong `Workshop Manual` title, but it does not yet reconstruct a family title split over multiple PDF text lines. Classification succeeds because its evidence normalization joins whitespace globally; title selection still examines one source line at a time. In this PDF, the Electrical Reference Library title is split across adjacent lines, so the exact family phrase is not found on one line and the fallback publication code is used.

Artifact: `RAVEMEMS-V2-RCL0213ENG-212PAGE-GENERALIZED-TEST`, ID `9910973725`, size 28,315,431 bytes, ZIP SHA256 `63f682ea0afbdc9ea6ab3ceabdde24fbb819e9bbbd08dbb0347d429f4876c609`.

Next action: make the generic family-aware title selector reconstruct adjacent non-empty source lines in a bounded window, preserving source wording, then rerun the same 212-page strict test. No page/publication hardcoding and no production changes.
