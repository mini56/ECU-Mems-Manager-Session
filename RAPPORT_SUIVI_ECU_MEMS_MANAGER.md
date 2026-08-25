# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.
>
> **RÈGLE DE SUIVI IMMÉDIAT : AVANT CHAQUE NOUVELLE ÉTAPE, inscrire dans ce rapport l’étape exacte qui va être exécutée et son objectif. Dès que cette étape produit un résultat, inscrire immédiatement ce résultat dans le rapport AVANT de commencer l’étape suivante. Ne jamais attendre la fin d’un lot, d’un build ou d’une discussion.**

## JOURNAL IMMÉDIAT — 25 août 2026

### Étape : recherche externe sur la bonne architecture d’intégration d’une IA locale dans une application desktop Qt

- **Étape inscrite / objectif :** ne plus chercher une succession de petits correctifs locaux au hasard ; vérifier sur les documentations et applications existantes comment une IA locale basée sur `llama.cpp` doit être intégrée proprement à une application desktop, puis comparer cette architecture à ECU MEMS Manager.
- **Sources externes consultées :** documentation officielle `llama.cpp`/`llama-server`, documentation Qt 5.15 `QProcess` et `QNetworkAccessManager`, documentation Qt sur le threading SQL, documentation Jan Local API Server, LM Studio headless/local service, Microsoft Windows process/memory isolation et exemples RAG locaux.
- **Résultat n°1 :** `llama.cpp` fournit officiellement `llama-server` comme serveur HTTP local séparé, avec API OpenAI-compatible et endpoint `/health`. Pendant le chargement du modèle `/health` répond 503 ; une fois le modèle prêt il répond 200 `{"status":"ok"}`. C’est exactement le modèle d’intégration prévu pour qu’une autre application consomme l’IA via HTTP.
- **Résultat n°2 :** Jan et LM Studio suivent la même séparation : moteur local/service séparé du GUI, exposé sur localhost. LM Studio va jusqu’à proposer un service headless distinct de l’interface. Cette séparation évite que le cycle de vie du moteur d’inférence soit mélangé au cycle de vie des widgets.
- **Résultat n°3 :** Qt 5.15 recommande `QProcess` pour lancer un programme externe et recevoir ses signaux `started`, `finished`, `errorOccurred`; `QNetworkAccessManager` fournit une API réseau asynchrone et doit rester dans son thread d’appartenance. L’utilisation de `QProcess` + HTTP localhost dans `LocalAiClient` est donc architecturalement correcte.
- **Résultat n°4 :** Qt impose qu’une connexion `QSqlDatabase` soit utilisée uniquement depuis le thread qui l’a créée. Toute construction/indexation SQL exécutée en parallèle doit donc rester strictement isolée par connexion/thread et ne doit pas être mélangée à l’interface ou à une autre connexion active.
- **Résultat n°5 :** les architectures RAG locales consultées séparent l’ingestion/indexation de la phase de question/réponse : les documents sont indexés avant les requêtes, puis le chat fait seulement `recherche -> contexte -> modèle`. Le chargement du LLM n’est pas utilisé comme moment pour reconstruire l’index documentaire.
- **Résultat n°6 — isolation Windows :** `llama-server.exe` lancé par `QProcess` est un processus Windows séparé avec son propre espace d’adressage privé. Microsoft documente qu’un crash d’une application user-mode n’affecte pas les autres applications ; Qt doit recevoir `CrashExit` / `errorOccurred`. Par conséquent, si `ecu_mems_manager.exe` se ferme lui aussi, le crash du serveur IA ne peut pas à lui seul corrompre directement la mémoire du processus MEMS Manager. Il faut alors rechercher un défaut exécuté dans le processus principal (thread/base/widget/cycle de vie), ou une pression ressource/système extrême distincte.
- **Comparaison avec ECU MEMS Manager :** `LocalAiClient` suit déjà le bon schéma : `QProcess` séparé pour `llama-server`, HTTP `127.0.0.1:18089`, `/health`, puis `/v1/chat/completions` avec `QNetworkAccessManager` asynchrone. Le problème structurel restant n’est donc pas « Qwen est mal embarqué dans Qt ».
- **Écart architectural identifié :** dans `IaMemsTab::showEvent()`, le clic/ouverture de l’onglet déclenche actuellement deux initialisations lourdes en parallèle : `startKnowledgeLoad()` qui lance `ExpertRuntimeDatabase::buildOrOpen()` dans un `QThread`, ET `LocalAiClient::initialize()` qui démarre/charge `llama-server` + Qwen. Cette simultanéité n’est pas le modèle utilisé par les intégrations locales robustes étudiées.
- **Conclusion de cette étape :** la voie correcte pour ECU MEMS Manager est de conserver `llama-server` comme processus séparé supervisé par Qt, d’attendre sa disponibilité via `/health`, et de traiter la base experte comme un index préparé/préconstruit puis ouvert en lecture seule au runtime. La reconstruction de la base experte ne doit pas être une charge lourde concurrente au chargement du modèle lors du simple affichage de l’onglet.
- **Niveau de preuve :** cette recherche établit une divergence architecturale concrète et une cause structurelle prioritaire à éliminer. Elle ne prouve pas encore à elle seule quelle instruction exacte déclenche le `0xC0000005` sur le PC utilisateur ; le prochain diagnostic doit vérifier la séparation stricte entre service IA, base experte préconstruite et GUI, sans modifier Qwen ni llama.cpp.

