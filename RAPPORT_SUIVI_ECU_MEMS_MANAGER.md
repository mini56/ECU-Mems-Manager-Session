# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.

Dernière mise à jour : **24 août 2026 — AUDIT PRÉ-ECU BUILD #26 TERMINÉ : CI VERT, MAIS NO-GO MATÉRIEL AVANT CORRECTION DES BLOQUEURS DE SÉCURITÉ PROTOCOLE/MODE.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 officielle : **`MEMSX64`**.
- Branche de rapport : **`RAPPORT`**.
- Branche 32 bits historique : **`lab-expert-engine`**, à laisser intacte.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- **BUILD X64 COURANT : BUILD #26 — v1.0.26.**
- Commit source x64 audité : **`3c4102eca34a2426970ee03e01830a6317b9db07`**.
- Workflow x64 unique sur `MEMSX64` : **`.github/workflows/memsx64.yml`**.
- **BUILD #26 EST VERT EN GITHUB ACTIONS.**
- **VERT CI signifie uniquement que la construction et les contrôles automatiques ont réussi.**
- **BUILD #26 n’est pas validé fonctionnellement sur ECU.**
- **AUDIT PRÉ-ECU : NO-GO pour un test matériel complet dans l’état du commit audité.**
- Le NO-GO provient principalement de commandes dont la signification change selon le mode diagnostic et la famille ECU, alors que certaines interfaces utilisateur restent accessibles sans verrouillage de contexte suffisant.
- **Aucun BUILD #27 ne doit être créé pour corriger ces points : les corrections restent BUILD #26 — v1.0.26 jusqu’à validation.**

### Règle BUILD / VERSION

Le numéro de BUILD est la version du programme :

- BUILD #14 = v1.0.14
- BUILD #26 = v1.0.26
- BUILD #100 = v1.1.0
- BUILD #588 = v1.5.88
- BUILD #662 = v1.6.62

Calcul : `1.(build / 100).(build % 100)`.

**Important : `GITHUB_RUN_NUMBER` ne doit plus décider de la version du programme.**
Le BUILD/version doit être fourni explicitement par `MEMS_BUILD_NUMBER`.
Le commit `3c4102eca34a2426970ee03e01830a6317b9db07` a supprimé le fallback de `CMakeLists.txt` vers `GITHUB_RUN_NUMBER`.

Un rerun GitHub d’un même BUILD reste le **même BUILD/version**.
Un nouveau numéro de BUILD n’est créé que lorsqu’on décide réellement de passer à un nouvel état du programme.

---

## 2. INCIDENT DE MÉTHODE À NE PAS REPRODUIRE

Pendant la migration x64, plusieurs workflows et déclenchements se sont empilés. Des runs #23, #24 et #25 sont apparus alors que la consigne était de travailler un seul build à la fois.

Ces numéros ont créé un brouillage inutile dans GitHub Actions.

### Décision définitive

- **#23 / #24 / #25 sont à considérer comme essais parasites/historiques, pas comme bases du projet.**
- Ne pas repartir dessus.
- Ne pas essayer de les « réparer » séparément.
- Ne pas utiliser leur numéro comme état courant.
- L’état courant repart proprement sur **BUILD #26 — v1.0.26**.

### Cause du désordre

Il existait plusieurs workflows x64/IA/tests parallèles avec des déclencheurs différents. Certains pouvaient se lancer à chaque modification `.cpp/.h`, d’autres sur des fichiers x64 spécifiques, et certains workflows écrivaient eux-mêmes des fichiers de statut dans la branche.

Cela pouvait provoquer :

- plusieurs runs simultanés ;
- des numéros GitHub qui montaient sans décision fonctionnelle ;
- des runs d’un ancien workflow après une correction d’un autre ;
- des fichiers de statut périmés donnant une fausse impression d’état courant ;
- un checkout flottant de `MEMSX64` dans un ancien workflow, donc possibilité de rejouer un ancien build avec un code plus récent.

### Nettoyage effectué

Sur `MEMSX64`, les anciens workflows de build x64/IA provisoires ont été retirés de la branche active.

Commit de nettoyage principal :

**`522fae53cb1573a956ce50941d5a185a4d245e66` — `Reset MEMSX64 CI to one clean native x64 build workflow`**

Après ce commit, le dossier `.github/workflows` de `MEMSX64` ne contient plus qu’un workflow :

**`memsx64.yml`**

Les anciens fichiers de statut `ECU_MEMS_X64_BUILD22_STATUS.txt` et `ECU_MEMS_X64_CLEAN_STATUS.txt` ont également été retirés de la branche active afin d’éviter les repères périmés.

L’historique Git conserve les anciens fichiers et workflows ; ils ne sont pas perdus, mais ils ne doivent plus polluer l’état actif.

---

## 3. DISCIPLINE DE BUILD OBLIGATOIRE À PARTIR DE MAINTENANT

