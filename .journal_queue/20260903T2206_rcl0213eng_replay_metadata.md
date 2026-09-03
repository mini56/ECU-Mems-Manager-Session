## 2026-09-03 — Additional generic replay metadata defect found before RCL0213ENG retest

While preparing the exact 212-page retest, inspection of `.github/pass2/rcl0193eng_visual_replay_validate.py` found one additional RCL0193ENG-specific residue in evidence metadata: the replay `method` string is hardcoded to `page 1..372`, even when `replay_visuals()` is invoked on another PDF such as 212-page RCL0213ENG.

This does not weaken or falsify pixel comparison itself, but it is incorrect provenance and must be removed before a zero-defect RCL0213ENG result can be accepted.

Next corrective push: make only that evidence range dynamic from `doc.page_count`, preserving the existing exact PNG replay and RCL0193ENG-specific success guard. Then pin/run the 212-page workflow on the resulting SHA. Production remains untouched; `MEMSX64` stays BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