Dernière mise à jour : **25 août 2026 — BUILD #29 / v1.0.29. État CI actuel : DEUX WORKFLOWS DISTINCTS, un vert et un rouge. Le workflow principal `memsx64.yml` est vert sur `209da29d3870fe34aec21c29f1b4182e2cb9372e`. Le workflow COMPAT IA `memsx64_compat_ai.yml` est rouge sur le HEAD actuel `fee195e88d3615613b8f92de83209da2cf8247c2`, uniquement au nouveau contrôle de génération/packaging de la base experte IA. Les tests PC ont déjà prouvé que le runtime llama.cpp et Qwen peuvent démarrer et que MEMS Manager atteint `IA locale prête`, mais un crash reste à éliminer dans le chemin d’ouverture/initialisation de l’onglet IA. Ne pas créer BUILD #30 sans demande explicite.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 actuel : **`fee195e88d3615613b8f92de83209da2cf8247c2`** — `BUILD #29 package prebuilt expert database for IA tab`.
- Parent immédiat : **`209da29d3870fe34aec21c29f1b4182e2cb9372e`** — `BUILD #29 compile expert database packager`.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Branche de sauvegarde x64 validée : **`MEMSX64-BUILD26-BASE`**.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- Référence x64 UI/IA/navigation figée : **BUILD #26 — v1.0.26**.
- Premier BUILD x64 testé sur véhicule : **BUILD #27 — v1.0.27**.
- Rollback x64 stable avant le lot IA : **BUILD #28 — v1.0.28**.
- BUILD actif : **BUILD #29 — v1.0.29**.
- Règle utilisateur : **un BUILD = une version**.
- Un rerun d’un même commit/build ne change pas la version.
- Workflow x64 principal : `.github/workflows/memsx64.yml`.
- Workflow temporaire de compatibilité IA #29 : `.github/workflows/memsx64_compat_ai.yml`.
- Le workflow compile le commit exact qui le déclenche.
- `GITHUB_RUN_NUMBER` ne décide jamais de la version logicielle.

### CI actuel : bien distinguer les deux builds/workflows

#### Workflow principal — VERT

- Fichier : `.github/workflows/memsx64.yml`.
- Run : **`32883089997`**.
- Run number : **42**.
- Commit : **`209da29d3870fe34aec21c29f1b4182e2cb9372e`**.
- Conclusion : **SUCCESS**.
- Ce commit ajoute la compilation de `expert_runtime_selftest`, c’est-à-dire l’outil qui exécute le vrai code `ExpertRuntimeDatabase` pour produire/valider une base experte préconstruite.

#### Workflow COMPAT IA — ROUGE