1. **Un seul BUILD actif à la fois.**
2. Le BUILD courant est **#26 — v1.0.26**.
3. BUILD vert en CI ≠ programme validé sur ECU.
4. L’audit pré-ECU du commit `3c4102e...` a conclu **NO-GO matériel** avant corrections.
5. Corriger les bloqueurs d’audit dans le **même BUILD #26**.
6. À chaque correction : identifier la cause précise et modifier uniquement ce qui est nécessaire.
7. Ne pas créer #27 pour essayer une variante.
8. Ne pas modifier le 32 bits de référence.
9. Le workflow ne doit pas écrire dans `MEMSX64` pendant son exécution.
10. Le workflow doit compiler le commit exact qui l’a déclenché.
11. Les corrections peuvent produire de nouveaux commits Git tout en restant **BUILD #26 — v1.0.26** tant que le jalon #26 n’est pas validé.
12. `concurrency` doit empêcher deux exécutions de BUILD #26 de tourner simultanément.
13. Une exécution annulée par `cancel-in-progress` après une correction n’est pas un nouvel échec fonctionnel.
14. Après corrections, ajouter des contrôles automatiques spécifiques de sécurité protocole avant le test matériel.
15. Test matériel uniquement quand les bloqueurs CRITIQUES et ÉLEVÉS de la section 18 sont levés.

### Workflow actuel

`memsx64.yml` contient :

- `run-name: BUILD #26 - v1.0.26 - ${{ github.sha }}` ;
- `MEMS_BUILD_NUMBER: '26'` ;
- checkout du **commit exact** `${{ github.sha }}` ;
- `concurrency: memsx64-build-26` ;
- `cancel-in-progress: true` ;
- permissions `contents: read` uniquement ;
- aucun commit automatique de fichier de statut ;
- aucun `git push` du workflow vers `MEMSX64`.

---

## 4. OBJECTIF X64 — QUALITÉ MAXIMALE

La x64 doit devenir la **future base principale, propre et durable** de MEMS Manager.

Ce n’est pas une conversion temporaire du programme x86.

Règles :

- application x64 native ;
- Qt 5.15.2 MSVC 2019 x64 ;
- plugins Qt x64 ;
- `mems_manager_x64.dll` x64 native ;
- tous les PE du package en AMD64 ;
- aucun binaire x86 dans le package final ;
- aucun mélange x86/x64 ;
- pas de wrapper/rustine ajouté uniquement pour faire passer un build ;
- corrections de code MSVC/x64 minimales, justifiées et durables ;
- corriger la cause d’une erreur, pas son symptôme ;
- 7-Zip est accepté uniquement comme **outil de build** pour l’extraction Qt via `aqtinstall` ; il ne doit jamais devenir une dépendance runtime.

Le 32 bits fonctionnel reste figé comme référence et ne doit pas être modifié pour faire avancer la x64.

---

## 5. PREMIER PACKAGE X64 CIBLE

Le premier package validable sur ECU doit être **MEMS Manager complet**, pas une version réduite.

Il doit conserver les 14 onglets officiels :

1. Aperçu
2. Injection
3. Réglages
4. Actionneurs
5. Erreurs
6. Diagnostic automatique
7. IA MEMS
8. Analyse
9. Toutes les mesures
10. ECU/ROSCO
11. Toutes les données
12. Base de données
13. Interactif
14. Test ECU 1.9

Doivent rester présents :

- UI complète ;
- base de données ;
- diagnostic automatique ;
- Injection RAM / Mode 4 ;
- réglages ;
- actionneurs ;
- erreurs ;
- analyse ;
- toutes les mesures ;
- ECU/ROSCO ;
- toutes les données ;
- Interactif ;
- Test ECU 1.9 ;
- traductions ;
- fonctions protocole existantes ;
- onglet IA MEMS ;
- moteur expert et mode de secours IA.

Pour la première validation ECU x64, seul le runtime IA lourd est volontairement absent du package :

- pas de `llama-server.exe` ;
- pas de GGUF local.

Ce retrait est temporaire et sert uniquement à isoler la validation du cœur ECU x64.

Dès validation ECU, réintégrer immédiatement :

- `llama-server.exe` x64 ;
- Qwen3-0.6B-Q8_0 ;
- communication HTTP locale ;
- puis reprendre les améliorations de pertinence/performance IA.

---

## 6. NAVIGATION — RÈGLE PROPRE

Ordre officiel des 14 onglets :

1. Aperçu
2. Injection
3. Réglages
4. Actionneurs
5. Erreurs
6. Diagnostic automatique
7. IA MEMS
8. Analyse
9. Toutes les mesures
10. ECU/ROSCO
11. Toutes les données
12. Base de données
13. Interactif
14. Test ECU 1.9

Règle d’architecture :

**identité stable → ordre → clé de traduction → icône**

Ne jamais :

- reconnaître un onglet par son texte traduit ;
- faire dépendre une traduction de la position visuelle ;
- réinsérer/reclasser périodiquement les onglets avec des timers ;
- traiter IA MEMS comme une exception hors de la table principale.

Au `LanguageChange`, changer uniquement les textes, jamais l’ordre ou l’identité.

Le code actuel de `navigationorderpatch.cpp` vise une navigation 14/14 déterministe et le workflow la contrôle automatiquement.

---

## 7. DLL PROTOCOLE X64

Nom : **`mems_manager_x64.dll`**.

Sources : `librosco-x64/`.

