# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> Ce rapport est volontairement en **Markdown lisible directement sur GitHub** : il ne doit pas être remplacé par un format encodé ou opaque.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — contrôle binaire librosco terminé, préparation x64 en cours**

---

## 1. Règles de travail à conserver

- Dépôt principal : `mini56/ECU-Mems-Manager-Session`.
- L’utilisateur compile et teste avec **GitHub Actions sous Windows**, pas avec Qt Creator.
- Ne modifier que ce qui est explicitement demandé.
- Ne pas faire régresser l’interface, les fonctions existantes ou les protocoles déjà validés.
- Conserver l’interface sombre et responsive.
- Les familles MEMS **1.2 / 1.3 / 1.6 / 1.9** doivent rester distinguées.
- Dans `memsinterface.h`, conserver impérativement `void onProtocolCommandRequested(quint8 command);`.
- Ne pas confondre les données de polling normal `0x7D/0x80`, les lectures RAM/Mode 4, les cartes/calibrations et les données issues de sources externes.

---

## 2. IA MEMS — état validé

Branche de développement IA : `lab-expert-engine`.

Dernier correctif de qualité IA :

- `cbcb8a14189b1bc013cb3519b0ba33aa3f85c072` — `Fix IA MEMS history scroll and factual answers`
- `896a59f762e648ce50023121491693ee70ad162d` — `Build IA MEMS quality patch`

Le package IA MEMS Windows complet a été validé par GitHub Actions :

- run : `32694753190`
- modèle : `Qwen3-0.6B-Q8_0`
- runtime : `llama.cpp b10516 Windows x64 CPU`
- artefact : `ECU-MEMS-Manager-IA-MEMS-Windows-Lab`
- taille : environ 798 Mo
- SHA256 : `753550992b43fff6a569752f818f8b5d208de3210d734d6d38e381b830112ec3`

Le correctif ajoute notamment :

- scrollbar verticale toujours visible dans la conversation IA ;
- réponses factuelles déterministes pour créateur, date, base de données, câble, dwell/bobine ;
- Claude Lespagnol est bien identifié comme concepteur/développeur d’ECU MEMS Manager ;
- Andrew Revill/RoverMEMS sont des références techniques, pas les créateurs du logiciel.

Points à surveiller plus tard, sans modification automatique :

- les réponses déterministes ne sont pas actuellement réinjectées dans l’historique Qwen ;
- certains typos peuvent rater les intentions déterministes ;
- le comptage DB peut inclure des tables techniques ;
- la récupération documentaire reste perfectible pour les questions trop générales.

---

## 3. Base Andrew Revill / MEMSTools — lot 1600

Le lot complet existe :

- `database/reference/research_enrichment_1600.qz64`
- taille compressée : environ **33,3 Mo**
- présent dans `database/reference/manifest.json`
- révision de base indiquée : 20

Il contient la couche de conservation complète des métadonnées Andrew Revill/MEMSTools :

- 478 730 propriétés DEF ;
- 45 934 propriétés DIM ;
- 3 524 519 cellules non vides de matrices de corrélation.

Le générateur `tools/build_memstools_complete_metadata.py` utilise un vrai lecteur CSV (`csv.reader`) : le problème ancien de virgules/champs CSV n’est plus le blocage principal.

Le parseur SQL du chargeur de référence a également été amélioré pour accepter les instructions SQL multilignes terminées par point-virgule en respectant les chaînes entre guillemets.

**Point essentiel :** le lot 1600 brut est volontairement livré et audité mais n’est pas chargé directement dans la base SQLite runtime 32 bits. Les deux chargeurs sautent explicitement `research_enrichment_1600.qz64` afin d’éviter de saturer l’espace mémoire 32 bits avec plus de 3,5 millions de cellules brutes.

Les éléments Andrew déjà compris et exploitables sont intégrés sous forme sémantique dans les lots précédents, notamment 1500–1540. Le lot 1600 reste une archive de conservation/audit et une source pour de futurs traitements compacts.

