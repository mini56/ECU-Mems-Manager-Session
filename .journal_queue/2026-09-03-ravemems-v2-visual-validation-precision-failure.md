## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — VALIDATION VISUELLE : ÉCHEC DE PRÉCISION BBOX IDENTIFIÉ

- Branche : `tmp-ravemems-v2-foundation`.
- Run de validation globale : `33794012663` — FAILURE au garde visuel, après extraction et gardes sémantiques/textes SUCCESS.
- SHA testé : `0d98db8433af2bc21b9b74d7f430c8c4c134819a`.
- Production protégée : `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
- Entrée sémantique confirmée propre avant validation visuelle : `numeric_phase_defect_count=0`, 0 folio collé, 0 référence tronquée, 0 titre d'opération suivant collé, SQLite OK, 0 FK.
- Validation des liens visuels : 401/401 VERIFIED, 0 échec. La reconstruction sémantique indépendante confirme donc tous les liens actuellement créés.
- Validation fidélité : 1/738 VERIFIED, 737/738 FAILED, tous pour `pixel mismatch`; aucune erreur SHA de fichier, dimensions, canaux ou containment bbox.
- Analyse de l'artefact ID `9908446700` : le seul visuel validé (`VIS_P0001_001`) possède un crop entièrement entier `[0.0,0.0,356.0,504.0]`. Les 737 visuels échoués possèdent tous au moins une coordonnée non entière. L'extracteur sérialise actuellement les BBOX avec `round(..., 3)`, puis le validateur rerend depuis cette BBOX arrondie. La perte de précision subpixel modifie l'antialiasing et empêche une comparaison pixel-identique alors que le PNG et son SHA stocké sont cohérents.
- Artefact d'échec : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-VISUAL-VALIDATED`, ID `9908446700`, 25 238 860 octets, digest ZIP `sha256:0af3fcfb64702f874f5f2b5767b37263c6550721357bda9675374789306ebf8b`.
- Audit après validation partielle : 737 constats, correspondant aux 737 fidélités non validées; les 401 liens ne génèrent plus de constat.
- PROCHAINE ACTION EXACTE : conserver le validateur strict inchangé et corriger la cause à la source en stockant les `source_bbox_json` et `crop_bbox_json` des visuels sans arrondi destructif, avec une sérialisation float round-trip exacte. Relancer RCL0193ENG complet uniquement, exiger 738/738 fidélités pixel-identiques, 401/401 liens vérifiés, 0 échec et `audit_issue_count=0`. Ne pas toucher à MEMSX64 ni aux 46 autres PDF.