Contraintes conservées :

- PE32+ AMD64 ;
- `frame80 = 28` octets ;
- `frame7d = 32` octets ;
- `mems_data = 60` octets ;
- exactement 22 exports historiques.

Commandes historiques importantes :

- init : `CA 75 F4 D0` ;
- polling : `0x80` puis `0x7D` ;
- IAC : `0xFB` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : `0xFA` ;
- reset adjustments : `0x0F`.

Le x86 historique `prebuilt-librosco/librosco.dll` reste une référence ABI/protocole, mais ne doit pas être livré dans le package x64.

**Règle ajoutée par l’audit : la valeur d’un octet n’est pas une fonction universelle. La famille ECU et le mode diagnostic doivent être connus avant d’autoriser une commande.**

---

## 8. 15 CONTRÔLES X64 OBLIGATOIRES

Le workflow doit échouer si l’un de ces contrôles échoue :

1. compilation propre x64 de l’application ;
2. compilation propre de `mems_manager_x64.dll` ;
3. EXE PE32+ AMD64 ;
4. DLL protocole PE32+ AMD64 ;
5. DLL Qt/plugins x64 ;
6. `qsqlite.dll`, Qt SerialPort et dépendances essentielles x64 ;
7. EXE importe `mems_manager_x64.dll` ;
8. EXE n’importe pas `librosco.dll` ;
9. ancienne DLL x86 absente du package ;
10. exactement 22 exports ;
11. ABI `frame80=28`, `frame7d=32`, `mems_data=60` ;
12. tests parseurs/garde-fous et commandes historiques ;
13. smoke launch du package ;
14. SQLite + ressources indispensables ;
15. inventaire final de tous les PE : AMD64 uniquement.

**BUILD #26 a passé ces contrôles CI.**

Mais les 15 contrôles actuels ne testent pas encore :

- les séquences de commandes interdites entre modes ;
- la contextualisation famille ECU + mode diagnostic ;
- l’interdiction de `D1/F7/F8` quand leur sens devient une écriture en Mode 4 ;
- le timing W4 de l’initialisation 5 bauds MEMS 1.9 ;
- la taille variable des trames MEMS 1.9 ;
- la stratégie de reconnexion spécifique MEMS 1.9 ;
- les timeouts/failsafe des actionneurs maintenus ON.

Ces contrôles doivent être ajoutés après correction des points de la section 18.

---

## 9. ÉTAT TECHNIQUE RÉCENT AVANT BUILD #26

### 9.1 Qt / 7-Zip

Le premier BUILD #22 avait échoué avant compilation de MEMS Manager pendant l’installation Qt :

`py7zr.exceptions.Bad7zFile: Specified path is bad: 5.15.2/msvc2019_64/include`

Correction propre retenue :

- Python 3.11 ;
- `aqtinstall 3.3.0` ;
- extraction Qt par **7-Zip externe supporté par aqtinstall** ;
- pas de downgrade Python ;
- pas de vieux `py7zr` épinglé ;
- pas de modification du programme pour contourner l’outil.

### 9.2 Self-test SQLite 14A

Le test sémantique renvoyait :

`FAIL search 'temperature air' category 'wiring': expected relationship not found`

alors que les résultats retournés étaient bien pertinents.

Correction appliquée dans `database/MemsSearchSelfTest.cpp` :

- avant : attente `température air` ;
- maintenant : attente normalisée ASCII `temperature air`.

Commit : **`ce7560d8316fc6e07aea01f400c41286144cd45a`**.

Aucune donnée de base, aucune logique de recherche réelle et aucun protocole n’ont été modifiés pour cette correction.

### 9.3 Nettoyage CI et numérotation

Commit de nettoyage des workflows :

**`522fae53cb1573a956ce50941d5a185a4d245e66`**.

Commit de verrouillage BUILD/version :

**`3c4102eca34a2426970ee03e01830a6317b9db07`**.

Ce dernier commit retire le fallback CMake vers `GITHUB_RUN_NUMBER`.

---

## 10. BUILD #26 — v1.0.26 — ÉTAT ACTUEL

### CI / compilation

- GitHub Actions : **VERT**.
- Les contrôles automatiques x64 ont terminé sans erreur bloquante.
- Le package/artifact de BUILD #26 est produit par le workflow unique `memsx64.yml`.

### Validation fonctionnelle

- **NON EFFECTUÉE.**
- **AUDIT STATIQUE PRÉ-ECU : NO-GO dans l’état du commit `3c4102e...`.**
- **BUILD #26 NE DOIT PAS ENCORE ÊTRE CONSIDÉRÉ COMME FONCTIONNEL OU COMME NOUVELLE BASE MATÉRIELLE.**
- Les corrections de l’audit doivent être intégrées dans le même BUILD #26, puis le CI doit repasser vert avant tout test matériel.

### Règle immédiate

**Ne pas créer BUILD #27. Corriger les bloqueurs CRITIQUES/ÉLEVÉS de la section 18 dans BUILD #26.**

---

## 11. RÉFÉRENCE MATÉRIELLE 32 BITS À CONSERVER

BUILD #14 — v1.0.14, retesté le 24 août 2026 sur vrai ECU :

