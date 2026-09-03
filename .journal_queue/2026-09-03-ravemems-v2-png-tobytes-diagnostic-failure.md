## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — DIAGNOSTIC PNG `tobytes()` NON CONCLUANT

- Branche : `tmp-ravemems-v2-foundation`.
- Run : `33794979941` — FAILURE uniquement au contrôle de fidélité PNG; tous les gardes sémantiques/textes et production restent SUCCESS.
- SHA testé : `7d820fb33754fa7c236636aefc441f92ac3fcbf3`.
- Production protégée : `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
- Sémantique/texte toujours à zéro défaut connu : 201 opérations, 397 phases, 3 104 étapes, 0 review flag, `numeric_phase_defect_count=0`, 0 folio collé, 0 cross-reference tronquée, 0 titre suivant collé, SQLite OK et 0 FK.
- Visuels : 738; liens : 401/401 VERIFIED, 0 échec.
- Fidélité : 1/738 VERIFIED, 737/738 FAILED avec comparaison entre le fichier PNG extrait (créé historiquement par `Pixmap.save(path)`) et `rerender.tobytes("png")`.
- Artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PNG-RERENDER`, ID `9908806057`, 25 303 887 octets, digest ZIP `sha256:136e8af1ef0d9e72e73bae03e2b0e24537c69aa3580497badbea3b7203065cc7`.
- Analyse : ce test n'implémente pas exactement la preuve prévue dans le rapport. `Pixmap.tobytes("png")` et `Pixmap.save(path)` utilisent des sorties pouvant différer au niveau encodage/métadonnées même à pixmap identique. Il ne faut donc pas conclure à une infidélité des 737 assets sur ce test.
- PROCHAINE ACTION EXACTE : conserver les gardes stricts; rerendre les crops depuis le PDF source avec le même PyMuPDF 1.26.4 puis utiliser exactement `Pixmap.save()` vers un fichier PNG temporaire, lire les octets de ce fichier et comparer SHA/bytes au PNG extrait. Si les fichiers ainsi créés sont identiques 738/738, considérer cette preuve byte-identique comme validation de fidélité; sinon poursuivre l'analyse des différences. Ne pas toucher à MEMSX64 ni aux 46 autres PDF.
