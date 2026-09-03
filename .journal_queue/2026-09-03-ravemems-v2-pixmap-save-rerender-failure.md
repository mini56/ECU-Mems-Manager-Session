## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — `Pixmap.save()` IDENTIQUE MAIS 737 CROPS NON REPRODUCTIBLES ISOLÉMENT

- Branche : `tmp-ravemems-v2-foundation`.
- Run : `33795303720` — FAILURE uniquement au contrôle de fidélité visuelle; tous les gardes sémantiques/textes et production sont SUCCESS.
- SHA réellement testé : `7265cfaca9f1a6a80c3f416e0ab36867d7d6ce1f`.
- Workflow déclencheur : `d63f72e16e8d56419096b18679bc7cc7d60baef0`.
- Production protégée : `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
- Sémantique/texte toujours à zéro défaut connu : 201 opérations, 397 phases, 3 104 étapes, 144 notices, 23 requirements, 738 visuels, 401 liens visuels, 0 review flag, `numeric_phase_defect_count=0`, 0 folio collé, 0 cross-reference tronquée, 0 titre suivant collé, SQLite OK et 0 FK.
- Le validateur utilise cette fois exactement `page.get_pixmap(Matrix(1.5,1.5), clip=...)` puis `Pixmap.save(path)` vers un fichier PNG temporaire, soit le même chemin de sauvegarde que l'extracteur.
- Résultat fidélité : 1/738 VERIFIED, 737/738 FAILED; validation des liens : 401/401 VERIFIED, 0 échec; audit restant 737.
- Artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PIXMAP-SAVE-RERENDER`, ID `9908932215`, 25 304 025 octets, digest ZIP `sha256:4fc96802d59d32c7a3fbe8100cc615180be8857a753e96ac39655c8464a6c456`.
- Conclusion : ni l'arrondi BBOX, ni le décodage PNG, ni la différence `tobytes()`/`save()` ne sont la cause racine. Le défaut est lié au contexte de rendu/reconstruction : l'extraction originale détecte les candidats et rend les crops dans l'ordre des pages sur le même document/page, alors que les validateurs précédents rerendent les clips depuis les BBOX persistées isolément.
- PROCHAINE ACTION EXACTE : rejouer depuis le PDF source gelé le chemin d'extraction visuelle dans son ordre naturel page 1→372 : exécuter les mêmes lectures de page et `visual_candidate_rects(page)`, recalculer `expanded_clip`, recréer chaque `VIS_P####_###` dans le même ordre et comparer candidate count, BBOX, dimensions et PNG SHA/bytes avec la base et les assets. Exiger 738/738 correspondances; si le replay exact échoue encore, instrumenter dans le même processus d'extraction le SHA des samples/pixmaps avant sauvegarde et comparer avec une seconde instance de document. Ne pas assouplir le garde; ne pas toucher à MEMSX64 ni aux 46 autres PDF.