- COM3 ;
- firmware **AANMP002** ;
- ID **AANMP002 — MNE101150** ;
- communication verte ;
- polling 7D/80 fonctionnel ;
- navigation stable ;
- IA stable connecté/déconnecté ;
- statut IA `base prête • IA locale prête`.

Injection RAM Mode 4 :

- passage 7D/80 → Mode 4 fonctionnel ;
- retour Mode 4 → polling normal fonctionnel ;
- injection finale exemple ≈ 2,47 ms ;
- `0x03C8 = 1233 ticks` ;
- `0x026E = 0` ;
- `0x0280 = 0`.

Trace de référence : `2026-08-24_18.14.txt`.

---

## 12. TEST MATÉRIEL X64 — UNIQUEMENT APRÈS LEVÉE DU NO-GO

**Ne pas utiliser l’artefact audité `3c4102e...` pour un test ECU complet.**

Après correction et nouveau CI vert du même BUILD #26, procéder par étapes :

### Phase A — lecture seule normale

1. lancement et stabilité ;
2. port COM explicitement choisi et interface K-Line adaptée ;
3. connexion ;
4. identification ECU ;
5. aucun actionneur, aucun reset, aucun réglage, aucun changement de mode manuel ;
6. polling normal uniquement.

### Phase B — comparaison 32 bits

1. vérifier AANMP002 / MNE101150 ;
2. comparer RPM, MAP, batterie, températures et autres valeurs de base avec BUILD #14 ;
3. vérifier acquisition/logging ;
4. vérifier déconnexion propre.

### Phase C — RAM/Injection Mode 4 contrôlé

Uniquement avec le lecteur dédié et son profil firmware exact :

1. entrée Mode 4 via la machine d’état dédiée ;
2. lecture uniquement `DC + offsets 0x00–0x7F` ;
3. lecture `0x03C8`, `0x026E`, `0x0280` sur le firmware validé ;
4. aucune commande générale/actionneur pendant Mode 4 ;
5. retour vérifié au mode normal ;
6. polling normal seulement après confirmation du retour.

### Phase D — MEMS 1.9

Uniquement après corrections spécifiques 1.9 :

1. interface K-Line/KKL électriquement adaptée ;
2. réveil 5 bauds avec timing W4 conforme ;
3. identification/read-only ;
4. parser de trame compatible longueur réelle ;
5. aucun actionneur/réglage/reset pendant la première validation ;
6. reconnexion avec procédure spécifique 1.9, y compris coupure contact si nécessaire.

### Phase E — actionneurs

Les actionneurs ne doivent être testés qu’après validation d’une table **famille + mode + commande** explicite et des timeouts/failsafe.

---

## 13. RÈGLES FONCTIONNELLES À NE PAS PERDRE

- `memsinterface.h` doit conserver `void onProtocolCommandRequested(quint8 command);`.
- Les familles MEMS 1.2 / 1.3 / 1.6 / 1.9 restent distinctes.
- Ne jamais confondre polling normal `0x7D/0x80`, RAM Mode 4, calibrations/cartes et données externes.
- Injection : ne jamais calculer le temps d’injection depuis le polling normal 7D/80.
- Adresses RAM Injection importantes : `0x03C8`, `0x026E`, `0x0280`.
- Injection reste entre Aperçu et Réglages.
- Dwell/temps bobine reste dans l’onglet Injection.
- Conserver le style dark et responsive.
- Ne pas engager de refonte graphique non demandée.
- **Nouvelle règle audit : aucune commande sensible ne doit être envoyée sans connaître la famille ECU et le mode diagnostic courant.**

---

## 14. DÉTECTION AUTOMATIQUE CÂBLE / COM

Objectif final après validation du cœur :

- détecter automatiquement l’interface ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- si plusieurs interfaces : afficher COM + type ;
- au clic Connexion : vérifier interface/protocole ;
- messages clairs : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Cette logique doit être générale aux MEMS supportés, pas réservée au MEMS 1.9.

**Correction minimale imposée par l’audit avant test 1.9 : ne pas envoyer des séquences MEMS à tous les ports série présents sur la machine. Préférer le port explicitement sélectionné ou une liste d’interfaces reconnues/validées.**

---

## 15. BASE MEMS / ANDREW REVILL

Lot brut complet conservé :

`database/reference/research_enrichment_1600.qz64`

Environ 33,3 Mo compressés :

- 478 730 propriétés DEF ;
- 45 934 propriétés DIM ;
- 3 524 519 cellules non vides de corrélations.

Ne pas charger automatiquement les 3,5 millions de cellules sans mesurer RAM, temps de démarrage et utilité diagnostique.

Base principale modifiable :

`<appdir>/database/ecu_mems_manager.sqlite`

---

## 16. IA LOCALE APRÈS VALIDATION ECU X64

Runtime déjà connu :

- llama.cpp Windows x64 CPU ;
- `llama-server.exe` x64 ;
- Qwen3-0.6B-Q8_0 GGUF ;
- communication HTTP locale `127.0.0.1:18089`.

Dès validation du cœur x64 :

