# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.

Dernière mise à jour : **24 août 2026 — REMISE À PLAT APRÈS DÉSORDRE DES WORKFLOWS X64.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 officielle : **`MEMSX64`**.
- Branche de rapport : **`RAPPORT`**.
- Branche 32 bits historique : **`lab-expert-engine`**, à laisser intacte.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- **SEUL BUILD X64 ACTIF : BUILD #26 — v1.0.26.**
- Commit source x64 actuellement visé : **`3c4102eca34a2426970ee03e01830a6317b9db07`**.
- Workflow x64 unique sur `MEMSX64` : **`.github/workflows/memsx64.yml`**.
- Il ne doit plus exister plusieurs workflows x64 concurrents sur `MEMSX64`.
- **Aucun BUILD #27 ne doit être créé tant que BUILD #26 n’a pas donné un résultat complet et analysé.**

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
2. Le BUILD actif actuel est **#26 — v1.0.26**.
3. S’il échoue : identifier la première erreur réelle.
4. Classer l’erreur : infrastructure / build / code / package / test.
5. Corriger uniquement cette cause.
6. Relancer **le même BUILD #26**.
7. Ne pas créer #27 pour essayer une variante.
8. Ne pas créer #28/#29/etc. en parallèle.
9. Ne pas modifier le protocole pour corriger une erreur CI, Qt, SQLite, packaging ou navigation.
10. Ne pas modifier le 32 bits de référence.
11. Le workflow ne doit pas écrire dans `MEMSX64` pendant son exécution.
12. Le workflow doit compiler le commit exact qui l’a déclenché.
13. Les corrections de code peuvent produire de nouveaux commits Git tout en restant **BUILD #26 — v1.0.26** tant que le jalon #26 n’est pas validé.
14. `concurrency` doit empêcher deux exécutions de BUILD #26 de tourner simultanément.
15. Une exécution annulée par `cancel-in-progress` après une correction n’est pas un nouvel échec fonctionnel ; elle est remplacée par l’exécution du même BUILD sur le dernier commit.

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

Exports attendus :

- `mems_cleanup`
- `mems_clear_faults`
- `mems_connect`
- `mems_disconnect`
- `mems_get_lib_version`
- `mems_heartbeat`
- `mems_init`
- `mems_init_link`
- `mems_is_connected`
- `mems_lock`
- `mems_move_iac`
- `mems_openserial`
- `mems_read`
- `mems_read_iac_position`
- `mems_read_raw`
- `mems_read_serial`
- `mems_reset_ECU`
- `mems_reset_adjustments`
- `mems_send_command`
- `mems_test_actuator`
- `mems_unlock`
- `mems_write_serial`

Commandes historiques importantes :

- init : `CA 75 F4 D0` ;
- polling : `0x80` puis `0x7D` ;
- IAC : `0xFB` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : `0xFA` ;
- reset adjustments : `0x0F`.

Le x86 historique `prebuilt-librosco/librosco.dll` reste une référence ABI/protocole, mais ne doit pas être livré dans le package x64.

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

Un CI vert prouve une construction x64 cohérente, **pas encore le comportement réel sur ECU**.

---

## 9. ÉTAT TECHNIQUE RÉCENT AVANT BUILD #26

### 9.1 Qt / 7-Zip

Le premier BUILD #22 avait échoué avant compilation de MEMS Manager pendant l’installation Qt :

`py7zr.exceptions.Bad7zFile: Specified path is bad: 5.15.2/msvc2019_64/include`

Cette erreur provenait de la chaîne d’extraction Qt, pas du code MEMS Manager.

Correction propre retenue :

- Python 3.11 ;
- `aqtinstall 3.3.0` ;
- extraction Qt par **7-Zip externe supporté par aqtinstall** ;
- pas de downgrade Python ;
- pas de vieux `py7zr` épinglé ;
- pas de modification du programme pour contourner l’outil.

Cette correction a permis de passer :

- installation Qt ;
- validation navigation ;
- garde-fous protocole ;
- configuration x64 ;
- compilation application ;
- compilation `mems_manager_x64.dll` ;
- test ABI.

### 9.2 Self-test SQLite 14A

Le test sémantique renvoyait :

`FAIL search 'temperature air' category 'wiring': expected relationship not found`

alors que les résultats retournés étaient bien pertinents :

- MEMS 1.9 — Signal IAT / température air admission ;
- MEMS 1.6 — Sonde température air ;
- MEMS 1.2 — Sonde température air d’admission.

La recherche fonctionnait ; le test était fragile parce que l’attente utilisait le littéral accentué `température air` dans un contexte MSVC alors que le moteur normalise déjà les accents.

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

**C’est le seul jalon x64 actif.**

Source actuelle :

**`3c4102eca34a2426970ee03e01830a6317b9db07`**

Workflow :

**`.github/workflows/memsx64.yml`**

Une exécution précédente de #26 a été annulée avec :

`START MEMS search semantic self-test`

puis :

`Error: The operation was canceled.`

