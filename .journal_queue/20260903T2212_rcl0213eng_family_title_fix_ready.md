## 2026-09-03 — RCL0213ENG family-aware source-title fix ready for exact retest

Temporary RAVEMEMS V2 branch correction SHA: `97d2e7b0a90825e2c2efbf928f2002623d16d81d` (`Select RAVE source title by detected document family`).

The only changed behavior is generic source-title selection: `_document_title_from_evidence` now receives the already-detected `document_kind` and searches only the title pattern for that family. An `electrical_reference_library` therefore selects source evidence matching `Electrical Reference Library`; a `workshop_manual` selects `Workshop Manual`. Unknown document families return no inferred family title rather than borrowing another family label.

No section, visual, protocol, UI, IA, production, or schema behavior was changed. `MEMSX64` remains protected at BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.

Next action after journal success: pin the isolated 212-page RCL0213ENG workflow to SHA `97d2e7b0a90825e2c2efbf928f2002623d16d81d`, rerun all strict identity/structure/visual/audit guards, inspect the artifact, then regression-test RCL0193ENG before declaring the generic correction safe.
