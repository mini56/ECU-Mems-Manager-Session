# RAVEMEMS V2 — lignée générique neutralisée, trois documents à zéro défaut

Date: 2026-09-03

## Résultat final
Workflow de validation/transport: `33806135888` — SUCCESS.
Commit de déclenchement transport corrigé: `0c4e56d7f2d9c3ee854791cca2fc1a44ea9e3f0d`.
Commit générique neutralisé réellement poussé sur `tmp-ravemems-v2-foundation`:
`745f41243adb202f638f4039016dedfff8f272d6`
Message: `Neutralize generic PDF extraction lineage`.

Le workflow a d'abord généré la lignée neutre, vérifié par grep qu'aucune référence `RCL0193` / `rcl0193` ni import de modules RCL0193 n'était présente dans le chemin d'exécution générique, compilé les modules, puis exécuté les trois documents complets. Les fichiers n'ont été commit/push qu'après passage de toutes les régressions.

## Régressions exactes
### RCL0179ENG — Owner's Handbook
- 94 pages
- publication détectée depuis le contenu: RCL0179ENG
- langue: en
- type: owner_handbook
- 0 opération / 0 phase / 0 étape
- 94 sections page-grounded
- 68 visuels
- 68/68 replay exact
- 0 lien visuel attendu
- audit final 0
- SQLite integrity ok
- FK 0

### RCL0213ENG — Electrical Library
- 212 pages
- publication détectée depuis le contenu: RCL0213ENG
- langue: en
- type: electrical_reference_library
- 0 opération / 0 phase / 0 étape
- 212 sections page-grounded
- 304 visuels
- 304/304 replay exact
- 0 lien visuel attendu
- audit final 0
- SQLite integrity ok
- FK 0

### RCL0193ENG — Workshop Manual
- 372 pages
- publication détectée depuis le contenu: RCL0193ENG
- langue: en
- type: workshop_manual
- 201 opérations
- 397 phases
- 3104 étapes
- 144 notices
- 23 requirements
- 738 visuels
- 738/738 replay exact
- 401/401 liens vérifiés
- 0 défaut numérique
- 0 review ouverte
- audit final 0
- SQLite integrity ok
- FK 0

Garde globale: `NEUTRAL_GENERIC_LINEAGE_THREE_DOCUMENT_GLOBAL_ZERO_PASS`.

## Lignée générique neutralisée
Les modules génériques validés utilisent désormais notamment:
- `ravemems/v2/core_extract.py`
- `.github/pass2/generic_semantic_pass2.py`
- `.github/pass2/generic_precise_visual_extract.py`
- `.github/pass2/ravemems_generic_precise_extract.py`
- `.github/pass2/generic_visual_validate.py`
- `.github/pass2/ravemems_generic_visual_replay_validate.py`

Le nom SQLite générique est `ravemems_v2.sqlite` dans ce chemin d'exécution. Les anciens modules RCL0193 restent seulement comme historique/référence de migration et ne sont plus importés par la lignée générique validée.

## Entrée source déjà validée avec cette architecture
Une source peut être un PDF direct, un dossier, un ZIP ou un TAR. Le nom physique du fichier, du dossier ou de l'archive ne sert pas à la classification. Publication, langue et type sont déterminés depuis le contenu/métadonnées. Une publication absente peut utiliser une identité de repli basée sur le SHA du contenu, sans inventer une référence constructeur.

## Artefact
`RAVEMEMS-V2-NEUTRAL-LINEAGE-THREE-DOCUMENT-REGRESSION`
Artifact ID: `9913015009`
Taille: 58,344,548 octets
ZIP SHA256: `53b7618141dc27924386c53fb3ed28dc1f5b7377f614d4585b9a3a34b806edb2`
1140 fichiers.

## Production protégée
`MEMSX64` reste strictement au BUILD #103:
`1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
Aucun BUILD #104.