- Fichier : `.github/workflows/memsx64_compat_ai.yml`.
- Run : **`32883199177`**.
- Job : **`97917392265`**.
- Commit : **`fee195e88d3615613b8f92de83209da2cf8247c2`**.
- Conclusion : **FAILURE**.
- Les étapes antérieures passent ; l’échec arrive à **`Build and validate packaged IA expert database`**.
- Le log valide d’abord la source : **`2134 entries, 433 semantic concepts`**.
- `expert_runtime_selftest.exe` est bien compilé.
- L’échec réel est ensuite un `FileNotFoundError` pendant la tentative de copie d’un chemin source absent : `expert/base_connaissance_ia` vers `build/expert_source`.
- Donc le rouge actuel **n’est pas un échec de compilation de MEMS Manager, ni un crash llama/Qwen dans GitHub Actions** ; il est localisé au nouveau contrôle/packaging de la base experte.

**Conclusion CI actuelle : un workflow vert + un workflow rouge. BUILD #29 n’est donc pas encore considéré totalement validé.**

---

## 2. BUILDS DE RÉFÉRENCE ET ROLLBACK

### BUILD #26 figé

Branche : `MEMSX64-BUILD26-BASE`.

Commit : `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`.

Run GitHub : `32816285887` — **SUCCESS**.

Validation réelle utilisateur : IA MEMS sans crash, base IA runtime prête, 14 onglets stables, `Aperçu` disponible, navigation IA MEMS / Mode interactif / Test ECU 1.9 / Aperçu fonctionnelle, package x64 local présent.

**Ne plus modifier cette branche. Elle sert de retour arrière x64 fiable.**

### BUILD #27 — premier x64 validé sur véhicule

Commit final : `a6f9b209f32b6dd77774832e8c84469c53deca47`.

Run final : `32832192437` — **SUCCESS COMPLET**.

Artefact : `ECU-MEMS-Manager-x64-BUILD-27-v1.0.27`.

Validation véhicule réelle : connexion AANMP002/MNE101150, COM3/FTDI FT232, polling 7D/80, logging et lecture RAM Injection Mode 4 fonctionnels.

### BUILD #28 — rollback x64 stable avant IA #29

HEAD : `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`.

Run : `32842049458` — **SUCCESS COMPLET**.

Artefact : **`ECU-MEMS-Manager-x64-BUILD-28-v1.0.28`**.

Artifact ID : `9561033224`.

SHA256 : `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.

BUILD #28 reste le rollback x64 stable si le lot IA #29 doit être isolé.

---

## 3. VALIDATION PC / VÉHICULE DÉJÀ EFFECTUÉE

### BUILD #27 — PC sans véhicule

- `v1.0.27 / BUILD #27` affiché ;
- 14 onglets présents ;
- IA MEMS ne plante pas ;
- question date répond ;
- absence moteur local signalée proprement quand le runtime lourd n’est pas livré ;
- plusieurs fermetures/réouvertures sans crash.

### BUILD #27 — véhicule réel AANMP002

- ECU : **AANMP002** ;
- référence : **MNE101150** ;
- port : **COM3** ;
- interface : **FTDI FT232** ;
- protocole : **ROSCO 1.3/1.6** ;
- D0 : `98 00 02 02` ;
- D1 : ASCII `AANMP002` répété ;
- polling `7D/80` stable ;
- logging fonctionnel ;
- batterie ≈ 13,8 V moteur tournant ;
- MAP ≈ 31–34 kPa au ralenti ;
- régime/MAP/TPS cohérents avec les sollicitations.

### Injection RAM Mode 4 validée sur AANMP002

- entrée Mode 4 réussie ;
- `0x03C8 = 1314 ticks` ;
- `0x026E = 0 ticks` ;
- `0x0280 = 0` ;
- base injection ≈ **2,63 ms** ;
- injection finale ≈ **2,63 ms** ;
- retour aux mesures diagnostiques observé ;
- programme resté actif.

---

## 4. CORRECTIONS BUILD #28 À CONSERVER

### Responsive sidebar

Le problème venait de la hauteur réellement disponible dans la sidebar, pas de la résolution écran comme règle absolue. Le BUILD #28 adapte la hauteur des 14 lignes à `nav->viewport()->height()` et évite que Qt fasse disparaître `Aperçu` lorsqu’on sélectionne `Test ECU 1.9`.

### Ralenti chaud

Règle projet obligatoire :

