# Audit Andrew Revill -> base de connaissance ECU MEMS Manager

## Principe d'audit

Cet audit compare la base de référence reconstruite sans les enrichissements Andrew Revill à la base reconstruite avec eux. 
Les données Andrew restent au niveau `source_externe`; un recoupement lexical n'est jamais promu automatiquement en `decoded_by_project` ou `verifie_constructeur`.

Les objets `mems_scalar_*` et `mems_table_*` décrivent des éléments de firmware/calibration. Ils ne doivent pas être interprétés automatiquement comme des adresses RAM de diagnostic en temps réel. `mems_variable_correlation` est la famille de données la plus pertinente pour les variables internes, mais elle reste une corrélation externe tant qu'elle n'est pas validée par le projet ou sur véhicule.

## Sources Andrew intégrées

- `andrew_revill_memstools` — MEMSTools – Rover MEMS knowledge corpus — portée **MEMS 1.3 / 1.6 / 1.9** — niveau `source_externe` — archive SHA-256 `12bb7dfc8c1f3bed1eed28b85b3bb2c344e4d7b5fcb0534e584092aaeb7ff1e8`
- `andrew_revill_memstools_mems12` — MEMSTools – Rover MEMS 1.2 corpus — portée **MEMS 1.2** — niveau `source_externe` — archive SHA-256 `12bb7dfc8c1f3bed1eed28b85b3bb2c344e4d7b5fcb0534e584092aaeb7ff1e8`

## Volume structuré

| Jeu de données | Lignes Andrew |
|---|---:|
| Firmwares | 286 |
| Définitions d'axes / unités / échelles | 320 |
| Définitions de scalaires | 18519 |
| Liaisons scalaire <-> firmware/adresse | 96240 |
| Définitions de tables/cartographies | 1815 |
| Liaisons table <-> firmware/index | 17458 |
| Corrélations de variables | 64706 |
| Fichiers sources indexés | 433 |

## Couverture firmware par famille

| Source | Famille | Firmwares |
|---|---|---:|
| `andrew_revill_memstools` | MEMS 1.3 | 3 |
| `andrew_revill_memstools` | MEMS 1.6 | 119 |
| `andrew_revill_memstools` | MEMS 1.9 | 164 |

## Comparaison avec la base antérieure

- Tables dans la base avant Andrew : **29** ; après intégration : **38**.
- Identifiants firmware Andrew déjà cités textuellement quelque part dans l'ancienne base : **41/286**.
- Identifiants de scalaires/tables Andrew retrouvés exactement comme jetons textuels dans l'ancienne base : **0/1091**.
- Ces deux derniers nombres sont des indicateurs de recouvrement documentaire, pas des validations sémantiques. Une absence signifie 'nouvelle connaissance structurée'; une présence signifie seulement 'déjà mentionné quelque part' tant qu'un recoupement technique précis n'est pas établi.

## Contrôles d'intégrité

- `orphan_scalar_bindings` : **0**
- `orphan_table_bindings` : **0**
- `variable_null_addresses` : **0**
- `variable_unknown_firmware` : **0**
- `shared_variable_addresses` : **0**
- `scalar_unresolved_axis` : **0**

## MEMS 1.2

Aucun élément MEMS 1.2 détecté dans les structures/fichiers indexés par le ZIP courant. Ce serait un trou explicite à compléter par une autre source Andrew, pas une donnée à inventer.

## Point de contrôle AANMP002

`inj_pw_rosco` est présent pour AANMP002 : adresse de scalaire `$00E`, libellé ``, unité `ms`, échelle `0.001333`, offset `0.0`.
Cette adresse appartient au modèle scalaire/calibration de MEMS Mapper; elle n'est pas assimilée ici à l'adresse RAM Mode 4 utilisée par ECU MEMS Manager.

## Statut pour le futur moteur expert

- **Utilisable immédiatement comme contexte externe** : familles/firmwares, noms de scalaires et tables, unités/échelles, corrélations variables, inventaire des désassemblages et fichiers de référence.
- **À recouper avant décision diagnostique forte** : toute valeur ou adresse issue uniquement de MEMSTools, particulièrement les variables internes et les éléments marqués non corrélés.
- **Ne pas mélanger** : cartographie/scalaires ROM, données de protocole 7D/80, et RAM Mode 4 sont trois couches différentes.
- **Conserver la provenance** : le moteur expert devra pouvoir indiquer si une conclusion vient de `verifie_constructeur`, `decoded_by_project`, `recoupee` ou `source_externe`.

