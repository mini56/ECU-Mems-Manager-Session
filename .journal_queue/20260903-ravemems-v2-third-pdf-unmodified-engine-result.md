## RAVEMEMS V2 — troisième PDF RAVE, moteur non modifié — résultat

Test GitHub Actions: `33802425656` — ROUGE attendu au garde strict de généralisation.

Source testée sans aucune adaptation spécifique du moteur ni du profil:
- `rave/xn/hr01795e.pdf`
- blob `63569965a1b6d62a5dd00553b1831254a9f2feb8`
- 94 pages, donc critère >=30 pages satisfait
- métadonnée titre: `Mini Owner's Handbook - 5th Edition - Eng`
- publication détectable directement dans le PDF: `RCL 0179ENG`
- famille source visible: `Owner's Handbook`

Moteur exact testé:
- SHA `7418419993af12d5d4c8a1e57673984dbedc4675`
- profil existant laissé inchangé: `ravemems/v2/profiles/rcl0193eng.json`
- aucun profil `RCL0179ENG` fabriqué dans le workflow.

Résultat:
- extraction return code = 1
- message exact: `Expected publication code RCL0193ENG not found in source PDF`
- l'extraction ne démarre donc pas réellement sur RCL0179ENG.
- artefact de preuve: `RAVEMEMS-V2-THIRD-PDF-UNMODIFIED-ENGINE`, ID `9911568813`, SHA256 ZIP `6242ed84525779e332aaf27c0942f8a36707c37dcde67738136e0d2c039d252b`.

Conclusion importante:
- la couche de règles documentaires est partiellement générique, mais l'entrée du moteur reste encore dépendante du profil RCL0193ENG.
- à ce stade le moteur final ne satisfait PAS encore la règle cible « recevoir n'importe quel PDF RAVE sans modification préalable ».
- le prochain défaut à supprimer est cette dépendance au `publication_code_regex`/profil spécifique RCL0193ENG, puis il faudra retester le même RCL0179ENG sans profil spécifique.

Protection production confirmée pendant le test:
- `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- aucun BUILD #104.