`idle_error_hot_corrected = raw - 32768 - correction`

La correction est la valeur réellement réglée dans l’onglet Réglages ; ne jamais coder `-3` en dur.

Exemple : raw `32772`, correction `-3` => attendu **`7`**.

### Déconnexion volontaire

Le BUILD #28 propage `m_disconnectRequested` dans les chemins ROSCO/recherche interface/réveil lent MEMS1.9 afin qu’une tentative de reconnexion déjà engagée s’annule quand l’utilisateur clique Déconnecter.

Test matériel encore à confirmer : laisser USB branché, arrêter moteur, couper contact, cliquer Déconnecter ; le bouton Connecter doit redevenir orange/actif sans reconnexion résiduelle.

---

## 5. RÈGLE BUILD / VERSION ET MÉTHODE

- BUILD #14 = v1.0.14
- BUILD #26 = v1.0.26
- BUILD #27 = v1.0.27
- BUILD #28 = v1.0.28
- BUILD #29 = v1.0.29
- BUILD #100 = v1.1.0
- BUILD #588 = v1.5.88
- BUILD #662 = v1.6.62

Formule : `1.(build / 100).(build % 100)`.

Règles :

1. un seul BUILD actif de développement à la fois ;
2. **un BUILD = une version** ;
3. un rerun GitHub du même commit/build n’est pas un nouveau BUILD ;
4. ne jamais utiliser `GITHUB_RUN_NUMBER` comme version ;
5. x64 uniquement sur `MEMSX64` ;
6. rapport uniquement sur `RAPPORT` ;
7. ne pas modifier `lab-expert-engine` pour résoudre un problème x64 ;
8. ne pas modifier `MEMSX64-BUILD26-BASE` ;
9. ne pas neutraliser une fonction pour masquer un crash ;
10. identifier une cause concrète avant modification ;
11. ne pas revenir sur une piste éliminée sans nouvelle preuve ;
12. CI vert signifie compilation/tests automatisés verts, pas validation PC/ECU réel ;
13. **ne pas créer BUILD #30 sans demande explicite utilisateur** ;
14. **avant chaque nouvelle étape, inscrire l’étape et son objectif dans le rapport ; dès qu’elle produit un résultat, inscrire le résultat avant l’étape suivante**.

---

## 6. INCIDENTS IA WINDOWS — HISTORIQUE UTILE

### Ancien incident BUILD #26 — corrigé

Symptôme : clic IA MEMS → fermeture immédiate du programme.

Donnée Windows : `MSVCP140.dll`, exception `0xc0000005`.

Correction historique : packaging du runtime x64. Commit `66fe69db556d83df56aa6ddd968cb48129cbcf95`.

Validation réelle : plus de crash IA, base IA recréée, fallback fonctionnel.

**Ne pas confondre cet ancien incident avec le problème BUILD #29. L’utilisateur a explicitement demandé de ne pas attribuer le problème actuel aux DLL MSVC sans preuve.**

### Méthode PowerShell obligatoire pour isoler le runtime IA

Depuis le dossier `ai` du package testé :

```powershell
.\llama-server.exe --version
$LASTEXITCODE
```

Si le runtime seul passe, lancer ensuite le serveur avec le modèle réel. Ce test sépare : runtime Windows, chargement GGUF et intégration QProcess/MEMS Manager.

---

## 7. BUILD #29 — IA LOCALE QWEN / LLAMA.CPP

### Contraintes conservées

- **Qwen3-0.6B-Q8_0** ;
- **llama.cpp b10516** ;
- local/offline sur `127.0.0.1:18089` ;
- langues **fr/en/es/it/pt/de** ;
- réponses déterministes pour batterie, état moteur, diagnostic, captures et dernière mesure ECU ;
- pas de modification volontaire du protocole ECU dans ce lot IA.

### Self-test multilingue

Le premier échec `spanish targeted answer` venait du self-test compilé sans `/utf-8`. Commit correctif : `e65b67fea31ec8848ec70ec904e8f36e7eb7a005`. Le self-test est ensuite devenu vert.

### Premier crash utilisateur du runtime #29

Symptôme via MEMS Manager : `QProcess: Process crashed`, code `-1073741819 / 0xC0000005`.

