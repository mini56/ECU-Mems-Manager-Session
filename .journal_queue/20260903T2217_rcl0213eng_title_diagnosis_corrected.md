## 2026-09-03 — RCL0213ENG title diagnosis corrected before next push

After the RED run `33800789001`, a safe re-read of the temporary branch following a GitHub 409 conflict showed that the branch already contains commit `7418419993af12d5d4c8a1e57673984dbedc4675` (`Accept source Electrical Library title generically`), parented directly on `97d2e7b0a90825e2c2efbf928f2002623d16d81d`.

This commit changes only the electrical family title regex from requiring `Electrical Reference Library` to accepting either `Electrical Reference Library` or the source title `Electrical Library`. The previous run was pinned to the parent SHA `97d2e7...`, so it intentionally did not test this newer correction.

The exact run output itself shows page-1 section source title `Electrical Library`. Therefore the prior working hypothesis that the title necessarily had to be reconstructed from split `ELECTRICAL / REFERENCE / LIBRARY` lines is not established and must not be implemented merely to satisfy an over-specific guard. The truthful source title may be `Electrical Library`, while the normalized document family remains `electrical_reference_library`.

Next action: test exact SHA `7418419993af12d5d4c8a1e57673984dbedc4675` on all 212 pages, and change the strict workflow identity guard so it accepts a source title containing either `electrical library` or `electrical reference library`. All other identity, structure, exact visual replay, audit, SQLite/FK and protected-production guards remain unchanged.

`MEMSX64` remains BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
