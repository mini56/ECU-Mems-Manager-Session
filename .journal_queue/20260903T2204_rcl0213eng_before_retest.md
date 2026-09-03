## 2026-09-03 — RCL0213ENG generalization correction ready for exact retest

Temporary branch `tmp-ravemems-v2-foundation` now contains the generic correction through SHA `722707ecd6c9a004575ec4ee80105045283c9f38` (including parent `ae93629ab9375c68a9778ad55fbea7a091e2976f`).

Changes are limited to the RAVEMEMS V2 generic extraction layer:
- publication identity is derived from the detected publication code, so `RCL 0213ENG` must yield `DOC_RCL0213ENG` / `REV_RCL0213ENG_SOURCE`; `RCL0193ENG` derives its historical keys unchanged;
- document family classification now uses stable source-title evidence such as `ELECTRICAL REFERENCE LIBRARY`, with corpus-path fallback only when title evidence is absent;
- Electrical Reference Library documents create source-grounded `ravemems_section` entries per reference page, with manufacturer identifiers when present and provenance tied to the page; no workshop operations/phases/steps are fabricated.

No production/UI/protocol/IA files were modified. `MEMSX64` remains protected at BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.

Next action after this journal is GREEN: pin the isolated 212-page ELXN970E workflow to SHA `722707ecd6c9a004575ec4ee80105045283c9f38`, require correct identity/type plus non-zero reference sections and zero spurious workshop operations, run all 212 pages, then inspect the exact artifact and journal the result.