Ne jamais promouvoir automatiquement une corrélation brute Andrew en adresse RAM live ou en fait diagnostic validé.

---

## 4. Travail en cours — future librosco x64

Branche de travail : `MEMSX64`.

Objectif demandé : **ne pas reconstruire une DLL x64 limitée aux seules fonctions actuellement appelées par MEMS Manager**. Il faut préserver le contrat existant, conserver les primitives génériques ROSCO et cataloguer les capacités supplémentaires sans importer aveuglément les erreurs ou ambiguïtés des anciens forks.

### Sources retrouvées et filiation

1. `colinbourassa/librosco`
   - source historique C disponible ;
   - transport série, initialisation, polling `0x80/0x7D`, IAC, actionneurs, défauts et heartbeat ;
   - tag `0.1.12` retrouvé et comparé.

2. `haro78/MEMS-Scan`
   - le `rosco.h` actuellement présent dans ECU MEMS Manager est **strictement identique octet pour octet** au header Haro ;
   - SHA Git du blob commun : `4c39acadc2e5c37b4d208ea0459d10e90bc7a0a4`.

3. `LeopoldG/Source-librosco`
   - basé sur Colin 0.1.7 ;
   - version déclarée 3.0.0 ;
   - catalogue de nombreuses commandes supplémentaires ;
   - **ne doit pas être utilisé comme remplacement direct** car certaines commandes de reset ont une signification différente de la DLL réellement utilisée par MEMS Manager.

4. Autres dépôts utiles retrouvés :
   - `LeopoldG/mems-rosco`
   - `LeopoldG/mems-spi`
   - `LeopoldG/sources-mems-rosco`

---

## 5. Contrôle complet de la DLL actuelle — TERMINÉ

Deux rapports lisibles supplémentaires sont générés sur la branche `RAPPORT` :

- `RAPPORT_LIBROSCO_BINAIRE.md` : identité PE, exports, imports, sections, empreintes ;
- `RAPPORT_LIBROSCO_DESASSEMBLAGE.md` : désassemblage ciblé des fonctions exportées et contrôle des commandes réellement envoyées.

### 5.1 Identité exacte du binaire

Fichier : `prebuilt-librosco/librosco.dll`

- taille : **49 672 octets** ;
- SHA-256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- SHA-1 : `70b8bf3265510fe06cf9ac8921b3a07c344d9132` ;
- MD5 : `b03655b490239a9d44c45babfa7de239` ;
- architecture PE : **IMAGE_FILE_MACHINE_I386** ;
- format : **PE32 / x86 32 bits** ;
- horodatage COFF : `2018-04-07T08:26:27Z` ;
- relocations : présentes ;
- signature Authenticode : absente.

Conclusion : la DLL actuelle ne peut pas être chargée directement par une application Windows x64 native.

### 5.2 Dépendances du binaire

Imports externes uniquement depuis :

- `KERNEL32.dll` ;
- `msvcrt.dll`.

Les appels Windows observés correspondent au transport série historique : `CreateFileA`, `ReadFile`, `WriteFile`, `GetCommState`, `SetCommState`, `GetCommTimeouts`, `SetCommTimeouts`, mutex, etc.

Les sections `/4`, `/14`, `/29`, `/41`, `/55`, `/67` et l’utilisation de `msvcrt.dll` sont cohérentes avec un ancien build GNU/MinGW.

Le script historique du projet confirme que l’ancien outillage force explicitement `i686-w64-mingw32.shared`. La limitation 32 bits vient donc du build historique, pas d’un besoin intrinsèque du protocole ROSCO.

### 5.3 Version interne réelle

Le désassemblage de `mems_get_lib_version()` écrit exactement :

- major = `0` ;
- minor = `1` ;
- patch = `12`.

La DLL actuelle se présente donc comme **librosco 0.1.12**.

