# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions, pas le BUILD logiciel.

## ÉTAT COURANT — 27 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 courant avant push 1700 : **`84e677a87067ae43054df7fc534d84a70b6908b7`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- GitHub Actions RAVE 1690 : **#89 — SUCCESS**, run **`33044364773`**, commit `84e677a8`.
- Artefact #89 : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID **`9635185747`**, taille **386 764 960** octets, SHA-256 **`1923f090fd3fdf6f13dcf9a089cc9588ac10c0f67605ef1bcfed6825b21b154c`**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## QUALITÉ IA #88 — VALIDÉE

#88 corrige le ciblage observé sur les captures utilisateur et valide compilation, base r20, Qwen, vrai `LocalAiClient` natif et packagé, package et smoke launch. Aucun changement protocole/UI/32 bits.

## RAVE 1690 — VALIDÉ DANS LE PACKAGE #89

Source : Rover Technical Communication `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition, SPi Japan 97MY from VIN `SAXXNNAXKBD 134455`, page 20.4 + légende constructeur.

Faits couleur : diagnostic WY/BG, ECT KG, masse capteurs KB, TPS YG/YP, IAT GB.

Validation réelle après build :
- #89 SUCCESS de bout en bout : compilation, tests, base r20, Qwen, package, smoke launch, artefact.
- artefact #89 téléchargé puis base `ia_mems_reference_r20.sqlite` ouverte directement.
- total `mems_rave_fact` : **67**.
- total `mems_expert_fact_external` : **79**.
- faits `RAVE-COLOR-SPIJ-*` : **7 RAVE + 7 miroirs experts**.
- `PRAGMA integrity_check` : **ok**.
- `PRAGMA user_version` : **20**.

## RAVE 1700 — PRÊT À POUSSER

Même source primaire, même variante SPi Japon 97MY, page 20.4 et légende Rover RCL 0145.

### 8 faits retenus
1. Injecteur C522-1 ↔ C159-1 : **SU = gris/bleu**.
2. Injecteur C522-2 ↔ C159-24 : **YN = jaune/brun**.
3. Purge C152-1 alimentation : **NK = brun/rose**.
4. Purge C152-2 ↔ C159-21 : **BW = noir/blanc**.
5. IAC phase 1 C177-1 ↔ C159-3 : **OS = orange/gris**.
6. IAC phase 4 C177-4 ↔ C159-27 : **OU = orange/bleu**.
7. IAC phase 3 C177-6 ↔ C159-22 : **OG = orange/vert**.
8. IAC phase 2 C177-3 ↔ C159-2 : **KU = rose/bleu**.

Validation locale après 1690 :
- 8 faits RAVE + 8 miroirs experts.
- total prévu : **75 RAVE / 87 experts**.
- integrity `ok`, r20 inchangé, tous `verifie_constructeur`.
- SQL 9917 octets, SHA-256 `29987cea579fbd7987e19eaf7655e3634df76353a3b42d07f09cb9977a0768a9`.
- qz64 1461 octets, SHA-256 `d3469e14460d5fdea3cfa4a0f6ae84d92bb69dcbb1bf98bcba1a2c301f0cfe06`.

Branche `tmp-rave-1700` comparée à `MEMSX64` : **ahead 3, behind 0**, exactement :
- `database/reference/research_enrichment_1700.qz64` ajouté ;
- `database/reference/audits/rave_1700_audit.md` ajouté ;
- `database/reference/manifest.json` : ajout du batch 1700 uniquement.

Aucun code IA, protocole, UI, packaging, 32 bits ou BUILD modifié.

## RAVE 1710 — RECHERCHE EN COURS

RCL0194ENG 20.3 a été retrouvé aussi sous forme de PDF Rover complet et le schéma est lisible visuellement. Il confirme des liaisons C159 + couleur + fonction pour : commande relais principal, double pressostat, relais sonde oxygène, inhibition automatique, blindage/sonde lambda, capteur vilebrequin, relais pompe et signal ignition. Ces candidats seront intégrés seulement après contrôle exact de chaque ligne et après validation 1700.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé. 32 bits inchangé. Aucun BUILD #31.

## PROCHAINE ACTION EXACTE

**Fast-forward `MEMSX64` sur `tmp-rave-1700` HEAD `fab2e4cfddb6507345049c915e2eaa4f7f583a8a`, suivre le nouveau run jusqu’au package et vérifier 75 faits RAVE / 87 experts dans l’artefact. En parallèle, préparer RAVE 1710 à partir de RCL0194ENG 20.3 sans généralisation.**