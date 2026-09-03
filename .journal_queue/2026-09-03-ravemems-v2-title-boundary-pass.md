## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — CORRECTION TITRES COLLÉS AUX ÉTAPES

- Branche de travail : `tmp-ravemems-v2-foundation`.
- Production protégée : `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
- Le run `33792257963` sur le SHA testé `826395938580116ba4ef3f71c4c8cbd245e4c32a` est SUCCESS avec `numeric_phase_defect_count=0`, `ravemems_review_flag=0`, SQLite `integrity_check=ok`, 0 FK cassée et le cas multi-pages 133–135 séquencé correctement.
- Un défaut sémantique résiduel reste visible dans le texte de certaines étapes : le titre de l’opération suivante et parfois son numéro de page restent collés à la fin de l’instruction précédente, par exemple `Connect harness to CKP sensor multiplug. THROTTLE POSITION (TP) SENSOR`, `Fit air cleaner. See this section. 12 MANIFOLD ABSOLUTE PRESSURE (MAP) SENSOR`, ou `Adjust throttle cable. See Adjustments. 16 FUEL INJECTORS`.
- Ce bruit ne doit pas être accepté comme un zéro défaut réel.
- PROCHAINE ACTION EXACTE : corriger génériquement les frontières texte entre fin d’étape et titre/repère de l’opération suivante à partir de la structure géométrique et des marqueurs déjà reconnus, sans hardcoding CKP/TP/MAP/HO2S/Fuel Rail/Injectors, puis relancer uniquement RCL0193ENG avec un garde explicite empêchant qu’un titre d’opération suivant soit incorporé dans `instruction_source`. Maintenir `numeric_phase_defect_count=0`, `ravemems_review_flag=0`, intégrité SQLite OK et 0 FK cassée.
