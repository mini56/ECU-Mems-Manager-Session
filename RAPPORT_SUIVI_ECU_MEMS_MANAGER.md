# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.

Dernière mise à jour : **25 août 2026 — BUILD #26 : CRASH IA X64 CORRIGÉ ET VALIDÉ SUR PC RÉEL ; CACHE IA R20 RECONSTRUIT ; ANOMALIE DE NAVIGATION “APERÇU DISPARAÎT” EN COURS DE CORRECTION.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 officielle : **`MEMSX64`**.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- **BUILD x64 courant : BUILD #26 — v1.0.26.**
- **Aucun BUILD #27 ne doit être créé tant que #26 n’est pas validé.**
- Le numéro BUILD = version logiciel ; ne jamais incrémenter pour un simple essai/correctif.
- Workflow x64 unique : `.github/workflows/memsx64.yml`.
- Le workflow compile le commit exact qui le déclenche.
- `concurrency: memsx64-build-26`, `cancel-in-progress: true`.
- `GITHUB_RUN_NUMBER` ne décide plus de la version ; `MEMS_BUILD_NUMBER=26` est explicite.

### État x64 actuel

**Dernier commit `MEMSX64` poussé :**

`12fef48c68807bc59d2f45f9cd8d86d2a42856ca`

Message :

`Stop visual completion from rebuilding BUILD #26 navigation`

Ce commit corrige uniquement la concurrence entre la couche visuelle et la navigation 14 onglets. Il ne touche ni au protocole ECU, ni à l’IA, ni au 32 bits.

Le run GitHub associé est :

`32816285887`

Au moment de cette mise à jour, ce run est **en cours**. Ne pas le déclarer vert avant conclusion `success`.

### Dernier état x64 réellement validé sur PC

Commit :

`66fe69db556d83df56aa6ddd968cb48129cbcf95`

Message :

`Package MSVC x64 runtime in BUILD #26`

Run GitHub :

`32814736178`

Résultat : **SUCCESS / BUILD #26 VERT**.

Artefact :

`ECU-MEMS-Manager-x64-BUILD-26-v1.0.26`

SHA256 artefact :

`b86f5cf0660f0099bae2a73d5c6d877587bdc5149ad56b1df7153d0d929b1c72`

Validation utilisateur réelle le 25 août 2026 :

- application démarre ;
- onglet **IA MEMS s’ouvre sans crash** ;
- base de connaissances annoncée prête en lecture seule ;
- absence du moteur local signalée proprement : `Moteur llama.cpp local absent du dossier IA` ;
- un nouveau `ia_mems_reference_r20.sqlite` est créé sur le PC ;
- l’ancien fichier renommé `.OLD` n’est plus nécessaire et peut être supprimé ;
- **le crash IA est donc considéré comme corrigé et validé sur PC réel pour ce package**.

Anomalie encore ouverte et séparée :

- **l’entrée “Aperçu” peut encore disparaître temporairement de la barre latérale** ;
- les captures du 25 août montrent `Aperçu` présent sur IA MEMS / Mode interactif, mais absent sur l’écran Test ECU 1.9 ;
- cette anomalie est traitée séparément du crash IA.

---

## 2. RÈGLE BUILD / VERSION ET MÉTHODE

Le numéro de BUILD est le numéro de version :

- BUILD #14 = v1.0.14
- BUILD #26 = v1.0.26
- BUILD #100 = v1.1.0
- BUILD #588 = v1.5.88
- BUILD #662 = v1.6.62

Formule : `1.(build / 100).(build % 100)`.

Règles obligatoires :

1. un seul BUILD actif à la fois ;
2. BUILD #26 reste v1.0.26 pendant toutes ses corrections ;
3. un build GitHub rouge se corrige sans changer de numéro ;
4. un rerun GitHub n’est pas un nouveau BUILD ;
5. ne jamais utiliser `GITHUB_RUN_NUMBER` comme version logicielle ;
6. ne jamais modifier la branche 32 bits de référence pour faire avancer la x64 ;
7. code x64 uniquement sur `MEMSX64` ;
8. rapport uniquement sur `RAPPORT` ;
9. ne pas empiler de workflows temporaires ;
10. ne pas pousser de “patch de contournement” qui neutralise une fonction au lieu de corriger la cause ;
11. identifier une cause précise avant modification ;
12. ne pas revenir sur une piste déjà éliminée sans nouvelle preuve.