1. réintégrer le runtime/modèle local ;
2. garder le même Qwen au premier test pour comparaison ;
3. reprendre les améliorations conversationnelles utiles du BUILD #15 ;
4. mesurer RAM, CPU, démarrage, stabilité, temps de réponse ;
5. améliorer ensuite pertinence diagnostique et performance.

---

## 17. PROCHAINE ACTION EXACTE

1. Lire ce rapport avant toute modification.
2. Ne pas toucher à `lab-expert-engine` / BUILD #14.
3. Travailler uniquement sur `MEMSX64` pour la x64.
4. **BUILD #26 — v1.0.26 est VERT EN CI mais NO-GO MATÉRIEL dans l’état audité `3c4102e...`.**
5. **Ne pas créer #27.**
6. Corriger d’abord le bloqueur CRITIQUE de contextualisation famille/mode et les collisions `D1/F7/F8/F4`.
7. Séparer/verrouiller les commandes accessibles en Mode 4.
8. Corriger le test injecteur MEMS 1.9/famille MPI avant tout essai d’injecteur.
9. Rendre le polling MEMS 1.9 compatible avec les longueurs de trames réelles plutôt que supposer universellement 28/32 octets.
10. Ajouter le délai W4 25–50 ms au réveil 5 bauds.
11. Corriger la reconnexion MEMS 1.9 : la fermeture du port + 450 ms/3 s n’est pas une validation suffisante ; prévoir la procédure adaptée, notamment coupure contact ~15 s si session interrompue.
12. Ajouter timeouts/failsafe aux commandes actionneurs maintenues ON et ne pas utiliser `F4` comme arrêt universel sans contexte.
13. Corriger les retours d’état trompeurs (`adjustmentsResetSuccess`, `moveIACComplete`, tests actionneurs).
14. Ajouter confirmations/verrouillages pour commandes qui modifient l’état ECU, dont clear faults.
15. Ne plus sonder tous les ports série avec des commandes MEMS.
16. Ajouter des tests CI interdisant les séquences cross-mode dangereuses.
17. Refaire BUILD #26 — v1.0.26 jusqu’au vert complet avec ces nouveaux contrôles.
18. Seulement ensuite appliquer le protocole de test matériel de la section 12.
19. Si le test matériel est valide : seulement alors déclarer BUILD #26 nouvelle base x64 fonctionnelle.
20. Réintégrer ensuite l’IA locale x64.

---

## 18. AUDIT PRÉ-ECU BUILD #26 — 24 AOÛT 2026

### 18.1 Périmètre

Audit statique réalisé sur **BUILD #26 — v1.0.26**, commit :

`3c4102eca34a2426970ee03e01830a6317b9db07`

Le CI x64 de ce commit est vert. L’audit a porté en priorité sur tout ce qui peut parler à l’ECU ou modifier son état :

- `mainwindow.cpp` ;
- `memsinterface.cpp` ;
- `memsinterface_dispatch.cpp` ;
- `memsinterface.h` ;
- `librosco-x64/protocol.c` ;
- `librosco-x64/setup.c` ;
- `librosco-x64/rosco.h` ;
- `librosco-x64/ROSCO_COMMAND_CARTOGRAPHY.md` ;
- lecteur RAM/Injection Mode 4 (`mappedinjection_*`) ;
- test RAM AANMP002 (`injectionramtest.cpp`) ;
- profils MEMS 1.9 ;
- `mems19testtab.h` ;
- `serialadapterdetector.h` ;
- diagnostic automatique ;
- chemins reset/clear/réglages/actionneurs ;
- fermeture/déconnexion/reconnexion ;
- architecture de package/CI déjà contrôlée par BUILD #26.

Références externes recoupées :

- Rover MEMS diagnostics/technical : `https://www.rovermems.com/diagnostics/technical/`
- Rover MEMS MEMS 1.9 : `https://www.rovermems.com/mems-1.9/`
- Rover MEMS web app / reconnexion 1.9 : `https://www.rovermems.com/diagnostics/web-app/`
- ISO 9141-2 : temporisation W4 de l’initialisation lente 5 bauds, 25–50 ms entre le second key byte et son inversion.

### 18.2 Verdict global

| Domaine | Verdict |
|---|---|
| Architecture x64 / package | **PASS CI** |
| EXE/DLL Qt/protocole AMD64 | **PASS CI** |
| Diagnostic automatique | **PASS statique / lecture seule** |
| Polling historique 1.2/1.3/1.6 | **hérité, à valider matériellement en x64** |
| Lecteur Injection RAM Mode 4 dédié | **PASS statique lecture seule pour les commandes utilisées** |
| Test RAM AANMP002 dédié | **PASS statique avec whitelist fermée** |
| Sécurité générale des commandes ECU | **NO-GO** |
| Test guidé MEMS 1.9 | **NO-GO avant corrections** |
| Actionneurs MEMS 1.9 | **NO-GO** |
| Garantie « le programme ne peut pas altérer l’ECU » | **IMPOSSIBLE dans l’état actuel : le programme contient volontairement des commandes d’altération et leur contexte n’est pas partout verrouillé** |

**Conclusion : ne pas effectuer un test ECU complet avec l’artefact audité.**

