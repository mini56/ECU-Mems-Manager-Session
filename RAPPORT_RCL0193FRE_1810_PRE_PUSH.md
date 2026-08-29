# RCL0193FRE — lot 1810 — PRE-POUSSE

Date : 2026-08-29
Branche documentaire cible : `tmp-rave-visual-backfill`
Production : `MEMSX64` doit rester strictement sur BUILD #101 / `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Périmètre

RCL0193FRE PDF 146-175 : refroidissement + collecteurs/admission/échappement. Pages réellement utiles : 146, 148, 149, 150, 152, 154-160, 162-174. Pages vides/intercalaires non intégrées.

## Validation locale avant pousse

- 25 vraies pages constructeur conservées en captures 150 dpi.
- 15 opérations de réparation constructeur.
- 44 connaissances structurées.
- 23 spécifications / 23 valeurs structurées.
- 30 phases de procédure.
- 271 étapes de procédure.
- 19 exigences / avertissements / outils / remplacements.
- 19 relations.
- 10 familles d’alias français tentées, sans forcer de doublons existants.
- `PRAGMA integrity_check = ok`.
- `PRAGMA user_version = 20`.
- Historiques préservés : 93 `mems_rave_fact`, 105 `mems_expert_fact_external`.
- SHA-256 SQL : `acc515ffc12b680339f0319aa1c10d335108590120008cc2b777746d959a1ecb`.
- SHA-256 QZ64 : `eb9e28913d2815b8a270ec4b3f4dd5fdfd69c23acf3a0cdbd3b07a1ab9365226`.
- TIFF G4 150 dpi : `8edf3246dbf0d0dd7ce8a65c94c66768790241fc0e972d9e4ac070385d5166c1`.
- Transport XZ : `dd4c5171b38f39b68f4b18e5002ae23c9234bc7f5ff9e27f963ae3388174c329`.

## Contrôles techniques particuliers

- Thermostat : température d’ouverture constructeur `88 °C` conservée comme spécification typée.
- Les dimensions d’outils ne sont pas converties en spécifications : la clé de 22 mm de la sonde HO2S reste une étape/outillage de procédure.
- Le bloc admission conserve explicitement MAP, IACV, IAT, rampe et injecteurs sans inférer une autre famille MEMS.
- Aucune donnée SPi/MPi n’est mélangée hors de la portée réellement prouvée par le document.

## Action suivante exacte

Pousser uniquement le lot additif 1810 sur `tmp-rave-visual-backfill`, faire reconstruire et valider la base et les 25 captures par GitHub Actions, ne committer le lot final que si tous les contrôles passent, puis écrire le rapport post-pousse avant de continuer le manuel.
