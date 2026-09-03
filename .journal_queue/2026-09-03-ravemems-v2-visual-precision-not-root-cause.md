## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — PRÉCISION BBOX NON CAUSE RACINE DU DÉFAUT VISUEL

- Branche : `tmp-ravemems-v2-foundation`.
- Commit testé pour extraction visuelle à géométrie float exacte : `fdec4ae8f0cb3515ee67f982ddec9b6bb934ec97`.
- Run complet : `33794551649` — FAILURE uniquement au validateur de fidélité visuelle.
- Production protégée : `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
- Extraction et garde sémantique/textuel : SUCCESS. Toujours 201 opérations, 397 phases, 3 104 étapes, 144 notices, 23 requirements, 738 visuels, 401 liens visuels, 0 review flag, `numeric_phase_defect_count=0`, 0 folio collé, 0 cross-reference tronquée, 0 titre suivant collé, SQLite OK et 0 FK.
- Validation des liens : 401/401 VERIFIED, 0 échec.
- Validation fidélité malgré BBOX float exactes : 1/738 VERIFIED, 737/738 FAILED, toujours par `pixel mismatch`; audit restant 737.
- Conclusion : l'arrondi BBOX à 3 décimales n'est pas la cause racine du mismatch. Le validateur compare actuellement les samples du rendu PDF source aux samples obtenus après sauvegarde PNG puis redécodage via `fitz.Pixmap(path)`. Cette chaîne de décodage/couleurs doit être vérifiée avant de conclure à une infidélité réelle des fichiers PNG.
- Artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-VISUAL-PRECISION`, ID `9908641967`, 25 252 057 octets, digest ZIP `sha256:9d52d221bae15aa666fd7212bef17c4fe811dcb656de2e2f60b0c97fe50ae350`.
- PROCHAINE ACTION EXACTE : conserver les gardes stricts et diagnostiquer la chaîne PNG sur un échantillon puis globalement : rerendre chaque crop exact avec le même PyMuPDF 1.26.4, sauvegarder ce rerendu en PNG et comparer directement les octets/SHA du PNG rerendu au PNG extrait. Si les PNG sont byte-identiques, remplacer uniquement le contrôle invalide `Pixmap(PNG).samples` par la preuve byte-identique de rerendu PNG; sinon poursuivre l'analyse des écarts. Ne pas toucher à MEMSX64 ni aux 46 autres PDF.
