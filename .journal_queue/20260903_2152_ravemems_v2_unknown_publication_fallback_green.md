## RAVEMEMS V2 — fallback générique identité SHA validé

- Run ciblé réel : `33809556572`, job `100827935379`, SUCCESS.
- PDF gelé testé : `rave/library/libxn.pdf` depuis le source commit `643de091b474f4e27917a065bdf46d5a0c764276`.
- Correction générique : lorsque le motif de publication est absent, `publication_code` reste `NULL` et l'identité stable calculée par le probe est utilisée.
- Identité obtenue : `PDFSHA256AC352F6D35C2F8A5`.
- `publication_detection=sha256_fallback`, langue `und`, type générique `document`, 1 page.
- Visuels : 1 en base, 1 rejoué, 1 fidélité vérifiée, 0 échec, 0 manquant, 0 inattendu.
- Liens : 0, échecs 0.
- Audit après replay : 0.
- SQLite : integrity_check=ok, FK=0, reviews ouvertes=0.
- Marqueurs : `RAVEMEMS_V2_GENERIC_EXACT_VISUAL_REPLAY_GLOBAL_ZERO_PASS` et `UNKNOWN_PUBLICATION_SHA_FALLBACK_GLOBAL_ZERO_PASS PDFSHA256AC352F6D35C2F8A5`.
- Commit moteur créé et poussé par le run : `5232215f5853241572f580cf1696c9f881b254f4` (`Allow generic SHA identity fallback for unidentified PDFs`), 1 insertion / 3 suppressions dans `ravemems/v2/core_extract.py`.
- Aucune règle spéciale `libxn`, aucun rôle sémantique du chemin/nom.
- Production protégée : `MEMSX64` reste #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730` avant et après pousse.
- Prochaine action exacte : relancer le corpus complet des 47 PDF en pinant le workflow sur le moteur corrigé `5232215f5853241572f580cf1696c9f881b254f4`, puis exiger le verdict strict global zéro.
