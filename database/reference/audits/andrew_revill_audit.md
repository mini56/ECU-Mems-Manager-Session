# Audit Andrew Revill -> base de connaissance ECU MEMS Manager

## Principe d'audit

La base `avant Andrew` exclut explicitement tous les lots Andrew 1500-1540 et 1560-1600. Le lot 1550 reste dans la base de comparaison car il appartient à la source distincte rover-mems-386.
Toutes les données Andrew restent `source_externe`; aucune corrélation n'est promue automatiquement en donnée validée projet ou constructeur.

Les scalaires/tables de cartographie, le protocole ROSCO 0x7D/0x80 et la RAM Mode 4 restent trois couches distinctes.

## Sources Andrew intégrées

- `andrew_revill_family_support_page` — Full Support Added for All Rover MEMS ECU Families — **MEMS 1.2 / 1.3 / 1.6 / 1.9** — `source_externe`
- `andrew_revill_memstools` — MEMSTools – Rover MEMS knowledge corpus — **MEMS 1.3 / 1.6 / 1.9** — `source_externe` — SHA-256 `12bb7dfc8c1f3bed1eed28b85b3bb2c344e4d7b5fcb0534e584092aaeb7ff1e8`
- `andrew_revill_memstools_complete_metadata` — MEMSTools complete DEF/DIM/correlation metadata — **MEMS 1.3 / 1.6 / 1.9** — `source_externe` — SHA-256 `12bb7dfc8c1f3bed1eed28b85b3bb2c344e4d7b5fcb0534e584092aaeb7ff1e8`
- `andrew_revill_memstools_mems12` — MEMSTools – Rover MEMS 1.2 corpus — **MEMS 1.2** — `source_externe` — SHA-256 `12bb7dfc8c1f3bed1eed28b85b3bb2c344e4d7b5fcb0534e584092aaeb7ff1e8`

## Volume sémantique MEMSTools

| Jeu de données | Lignes |
|---|---:|
| Firmwares | 286 |
| Définitions d'axes / unités / échelles | 320 |
| Définitions de scalaires | 18519 |
| Liaisons scalaire <-> firmware/adresse | 96240 |
| Définitions de tables/cartographies | 1815 |
| Liaisons table <-> firmware/index | 17458 |
| Corrélations de variables | 64706 |
| Fichiers sources MEMSTools indexés | 433 |

## Couverture firmware

| Source | Famille | Firmwares |
|---|---|---:|
| `andrew_revill_memstools` | MEMS 1.3 | 3 |
| `andrew_revill_memstools` | MEMS 1.6 | 119 |
| `andrew_revill_memstools` | MEMS 1.9 | 164 |

## Comparaison avec la base antérieure

- Tables avant Andrew : **29** ; après tous les lots Andrew actuellement présents : **45**.
- Firmwares Andrew déjà cités textuellement dans l'ancienne base : **41/286**.
- Identifiants scalaires/tables Andrew retrouvés comme jetons exacts dans l'ancienne base : **0/1091**.
Ces recouvrements sont documentaires uniquement; ils ne constituent pas une validation technique.

## Contrôles d'intégrité

- `orphan_scalar_bindings` : **0**
- `orphan_table_bindings` : **0**
- `variable_null_addresses` : **0**
- `variable_unknown_firmware` : **0**
- `shared_variable_addresses` : **0**
- `scalar_unresolved_axis` : **0**

## MEMS 1.2

- Firmwares corrélés dans le ZIP MEMSTools : **0**
- Liaisons scalaires : **0**
- Liaisons tables : **0**
- Corrélations variables : **0**
- Fichiers 1.2 détectés par l'extracteur : **0**
Le ZIP courant ne fournit donc pas de corpus de corrélation 1.2 équivalent à 1.3/1.6/1.9. Ce manque reste explicite; aucune adresse n'est inventée.

## Point de contrôle AANMP002

`inj_pw_rosco` : scalaire `$00E`, unité `ms`, échelle `0.001333`, offset `0.0`.
Cette information appartient au modèle de calibration MEMS Mapper; elle n'est pas assimilée à l'adresse RAM Mode 4 utilisée par ECU MEMS Manager.

## Règle pour le moteur expert

- Utiliser la provenance et le niveau de confiance de chaque fait.
- Une connaissance Andrew seule peut soutenir une hypothèse ou un contrôle, pas une conclusion forte.
- Une adresse corrélée ne devient une adresse RAM live qu'après validation du chemin de lecture et de sa conversion pour le firmware concerné.

## Complément Andrew publié hors tables MEMSTools

Les pages techniques Andrew ont été intégrées comme sources séparées afin de ne pas confondre une capacité documentée, une architecture firmware et une corrélation d'adresse.

### Sources Andrew structurées au total

- `andrew_revill_family_support_page` — Full Support Added for All Rover MEMS ECU Families — **MEMS 1.2 / 1.3 / 1.6 / 1.9** — `source_externe` — https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
- `andrew_revill_map_firmware_library` — MEMS Map & Firmware Library — **MEMS 1.3 / 1.6 / 1.9; explicit 1.2 coverage gap recorded** — `source_externe` — https://andrewrevill.co.uk/MapFirmwareLibrary/
- `andrew_revill_mems19_architecture` — Remapping the Rover MEMS 1.9 ECU — **MEMS 1.9 (with correlation context for MEMS 1.6 and MEMS 1.X)** — `source_externe` — https://andrewrevill.co.uk/MEMSMapperMems19.htm
- `andrew_revill_memstools` — MEMSTools – Rover MEMS knowledge corpus — **MEMS 1.3 / 1.6 / 1.9** — `source_externe` — https://andrewrevill.co.uk/MEMSToolsIndex.htm
- `andrew_revill_memstools_complete_metadata` — MEMSTools complete DEF/DIM/correlation metadata — **MEMS 1.3 / 1.6 / 1.9** — `source_externe` — https://andrewrevill.co.uk/MEMSToolsIndex.htm
- `andrew_revill_memstools_mems12` — MEMSTools – Rover MEMS 1.2 corpus — **MEMS 1.2** — `source_externe` — https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm

- Capacités famille documentées : **28**
- Descriptions de transport/câble : **4**
- Correspondances de broches câble 3-pin : **3**
- Faits d'architecture/provenance MEMS 1.9 : **13**

### Statut MEMS 1.2 après audit complet

Andrew publie explicitement le support de MEMS 1.2 pour ROSCO, lecture ROM, données live, défauts, tableau de bord, antidémarrage, adaptations et réglages de service. Ces capacités et le câblage sont maintenant structurés dans la base.

En revanche le ZIP MEMSTools courant fournit **0 firmware(s) 1.2 corrélé(s), 0 liaison(s) scalaire(s), 0 liaison(s) table(s) et 0 corrélation(s) variable(s)** dans le format de corrélation utilisé pour 1.3/1.6/1.9. L'absence reste donc enregistrée comme une limite réelle, pas comblée artificiellement.

### Règle d'utilisation par le moteur expert

1. Les connaissances `source_externe` peuvent proposer une hypothèse ou un contrôle, mais ne doivent pas à elles seules déclencher une conclusion forte.
2. Les corrélations de variables firmware ne deviennent des adresses RAM live qu'après validation explicite du chemin de lecture et de la conversion sur le firmware concerné.
3. Les scalaires/tables de cartographie ne doivent jamais être assimilés aux paquets ROSCO 0x7D/0x80 ou à la RAM Mode 4.
4. Une connaissance recoupée avec décodage projet ou mesure ECU réelle peut ensuite être promue vers `recoupee` ou `decoded_by_project`, sans modifier la source Andrew originale.

## Index Andrew Map/Firmware Library

La bibliothèque publique Andrew est indexée dans la base sans embarquer les gros binaires : **1382 fichiers/liens**, **348 identifiants firmware distincts** et **50 entrées contenant au moins une référence MNE/MKC/etc.**

| Famille | Zone | Statut | Fichiers |
|---|---|---|---:|
| MEMS 1.2 | firmware_archive | not_confirmed_by_index | 0 |
| MEMS 1.2 | identified_library | not_listed_at_top_level | 0 |
| MEMS 1.3 | firmware_archive | indexed | 12 |
| MEMS 1.3 | identified_library | indexed | 6 |
| MEMS 1.6 | firmware_archive | indexed | 742 |
| MEMS 1.6 | identified_library | indexed | 260 |
| MEMS 1.9 | firmware_archive | indexed | 1294 |
| MEMS 1.9 | identified_library | indexed | 450 |

Répartition des types de fichiers : `rom` 354, `disassembly` 345, `firmware_hex` 338, `peta_xf1` 176, `peta_xf3` 162, `binary` 7.

Pour MEMS 1.2, l'absence de répertoire de bibliothèque/corrélation confirmé reste explicitement enregistrée. Les capacités 1.2 publiées par Andrew sont conservées séparément dans les tables de capacités/transport ; aucune cartographie ou adresse 1.2 n'est inventée.

## Conservation complète des métadonnées MEMSTools

En plus des tables sémantiques déjà décodées, la base conserve **478730 propriétés DEF**, **45934 propriétés DIM** et **3524519 cellules de matrices de corrélation non vides**. Cette couche est volontairement générique : elle empêche de perdre aujourd'hui une donnée Andrew dont la signification ne serait exploitée que plus tard par le moteur expert.

| Famille | Variante | Dataset | Lignes | Colonnes | Cellules non vides |
|---|---|---|---:|---:|---:|
| MEMS 1.3 | standard | `Firmware` | 10993 | 5 | 51152 |
| MEMS 1.3 | standard | `Scalars` | 800 | 5 | 2895 |
| MEMS 1.3 | standard | `Tables` | 118 | 8 | 596 |
| MEMS 1.3 | standard | `Variables` | 184 | 5 | 855 |
| MEMS 1.6 | NA | `Firmware` | 89405 | 117 | 1238736 |
| MEMS 1.6 | NA | `Scalars` | 3092 | 117 | 34961 |
| MEMS 1.6 | NA | `Tables` | 569 | 232 | 14130 |
| MEMS 1.6 | NA | `Variables` | 1372 | 117 | 27248 |
| MEMS 1.6 | Turbo | `Firmware` | 17568 | 6 | 98418 |
| MEMS 1.6 | Turbo | `Scalars` | 601 | 6 | 3470 |
| MEMS 1.6 | Turbo | `Tables` | 72 | 10 | 718 |
| MEMS 1.6 | Turbo | `Variables` | 316 | 6 | 1614 |
| MEMS 1.9 | standard | `Firmware` | 63306 | 166 | 1928591 |
| MEMS 1.9 | standard | `Scalars` | 3038 | 166 | 57626 |
| MEMS 1.9 | standard | `Tables` | 784 | 330 | 22570 |
| MEMS 1.9 | standard | `Variables` | 1103 | 166 | 40939 |

Les tables sémantiques (`mems_scalar_*`, `mems_table_*`, `mems_variable_correlation`) restent préférées pour les champs déjà compris. La couche générique sert de conservation/audit et ne transforme pas automatiquement un champ brut en fait diagnostique.
