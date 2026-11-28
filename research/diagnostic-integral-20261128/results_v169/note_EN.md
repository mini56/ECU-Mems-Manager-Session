# V1.69 — Direct binary review of 80196 selector inventory

V1.68 run 46137949056 succeeded: 449 petrol firmwares, 30,839 entries, 2 new sites in KMJ7V001; no remaining previously inventoried 68K service comparison candidates. Full service coverage and semantics are not established.

Seven ROMs checked directly: all 295 prior occurrences match 99 xx 47 followed by a conditional branch; relative target and fallthrough recorded. There are 272 distinct ROM/address sites, 209 equality comparisons and 63 ordering comparisons, with 23 duplicated window hits. Equality does not establish an independent external command; ordering does not necessarily establish an external range.

| ROM | Occurrences | Sites | Duplicates | Equality | Ordering |
|---|---:|---:|---:|---:|---:|
| ABEMR002 | 43 | 40 | 3 | 31 | 9 |
| KBE6R003 | 43 | 40 | 3 | 31 | 9 |
| MGE7R002 | 43 | 40 | 3 | 31 | 9 |
| TCF7X005 | 42 | 38 | 4 | 29 | 9 |
| TCF8X005 | 42 | 38 | 4 | 29 | 9 |
| TFF8X000 | 41 | 38 | 3 | 29 | 9 |
| TFFSX000 | 41 | 38 | 3 | 29 | 9 |
| Total | 295 | 272 | 23 | 209 | 63 |

All seven mode 1E pointer-list bounds (18 pointers each) and indirect dispatch routines are verified in bytes: 2E12 (1.3), 374B (TCF), 374D (TFF). Evidence is in preuves_selecteurs_80196.json.

Limitation: the original 96-byte windows do not prove reachability. For example ABEMR002 site 30CA lies after the return at 3095. Next stage is actual control-flow qualification and Manager comparison. No physical test, diesel or Manager/DLL change. Cumulative PDFs and ZIP not updated.