Commits de discipline importants :

- `522fae53cb1573a956ce50941d5a185a4d245e66` — nettoyage CI x64 vers un seul workflow ;
- `3c4102eca34a2426970ee03e01830a6317b9db07` — verrouillage explicite BUILD/version, suppression du fallback `GITHUB_RUN_NUMBER`.

Les anciens #23/#24/#25 sont historiques/parasites et ne doivent pas redevenir des bases de travail.

---

## 3. INCIDENT IA X64 BUILD #26 — DIAGNOSTIC COMPLET

### 3.1 Symptôme initial

Sur le PC utilisateur :

- clic sur `IA MEMS` ;
- fermeture immédiate du programme ;
- crash avant création d’un nouveau cache IA ;
- aucun nouveau `.sqlite`, `.tmp` ou `.lock` dans le dossier `ia-mems`.

Un premier artefact testé avait le SHA256 :

`0e78c77bf9b7171412e7986e5a50542d261aa41f9203c9758610e86d82a0e636`

Ce package correspondait bien au commit `eb762546584567b705cd7beb88c13b5da18008ed` ; il ne s’agissait donc pas d’un ancien ZIP.

### 3.2 Correction `iamemsqualitypatch.cpp` insuffisante

Commit :

`eb762546584567b705cd7beb88c13b5da18008ed`

Message :

`Fix BUILD #26 IA transcript show/resize reentrancy`

Le CI était vert, mais le test utilisateur réel a confirmé :

- crash IA toujours présent ;
- menu toujours imparfait.

Conclusion : cette correction n’était pas la cause racine du crash réel.

### 3.3 Pistes définitivement éliminées

Ne pas recommencer ces recherches sans nouvelle preuve.

#### Bloc 1600

`database/reference/research_enrichment_1600.qz64` existe bien dans le package de référence, mais le cache IA runtime l’exclut explicitement.

Le bloc 1600 n’est donc **pas** la cause du crash au clic IA.

#### ExpertRuntimeDatabase x64

Self-test natif x64 déjà validé :

Run : `32721284999`

Job : `97413054422`

Résultats :

- `IA_X64_RUNTIME_DB=STARTED`
- `first_exit_code=0`
- `first_elapsed_seconds=1.89`
- `FIRST_BUILD=PASS`
- `second_exit_code=0`
- `second_elapsed_seconds=0.04`
- `REUSE_CACHE=PASS`

Le self-test ouvre également le cache via `ExpertKnowledgeReader` et fait tourner `ExpertEngine::analyze()`.

Conclusion : le moteur DB/cache x64 fonctionne dans un environnement Windows propre.

#### Cache utilisateur ancien

Le cache utilisateur attendu est :

`%LOCALAPPDATA%\ECU Mems Manager\ECU Mems Manager\ia-mems\ia_mems_reference_r20.sqlite`

Test effectué :

- ancien cache renommé en `ia_mems_reference_r20.sqlite.OLD` ;
- relance ;
- crash toujours présent ;
- aucun nouveau `.sqlite`, `.tmp` ou `.lock` créé à ce moment.

Conclusion : l’ancien cache corrompu n’était pas la cause du crash.

Note de robustesse : `ExpertRuntimeDatabase::buildOrOpen()` accepte actuellement un cache existant sans contrôle complet d’intégrité/schéma. Ce défaut reste à améliorer ultérieurement, mais il n’expliquait pas ce crash.

#### Dossier IA absent

Le premier package x64 volontairement validable sur ECU n’embarque pas encore :

- `ai/llama-server.exe` ;
- modèle GGUF.

C’est volontaire.

`LocalAiClient` doit gérer cette absence sans crash et afficher un état `MissingRuntime` / moteur absent.

L’absence du dossier `ai` n’était pas la cause.

#### UTF-8 / wrapper / SQLite générique

Les diagnostics historiques n’ont pas démontré que :

- le flag UTF-8 ;
- `iamemstab_clean.cpp` ;
- un simple conflit QSQLITE de connexion ;

étaient la cause racine du crash réel.

### 3.4 Donnée décisive : journal Windows

Le journal d’événements Windows a montré plusieurs crashs strictement identiques :

