# V1.70 — 80196 bounded control-flow and alternate tables

Seven petrol ROMs (three MEMS 1.3, four 1.6 Turbo); 272 distinct V1.69 sites. A restricted binary decoder follows recognized instructions and both conditional outcomes from the 18 mode-1E pointers per ROM. 217 sites have a documented prefix path (31 per ROM). Seven additional 7F reader-bound comparisons were found, not seven new commands.

Of the other 55 sites, 49 (seven per ROM: 79, 7C, 89, 94, 7E, 7F, CB) lie in a block referenced by a second bounded pointer list and distinct indirect interpreter, verified in every ROM. External entry context is not established. Three F5 sites in MEMS 1.3 are referenced by a five-pointer list loaded after state byte C8 comparison, which does not establish a serial service. Three C4 sites at 30CA remain unresolved; ABEMR002's original 307E window crosses the return at 3095.

| Family | Sites | 1E prefix | Alternate table | C8 state | Unresolved |
|---|---:|---:|---:|---:|---:|
| MEMS 1.3 | 120 | 93 | 21 | 3 | 3 |
| MEMS 1.6 Turbo | 152 | 124 | 28 | 0 | 0 |
| Total | 272 | 217 | 49 | 3 | 3 |

269 sites have differing levels of static association, not validated function semantics. Decoder stops at unsupported instructions; no proof of input feasibility, serial provenance or full coverage. Global scope remains 449 petrol ROMs; this batch only covers seven outstanding ROMs. No physical tests, diesel, or Manager/DLL modifications. Cumulative PDFs/ZIP not updated.
