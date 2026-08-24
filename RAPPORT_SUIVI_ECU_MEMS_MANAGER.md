# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> Ce rapport est volontairement en **Markdown lisible directement sur GitHub** : il ne doit pas être remplacé par un format encodé ou opaque.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026**

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

Branche de travail concernée : `MEMSX64`.

Objectif demandé : **ne pas reconstruire une DLL x64 limitée aux seules fonctions actuellement appelées par MEMS Manager**. Il faut d’abord établir une cartographie complète des capacités ROSCO/librosco, puis fabriquer une base x64 qui conserve les possibilités connues et permette les extensions futures.

### Sources déjà retrouvées

1. Version historique publique de Colin Bourassa : `colinbourassa/librosco`.
   - code source C disponible ;
   - fichiers principaux : `src/setup.c`, `src/protocol.c`, `src/rosco.h` ;
   - version CMake annoncée 1.0.0 dans la branche actuelle du dépôt public ;
   - transport série Windows/Linux, polling 0x7D/0x80, IAC, actionneurs, effacement défauts, heartbeat.

2. Version/famille étendue utilisée par MEMS-Scan/Haro et apparentée à LeopoldG.
   - le `rosco.h` présent dans ECU MEMS Manager contient une API plus riche que la version historique Colin ;
   - fonctions supplémentaires : reset ECU, reset adaptations, trims, ralenti, avance, purge, lambda heater, ventilateurs, boost, mode interactif, etc.

3. Dépôts LeopoldG retrouvés :
   - `LeopoldG/mems-rosco`
   - `LeopoldG/mems-spi`
   - `LeopoldG/sources-mems-rosco`

Ces sources/forks doivent être comparés fonction par fonction avant de choisir la base x64.

### API déclarée dans le `rosco.h` actuellement présent dans ECU MEMS Manager

Fonctions principales déclarées :

- `mems_init`
- `mems_init_link`
- `mems_cleanup`
- `mems_connect`
- `mems_disconnect`
- `mems_is_connected`
- `mems_read_raw`
- `mems_read`
- `mems_read_iac_position`
- `mems_move_iac`
- `mems_test_actuator`
- `mems_clear_faults`
- `mems_reset_ECU`
- `mems_reset_adjustments`
- `mems_heartbeat`
- `mems_fuel_trim_plus/minus`
- `mems_idle_decay_plus/minus`
- `mems_idle_speed_plus/minus`
- `mems_ignition_advance_plus/minus`
- `mems_interactive_mode`
- `mems_Purge_Valve_On/Off`
- `mems_O2Heater_On/Off`
- `mems_Fan1_On/Off`
- `mems_Fan2_On/Off`
- `mems_Boost_Valve_On/Off`
- `mems_get_lib_version`

### Commandes déclarées dans ce header étendu

Données/générales :

- `0x7D` — requête trame 7D
- `0x80` — requête trame 80
- `0xCC` — effacement défauts
- `0xF4` — heartbeat / all actuators off selon contexte
- `0xFB` — position IAC
- `0x0F` — déclaré `ResetAdj` dans ce header
- `0xFA` — déclaré `ResetECU` dans ce header

Actionneurs / réglages :

- `0x11/0x01` pompe carburant ON/OFF
- `0x12/0x02` relais PTC ON/OFF
- `0x13/0x03` relais climatisation ON/OFF
- `0x18/0x08` purge ON/OFF
- `0x19/0x09` chauffage lambda ON/OFF
- `0x1B/0x0B` boost/wastegate ON/OFF
- `0x1D/0x0D` ventilateur 1 ON/OFF
- `0x1E/0x0E` ventilateur 2 ON/OFF
- `0x6F/0x67` ventilateur 3 ON/OFF
- `0xF7` test injecteurs
- `0xEF` test injecteurs MPI
- `0xF8` déclenchement bobine
- `0xFD/0xFE` ouverture/fermeture IAC
- `0x79/0x7A` fuel trim +/-
- `0x89/0x8A` idle decay +/-
- `0x91/0x92` idle speed +/-
- `0x93/0x94` ignition advance +/-

### Ambiguïtés déjà identifiées

La signification de certaines commandes varie entre forks/sources. Ne pas compiler une nouvelle DLL en recopiant aveuglément un fork.

Exemple important :

- header Haro actuel : `0x0F = ResetAdj`, `0xFA = ResetECU` ;
- d’autres sources Leopold attribuent différemment `0x0F`, `0xFA`, voire `0xAF`.

Ces divergences doivent être résolues par comparaison de code, protocoles et familles ECU avant de figer l’API x64.

### Architecture cible recommandée

Une future librosco x64 devrait séparer :

- transport série générique ;
- moteur générique envoi/réception ROSCO ;
- profils MEMS 1.2 / 1.3 / 1.6 / 1.9 ;
- modes diagnostics explicites ;
- compatibilité avec les anciennes fonctions validées ;
- primitive générique permettant d’envoyer des commandes ROSCO supplémentaires sans reconstruire l’architecture ;
- opérations sensibles Mode 4 / programmation isolées des lectures normales.

---

## 5. Étape immédiate en cours au moment de cette mise à jour

L’utilisateur a demandé de terminer les contrôles restants avant d’aller plus loin avec la DLL x64.

À contrôler maintenant sur la DLL librosco actuellement utilisée :

1. architecture PE exacte (32/64 bits) ;
2. taille et empreinte du binaire ;
3. table exacte des exports ;
4. présence éventuelle d’exports non déclarés dans `rosco.h` ;
5. dépendances DLL/imports Windows ;
6. correspondance export par export avec les différentes sources Colin/Haro/Leopold ;
7. identification de la source la plus proche du binaire actuel ;
8. liste des fonctions à préserver impérativement dans la future x64 ;
9. liste des fonctions/commandes à garder disponibles mais considérées expérimentales ou non validées.

**Le travail doit reprendre directement par ces contrôles.**