- module fautif : **`MSVCP140.dll`** ;
- exception : **`0xc0000005`** ;
- signification : violation d’accès mémoire ;
- offset : **`0x12EB0`** ;
- même EXE BUILD #26 à chaque fois.

Le package x64 testé n’embarquait pas :

- `MSVCP140.dll` ;
- `VCRUNTIME140.dll` ;
- `VCRUNTIME140_1.dll`.

L’application chargeait donc le runtime Visual C++ de `C:\Windows\System32`.

Le workflow utilisait pourtant `windeployqt --compiler-runtime`, mais les DLL MSVC nécessaires n’étaient pas réellement présentes dans l’artefact.

### 3.5 Correction réelle retenue

Commit :

`66fe69db556d83df56aa6ddd968cb48129cbcf95`

Message :

`Package MSVC x64 runtime in BUILD #26`

Objectif :

- embarquer explicitement le runtime MSVC x64 attendu par le binaire ;
- ne plus dépendre uniquement du runtime installé dans `System32` ;
- contrôler le package x64 final.

Le ZIP contient désormais plusieurs DLL `MSVCP*.dll`, ce qui est normal pour le runtime Visual C++ récent.

### 3.6 Résultat CI

Run `32814736178` : **SUCCESS**.

Tous les contrôles du workflow ont passé :

- installation Qt ;
- validation navigation source ;
- garde-fous protocole historiques ;
- configuration x64 ;
- compilation application + DLL protocole ;
- ABI ;
- SQLite sémantique ;
- assemblage package ;
- inventaire PE x64 ;
- ressources indispensables ;
- smoke launch ;
- upload artefact.

### 3.7 Résultat réel utilisateur

Avec l’artefact SHA256 :

`b86f5cf0660f0099bae2a73d5c6d877587bdc5149ad56b1df7153d0d929b1c72`

le test réel montre :

- **plus de crash quand on clique sur IA MEMS** ;
- la page IA s’affiche complètement ;
- la base est annoncée prête ;
- le moteur local absent est signalé proprement ;
- `ia_mems_reference_r20.sqlite` est recréé sur le PC à côté de l’ancien `.OLD` ;
- le `.OLD` peut être supprimé.

**Verdict IA x64 BUILD #26 : crash au clic corrigé et validé sur le PC réel.**

La corrélation la plus forte est le packaging du runtime MSVC x64. Ne pas réouvrir les anciennes pistes DB/1600/absence dossier IA sans nouvelle donnée.

---

## 4. NAVIGATION 14 ONGLETS — ÉTAT ACTUEL

Ordre officiel obligatoire :

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

Architecture voulue :

**identité stable → ordre → clé de traduction → icône**

Ne jamais :

- identifier un onglet par son texte traduit ;
- réordonner périodiquement avec des timers ;
- reconstruire la navigation depuis une deuxième couche concurrente ;
- traiter IA MEMS comme une exception hors de la table 14 onglets.

### 4.1 Correction précédente

Commit :

`a005501c94d7e5c949926a7fa527690804994154`

Message :

`Fix BUILD #26 sidebar navigation mapping`

Modifications importantes :

- abandon du mapping par pointeurs stockés dans les items de `QListWidget` ;
- mapping direct ligne sidebar ↔ index fixe du `QTabWidget` ;
- clé MEMS1.9 corrigée vers `19000` ;
- fallback `Test ECU 1.9` ;
- ordre officiel 0..13 figé.

Cette correction a permis à la navigation d’être utilisable, mais une anomalie subsiste : `Aperçu` peut disparaître de la barre latérale selon la page courante.

### 4.2 Cause distincte trouvée pour la disparition d’Aperçu

`navigationorderpatch.cpp` construit la navigation officielle 14/14 et installe les connexions déterministes.

Mais `visualcompletion.cpp` contenait encore une deuxième fonction :

`syncNavigation(QMainWindow *w)`

Cette fonction :

- récupérait `Tab_main` ;
- vidait `uiRebuildNav` avec `nav->clear()` ;
- recréait ensuite les lignes depuis `tabs->tabText(i)` ;
- était rappelée dans `apply()` ;
- `apply()` est déclenché par plusieurs timers `120/450/900 ms` et sur `Resize`.

Donc deux systèmes différents possédaient encore la même barre latérale :

