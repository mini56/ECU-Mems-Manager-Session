# CHANTIER EN COURS — intégration ECU MEMS 1.9 puis MEMS 2J

**Pour reprendre le travail, commencer par le [journal de développement du 20/11/2026](13_JOURNAL_DEVELOPPEMENT_MEMS19_PUIS_2J_20261120.md).**
Le journal indique les commits réalisés, les fichiers modifiés, le statut des tests et la prochaine étape exacte.

**Dépôt :** `mini56/ECU-Mems-Manager-Session`. **Branche unique :** `fix-documentary-additive-session-161-20261118`. Ne pas créer de branche de plus ou copier les changements d'une autre branche sans vérifier les diffs.

## Référence exhaustive RoverMEMS Windows i386 (20/11/2026)

- **[Rapport technique détaillé des séquences, dialogues, données et fonctions modifiant les ECU](14_RAPPORT_DETAILLE_ROVERMEMS_I386_TOUTES_SEQUENCES_20261120.md)** — couvre **1.2/1.3/1.6, 1.9, 2J, 3 et RC5 airbag/SRS**.
- **[Catalogue JSON pour le futur développement](15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json)** — profils, hexadécimal, mesures, commandes modifiantes, DTC et provenance.
- **[Sources Go figées sur la révision de l'exécutable](rovermems-reference/i386-20230128/)** — distinctes des six anciennes sources déjà conservées ; GPLv3.

Documentation uniquement : le runtime SESSION et la Release 182 ne sont pas modifiés. Les tests sur ECU réel restent nécessaires.

## Bancs d'essai MEMS — deux pages Andrew Revill (23/11/2026)

- **[Rapport 34 — Banc minimal MEMS et interface ECU/antidémarrage](34_BANCS_ESSAI_MEMS_ANDREW_REVILL_REFERENCES_20261123.md)** — conserve les **deux pages sources** et les **quatre liens originaux vers les schémas** (MEMS 3, MEMS 2J, MEMS 1.9 et TD5). Le banc plus complet ajoute antidémarrage, indicateurs et simulation vilebrequin/arbre à cames. **Aucun schéma minimal MEMS 1.6 n'est présenté** : ne pas transposer ces brochages à la voiture MEMS 1.6.

## Bibliothèque de firmwares Andrew Revill — référence externe (21/11/2026)

- **[Archive Andrew Revill : source et cadre d'utilisation](29_ARCHIVE_FIRMWARE_ANDREW_REVILL_REFERENCE_20261121.md)** — lien externe vers les fichiers de firmwares Rover. **Pas de copie binaire dans le dépôt, pas d'intégration dans le Setup ni de modification de Manager.**

## Archive EEPROM Lucas 5AS — 18 échantillons (21/11/2026)

- [Répertoire Andrew Revill, variantes 97MY / 98MY / 02MY, étude et précautions](31_ARCHIVE_LUCAS5AS_EEPROM_ANDREW_REVILL_20261121.md). Conserver les BIN comme référence externe uniquement ; ne pas diffuser de données de clés ni de sécurité.

## Archive Lucas 10AS — référence externe (21/11/2026)

- [Andrew Revill : échantillon BIN 256 octets et pistes de recherche Lucas 10AS](32_ARCHIVE_LUCAS10AS_EEPROM_ANDREW_REVILL_20261121.md). Le format et la cartographie des données restent à vérifier ; aucune commande ou adaptation 5AS n'est automatiquement réutilisée.

## Archive Lucas 8AS — référence externe (21/11/2026)

- [Répertoire Andrew Revill : un fichier Lucas 8AS (256 octets), datation 1994 et limites d'interprétation](33_ARCHIVE_LUCAS8AS_ANDREW_REVILL_20261121.md). Distinct des Lucas 5AS et 10AS : protocole et structure mémoire à étudier séparément.

## Apprentissage PLIP Lucas 5AS (SESSION 192)

- [Commandes documentées 5F/6F, confirmation obligatoire et limites des validations](30_LUCAS5AS_PLIP_APPRENTISSAGE_SESSION192_20261121.md). Les tests logiciels ne prouvent pas que des télécommandes ont été associées à un boîtier réel.

## Où trouver les éléments

- [Journal de reprise et avancement réel](13_JOURNAL_DEVELOPPEMENT_MEMS19_PUIS_2J_20261120.md) — point d'entrée obligatoire.
- [Audit 2J des variantes, des unités et des défauts](10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md).
- [Vérification des trames et calculs MEMS 1.9/2J](11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md).
- [Bilan de recherche avant intégration](12_BILAN_RECHERCHE_ET_DECODAGE_19_2J_AVANT_INTEGRATION_20261119.md).
- [Sources originales Go RoverMEMS et licence](rovermems-reference/SOURCE_ET_LICENCE.md).
- [Décision câbles USB CH340/FT232 du 19/11 (branche de recherche historique)](https://github.com/mini56/ECU-Mems-Manager-Session/blob/research-mems2j-rovermems-protocol-20261117/research/mems/lecture-ecu-19-2j-3/09_DECISIONS_CABLES_USB_CONNEXION_19_2J_20261119.md).

## État de chantier au 20/11/2026

> **Attention : état historique ci-dessous.** Les affirmations de ce bloc datant de l'étape initiale de développement (notamment « aucun pilote 2J » et « compilation non effectuée ») ont été dépassées par les builds suivants. Au **BUILD 182**, les chemins C++ MEMS 1.9 et **MEMS 2J** sont présents et les tests Windows ont réussi ; la réussite d'une connexion sur calculateur réel reste à confirmer. Le nouveau rapport 14 et le catalogue 15 ci-dessus constituent la référence documentaire des autres familles, dont **MEMS 3** ; ces documents ne les implémentent pas automatiquement.


**MEMS 1.9** : connexion automatique selon le câble et réveil ISO9141 en C++ révisés, autotest indépendant ajouté, diagnostic de réveil relié au journal du test ECU 1.9. **Compilations et tests sur ECU non encore effectués pour ces commits.**

**MEMS 2J** : recherche documentaire et six sources de référence conservées ; **aucun pilote 2J dans le code applicatif**.

**Contraintes** : conserver le protocole ROSCO qui fonctionne déjà sur 1.2/1.3/1.6, les pages existantes, les traductions, les données et l'Aide ; zéro patch ajouté ; aucun nouveau verrou sans accord. Le câble physique est déjà repéré côté Manager. Les tests sur calculateurs réels sont à faire *après* les tests logiciels, sans interrompre le développement.

**Le code Go de RoverMEMS est conservé à titre de référence sous GPLv3**, pas compilé dans Manager.

## Commandes et fichiers importants

- `serialadapterdetector.h` — identification du câble et priorité des tentatives.
- `memsinterface_dispatch.cpp` — réveil 1.9 et sélection de connexion.
- `mems19protocol.h` — génération des bits et reconnaissance des réponses ISO9141.
- `mems19protocol_test.cpp` — autotests sans calculateur ; cible CMake `mems19_protocol_selftest`.
- `memsinterface.cpp` — liaison ROSCO historique (`mems_connect`, `mems_init_link`, `mems_read`), laissée inchangée.
- `librosco-x64/protocol.c` — initialisation CA/75/F4/D0, lectures 80 et 7D.
- `mems19testtab.h` — journal des essais réels existant, désormais enrichi des statuts de réveil.

**Ne déclarer aucun résultat matériel « validé » sans logs et calculateur réel.**
