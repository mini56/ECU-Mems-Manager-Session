# RAVE batch 1700 — RCL0194ENG SPi Japan 97MY injector, purge and IAC wire colours

## Source

- Rover Technical Communication factory document: `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition.
- Vehicle scope: Mini 97MY SPi Japan from VIN `SAXXNNAXKBD 134455`.
- Factory page used: `Engine Management System (MEMS) SPi (JAPAN) 20.4`.
- Factory colour legend: Rover `RCL 0145` colour codes embedded in the electrical-diagram publication.

## Data policy

- Only connector/function/colour relationships already traceable without ambiguity on RCL0194ENG 20.4 are retained.
- Compound wire codes are expanded using the Rover factory legend: S=slate/grey, U=blue, Y=yellow, N=brown, K=pink, B=black, W=white, O=orange, G=green.
- No ignition-coil colour is added in this batch because the extracted text does not give an equally unambiguous connector-pin-to-colour pairing.
- No fact is generalized to MPi, European SPi, or another MEMS generation.
- All entries are `verifie_constructeur` and mirrored into `mems_expert_fact_external`.

## Added facts

| Fact key | Function | Factory relationship |
|---|---|---|
| RAVE-COLOR-SPIJ-008 | injector | C522-1 ↔ C159-1: SU = slate/blue = gris/bleu |
| RAVE-COLOR-SPIJ-009 | injector | C522-2 ↔ C159-24: YN = yellow/brown = jaune/brun |
| RAVE-COLOR-SPIJ-010 | purge supply | C152-1: NK = brown/pink = brun/rose |
| RAVE-COLOR-SPIJ-011 | purge control | C152-2 ↔ C159-21: BW = black/white = noir/blanc |
| RAVE-COLOR-SPIJ-012 | IAC phase 1 | C177-1 ↔ C159-3: OS = orange/slate = orange/gris |
| RAVE-COLOR-SPIJ-013 | IAC phase 4 | C177-4 ↔ C159-27: OU = orange/blue = orange/bleu |
| RAVE-COLOR-SPIJ-014 | IAC phase 3 | C177-6 ↔ C159-22: OG = orange/green = orange/vert |
| RAVE-COLOR-SPIJ-015 | IAC phase 2 | C177-3 ↔ C159-2: KU = pink/blue = rose/bleu |

## Validation before push

The SQL payload was applied after batch 1690 on a copy of the r20 expert database extracted from GitHub Actions #88.

- new `RAVE-COLOR-SPIJ-008..015` in `mems_rave_fact`: 8
- mirrored entries in `mems_expert_fact_external`: 8
- total RAVE facts after 1690 + 1700: 75
- total expert facts after 1690 + 1700: 87
- database integrity: `ok`
- database schema/revision unchanged: r20
- all new verification levels: `verifie_constructeur`
- one variant only: `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`
- decompressed SQL length: 9917 bytes
- decompressed SQL SHA-256: `29987cea579fbd7987e19eaf7655e3634df76353a3b42d07f09cb9977a0768a9`
- qz64 file length: 1461 bytes
- qz64 SHA-256: `d3469e14460d5fdea3cfa4a0f6ae84d92bb69dcbb1bf98bcba1a2c301f0cfe06`
