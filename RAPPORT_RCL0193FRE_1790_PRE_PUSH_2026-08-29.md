# RCL0193FRE — LOT 1790 — ÉTAT FINAL AVANT POUSSE

Date : 2026-08-29

## État à préserver
- Production `MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Travail documentaire uniquement sur `tmp-rave-visual-backfill`.
- Aucun #102, aucun changement protocole ECU, acquisition/RAM, write/reset, UI, Qwen/ONNX ou 32 bits.

## Source exacte
- `Manuel Rover MPI.pdf` fourni par l'utilisateur.
- Publication vérifiée : `RCL0193FRE`, Mini Workshop Manual français, 5e édition / 1999.
- 371 pages, 67 009 217 octets.
- SHA-256 PDF : `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.
- Portée générale : Mini construits depuis NIV `SAXXNNAZEBD134455`.

## Lot 1790 final préparé
52 pages constructeur françaises conservées comme captures originales :
`34-40, 42-46, 48-49, 104, 106-118, 120-136, 138, 140-145`.
Les pages blanches/intercalaires `41,47,105,119,137,139` sont exclues.

Comptes du candidat V4 :
- 52 assets PNG ;
- 429 connaissances ;
- 429 liaisons connaissance->portée ;
- 350 spécifications ;
- 351 valeurs structurées ;
- 27 numéros d'opération constructeur ;
- 51 phases de procédure dépose/repose/réglage/test ;
- 344 étapes ordonnées ;
- 50 exigences (avertissements, attentions, outils spéciaux, pièces à remplacer) ;
- 379 relations de preuve/cross-check ;
- 51 alias français.

`research_enrichment_1790.qz64` : 84 037 octets, SHA-256 `169725ef043b03f776500f41508a46eeec9052411a3d1e4fec32ab47e1967e65`.
SQL décompressé : 828 034 octets, SHA-256 `78bd0cbd02418d9c8f40acbe6a86b602c49eb84f1be619c167bd6cb7bbaba1eb`.
Captures : 11 052 051 octets au total.

## Corrections importantes avant pousse

🔴 IMPORTANT — Le manuel n'est pas traité comme « tout MPi ». Les pages 38 et 39 restent explicitement SPi manuelle / SPi automatique. La page 40 est MPi. Le corps `Système de gestion moteur MEMS` + `Système d'alimentation` 104-145 est porté MPi, preuves internes cohérentes : MAP externe, CMP, deux injecteurs, rampe carburant.

🔴 IMPORTANT — Aucune valeur `mems_family=1.9` ou `1.6` n'est inventée dans le scope : le manuel dit MEMS mais ne donne pas dans ces pages un numéro de version à utiliser comme preuve de classification. L'induction MPi/SPi est structurée ; la famille MEMS reste NULL/UNKNOWN tant qu'une source explicite ne la prouve pas.

🔴 IMPORTANT — La culasse possède une séquence constructeur multi-étapes page 42 : `34 N.m`, puis `34 N.m de plus`. Elle est conservée en deux valeurs ordonnées ; aucun faux couple final n'est calculé.

🔴 IMPORTANT — Page 45, le manuel imprime deux fois le même libellé `Boulon d'ancrage supérieur de ceinture avant`, une fois à 32 N.m et une fois à 30 N.m. Les deux valeurs sont conservées avec `conflit_a_verifier` + relation `conflicts_with`; aucune valeur n'est choisie arbitrairement.

## Validation locale avant pousse
- schéma additif 1730 exact : PASS ;
- `PRAGMA foreign_keys=ON` ;
- `PRAGMA integrity_check = ok` ;
- `PRAGMA user_version = 20` ;
- 0 connaissance sans portée ;
- toutes les captures ont un SHA-256 ;
- aucune table historique RAVE/expert n'est réécrite par le SQL 1790.

## Prochaine action
Pousser atomiquement le lot 1790 sur `tmp-rave-visual-backfill` : qz64 + 52 captures + audit + manifeste, puis relire les octets distants et reconstruire/valider la base complète. Ensuite poursuivre le manuel RCL0193FRE par blocs jusqu'aux 371 pages.