- `navigationorderpatch.cpp` : source officielle 14 onglets ;
- `visualcompletion.cpp` : reconstruction visuelle secondaire et répétée.

C’est exactement le type de concurrence qui devait être supprimé.

### 4.3 Correction poussée

Commit :

`12fef48c68807bc59d2f45f9cd8d86d2a42856ca`

Message :

`Stop visual completion from rebuilding BUILD #26 navigation`

Modification :

- suppression de la fonction `syncNavigation()` dans `visualcompletion.cpp` ;
- suppression de son appel depuis `apply()` ;
- conservation des fonctions purement visuelles/responsive de `visualcompletion.cpp` ;
- `navigationorderpatch.cpp` devient le seul propriétaire de la barre latérale 14 onglets.

Aucun protocole, aucune base IA, aucun runtime IA, aucune branche 32 bits modifiés.

Run GitHub associé :

`32816285887`

État au moment de rédaction : **en cours**.

À faire après CI vert :

1. télécharger l’artefact du même BUILD #26 ;
2. ouvrir successivement Aperçu, IA MEMS, Mode interactif, Test ECU 1.9 ;
3. vérifier que `Aperçu` reste toujours visible ;
4. vérifier que les 14 entrées restent présentes dans le même ordre ;
5. vérifier que le clic sur chaque entrée ouvre la bonne page ;
6. vérifier que IA MEMS ne recommence pas à planter.

---

## 5. OBJECTIF X64 ET PACKAGE

La x64 doit devenir la future base principale propre et durable.

Contraintes :

- application x64 native ;
- Qt 5.15.2 MSVC x64 ;
- plugins Qt x64 ;
- `mems_manager_x64.dll` native x64 ;
- aucun mélange x86/x64 ;
- aucun `librosco.dll` x86 dans le package ;
- tous les PE livrés en AMD64 ;
- runtime MSVC x64 désormais emballé avec l’application ;
- le 32 bits stable reste intact.

Premier package x64 : application complète 14 onglets, base de données et moteur expert présents.

Seuls les éléments IA lourds sont volontairement absents pendant la validation du cœur ECU x64 :

- `llama-server.exe` ;
- Qwen GGUF.

Leur absence doit toujours produire un état clair, jamais un crash.

---

## 6. 15 CONTRÔLES X64 OBLIGATOIRES

Le workflow doit vérifier :

1. application x64 compile ;
2. `mems_manager_x64.dll` compile ;
3. EXE PE32+ AMD64 ;
4. DLL protocole PE32+ AMD64 ;
5. Qt/plugins x64 ;
6. QSQLITE et Qt SerialPort x64 ;
7. EXE importe `mems_manager_x64.dll` ;
8. EXE n’importe pas `librosco.dll` ;
9. ancienne DLL x86 absente ;
10. exactement 22 exports historiques ;
11. ABI `frame80=28`, `frame7d=32`, `mems_data=60` ;
12. garde-fous protocole historiques ;
13. smoke launch du package ;
14. SQLite + ressources indispensables ;
15. tous les PE finaux AMD64.

Le run `32814736178` du commit `66fe69db...` a passé ces contrôles.

À compléter plus tard avec des tests de sécurité de machine d’état famille/mode, décrits ci-dessous.

---

## 7. DLL PROTOCOLE X64

Nom : `mems_manager_x64.dll`.

Sources : `librosco-x64/`.

Contraintes ABI :

- `frame80 = 28` octets ;
- `frame7d = 32` octets ;
- `mems_data = 60` octets ;
- 22 exports historiques exacts.

Commandes historiques principales :

- init : `CA 75 F4 D0` ;
- polling : `0x80` puis `0x7D` ;
- IAC : `0xFB` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : `0xFA` ;
- reset adjustments : `0x0F`.

Règle fondamentale : **un octet n’a pas une signification universelle ; la famille ECU et le mode diagnostic doivent être connus avant d’autoriser une commande.**

---

## 8. AUDIT PRÉ-ECU BUILD #26 — VERDICT TOUJOURS APPLICABLE

Audit de référence réalisé sur :

`3c4102eca34a2426970ee03e01830a6317b9db07`

Le CI était vert mais le verdict était :

**NO-GO POUR TEST ECU COMPLET tant que les bloqueurs sécurité famille/mode ne sont pas corrigés.**

Les corrections IA/navigation effectuées depuis ne modifient pas ce verdict protocole.

