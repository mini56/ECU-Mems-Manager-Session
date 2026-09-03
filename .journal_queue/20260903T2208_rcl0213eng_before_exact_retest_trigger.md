## 2026-09-03 — RCL0213ENG exact retest trigger preparation

Generic correction state is now at temporary-branch SHA `ba96a4795cba4b8cb44bd13cf7b6c1637009cede`.

In addition to publication-derived identity and electrical-reference section creation, a generic visual replay adapter now corrects only replay evidence metadata so the reported page range is the actual PDF page count. The validated RCL0193ENG replay implementation and its 738/401 standalone guard are unchanged.

Next push will only repoint/strengthen the isolated `tmp-ravemems-v2-elxn970e-full-test.yml` workflow to this SHA and require:
- exact document key `DOC_RCL0213ENG`;
- exact revision key `REV_RCL0213ENG_SOURCE`;
- `electrical_reference_library` document kind;
- non-zero page-grounded electrical reference sections and provenance;
- zero fabricated workshop operations/phases/steps;
- exact visual replay with zero failures/missing/unexpected visuals;
- zero open reviews, zero audit issues, SQLite integrity OK, zero FK issues;
- protected `MEMSX64` still BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