Cependant, elle n’est **pas** la version Colin 0.1.12 d’origine : son API et sa structure `mems_data` ont été modifiées pour la lignée Haro/MEMS-Scan.

### 5.4 Table exacte des 22 exports

La DLL exporte exactement :

1. `mems_cleanup`
2. `mems_clear_faults`
3. `mems_connect`
4. `mems_disconnect`
5. `mems_get_lib_version`
6. `mems_heartbeat`
7. `mems_init`
8. `mems_init_link`
9. `mems_is_connected`
10. `mems_lock`
11. `mems_move_iac`
12. `mems_openserial`
13. `mems_read`
14. `mems_read_iac_position`
15. `mems_read_raw`
16. `mems_read_serial`
17. `mems_reset_ECU`
18. `mems_reset_adjustments`
19. `mems_send_command`
20. `mems_test_actuator`
21. `mems_unlock`
22. `mems_write_serial`

### 5.5 Différence entre le header actuel et la DLL

Le header Haro actuel déclare **35 fonctions `mems_*`** mais le binaire n’en exporte que 22.

Six fonctions techniques sont exportées mais non déclarées dans le header public actuel :

- `mems_lock`
- `mems_openserial`
- `mems_read_serial`
- `mems_send_command`
- `mems_unlock`
- `mems_write_serial`

Dix-neuf wrappers déclarés dans le header ne sont pas présents dans la DLL, notamment les wrappers dédiés fuel trim, idle, avance, purge, lambda heater, ventilateurs et boost, ainsi que `mems_interactive_mode`.

Ce n’est pas nécessairement une perte fonctionnelle : MEMS Manager utilise déjà `mems_test_actuator(info, commande, ...)`, qui est une primitive générique capable d’envoyer les octets de commande correspondants.

### 5.6 Commandes confirmées directement dans le vrai binaire

Le désassemblage lève l’ambiguïté principale :

- `mems_reset_ECU()` envoie réellement **`0xFA`** ;
- `mems_reset_adjustments()` envoie réellement **`0x0F`** ;
- `mems_clear_faults()` envoie **`0xCC`** ;
- `mems_heartbeat()` envoie **`0xF4`** ;
- `mems_read_iac_position()` envoie **`0xFB`** ;
- `mems_init_link()` utilise exactement **`CA 75 F4 D0`** comme séquence de démarrage ;
- `mems_read_raw()` envoie `0x80`, lit **28 octets**, puis envoie `0x7D` et lit **32 octets** ;
- `mems_test_actuator()` transmet le byte de commande fourni par l’appelant, puis lit un byte de réponse.

**Règle de compatibilité désormais fixée pour notre future x64 :** ne pas remplacer `0xFA = reset ECU` et `0x0F = reset adjustments` par les significations différentes du fork Leopold 3.0.

### 5.7 Conflit confirmé avec Leopold 3.0

Leopold 3.0 déclare actuellement :

- `0xAF` = reset ECU ;
- `0xFA` = reset adjustments ;
- `0x0F` = reset émissions ;
- `0x81` = sauvegarde.

Cela entre directement en conflit avec le binaire utilisé par MEMS Manager. Les sources Leopold restent donc une **source de cartographie**, pas une autorité de remplacement.

### 5.8 Structure `mems_data` réellement attendue par la DLL

Le vrai `mems_read()` exécute `memset(..., 0, 0x3C)` : il remplit donc une structure de **60 octets**.

C’est exactement la taille de la structure Haro actuellement utilisée par ECU MEMS Manager et non celle de la structure Colin 0.1.12 d’origine.

Cartographie confirmée de la sortie `mems_data` :