### 8.1 CRITIQUE — collisions de commandes selon le mode

Exemples :

- `D1` : lecture/identification en normal ; écriture banques RAM calibration vers ROM en Mode 4 ;
- `D3` : recodage / écriture calibration selon mode ;
- `F7` : injecteur SPi / stream calibration selon mode ;
- `F8` : bobine / écriture calibration complète selon mode ;
- `F4` : heartbeat/all-actuators-off ou changement de mode selon contexte.

Correction obligatoire : contexte central `{famille ECU, mode diagnostic}` + whitelist stricte au niveau `MEMSInterface`.

En Mode 4, l’interface générale doit bloquer les commandes génériques ; seul le lecteur dédié read-only doit être autorisé.

### 8.2 ÉLEVÉ — injecteur MEMS1.9/MPI

`rosco.h` distingue :

- `MEMS_TestInjectors = 0xF7` ;
- `MEMS_TestInjectorsMPi = 0xEF`.

Le code actuel envoie historiquement `0xF7` dans un chemin générique.

Ne pas tester l’injecteur MEMS1.9 tant qu’une table famille/profile → commande n’est pas prouvée et que le défaut n’est pas corrigé.

### 8.3 ÉLEVÉ — tailles fixes 7D/80 non prouvées universelles pour 1.9

Le parser historique lit :

- `0x80` sur 28 octets ;
- `0x7D` sur 32 octets.

La documentation MEMS indique que la longueur peut varier selon génération/profil.

À corriger avant validation lecture 1.9 : parser la longueur réelle.

### 8.4 ÉLEVÉ — timing W4 5 bauds

L’initialisation MEMS 1.9 5 bauds doit respecter W4 25–50 ms avant l’inversion du second key byte.

Ce délai doit être explicitement garanti et testé.

### 8.5 ÉLEVÉ — reconnexion MEMS 1.9

La fermeture COM + reconnexion 450 ms/3 s n’est pas suffisante pour considérer la reconnexion 1.9 fiable.

Prévoir une stratégie dédiée, avec coupure contact ~15 s si nécessaire après interruption de session.

### 8.6 MOYEN — actionneurs maintenus ON

Ajouter :

- timeout automatique ;
- suivi de l’état ;
- OFF spécifique quand sûr ;
- pas de `F4` universel sans contexte.

### 8.7 MOYEN — commandes qui altèrent l’ECU

Le programme contient volontairement des commandes d’altération : clear faults, reset, adjustments, réglages, actionneurs, IAC.

Exigence : aucune altération ne doit pouvoir se produire par erreur, hors contexte ou sans action utilisateur claire.

Clear faults doit recevoir une confirmation équivalente aux resets.

### 8.8 MOYEN — retours d’état

Séparer clairement :

- succès ;
- échec ;
- terminé.

Ne pas afficher un succès vert sur une simple fin d’appel.

### 8.9 MOYEN — détection ports série

Ne pas envoyer de séquences MEMS à tous les ports série arbitraires.

Avant essai 1.9 : port explicitement sélectionné ou interface reconnue/validée.

### 8.10 MOYEN — profils RAM 1.9

Les 164 profils corrélés ne sont pas tous validés physiquement.

Conserver des statuts distincts :

- corrélé statiquement ;
- testé ECU réel ;
- version/ECU/date.

### 8.11 Limite matérielle

Le logiciel ne peut pas certifier l’électronique du câble.

MEMS1.9 nécessite une interface K-Line/KKL avec transceiver adapté ; ne jamais connecter un UART/TTL direct arbitraire à la K-Line.

---

## 9. CORRECTIONS OBLIGATOIRES AVANT TEST ECU COMPLET

Toujours dans BUILD #26 :

1. contexte protocole central `{famille, mode}` ;
2. whitelist famille/mode dans `MEMSInterface` ;
3. Mode 4 bloque commandes générales ;
4. rendre impossibles Mode4 + `D1/D3/F8` et autres écritures ;
5. mapping injecteur 1.9/MPI correct ;
6. parser 7D/80 longueur réelle 1.9 ;
7. W4 25–50 ms ;
8. session/reconnexion 1.9 dédiée ;
9. timeout/failsafe actionneurs ;
10. confirmation clear faults + guards réglages ;
11. vrais signaux succès/échec ;
12. ne plus sonder tous les ports ;
13. tests CI machine d’état sécurité ;
14. refaire les 15 contrôles x64 ;
15. seulement ensuite test matériel progressif.