### 18.3 CRITIQUE — collisions de commandes selon le mode diagnostic

La cartographie interne du projet dit explicitement qu’un même octet peut changer de signification selon la famille ECU et surtout selon le mode diagnostic.

Exemples particulièrement sensibles :

- `D1` : identifiant/lecture en mode normal ; **écriture des deux banques RAM calibration vers ROM en Mode 4** selon la cartographie Mode 4 ;
- `D3` : recodage en mode normal / **écriture calibration RAM -> ROM** en Mode 4 ;
- `F7` : injecteur SPi en mode normal / lecture-stream calibration en Mode 4 ;
- `F8` : commande bobine en mode normal / **écriture calibration complète en Mode 4** ;
- `F4` : init/heartbeat/« all actuators off » dans l’historique, mais changement de mode dans d’autres contextes.

Le code UI expose dans ECU/ROSCO :

- `D0`, **`D1`**, `D2`, `F0` ;
- `F2`, **`F3 -> Mode 4`**, `F4`, `F5`.

`D3` est correctement désactivé, ce qui est positif.

**Mais D1 reste accessible après un changement manuel de mode.** Le code ne dispose pas encore d’un verrou central « famille ECU + mode diagnostic + commande autorisée » qui rende impossible une séquence cross-mode dangereuse.

De même, `onIgnitionCoilTest()` envoie `F8` sans vérifier explicitement que la session est en mode normal et que le profil ECU autorise cette signification.

**Risque : une commande portant un nom inoffensif dans l’UI peut avoir une sémantique d’écriture/programming dans un autre mode.**

**Blocage obligatoire avant test :** créer un contexte protocole central et refuser toute commande non autorisée pour la combinaison `{famille, mode}`. En Mode 4, l’interface générale doit être verrouillée et seul le lecteur dédié doit pouvoir envoyer sa whitelist read-only.

### 18.4 ÉLEVÉ — test injecteur non contextualisé MEMS 1.9

`rosco.h` distingue :

- `MEMS_TestInjectors = 0xF7` ;
- `MEMS_TestInjectorsMPi = 0xEF`.

Mais `MEMSInterface::onFuelInjectorTest()` envoie toujours `MEMS_TestInjectors`, donc `0xF7`.

La documentation technique recoupée associe `0xEF` à l’action injecteurs MPI et mentionne aussi des commandes `DA/DB` pour certains MEMS 1.9. `0xF7` est par ailleurs contextuel en Mode 4.

**Verdict : ne pas tester les injecteurs sur MEMS 1.9 avec le code actuel.**

Correction : table explicite famille/ECU -> commande injecteur, avec refus par défaut si non prouvé.

### 18.5 ÉLEVÉ — polling 7D/80 à taille fixe, non prouvé universel pour MEMS 1.9

`librosco-x64/protocol.c` reproduit l’historique :

- trame `0x80` lue sur `sizeof(frame80) = 28` ;
- trame `0x7D` lue sur `sizeof(frame7d) = 32`.

La documentation Rover MEMS indique que les différentes générations MEMS fournissent des quantités de données légèrement différentes et que le premier octet permet de déterminer la longueur du paquet.

Conséquences possibles sur MEMS 1.9 :

- lecture tronquée ou attente de trop d’octets ;
- désynchronisation série ;
- valeurs affichées incorrectes ;
- faux diagnostic ;
- échec de la séquence du test guidé.

Ce point n’est pas identifié comme une écriture ECU, mais il empêche de considérer la lecture 1.9 comme fiable.

Correction : parser la longueur réelle du paquet/profil 1.9 au lieu de supposer universellement 28/32.

### 18.6 ÉLEVÉ — timing ISO 9141 W4 non explicitement respecté

Le réveil MEMS 1.9 envoie correctement :

- adresse `0x16` à 5 bauds ;
- start bit ;
- 8 bits LSB first ;
- stop bit ;
- synchronisation `0x55` ;
- key bytes dynamiques ;
- inversion du second key byte.

Point positif : les key bytes ne sont pas hardcodés.

Mais le code envoie l’inversion du second key byte dès que la réponse a été extraite, sans délai W4 explicite.

ISO 9141-2 fixe W4 à **25–50 ms**.

Risque principal : échec ou fonctionnement aléatoire avec certains ECU/câbles, pas écriture ECU.

Correction : temporisation monotone contrôlée 25–50 ms avant l’inversion, avec validation de la réponse inverse adresse.

### 18.7 ÉLEVÉ — modèle de reconnexion MEMS 1.9 non valide dans l’état actuel

`mems_disconnect()` de la DLL x64 ferme le handle COM ; il n’envoie pas de commande `F6` de fin/reset de session diagnostic.

Le test guidé MEMS 1.9 relance une connexion environ **450 ms** après déconnexion.

La reconnexion automatique générale de MainWindow utilise un timer de **3 secondes**.

La documentation Rover MEMS avertit que MEMS 1.9 peut être très difficile à reconnecter après une interruption et recommande de couper le contact environ **15 secondes** pour permettre l’arrêt complet de l’ECU avant une nouvelle tentative.

**Conclusion : la phase « déconnexion/reconnexion » du modèle de test 1.9 n’est pas actuellement une validation fiable.**

