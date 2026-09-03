## RAVEMEMS V2 — test correctif fallback identité SHA rouge avant modification moteur

- Run ciblé : `33809601699`.
- Résultat : RED dans l'étape `Apply exact generic fallback patch`.
- Cause : le script de patch exigeait une séquence multiligne textuellement exacte et a trouvé `0` occurrence (`expected exactly one legacy publication gate, found 0`).
- Important : aucune modification de `ravemems/v2/core_extract.py` n'a été commitée ni poussée ; les étapes extraction/replay/commit ont été skipped.
- `MEMSX64` est resté protégé sur #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- Prochaine action exacte : rendre le patcher robuste en localisant l'unique ligne `Expected document identity pattern not found in source PDF`, vérifier les lignes adjacentes attendues puis remplacer uniquement le garde par `publication_code = ... if publication_match else None`; relancer le même test ciblé avant toute pousse du coeur.
