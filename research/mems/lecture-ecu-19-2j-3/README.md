# CHANTIER EN COURS — intégration ECU MEMS 1.9 puis MEMS 2J

**Pour reprendre le travail, commencer par le [journal de développement du 20/11/2026](13_JOURNAL_DEVELOPPEMENT_MEMS19_PUIS_2J_20261120.md).**
Le journal indique les commits réalisés, les fichiers modifiés, le statut des tests et la prochaine étape exacte.

**Dépôt :** `mini56/ECU-Mems-Manager-Session`. **Branche unique :** `fix-documentary-additive-session-161-20261118`. Ne pas créer de branche de plus ou copier les changements d'une autre branche sans vérifier les diffs.

## Référence exhaustive RoverMEMS Windows i386 (20/11/2026)

- **[Rapport technique détaillé des séquences, dialogues, données et fonctions modifiant les ECU](14_RAPPORT_DETAILLE_ROVERMEMS_I386_TOUTES_SEQUENCES_20261120.md)** — couvre **1.2/1.3/1.6, 1.9, 2J, 3 et RC5 airbag/SRS**.
- **[Catalogue JSON pour le futur développement](15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json)** — profils, hexadécimal, mesures, commandes modifiantes, DTC et provenance.
- **[Sources Go figées sur la révision de l'exécutable](rovermems-reference/i386-20230128/)** — distinctes des six anciennes sources déjà conservées ; GPLv3.

Documentation uniquement : le runtime SESSION et la Release 182 ne sont pas modifiés. Les tests sur ECU réel restent nécessaires.

## Où trouver les éléments

- [Journal de reprise et avancement réel](13_JOURNAL_DEVELOPPEMENT_MEMS19_PUIS_2J_20261120.md) — point d'entrée obligatoire.
- [Audit 2J des variantes, des unités et des défauts](10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md).
- [Vérification des trames et calculs MEMS 1.9/2J](11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md).
- [Bilan de recherche avant intégration](12_BILAN_RECHERCHE_ET_DECODAGE_19_2J_AVANT_INTEGRATION_20261119.md).
- [Sources originales Go RoverMEMS et licence](rovermems-reference/SOURCE_ET_LICENCE.md).
- [Décision câbles USB CH340/FT232 du 19/11 (branche de recherche historique)](https://github.com/mini56/ECU-Mems-Manager-Session/blob/research-mems2j-rovermems-protocol-20261117/research/mems/lecture-ecu-19-2j-3/09_DECISIONS_CABLES_USB_CONNEXION_19_2J_20261119.md).

## État de chantier au 20/11/2026

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