Correction : procédure spécifique 1.9, avec fin de session si validée pour cette famille et/ou demande utilisateur de couper le contact ~15 s avant nouvelle initialisation 5 bauds.

### 18.8 MOYEN — actionneurs maintenus ON sans timeout/failsafe global garanti

Les tests automatiques ON/OFF d’une seconde sont plutôt bien protégés : le code tente la commande OFF même si l’acquittement ON a échoué.

Mais les commandes manuelles séparées `FuelPumpOn`, `PTCRelayOn`, `ACRelayOn`, etc. peuvent maintenir un actionneur actif sans timeout applicatif.

La fermeture/déconnexion arrête la communication, mais le code n’émet pas systématiquement un OFF spécifique avant fermeture.

Le bouton générique « All actuators off » s’appuie sur `F4`, qui est lui-même contextuel ; il ne peut donc pas être considéré comme un coupe-circuit universel sûr pour toutes les familles/modes.

Correction :

- timeout automatique par actionneur ;
- état actionneur suivi ;
- OFF explicite de la même famille/commande avant déconnexion quand c’est sûr ;
- pas de `F4` universel sans contexte.

### 18.9 MOYEN — commandes qui altèrent volontairement l’état ECU

Le programme contient volontairement :

- clear faults `0xCC` ;
- reset ECU `0xFA` selon compatibilité historique ;
- reset adjustments `0x0F` selon compatibilité historique ;
- réglages `79/7A`, `89/8A`, `91/92`, `93/94` ;
- actionneurs ;
- mouvement IAC.

Donc il est faux d’affirmer que « MEMS Manager ne peut jamais altérer l’ECU ».

La bonne exigence est : **aucune altération ne doit être possible par erreur, hors contexte ou sans action volontaire claire de l’utilisateur.**

Points actuels :

- Reset ECU : confirmation UI forte — positif ;
- Reset adjustments : confirmation UI — positif ;
- Clear faults : pas de confirmation équivalente trouvée — à corriger ;
- réglages ± : pas de garde famille/mode central — à corriger.

### 18.10 MOYEN — retours d’état trompeurs

Dans `memsinterface.cpp` :

- le signal `adjustmentsResetSuccess()` est commenté même lorsque `mems_reset_adjustments()` réussit ;
- `moveIACComplete()` est émis même si le mouvement a échoué ou si l’ECU n’est pas connecté ;
- plusieurs tests actionneurs émettent leur signal `...TestComplete()` après appel, même si `actuatorOnOffDelayTest()` a signalé une erreur.

Risque : l’UI peut afficher un état « terminé » qui n’est pas équivalent à « réussi ».

Correction : séparer explicitement succès / échec / terminé et n’utiliser le vert que sur succès prouvé.

### 18.11 MOYEN — sondage de tous les ports série

`SerialAdapterDetector::availableAdapters()` retourne tous les ports série détectés et classe leur type, mais ne filtre pas strictement uniquement des interfaces K-Line compatibles.

La connexion essaie d’abord l’initialisation ROSCO historique sur les candidats, puis le réveil MEMS 1.9 si nécessaire.

Risque : envoyer des octets de diagnostic à un périphérique série sans rapport avec l’ECU.

Correction minimale avant essai : port explicitement choisi ou filtrage fort des interfaces reconnues. La détection automatique complète viendra ensuite.

### 18.12 MOYEN — profils RAM MEMS 1.9 corrélés ≠ tous validés sur matériel

Le projet contient 164 profils MEMS 1.9 corrélés/identifiés par firmware, avec une barrière importante : un firmware inconnu est refusé avant entrée Mode 4.

C’est un bon garde-fou.

Mais le terme « validated profile » dans le code représente une validation de correspondance documentaire/disassembly ; il ne prouve pas qu’un test physique a été réalisé sur chacun des 164 ECU/firmwares.

Règle : conserver la porte d’identification exacte et ajouter un statut séparé :

- corrélé statiquement ;
- testé sur ECU réel ;
- version/ECU/date de validation.

### 18.13 FAIBLE — hygiène du dépôt

Le dépôt contient encore des marqueurs historiques, anciens rapports/status, vieux exécutables/zips et références x86 qui ne sont pas utilisés par le package x64 courant.

Ils ne sont pas un risque runtime si le package final reste contrôlé par l’inventaire PE, mais ils peuvent induire en erreur lors d’un audit futur ou d’une reprise de discussion.

Nettoyage recommandé ultérieurement, sans toucher à la référence historique utile.

### 18.14 Limite matérielle — le logiciel ne peut pas certifier l’électronique du câble

Un audit source ne peut pas prouver qu’un câble physique protège électriquement l’ECU.

Le réveil 5 bauds suppose une **interface K-Line/KKL avec transceiver adapté**, pas une connexion directe arbitraire d’un UART/TTL sur la K-Line.

Avant essai 1.9 : vérifier le type d’interface, les niveaux électriques, masse commune, alimentation et brochage. La réussite d’un handshake logiciel ne remplace pas cette vérification électrique.

### 18.15 Points positifs confirmés par l’audit