| Offset | Champ | Provenance |
|---:|---|---|
| 0–1 | `engine_rpm` | 80[1:2], assemblage 16 bits |
| 2 | `coolant_temp` | 80[3] brut |
| 3 | `ambient_temp` | 80[4] brut |
| 4 | `intake_air_temp` | 80[5] brut |
| 5 | `fuel_temp` | 80[6] brut |
| 6 | `map_kpa` | 80[7] brut |
| 7 | `battery_voltage` | 80[8] brut |
| 8 | `throttle_pot` | 80[9] brut |
| 9 | `idle_switch` | 80[10] brut |
| 10 | `uk1` | 80[11] |
| 11 | `park_neutral_switch` | 80[12] brut |
| 12 | `fault_codes` | synthèse de 80[13]/80[14] |
| 13 | `idle_set_point` | 80[15] |
| 14 | `idle_hot` | 80[16] |
| 15 | `uk2` | 80[17] |
| 16 | `iac_position` | 80[18] |
| 17 | padding | zéro |
| 18–19 | `idle_error` | 80[19:20], assemblage 16 bits |
| 20 | `ignition_advance_offset` | 80[21] brut |
| 21 | `ignition_advance` | 80[22] brut |
| 22–23 | `coil_time` | 80[23:24], assemblage 16 bits brut |
| 24–26 | `uk3/uk4/uk5` | 80[25:27] |
| 27–57 | données 7D | 7D[1:31], ordre du header Haro |
| 58 | `dtc0` | 80[13] brut |
| 59 | `dtc1` | 80[14] brut |

Le champ synthétique `fault_codes` à l’offset 12 est construit exactement ainsi :

- bit 0 si `dtc0 & 0x01` ;
- bit 1 si `dtc0 & 0x02` ;
- bit 2 si `dtc1 & 0x02` ;
- bit 3 si `dtc1 & 0x80`.

Le comportement du binaire confirme donc la filiation suivante :

**Colin 0.1.12 comme base historique + modifications Haro/MEMS-Scan de la structure de données + ajout des deux resets.**

### 5.9 Fonctions à préserver impérativement dans la future x64

Le premier objectif x64 doit conserver au minimum le contrat réel des 22 exports ci-dessus, notamment les primitives basses `mems_send_command`, `mems_read_serial`, `mems_write_serial`, car elles permettent de ne pas enfermer la future DLL dans la liste des seules fonctions GUI actuelles.

Les commandes supplémentaires issues de Haro/Leopold/ROSCO doivent être cataloguées et ajoutées par profils/familles lorsque leur sémantique est suffisamment sûre.

---

## 6. Architecture x64 décidée pour la prochaine étape

La migration se fera en deux niveaux afin d’éviter toute régression :

### Niveau A — bibliothèque x64 de compatibilité

- source C contrôlée dans le dépôt ;
- compilation native Windows x64 ;
- conservation des **22 exports** du vrai binaire ;
- conservation de la structure Haro de **60 octets** ;
- conservation exacte des commandes déjà vérifiées ;
- tests automatiques ABI sans ECU ;
- contrôle PE32+ et table d’exports par GitHub Actions ;
- **aucun remplacement de la DLL du programme tant que ce niveau n’est pas vert.**

### Niveau B — moteur ROSCO extensible

Après validation du niveau A :

- profils séparés MEMS 1.2 / 1.3 / 1.6 / 1.9 ;
- transport et transaction ROSCO génériques ;
- catalogue de commandes avec statut `confirmé`, `source externe`, `expérimental`, `conflit` ;
- séparation nette polling `7D/80`, Mode 4/RAM, cartes/calibrations et programmation ;
- ajout progressif des capacités supplémentaires sans casser l’API de compatibilité.

---

## 7. Prochaine action en cours

Construire sur `MEMSX64` une première source `librosco-x64` de compatibilité, puis une GitHub Action dédiée qui vérifie :

- compilation native x64 ;
- `sizeof(mems_data_frame_80) == 28` ;
- `sizeof(mems_data_frame_7d) == 32` ;
- `sizeof(mems_data) == 60` ;
- version de compatibilité 0.1.12 ;
- présence exacte des 22 exports historiques ;
- format DLL **PE32+ / AMD64**.

Le programme principal ne doit pas encore être basculé sur cette DLL avant validation complète de ces contrôles.
