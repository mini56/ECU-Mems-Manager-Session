## 2026-09-04 — RAPPORT maître — réparation UTF-8 prioritaire avant toute autre action

Constat confirmé : `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` n'est pas vide, mais contient au moins un octet historique invalide pour UTF-8 (première erreur observée vers l'offset 834351, octet `0xD5`). Cette corruption empêche les lectures complètes via le connecteur GitHub et a déjà provoqué plusieurs faux diagnostics de fichier vide.

Décision : conformément à la règle fondamentale de traçabilité du projet, toute progression Qwen/MEMSLibrary est suspendue jusqu'à réparation du rapport maître.

Correction autorisée :
- réparer uniquement les octets non UTF-8 du fichier maître en conservant tous les octets déjà UTF-8 valides inchangés ;
- interpréter uniquement les octets isolés invalides selon Windows-1252, avec repli Latin-1 si nécessaire ;
- vérifier ensuite que le rapport entier se décode strictement en UTF-8 ;
- renforcer `tools/append_master_report.py` pour refuser à l'avenir tout rapport non UTF-8 après écriture ;
- ne toucher ni à `MEMSX64`, ni au BUILD #103, ni à Qwen/MEMSLibrary pendant cette réparation.

État production protégé : `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104.