Cette annulation n’était pas un nouvel échec du self-test MEMS. Elle a été provoquée par `cancel-in-progress: true` après le push du correctif de numérotation `3c4102e...`.

Le principe est volontaire : lorsqu’une correction du même BUILD #26 arrive, l’ancienne exécution devient obsolète et est annulée afin qu’une seule exécution de #26 reste active.

### Règle immédiate

**Ne plus toucher au code pendant qu’une exécution #26 courante est en cours, sauf si elle a terminé avec une première erreur réelle identifiée.**

Aucun #27.

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

- 705 lignes ;
- ~5 min 40 s ;
- RPM 0 → 1657 puis ~1180–1200 ;
- MAP ~100 → ~30–35 kPa ;
- batterie ~12,2 V → ~11,0 V lancement puis ~13,8 V ;
- dwell ~6,274 ms arrêté, ~7,170 ms lancement, ~3,2–3,4 ms moteur tournant.

Cette référence 32 bits sert uniquement à comparer le comportement x64 réel.

---

## 12. TEST MATÉRIEL X64 PRIORITAIRE QUAND #26 EST VERT

Sur AANMP002 / MNE101150 :

1. lancement et stabilité ;
2. port COM visible/sélectionnable ;
3. connexion ;
4. identification AANMP002 / MNE101150 ;
5. polling 7D/80 stable ;
6. comparaison valeurs principales avec BUILD #14 ;
7. acquisition/logging ;
8. Injection RAM Mode 4 ;
9. lecture `0x03C8`, `0x026E`, `0x0280` ;
10. retour Mode 4 → 7D/80 ;
11. déconnexion ;
12. reconnexion ;
13. navigation sans perte de communication.

Si ce test est valide, BUILD #26 devient la nouvelle base x64 principale.

---

## 13. RÈGLES FONCTIONNELLES À NE PAS PERDRE

- `memsinterface.h` doit conserver `void onProtocolCommandRequested(quint8 command);`.
- Les familles MEMS 1.2 / 1.3 / 1.6 / 1.9 restent distinctes.
- Ne jamais confondre polling normal `0x7D/0x80`, RAM Mode 4, calibrations/cartes et données externes.
- Injection : ne jamais calculer le temps d’injection depuis le polling normal 7D/80.
- Adresses RAM Injection importantes : `0x03C8`, `0x026E`, `0x0280`.
- Injection reste entre Aperçu et Réglages.
- Dwell/temps bobine reste dans l’onglet Injection.
- Éviter de mélanger inutilement plusieurs modes de lecture dans une même page.
- Conserver le style dark et responsive.
- Ne pas engager de refonte graphique non demandée.

---

## 14. DÉTECTION AUTOMATIQUE CÂBLE / COM — PLUS TARD

Après validation de la connexion x64 de base :

- détecter automatiquement l’interface ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- si plusieurs interfaces : afficher COM + type ;
- au clic Connexion : vérifier interface/protocole ;
- messages clairs : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Cette logique doit être générale aux MEMS supportés, pas réservée au MEMS 1.9.

Ne pas l’introduire avant la première validation matérielle du cœur x64.

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

Objectif : **la meilleure IA diagnostique possible, mais seulement sur une base ECU x64 fiable.**

---

## 17. PROCHAINE ACTION EXACTE

1. Lire ce rapport avant toute modification.
2. Ne pas toucher à `lab-expert-engine` / BUILD #14.
3. Travailler uniquement sur `MEMSX64` pour la x64.
4. **BUILD #26 — v1.0.26 est le seul build actif.**
5. Ne pas créer #27 tant que #26 n’est pas terminé et analysé.
6. Vérifier le résultat de l’exécution courante #26 sur le commit `3c4102eca34a2426970ee03e01830a6317b9db07`.
7. Si échec : prendre la première erreur réelle et corriger uniquement cette cause.
8. Relancer le même BUILD #26 après correction.
9. Ne jamais revenir à #16/#22/#23/#24/#25 comme cible active.
10. Conserver un seul workflow x64 : `.github/workflows/memsx64.yml`.
11. Ne pas recréer de fichiers de statut auto-commités par le workflow.
12. Ne pas utiliser `GITHUB_RUN_NUMBER` comme version du programme.
13. Ne pas toucher au protocole pour corriger un problème CI/build/package/navigation.
14. Exécuter les 15 contrôles x64.
15. Quand #26 est vert et l’artefact disponible, faire le test ECU réel AANMP002/MNE101150.
16. Si validé matériellement, faire de #26 la nouvelle base x64.
17. Réintégrer ensuite l’IA locale x64 et reprendre l’amélioration IA.

---

## PRINCIPE DIRECTEUR

**32 bits figé comme référence ; avenir en x64 natif et propre ; un seul build/version actif ; BUILD #26 = v1.0.26 ; aucune multiplication de numéros ; aucun workflow parallèle ; aucune architecture hybride ; aucune rustine ; programme complet ; ECU d’abord ; 15 contrôles stricts ; test matériel ; IA locale ensuite.**
