# rover-mems 386 — source complémentaire

Cette source est volontairement séparée du corpus Andrew Revill.

Archive auditée : `rover-mems-386.zip`
SHA-256 : `8f1272530c2f850461d270ab41dfdfd5ddea9bc0469782eea0d6bc2d1b8aba7a`

Le fichier `web-static/script.js` contient huit associations explicites entre l'identifiant ECU retourné par le logiciel et une description/référence :

- `9A,00,02,02` — `MNE??????` — Rover Mini SPI
- `99,00,06,03` — `MNE101351` — Rover Mini SPI JDM, climatisation, MEMS 1.6, 2 prises, volant moteur/bobines MPI
- `99,00,03,03` — `MNE101170` — Rover Mini SPI JDM, climatisation, MEMS 1.6
- `22,00,00,82` — `MNE10077` — Rover Metro 1.4 Auto MEMS 1.3
- `AD,00,05,09` — `MKC104052` — Rover, modèle non précisé par la source
- `C7,00,06,CB` — `MKC103111` — MGF 1.8 MPi
- `3A,00,00,14` — Rover Mini SPI JDM
- `10,88,88,36` — `MNE10039` — Rover Metro ? 1 prise 36 broches

Ces données sont intégrées dans `external_ecu_identification` via `research_enrichment_1550.qz64`, avec `verification_level=source_externe`. Les points d'interrogation présents dans la source sont conservés : aucune précision n'est inventée.