La tentative de ne garder que le backend CPU générique (`83a2b922a5cfc38da5bfb467e17ef4775ac88d57`) n’a pas corrigé le crash réel sur le PC utilisateur.

### Runtime COMPAT statique conservateur

Workflow dédié : `.github/workflows/memsx64_compat_ai.yml`.

Profil : runtime CPU statique, `GGML_NATIVE=OFF`, backend dynamique OFF, variantes CPU spécialisées OFF, OpenMP OFF, AVX/AVX2/AVX512/BMI2 désactivés, Qwen inchangé.

Commit ayant obtenu le premier COMPAT vert : `6b0df9c34a456b7c59af00bcab0acf82abcd7b80`.

Run : `32874029233` — SUCCESS.

Artefact : `ECU-MEMS-Manager-x64-BUILD-29-v1.0.29-COMPAT-IA`.

### Diagnostic PowerShell `0xC0000135`

Sur un package COMPAT antérieur, test direct :

```powershell
.\llama-server.exe --version
$LASTEXITCODE
```

Résultat : `-1073741515` = **`0xC0000135 / STATUS_DLL_NOT_FOUND`**.

La corrélation avec llama.cpp b10516 a conduit à désactiver OpenSSL pour le serveur HTTP local uniquement : `LLAMA_OPENSSL=OFF`.

Commit : `1bbe9923a7a655dcf51d7cd73f1b0e46ad8f1fd0` — `BUILD #29 make COMPAT IA runtime self-contained`.

Run : `32878926411` — SUCCESS COMPLET.

Le CI a alors réussi : imports PE, `llama-server.exe --version` avec PATH Windows réduit, chargement Qwen, `/health`, requête `/v1/chat/completions`, smoke launch MEMS Manager.

### Tests PC plus récents — état réel à conserver

Les tests manuels suivants ont ensuite avancé plus loin que le précédent `0xC0000135` :

- le test PowerShell du runtime ne bloque plus au même défaut de dépendance ;
- le serveur local peut être lancé avec le modèle ;
- **Qwen3-0.6B-Q8_0 se charge réellement** ;
- MEMS Manager atteint le statut **`IA locale prête`** ;
- malgré cela, l’application peut encore se fermer/crasher pendant le chemin d’ouverture/initialisation de l’onglet IA.

**Conclusion importante : le problème restant ne doit plus être résumé comme « Qwen ne démarre pas ». Qwen et llama.cpp ont déjà été observés en fonctionnement. Le défaut restant doit être isolé dans ce qui se passe autour de l’initialisation de l’IA, notamment la base experte/runtime de l’onglet et son cycle de vie, sans toucher au protocole ECU.**

---

## 8. BASE EXPERTE IA — NOUVELLE VOIE BUILD #29

Objectif du dernier lot : éviter de reconstruire toute la base experte au moment où Qwen est déjà résident. Le CI doit exécuter le même `ExpertRuntimeDatabase` une fois, valider le SQLite produit, puis emballer cette base préconstruite. Le runtime conserve la reconstruction uniquement comme fallback si la base emballée est absente.

### Commit `209da29d...` — workflow principal vert

Ajout d’un exécutable `expert_runtime_selftest` à partir de :

- `expert/ExpertRuntimeDatabaseSelfTest.cpp` ;
- `expert/ExpertRuntimeDatabase.cpp` ;
- `expert/ExpertRuntimeDatabase.h`.

Le workflow principal correspondant est **vert**.

### Commit `fee195e8...` — HEAD actuel, COMPAT rouge

Le workflow COMPAT tente désormais de :

1. exécuter `expert_runtime_selftest.exe` dans un cache isolé ;
2. récupérer `ia_mems_reference_r20.sqlite` ;
3. le copier dans `database/expert/` du package final ;
4. vérifier `PRAGMA user_version == 20` et la présence de tables ;
5. lancer ensuite le runtime IA et Qwen.

Le run `32883199177` s’arrête avant d’aller au bout de cette chaîne.

État exact du log :

- **source expert DB validée : 2134 entrées / 433 concepts sémantiques** ;
- `expert_runtime_selftest.exe` compilé ;
- échec pendant une copie de `expert/base_connaissance_ia` qui n’existe pas à cet emplacement dans le checkout ;
- erreur : `FileNotFoundError [WinError 2]`.

