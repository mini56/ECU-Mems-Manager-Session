# Andrew Revill / MEMSTools — intégration de connaissances

Cette intégration est confinée à la branche expérimentale `lab-andrew-revill-knowledge`, créée depuis `a02b6229c3b397e001ab92bbbc333db1ca33a6c6`. Elle ne modifie pas `ui-rebuild` et ne déclenche aucune fusion.

## Source auditée

- Auteur/source : Andrew Revill — MEMSTools
- Archive auditée : `MEMSTools(4).zip`
- SHA-256 : `f652c214641c0c5ffb3d379fe8062dae818e99e5a3c2e47f31165f7ea76d5f6d`
- Périmètre ECU MEMS Manager : MEMS 1.2 / 1.3 / 1.6 / 1.9
- Données effectivement présentes dans le corpus de corrélation/definition fourni : MEMS 1.3 / 1.6 / 1.9. Aucun jeu `RoverMems12` n'est présent dans cette archive ; aucune connaissance MEMS 1.2 n'est donc inventée à partir de cette source.

## Données structurées extraites

L'importeur `tools/build_memstools_enrichment.py` transforme les fichiers de définition/corrélation en tables SQLite de la base de référence :

- `mems_knowledge_source` : provenance, empreinte de l'archive et niveau de vérification ;
- `mems_firmware_catalog` : catalogue des firmwares et famille MEMS ;
- `mems_axis_definition` : unités, largeur, décimales, échelle, offset, signé/non signé ;
- `mems_scalar_definition` : identifiant sémantique, nom, conversion et commentaires source ;
- `mems_scalar_binding` : rattachement d'un scalaire à son adresse pour chaque firmware ;
- `mems_table_definition` : identifiant, nom, axes et dimensions des tables ;
- `mems_table_binding` : rattachement table/firmware ;
- `mems_variable_correlation` : corrélations d'adresses de variables internes entre firmwares, sans inventer de sémantique lorsque la source n'en fournit pas ;
- `mems_source_asset` : inventaire et SHA-256 des désassemblages, ROM, fichiers de corrélation, définitions et aide utiles au périmètre.

Sur l'archive auditée, la génération produit :

- 286 firmwares ;
- 320 classes d'axes/conversions ;
- 18 519 classes de scalaires ;
- 96 240 liaisons scalaire/firmware/adresse ;
- 1 815 classes de tables ;
- 17 458 liaisons table/firmware ;
- 64 706 liaisons de corrélation de variables internes ;
- 433 fichiers sources tracés par chemin, taille et SHA-256.

## Exemple AANMP002

La source définit notamment le scalaire `inj_pw_rosco` sur AANMP002 avec unité `ms` et facteur d'échelle `0.001333`. Cette information est enregistrée comme donnée source externe et n'écrase pas les valeurs RAM validées par les essais réels ECU du projet.

## Règles de confiance

Les données Andrew Revill restent marquées `source_externe`. Elles peuvent être croisées avec les niveaux existants de la base (`verifie_constructeur`, `decoded_by_project`, `recoupee`, etc.), mais une donnée externe ne devient pas automatiquement une vérité constructeur.

Une adresse corrélée sans nom sémantique reste une corrélation d'adresse : l'importeur ne lui invente ni fonction ni unité. C'est essentiel pour le futur moteur expert.

## Ce qui n'est pas embarqué brut

Les exécutables Windows, DLL, images complètes de firmware et listings ASM complets du ZIP ne sont pas copiés tels quels dans le dépôt. La base embarque leurs connaissances structurées utiles et conserve les empreintes/provenances permettant de remonter au fichier source. Cela évite de gonfler inutilement le programme et le temps de reconstruction de la base.

Les grandes corrélations d'adresses de code restent des sources de recherche et ne sont pas chargées dans la base runtime tant qu'un usage concret du moteur expert ne le justifie pas. Les corrélations de variables, tables et scalaires utiles au raisonnement par firmware sont, elles, intégrées.
