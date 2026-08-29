# RCL0193FRE — lot 1820 — PRE-POUSSE

Date : 2026-08-29
Branche documentaire cible : `tmp-rave-visual-backfill`
Production : `MEMSX64` doit rester strictement sur BUILD #101 / `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Périmètre

RCL0193FRE PDF 176-225 : embrayage, boîte de vitesses manuelle, arbres de transmission et direction.

43 pages constructeur réellement utiles retenues : 176, 178-190, 192, 194-200, 202, 204-210, 212, 214-225. Les intercalaires/pages blanches sont exclus.

## Validation locale avant pousse

- 43 captures constructeur 150 dpi préparées à partir des vraies pages du manuel.
- 22 opérations de réparation constructeur.
- 69 connaissances structurées.
- 43 spécifications / 43 valeurs structurées.
- 40 phases de procédure.
- 509 étapes de procédure.
- 51 exigences / avertissements / outils / remplacements.
- 26 relations.
- 21 groupes d’alias français tentés, sans forcer les doublons existants.
- `PRAGMA integrity_check = ok`.
- `PRAGMA user_version = 20`.
- Historiques préservés : 93 `mems_rave_fact`, 105 `mems_expert_fact_external`.
- 69/69 connaissances possèdent une portée ; aucune portée incompatible n’est injectée.
- Portée unique du lot : `SCOPE-RCL0193FRE-MINI-VIN134455`.

## Hashes candidats

- SQL SHA-256 : `1b282d86f7093b18e153079b102da8c87759d4343bfc7fddf0297675c6b427a8`.
- QZ64 SHA-256 : `a54409e1e2cf4ddfe5db91e98a94ae5260dad933feb3a8c0c0c870d665dee3a0`.
- TIFF G4 150 dpi SHA-256 : `82d307a598df5af48ec7ef58ae376633298e9412f413362c170ec0f665ee3f61`.
- Transport TIFF XZ SHA-256 : `2f73a5b6009b8b2607439752bb65290d39744d9b27081e99a496b35a026b50a3`.

## Contrôles techniques particuliers

- Jeu de butée d’embrayage : `6,5 mm`, conservé comme valeur constructeur typée.
- Jeu axial du pignon de renvoi : `0,101 à 0,177 mm`, conservé comme plage constructeur.
- Parallélisme roues avant : écartement `0°15′ ± 7,5′ par côté`, conservé tel qu’imprimé.
- Les tailles de clés, forets, piges et outils restent des informations de procédure/outillage et ne sont pas transformées en caractéristiques véhicule.
- La procédure de volant conserve explicitement le renvoi au système SRS ; aucune procédure airbag n’est inventée dans ce lot.

## Contrôle visuel

Échantillons contrôlés manuellement : PDF 179, 195, 212 et 224. Texte, tableaux/dimensions et dessins sont lisibles et conformes aux pages constructeur.

## Action suivante exacte

Pousser uniquement le lot additif 1820 sur `tmp-rave-visual-backfill`, reconstruire et valider la base et les 43 captures dans GitHub Actions, ne committer le lot final que si tous les contrôles passent, puis écrire le rapport post-pousse avant de poursuivre avec suspension/freins.