**Ce point est le seul rouge CI actuellement identifié dans le workflow COMPAT. Aucun artefact COMPAT final de ce HEAD ne doit être donné comme validé tant que cette étape n’est pas verte.**

---

## 9. NAVIGATION OFFICIELLE — 14 ONGLETS

Ordre officiel :

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

Architecture : **identité stable → ordre → clé traduction → icône**.

Ne jamais identifier/réordonner par texte traduit ni reconstruire la barre par timers concurrents.

L’onglet Injection reste entre Aperçu et Réglages.

---

## 10. SÉCURITÉ PROTOCOLE — BLOC À CONSERVER

Contexte central :

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ;
- `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.

Principes :

- F0 lecture mode ;
- D0/D1/D2 uniquement session normale ;
- D1 refusé Mode4 ;
- D3/F3/F4/F5 refusés depuis interface générique ;
- mutation exige famille prouvée Rosco13_16 + Normal ;
- famille inconnue fail-closed ;
- MEMS1.9 mutations bloquées ;
- F7/EF injecteurs bloqués tant que sous-type non prouvé ;
- transactions RAM Injection bloquent les commandes génériques concurrentes.

`memsinterface.h` doit conserver obligatoirement :

`void onProtocolCommandRequested(quint8 command);`

Commandes/identification de référence :

- D0 → `D0 98 00 02 02` ;
- D1 → ASCII `AANMP002` ;
- F0 → `F0 50` ;
- D2 → `D2 00 01` ;
- F4 → `F4 00`.

Règles :

- tests actionneurs seulement si contexte exact autorisé, `uk3==0` pour le cas historique concerné ;
- nouveau DTC => alerte/beep ;
- `closed_loop != 0` => LED ON ;
- `idle_switch == 0` => ON ;
- ralenti chaud : `raw - 32768 - correction` ;
- dwell primaire attendu ≈ 1,9–3,1 ms vers 14 V.

---

## 11. DLL PROTOCOLE X64 / ABI

DLL : `mems_manager_x64.dll`. Sources : `librosco-x64/`.

ABI à préserver : `frame80=28`, `frame7d=32`, `mems_data=60`, 22 exports historiques exacts.

Commandes historiques : init `CA 75 F4 D0`, polling `0x80/0x7D`, IAC `0xFB/FD/FE`, clear faults `0xCC`, heartbeat `0xF4`, reset ECU `0xFA`, reset adjustments `0x0F`.

Principe : **la signification d’un octet n’est jamais universelle ; famille + mode sont nécessaires.**

---

## 12. INJECTION / RAM — RÈGLES

- Ne pas calculer le temps d’injection depuis `0x7D/0x80`.
- Les trames normales 7D/80 ne contiennent pas les adresses RAM `0x03C8`, `0x026E`, `0x0280`.
- Lecteur Mode4 dédié read-only.
- Sélection bloc `DC`, offsets `0x00–0x7F`.
- Ne pas utiliser calibration `0x80–0xBF`.
- Firmware sans profil exact => refus avant Mode4.
- Échec restauration normal => polling arrêté.
- Injection entre Aperçu et Réglages.
- Dwell reste dans Injection.
- Les sous-vues Injection partagent le même mode de lecture afin de ne pas ajouter de polling incompatible.

Validation x64 réelle #27 : `0x03C8=1314`, `0x026E=0`, `0x0280=0`, injection ≈2,63 ms.

---

## 13. BLOQUEURS SÉCURITÉ ENCORE NO-GO

### MEMS1.9 injecteurs

`MEMS_TestInjectors = 0xF7` ; `MEMS_TestInjectorsMPi = 0xEF`. Ne pas tester avant mapping exact famille/sous-type.

### 7D/80 MEMS1.9

Parser historique tailles fixes 28/32 ; valider longueur réelle/per-famille avant lecture 1.9.

### W4 ISO 9141 / 5 bauds

Le wake-up doit respecter W4 **25–50 ms** avant réponse key byte. Ajouter délai monotone explicite + test CI avant validation 1.9.

### Reconnexion MEMS1.9

Stratégie dédiée à prévoir ; après session cassée une coupure contact ~15 s peut être nécessaire. Ne pas considérer les temporisations actuelles comme universelles.

### Actionneurs

Timeout global/failsafe + suivi ON/OFF + perte communication encore à finaliser. `F4` n’est pas un OFF universel hors contexte prouvé.

### Ports série

Le dispatcher peut encore parcourir plusieurs ports détectés. **Ne jamais envoyer de séquence ECU à un périphérique série arbitraire.**

### Profils RAM 1.9

164 profils corrélés ne signifient pas 164 profils testés matériellement.

### Mutations ECU

Actionneurs, reset, clear faults, trims et écritures restent **NO-GO** pendant les validations actuelles.

---

## 14. DÉTECTION AUTOMATIQUE CÂBLE / COM — OBJECTIF

Exigence générale : détecter interface + COM, afficher COM/type, différencier les interfaces, valider interface/protocole au clic Connecter, messages clairs en cas d’incompatibilité/absence ECU.

Résultat réel #27 : **COM3 / FTDI FT232 détecté et connexion ROSCO réussie sur AANMP002.**

Avant test MEMS1.9 : ne pas envoyer de séquences ECU sur tous les ports série arbitraires.

---

## 15. BASE MEMS / IA

Base MEMS principale : `<appdir>/database/ecu_mems_manager.sqlite`.

Ancien cache IA : `%LOCALAPPDATA%\ECU Mems Manager\ECU Mems Manager\ia-mems\ia_mems_reference_r20.sqlite`.

Runtime IA BUILD #29 : llama.cpp b10516, `llama-server.exe` Windows x64 CPU COMPAT, Qwen3-0.6B-Q8_0 GGUF, HTTP local `127.0.0.1:18089`.

Le modèle GGUF n’est pas spécifique à Windows ; le binaire llama.cpp l’est.

Lot `database/reference/research_enrichment_1600.qz64` : volumineux, ne pas charger automatiquement sans mesurer RAM/temps/utilité.

La nouvelle voie #29 est de livrer une base experte SQLite r20 préconstruite afin d’éviter une reconstruction lourde pendant l’initialisation de l’onglet IA lorsque Qwen est déjà en mémoire.

---

## 16. TESTS DÉJÀ FAITS POUR LE BUILD #29 IA

1. Self-test réponses déterministes FR/EN/ES/IT/PT/DE : corrigé pour UTF-8 et vert en CI.
2. Test package officiel llama : crash réel PC observé, code QProcess `0xC0000005`.
3. Test backend CPU générique seul : CI vert mais crash PC identique ; piste insuffisante.
4. Test PowerShell direct d’un premier COMPAT : `llama-server.exe --version` sans sortie, `$LASTEXITCODE = -1073741515 / 0xC0000135`.
5. Suppression OpenSSL du runtime local COMPAT : CI `32878926411` entièrement vert avec `--version`, imports, `/health`, Qwen et chat completion.
6. Tests PowerShell plus récents : runtime et modèle démarrent, Qwen se charge.
7. Test MEMS Manager plus récent : statut **`IA locale prête`** atteint avant le crash restant.
8. Nouveau test CI base experte : la source est bien analysée, avec **2134 entrées et 433 concepts sémantiques**.
9. Workflow principal du commit `209da29d...` : **VERT**.
10. Workflow COMPAT du HEAD `fee195e8...` : **ROUGE uniquement au nouveau contrôle/packaging de base experte**.

---

## 17. TESTS À FAIRE ENSUITE

### D’abord : obtenir le même BUILD #29 avec les deux workflows verts

Corriger uniquement la cause exacte du rouge COMPAT concernant le chemin/source de la base experte, sans toucher au modèle, au runtime b10516, au protocole ECU ou au 32 bits.

Une fois un nouvel artefact COMPAT final produit et vert :

1. télécharger et extraire proprement le nouvel artefact ;
2. dans `ai`, exécuter `./llama-server.exe --version` puis `$LASTEXITCODE` ;
3. si le runtime seul est bon, lancer le serveur avec `ai/models/ia-mems.gguf` et les arguments réels ;
4. confirmer que Qwen termine son chargement sans fermeture du serveur ;
5. lancer MEMS Manager et ouvrir `IA MEMS` ;
6. confirmer `IA locale prête` **sans fermeture de l’application** ;
7. poser plusieurs questions connues de la base experte, dont au moins une qui dépend de la base préconstruite ;
8. vérifier les réponses déterministes batterie / état moteur / diagnostic / captures ;
9. vérifier au minimum le français et une autre langue, puis contrôler que les mêmes faits techniques restent identiques en `en/es/it/pt/de` ;
10. fermer/réouvrir l’onglet IA et relancer MEMS Manager pour vérifier la stabilité répétée ;
11. seulement après stabilité PC, reprendre les tests ECU non dangereux.

### Tests véhicule encore à confirmer sur le build retenu

- AANMP002 : connexion et données normales ;
- ralenti chaud : raw `32772`, correction `-3` => valeur corrigée **7** ;
- déconnexion : moteur arrêté, contact coupé, USB laissé branché, clic Déconnecter => Connecter orange/actif sans reconnexion résiduelle ;
- aucun actionneur, reset, clear faults ou écriture pendant cette validation.

---

## 18. PROCHAINE ACTION EXACTE

**Rester sur BUILD #29 / v1.0.29. Ne pas créer BUILD #30.**

État de départ pour la prochaine reprise :

- `MEMSX64` HEAD = `fee195e88d3615613b8f92de83209da2cf8247c2` ;
- workflow principal `memsx64.yml` = **VERT**, run `32883089997`, commit `209da29d3870fe34aec21c29f1b4182e2cb9372e` ;
- workflow COMPAT `memsx64_compat_ai.yml` = **ROUGE**, run `32883199177`, HEAD `fee195e88d3615613b8f92de83209da2cf8247c2` ;
- cause rouge actuelle = étape `Build and validate packaged IA expert database`, copie d’un chemin `expert/base_connaissance_ia` absent ;
- validation source déjà réussie = **2134 entrées / 433 concepts sémantiques** ;
- runtime llama.cpp/Qwen a déjà été observé fonctionnel en PowerShell ;
- MEMS Manager a déjà atteint **`IA locale prête`** avant le crash restant ;
- recherche externe architecture IA : `LocalAiClient` suit déjà le bon modèle `QProcess + localhost HTTP + /health`; Windows isole le processus `llama-server.exe`, donc un crash de celui-ci ne doit pas faire tomber directement `ecu_mems_manager.exe` ; la divergence à éliminer est la reconstruction/initialisation lourde de la base experte en parallèle du chargement Qwen au simple affichage de l’onglet ;
- l’objectif immédiat est donc de terminer le packaging d’une base experte préconstruite et d’ouvrir cette base en lecture seule au runtime, afin que l’ouverture de l’onglet ne lance plus une reconstruction lourde concurrente au modèle ;
- ne pas modifier le protocole ECU, `lab-expert-engine`, `MEMSX64-BUILD26-BASE`, Qwen3-0.6B-Q8_0 ou llama.cpp b10516 pendant cette étape.

---

## PRINCIPE DIRECTEUR

**BUILD #26 reste la base x64 figée. BUILD #27 est le premier x64 réellement testé sur AANMP002 avec connexion, polling 7D/80 et Injection RAM Mode4 fonctionnels. BUILD #28 reste le rollback x64 stable avant le lot IA. BUILD #29 conserve Qwen3-0.6B-Q8_0, llama.cpp b10516 et les six langues. Le runtime et Qwen ont maintenant été observés en fonctionnement sur le PC utilisateur. La recherche externe confirme que le schéma correct est un moteur `llama-server` séparé du GUI, consommé en HTTP asynchrone, avec la base de connaissances préparée/indexée séparément puis ouverte en lecture seule ; la reconstruction de cette base ne doit pas être concurrente au chargement du modèle lors du simple affichage de l’onglet. Windows isole les processus user-mode : si MEMS Manager se ferme aussi, une cause dans son propre processus doit être recherchée. Toute commande susceptible de modifier l’ECU reste fail-closed si famille ou mode ne sont pas prouvés. La référence 32 bits reste intacte.**