Nouveaux tests CI à ajouter :

- Mode4 + D1 => refus ;
- Mode4 + D3 => refus ;
- Mode4 + F8 => refus ;
- Mode4 + actionneur => refus ;
- commande Mode4 brute hors lecteur dédié => refus ;
- firmware RAM inconnu => refus avant Mode4 ;
- échec restore normal => polling normal arrêté ;
- W4 1.9 dans 25–50 ms ;
- trames 1.9 de longueurs attendues consommées correctement ;
- actionneur ON possède toujours OFF/timeout ;
- perte communication bloque réglages/actionneurs ;
- reconnexion 1.9 suit stratégie dédiée ;
- aucun port non sélectionné/non validé ne reçoit de séquence ECU.

---

## 10. RÉFÉRENCE MATÉRIELLE 32 BITS À CONSERVER

BUILD #14 — v1.0.14, retesté le 24 août 2026 sur vrai ECU :

- COM3 ;
- firmware AANMP002 ;
- ID AANMP002 / MNE101150 ;
- communication verte ;
- polling 7D/80 fonctionnel ;
- navigation stable ;
- IA stable connecté/déconnecté ;
- IA locale prête avec llama-server x64 séparé ;
- Qwen3-0.6B-Q8_0 ;
- HTTP local port 18089.

Injection RAM Mode 4 :

- transition 7D/80 → Mode 4 fonctionnelle ;
- retour Mode 4 → polling normal fonctionnel ;
- injection exemple ≈ 2,47 ms ;
- `0x03C8 = 1233` ;
- `0x026E = 0` ;
- `0x0280 = 0`.

Trace de référence : `2026-08-24_18.14.txt`.

Ne pas modifier cette branche pour résoudre un problème x64.

---

## 11. INJECTION / RAM — RÈGLES À NE PAS PERDRE

- Ne pas calculer le temps d’injection depuis le polling normal `0x7D/0x80`.
- Adresses RAM importantes : `0x03C8`, `0x026E`, `0x0280`.
- Le lecteur Mode 4 dédié doit rester read-only.
- Il utilise `DC` puis des offsets `0x00–0x7F`.
- Il ne doit pas utiliser les plages de modification calibration `0x80–0xBF`.
- Firmware sans profil exact => refus avant Mode4.
- Échec du retour au mode normal => polling normal reste arrêté.
- Injection reste entre Aperçu et Réglages.
- Dwell/temps bobine reste dans l’onglet Injection.
- Les sous-vues d’Injection doivent partager le même mode de lecture pour ne pas multiplier le polling inutilement.

---

## 12. BASE MEMS / BLOC 1600

Lot brut :

`database/reference/research_enrichment_1600.qz64`

Environ 33,3 Mo compressés, avec plusieurs millions de cellules de corrélation.

Ne pas charger automatiquement l’ensemble sans mesurer RAM/temps/utilité.

Point définitivement établi pour le crash IA :

**le runtime ExpertRuntimeDatabase exclut `research_enrichment_1600.qz64` de son import cache ; le bloc 1600 n’est pas la cause du crash au clic IA.**

Base principale modifiable :

`<appdir>/database/ecu_mems_manager.sqlite`

Cache IA runtime utilisateur :

`%LOCALAPPDATA%\ECU Mems Manager\ECU Mems Manager\ia-mems\ia_mems_reference_r20.sqlite`

Le 25 août 2026, ce cache a été recréé avec succès sur le PC réel après correction du runtime MSVC.

---

## 13. DÉTECTION AUTOMATIQUE CÂBLE / COM — OBJECTIF FUTUR

La détection doit être générale, pas uniquement MEMS1.9 :

- détecter automatiquement l’interface et le COM ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- si plusieurs interfaces : afficher COM + type ;
- au clic Connecter : vérifier l’interface/protocole ;
- messages clairs : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Avant test MEMS1.9, minimum obligatoire : ne pas envoyer de séquences ECU sur un port série arbitraire.

---

## 14. IA LOCALE APRÈS VALIDATION DU CŒUR X64

Runtime connu :

