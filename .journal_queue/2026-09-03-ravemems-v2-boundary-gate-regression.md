## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — GATE FRONTIERES ROUGE : VRAIES ETAPES PRISES POUR DES TITRES

- Branche : `tmp-ravemems-v2-foundation`.
- Correctif testé : `dc923499669c51ef17349be16ea6460fb1a1a889` (`Stop RCL0193ENG step text at structural page and operation boundaries`).
- Workflow/run : `33792860411` — **FAILURE au gate final**, extraction complète réussie.
- `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- Extraction : 372 pages, SQLite `integrity_check=ok`, 0 FK cassée, `numeric_phase_defect_count=0`, `ravemems_review_flag=0`.
- Le mécanisme a bien détecté 299 folios de bas de page et 73 frontières de titres, et a nettoyé les contaminations visibles pages 131-136.
- Mais il a créé une régression : nombre d'étapes tombé de 3 104 à 3 033. Le contrôle explicite `33.25.03=1..13` échoue avec seulement `1..12`.
- Cause racine identifiée : `_is_next_operation_title()` accepte actuellement `item.bold` comme preuve d'en-tête sans exclure d'abord les lignes qui sont déjà reconnues structurellement comme vraies étapes. Une étape contenant un span de renvoi en gras et située juste avant la prochaine opération peut donc être supprimée à tort.
- Artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PASS2-BOUNDARY`, ID `9908020534`, 25 168 639 octets, digest ZIP `sha256:30b0080519c7d5cb4d3c46a91ec54e2a17690996d376836c813f7a35a1b29d19`.
- SQLite de cette passe : SHA-256 `02bc12b60f2ff25006213d38493d5e872b8b31cb30bc06cb6e1d44c4c2916763`.

PROCHAINE ACTION EXACTE : dans le détecteur générique de frontière de titre, interdire explicitement le classement en titre si `_step_match(item)` reconnaît déjà une vraie étape ; ne modifier aucune autre logique ; relancer le même gate global et exiger simultanément 0 défaut de séquence, restauration des séquences de référence dont `33.25.03=1..13`, 0 folio final concaténé et 0 titre d'une autre opération concaténé.
