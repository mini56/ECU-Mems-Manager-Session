
## RAVEMEMS V2 — régression complète RCL0193ENG après règles génériques date / Technical Bulletin

Run exact : `33798003291` — SUCCESS.
Job : `100790265105`.
SHA code réellement testé : `a9e47832a2fbd3bf365a32e00d374ed7c70fba5c`.
Source : `rave/xn/wmxn990e.pdf`, commit `643de091b474f4e27917a065bdf46d5a0c764276`, blob `82263eb36bb194dfa969d0471d10ef11078ce521`.

But de cette régression : prouver que la règle générique qui rejette les identifiants nus `dd.mm.yy` lorsqu'ils sont des dates calendrier plausibles, ainsi que la classification par chemin des Technical Bulletins, ne régressent pas le manuel RCL0193ENG déjà validé à zéro défaut.

Résultat exact :
- document_kind : `workshop_manual` — PASS.
- pages : 372.
- opérations : 201 — inchangé.
- phases : 397 — inchangé.
- étapes : 3 104 — inchangé.
- visuels : 738 — inchangé.
- liens visuels : 401 — inchangé.
- défauts numériques : 0.
- review flags : 0.
- visuels rejoués : 738/738.
- fidélité visuelle vérifiée : 738/738.
- échecs fidélité : 0.
- liens vérifiés : 401/401.
- échecs liens : 0.
- replay manquant : 0.
- replay inattendu : 0.
- audit final après replay : 0.
- SQLite integrity_check : ok.
- FK cassées : 0.

Marqueurs de preuve du run :
- `RCL0193ENG_GENERIC_OPERATIONS_PASS 201`
- `RCL0193ENG_GENERIC_PHASES_PASS 397`
- `RCL0193ENG_GENERIC_STEPS_PASS 3104`
- `RCL0193ENG_GENERIC_VISUALS_PASS 738`
- `RCL0193ENG_GENERIC_LINKS_PASS 401`
- `RCL0193ENG_GENERIC_AUDIT_ZERO_PASS`
- `RAVEMEMS_V2_RCL0193ENG_EXACT_VISUAL_REPLAY_GLOBAL_ZERO_PASS`

Artefact : `RAVEMEMS-V2-RCL0193ENG-GENERIC-RULES-REGRESSION`, ID `9909952386`, taille zip 25 246 054 octets, digest `sha256:163a15476b14146a42ac745d9ee93ed8cb93d04d97d0fb460af65af0457d5106`.

Conclusion : la correction générique révélée par R8411BU est compatible avec le baseline RCL0193ENG complet. R8411BU peut être classé `technical_bulletin` sans inventer l'opération `23.12.98`, tandis que RCL0193ENG conserve exactement ses 201 opérations, 397 phases, 3 104 étapes, 738 visuels, 401 liens et son audit global à zéro.

Production protégée : `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104.