- llama.cpp Windows x64 CPU ;
- `llama-server.exe` x64 ;
- Qwen3-0.6B-Q8_0 GGUF ;
- HTTP local `127.0.0.1:18089`.

Le premier package x64 garde volontairement ce runtime lourd absent pour isoler le cœur ECU.

Maintenant que l’onglet IA et le moteur expert sont stables sans runtime local, la réintégration de llama/Qwen devra se faire **seulement après validation du cœur ECU x64 et levée du NO-GO protocole**.

À ce moment :

1. réintégrer llama-server x64 ;
2. réintégrer le même Qwen pour comparaison ;
3. vérifier RAM/CPU/démarrage/stabilité ;
4. reprendre ensuite les améliorations conversationnelles/diagnostiques.

---

## 15. TEST MATÉRIEL X64 — APRÈS LEVÉE DU NO-GO

### Phase A — lecture seule normale

- lancer et vérifier stabilité ;
- choisir explicitement le bon port/interface K-Line ;
- connecter ;
- identifier ECU ;
- polling normal uniquement ;
- aucun actionneur/reset/réglage/changement de mode manuel.

### Phase B — comparaison BUILD #14

Comparer :

- AANMP002 / MNE101150 ;
- RPM ;
- MAP ;
- batterie ;
- températures ;
- acquisition/logging ;
- déconnexion.

### Phase C — RAM Injection Mode 4

Seulement avec lecteur dédié + profil exact :

- entrée Mode4 ;
- lecture `DC + 0x00–0x7F` ;
- lecture `0x03C8`, `0x026E`, `0x0280` ;
- aucune commande générale/actionneur ;
- retour normal vérifié ;
- polling seulement après confirmation.

### Phase D — MEMS1.9

Seulement après corrections 1.9 :

- K-Line/KKL adaptée ;
- W4 conforme ;
- identification read-only ;
- parser longueur réelle ;
- aucune écriture/actionneur ;
- reconnexion dédiée.

### Phase E — actionneurs

Uniquement après validation famille + mode + commande + timeouts/failsafe.

---

## 16. AUTRES POINTS HISTORIQUES À CONSERVER

- `memsinterface.h` doit conserver `void onProtocolCommandRequested(quint8 command);`.
- Qt x64 : 5.15.2 MSVC 2019 x64.
- Installation Qt CI stabilisée avec Python 3.11 + `aqtinstall 3.3.0` + 7-Zip externe.
- Ancien problème `py7zr Bad7zFile` corrigé sans modifier l’application.
- Self-test SQLite `temperature air` corrigé au commit `ce7560d8316fc6e07aea01f400c41286144cd45a` sans modifier la base réelle.
- Style UI : dark et responsive ; ne pas refondre graphiquement sans demande explicite.
- Le bouton/onglet Injection doit rester dans sa position officielle.
- La traduction ne doit jamais modifier l’identité ou l’ordre des onglets.

---

## 17. PROCHAINE ACTION EXACTE

1. Attendre la fin du run `32816285887` du commit `12fef48c...`.
2. Si CI vert : télécharger l’artefact du **même BUILD #26**.
3. Test utilisateur navigation : vérifier les 14 entrées et particulièrement `Aperçu` après passage par `Test ECU 1.9`, `Mode interactif`, `IA MEMS` et d’autres pages.
4. Vérifier que le clic sur chaque entrée ouvre la bonne page.
5. Vérifier que IA MEMS reste stable et que le crash ne revient pas.
6. Si navigation validée : inscrire le résultat dans ce rapport.
7. Ensuite seulement reprendre les bloqueurs de sécurité protocole de la section 9, toujours dans BUILD #26.
8. Ne pas créer BUILD #27.
9. Ne pas toucher au 32 bits de référence.

---

## PRINCIPE DIRECTEUR

**BUILD #26 = v1.0.26 ; aucun #27 avant validation. IA x64 : crash MSVCP140 corrigé par packaging runtime MSVC et validé sur PC réel. Cache `ia_mems_reference_r20.sqlite` recréé avec succès. Navigation : une seule couche doit posséder la barre 14 onglets ; `visualcompletion.cpp` ne doit plus la reconstruire. CI vert ne signifie pas sécurité ECU : le NO-GO protocole/famille/mode reste applicable jusqu’aux corrections et tests dédiés. 32 bits figé comme référence.**