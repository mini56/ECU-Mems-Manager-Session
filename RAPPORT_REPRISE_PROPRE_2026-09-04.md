# RAPPORT DE REPRISE PROPRE — ECU MEMS MANAGER

Date : 2026-09-04

## 1. État protégé

- Branche de production : `MEMSX64`
- BUILD protégé : `#103`
- SHA protégé : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
- Aucun BUILD #104 ne doit être lancé avant validation explicite de la voie temporaire.

## 2. Socle vert de référence

La reprise propre est basée strictement sur :

- commit : `9da599e697aa96d099982a30f5f5c8a562a7c788`
- run GitHub Actions : `33889068203` — SUCCESS
- artefact : `TMP-ECU-MEMS-Manager-x64-IA-MEMSLibrary-33889068203`
- artifact ID : `9943343980`
- SHA256 artefact : `4cf818b64670a8fb4dc5e6f77b7b30c35a3ab95251046ca24c529194a622f7f5`

Ce socle conserve : BUILD #103 + corpus RAVEMEMS complet + Pack001/MEMSLibrary + `MEMSLibrary.dll` ABI2 x64 + bridge IA/Qwen + grounding RAVEMEMS validé.

## 3. Branche de reprise propre

Branche : `tmp-ai-memslibrary-reprise-clean`

Elle a été créée directement depuis `9da599e697aa96d099982a30f5f5c8a562a7c788`.

Depuis `16b42634377e37010d258e09c0d0a40966721aa3`, seuls les trois fichiers applicatifs utiles de restauration scroll/visuels doivent être repris bit pour bit :

- `iamemstab.cpp`
- `expert/IaMemsDiagramCatalog.cpp`
- `expert/IaMemsDiagramSelfTest.cpp`

Les workflows temporaires présents dans le delta jusqu'à `16b426…` sont volontairement exclus.

## 4. Historique expérimental conservé mais non autoritaire

L'ancienne branche `tmp-ai-memslibrary-bridge` est conservée uniquement comme trace historique. Son HEAD observé lors de la reprise était `35c539bfa7c64d416a202e66dc913904cbe67fd3`.

Tout ce qui a été empilé après `16b426…` sous forme de scripts `tmp_*`, CMake temporaire, workflows de diagnostic/package et commits de déclenchement est considéré comme expérimental et non validé.

En particulier :

- run `33894656405` : FAILURE — ne pas considérer comme vert ;
- run `33895321969` : FAILURE — aucun artefact final validé ;
- commits `d52a91…` et `35c539…` : ne sont pas des points de validation.

## 5. Prochaine action exacte après assainissement

Ne pas modifier `MEMSX64`.

Sur `tmp-ai-memslibrary-reprise-clean` uniquement :

1. vérifier la restauration du scroll et du système visuel sur le socle vert ;
2. corriger le routage de `Voir le schéma` pour qu'une illustration RAVEMEMS réellement pertinente et retrouvée par la réponse puisse être proposée même si la question ne contient pas le mot `schéma` ;
3. conserver un garde négatif strict : une question diagnostique ordinaire ne doit recevoir aucune image parasite ;
4. valider les self-tests historiques et les cas réels ciblés avant tout nouveau package ;
5. aucun BUILD #104 avant validation explicite.

## 6. Règle de traçabilité

À partir de cette reprise : chaque modification, run et verdict doit être journalisé. Aucun run rouge ne doit être présenté comme vert et aucun commit temporaire ne doit devenir implicitement une nouvelle base de référence.
