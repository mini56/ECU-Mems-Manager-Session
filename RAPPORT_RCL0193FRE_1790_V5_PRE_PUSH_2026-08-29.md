# RCL0193FRE — LOT 1790 V5 — ÉTAT FINAL AVANT POUSSE

Date : 2026-08-29

## État à préserver
- Production `MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Travail documentaire uniquement sur `tmp-rave-visual-backfill`.
- Aucun #102, aucun changement protocole ECU, acquisition/RAM, write/reset, UI, Qwen/ONNX ou 32 bits.

## Source
- `RCL0193FRE`, manuel d'atelier Mini français, PDF fourni par l'utilisateur.
- 371 pages, 67 009 217 octets.
- SHA-256 PDF : `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.

## Candidat 1790 V5
- 52 captures constructeur originales.
- 429 connaissances / 429 liaisons de portée.
- 350 spécifications / 351 valeurs.
- 27 numéros d'opération, 51 phases de procédure, 344 étapes, 50 exigences.
- 379 relations de preuve/cross-check.
- 51 alias français déclarés.
- SQL : 828 045 octets, SHA-256 `28f0ffc78b957679672608731f0cb0faa3487d1646977304423e23d701465725`.
- qz64 : 84 037 octets, SHA-256 `165d3bedb57b38b7fa1550a4360c9f484828209beea51b63b4aab20c5f9885da`.
- Captures : 11 052 051 octets.

## Corrections V5

🔴 IMPORTANT — La page 43 est un tableau composite. Elle contient `CONTROLE DE DEPOLLUTION`, `SYSTEME DE GESTION MOTEUR`, `SYSTEME D'ALIMENTATION` et `REFROIDISSEMENT`. La page elle-même ainsi que dépollution/refroidissement restent dans la portée générale du manuel. Seules les rubriques gestion moteur et alimentation sont portées MPi. La V4 trop large est donc remplacée par V5 avant toute pousse.

🔴 IMPORTANT — Les pages 38 et 39 restent SPi manuelle / SPi automatique ; la page 40 est MPi. Le bloc 104-145 est porté MPi sur preuve interne cohérente (MAP externe, CMP, deux injecteurs, rampe carburant).

🔴 IMPORTANT — `mems_family` reste volontairement NULL/UNKNOWN. Le manuel emploie MEMS mais ces pages ne donnent pas un numéro de génération suffisant pour enregistrer 1.6 ou 1.9 comme fait constructeur.

🔴 IMPORTANT — Culasse p.42 : séquence `34 N.m`, puis `34 N.m de plus`, conservée en deux valeurs ordonnées ; aucun faux couple final calculé.

🔴 IMPORTANT — P.45 : le même libellé `Boulon d'ancrage supérieur de ceinture avant` apparaît à 32 N.m et 30 N.m. Les deux valeurs sont conservées en `conflit_a_verifier` avec relation `conflicts_with`, sans arbitrage.

## Validation locale V5
- schéma 1730 exact : PASS ;
- `PRAGMA integrity_check = ok` ;
- `user_version = 20` ;
- 0 connaissance sans portée ;
- 52/52 captures hashées ;
- aucune table historique supprimée ou réécrite par le SQL 1790.

## Transport/installateur temporaire prévu
Pour éviter 52 pousses binaires séparées, le lot sera transporté sur la branche temporaire en trois fragments d'une archive vérifiée, avec un workflow d'installation auto-nettoyant. Il devra : vérifier les hashes, extraire qz64 + 52 PNG + audit, modifier le manifeste additivement, reconstruire la base complète, exiger 93 faits RAVE / 105 faits experts historiques, `integrity_check=ok`, `user_version=20`, vérifier les comptes 1790 et les invariants de portée, puis committer le lot final et supprimer fragments/helper. Aucun build officiel n'est déclenché.

Archive V5 : 9 762 925 octets, SHA-256 `999e96c3e4554556078fe0036e51292ea02c593d15c9e64a1e96fe550a2cf4ac`.
Fragments :
- part01 : 3 500 000 octets, `cf92fc9a2da81ac46d97f7a56c025c46488d51e57a03da61d7a12579cd7217a7` ;
- part02 : 3 500 000 octets, `2421620b00a8ce022cc9ce78537ee3bf8c83ad3020db8c8f0a916309fbc1a85a` ;
- part03 : 2 762 925 octets, `0649e3cf843fb47355c4098ea762f6d782622e26b5482630facf55e6d1d5016a`.

Prochaine action : pousser uniquement le transport/helper sur `tmp-rave-visual-backfill`, laisser la validation produire le commit documentaire final, puis journaliser immédiatement le résultat avant de poursuivre les pages 146-371.
