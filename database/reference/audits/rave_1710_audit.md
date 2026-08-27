# RAVE batch 1710 — RCL0194ENG SPi Japan 97MY MEMS 20.3 wire colours

## Source

- Rover Technical Communication factory document: `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition.
- Vehicle scope: Mini 97MY SPi Japan from VIN `SAXXNNAXKBD 134455`.
- Factory page used: `Engine Management System (MEMS) SPi (JAPAN) 20.3`.
- Factory colour legend: RCL0194ENG / Rover RCL 0145 colour-code page.

## Data policy

- Only connector/function/colour relationships directly visible on factory page 20.3 are retained.
- The two crankshaft facts are explicitly limited to the manual-transmission path shown on the diagram.
- The automatic crankshaft alternative is not merged with the manual path.
- No fact is generalized to MPi, European SPi, or another MEMS generation.
- All entries are `verifie_constructeur` and mirrored into `mems_expert_fact_external` for IA MEMS retrieval.

## Added facts

| Fact key | Function | Factory relationship |
|---|---|---|
| RAVE-COLOR-SPIJ-016 | main relay control | C159-4: WK = white/pink = blanc/rose |
| RAVE-COLOR-SPIJ-017 | dual pressure switch | C159-35: GW = green/white = vert/blanc |
| RAVE-COLOR-SPIJ-018 | oxygen sensor relay | C159-36: BG = black/green = noir/vert |
| RAVE-COLOR-SPIJ-019 | automatic inhibitor | C159-14: WLG = white/light green = blanc/vert clair |
| RAVE-COLOR-SPIJ-020 | oxygen sensor screen ground | C159-29: B = black = noir |
| RAVE-COLOR-SPIJ-021 | oxygen sensor +VE | C159-7: S = slate/grey = gris |
| RAVE-COLOR-SPIJ-022 | oxygen sensor -VE | C159-18: LGS = light green/slate = vert clair/gris |
| RAVE-COLOR-SPIJ-023 | crankshaft sensor -VE, manual | C159-32: WU = white/blue = blanc/bleu |
| RAVE-COLOR-SPIJ-024 | crankshaft sensor +VE, manual | C159-31: UP = blue/purple = bleu/violet |
| RAVE-COLOR-SPIJ-025 | fuel pump relay control | C159-20: BP = black/purple = noir/violet |
| RAVE-COLOR-SPIJ-026 | ignition sense | C159-11: W = white = blanc |

## Validation before push

The SQL payload was applied to the real r20 expert database extracted from GitHub Actions #89.

- new facts in `mems_rave_fact`: 11
- mirrored entries in `mems_expert_fact_external`: 11
- total on the #89 base before batch 1700: 78 RAVE / 90 expert after applying 1710 alone
- expected total after validated batch 1700 + 1710: 86 RAVE / 98 expert
- database integrity: `ok`
- database schema/revision unchanged: r20
- all new verification levels: `verifie_constructeur`
- decompressed SQL length: 12903 bytes
- decompressed SQL SHA-256: `a1d1283c0f38a1b3f65994abd7f8d2ce5c34d9e1f0798c447c5862114316c6de`
- qz64 file length: 1945 bytes
- qz64 SHA-256: `fa9ec2dbdf2178c30f47a1f6cc356c97035c46b780d1e5adced765335af412dc`