1. **D3 recodage n’est pas exposé dans l’onglet ECU/ROSCO.**
2. Le diagnostic automatique est passif : analyse/rapport, pas de commande d’écriture trouvée.
3. Le lecteur Injection RAM dédié refuse un firmware sans profil exact.
4. Le lecteur Mode 4 dédié utilise `DC` puis des offsets `0x00–0x7F`, plage documentée comme lecture ; il n’utilise pas les plages `0x80–0xBF` de modification calibration.
5. Le lecteur Mode 4 tente systématiquement un retour vers la session normale à la sortie ; si le retour échoue, il arrête le polling normal au lieu de continuer aveuglément.
6. Le test RAM AANMP002 spécialisé utilise une whitelist fermée de commandes et n’expose pas les commandes clear/reset/adaptation/programming/actionneur.
7. Les tests actionneurs temporisés tentent OFF même si ON n’a pas été acquitté.
8. Reset ECU et Reset Adjustments ont une confirmation utilisateur.
9. La détection MEMS 1.9 du test guidé vérifie qu’un protocole MEMS 1.9 a réellement été reconnu avant de poursuivre.
10. La navigation 14 onglets, l’architecture AMD64, l’ABI et le packaging sont contrôlés par CI et BUILD #26 est vert sur ces aspects.

### 18.16 Corrections obligatoires avant branchement complet

Ordre recommandé, **dans BUILD #26** :

1. créer un contexte protocole central `{famille ECU, mode diagnostic}` ;
2. appliquer une whitelist stricte par famille/mode au niveau `MEMSInterface`, pas seulement dans l’UI ;
3. en Mode 4, bloquer toutes les commandes générales et n’autoriser que le lecteur dédié read-only ;
4. rendre impossible par construction la séquence Mode 4 -> `D1`, `D3`, `F8` et toute autre écriture hors module explicitement autorisé ;
5. corriger le mapping injecteur 1.9/MPI et refuser l’actionneur si la famille n’est pas prouvée ;
6. corriger le parser 7D/80 pour les longueurs MEMS 1.9 réelles ;
7. ajouter W4 25–50 ms au réveil 5 bauds ;
8. revoir la fin de session/reconnexion 1.9 ;
9. ajouter timeout/failsafe aux commandes actionneur ON ;
10. ajouter confirmation à clear faults et garde mode/famille aux réglages ;
11. corriger les signaux de succès/échec ;
12. ne plus sonder tous les ports série sans filtre/choix ;
13. ajouter des tests CI de machine d’état qui prouvent que les séquences dangereuses sont refusées ;
14. refaire les 15 contrôles x64 existants ;
15. seulement ensuite effectuer le test matériel progressif de la section 12.

### 18.17 Nouveaux contrôles CI à ajouter

Le CI doit notamment prouver :

- `Mode4 + D1` => refus ;
- `Mode4 + D3` => refus ;
- `Mode4 + F8` => refus ;
- `Mode4 + actionneur quelconque` => refus ;
- `Normal + commande Mode4 read brute hors lecteur dédié` => refus ;
- firmware RAM inconnu => refus avant entrée Mode 4 ;
- échec restore normal => polling normal reste arrêté ;
- MEMS 1.9 W4 reste dans 25–50 ms ;
- trames 7D/80 de longueurs MEMS 1.9 attendues sont correctement consommées ;
- actionneur ON a toujours une sortie OFF/timeout ;
- perte de communication bloque immédiatement réglages/actionneurs ;
- reconnexion 1.9 suit la stratégie dédiée et ne boucle pas toutes les 3 s ;
- aucun port série non sélectionné/non validé ne reçoit une séquence ECU.

### 18.18 Verdict final de l’audit

**BUILD #26 — v1.0.26, commit `3c4102e...` : CI VERT, MAIS NO-GO POUR TEST ECU COMPLET.**

Le lecteur RAM/Injection dédié est statiquement conçu comme un lecteur et présente de bons garde-fous. Le problème principal se situe dans l’API/les commandes générales, où les mêmes octets peuvent représenter une lecture, un actionneur, un changement de mode ou une programmation selon le contexte.

**Tant que la famille et le mode ne sont pas verrouillés au niveau central, le programme ne peut pas être certifié comme incapable d’altérer involontairement l’ECU.**

Le test MEMS 1.9 n’est pas validé sans risque dans son état actuel, principalement à cause :

- des collisions de commandes cross-mode ;
- du mapping injecteur non spécifique ;
- du parser à tailles fixes ;
- du timing W4 non explicite ;
- de la reconnexion trop rapide/non spécifique.

**Décision : corriger ces points dans BUILD #26, refaire CI + nouveaux tests de sécurité, puis seulement brancher sur l’ECU.**

---

## PRINCIPE DIRECTEUR

**VERT CI ≠ SÛR POUR ECU. 32 bits figé comme référence ; BUILD #26 = v1.0.26 ; aucun #27 avant validation ; x64 natif propre ; commandes contextualisées famille + mode ; aucune écriture possible par accident ; Mode 4 dédié read-only ; MEMS 1.9 validé par étapes ; nouveaux contrôles sécurité CI ; test matériel seulement après levée du NO-GO ; IA locale ensuite.**
