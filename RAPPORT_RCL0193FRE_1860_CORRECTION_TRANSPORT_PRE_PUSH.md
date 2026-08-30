# RCL0193FRE — lot 1860 — CORRECTION TRANSPORT AVANT REPOUSSE

Date : 2026-08-30.

## Contexte
Le run `33302467286` a échoué avant tout commit final 1860. Le garde SHA a détecté un transport TIFF incomplet : SHA obtenu `6a8f5c154a8b6c995d4c3f169d6dbb0e7581eff99a33df3a4a07d301e78c6eb6` au lieu du SHA attendu `fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08`.

L'échec a été consigné séparément dans `RAPPORT_RCL0193FRE_1860_RUN1_FAILURE.md`. Aucun fichier final 1860 n'a été intégré par ce run.

## Correction autorisée maintenant
Correction strictement limitée au transport temporaire. Les données techniques 1860, le SQL, les compteurs, les pages sélectionnées et les règles de validation restent inchangés.

Fichiers locaux exacts récupérés et revérifiés :
- `research_enrichment_1860.sql` SHA-256 `1cf7cd58e71d436fc701401d841f0a4b521c9f386ff23a5720d8fb7a5b86fd54` ;
- `research_enrichment_1860.qz64` SHA-256 `18e859c236a7e865b3416e0083f024e872bebda540c2dbf15ce233c0900c18be` ;
- TIFF G4 150 dpi SHA-256 `d25fd347ccf9fd795c635fffff5a0dcc2800bbe33e185416c9a188d034b8f2c4` ;
- TIFF G4 compressé XZ SHA-256 `fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08`.

Le transport sera découpé en fragments texte Base64/ASCII, réassemblé dans GitHub Actions, puis les SHA ci-dessus seront vérifiés avant toute installation.

## Invariants à conserver
- `MEMSX64` exactement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48` ;
- 93 faits RAVE historiques et 105 faits experts historiques inchangés ;
- `PRAGMA integrity_check = ok`, `user_version = 20` ;
- lot 1860 : 112 connaissances, 18 spécifications/valeurs, 100 phases, 516 étapes, 29 exigences, 79 relations, 45 captures réelles constructeur ;
- aucune modification protocole, UI, IA, 32 bits ou production.

## Prochaine action exacte
1. Remplacer uniquement le transport temporaire du commit `908da678c67dcc6066a8991b69b0feb6e7923cdc` par les fragments exacts.
2. Relancer le validateur 1860.
3. Si vert : commit final documentaire, nettoyage automatique, contrôle distant et rapport post-pousse immédiat.
4. Ensuite seulement : audit final de couverture des 371 pages et éventuel backfill 1870 si un trou réel est détecté.
