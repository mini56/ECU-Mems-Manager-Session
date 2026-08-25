# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.

Dernière mise à jour : **25 août 2026 — BUILD #29 / v1.0.29 COMPAT IA recompilé avec runtime llama.cpp b10516 Windows x64 autonome sans OpenSSL. Le diagnostic PowerShell réel a isolé un code `-1073741515 / 0xC0000135` sur `llama-server.exe --version`, cohérent avec une dépendance DLL manquante et avec un incident upstream llama.cpp lié à OpenSSL. Le nouveau CI est entièrement vert, y compris `llama-server --version` avec PATH isolé et démarrage Qwen local. IMPORTANT : le nouvel artefact n’est pas encore testé sur le PC utilisateur et reste donc à valider matériellement.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 actuel : **`1bbe9923a7a655dcf51d7cd73f1b0e46ad8f1fd0`**.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Branche de sauvegarde x64 validée : **`MEMSX64-BUILD26-BASE`**.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- Référence x64 UI/IA/navigation figée : **BUILD #26 — v1.0.26**.
- Premier BUILD x64 testé sur véhicule : **BUILD #27 — v1.0.27**.
- BUILD x64 actuel : **BUILD #29 — v1.0.29**, voie COMPAT IA en validation.
- BUILD #28 reste le dernier rollback x64 stable disponible avant le lot IA #29.
- Règle utilisateur : **un BUILD = une version**.
- Un rerun d’un même commit/build ne change pas la version.
- Workflow x64 principal : `.github/workflows/memsx64.yml`.
- Workflow temporaire de compatibilité IA #29 : `.github/workflows/memsx64_compat_ai.yml`.
- Le workflow compile le commit exact qui le déclenche.
- `GITHUB_RUN_NUMBER` ne décide jamais de la version logicielle.

### BUILD #26 figé

Branche : `MEMSX64-BUILD26-BASE`

Commit : `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`

Message : `Stop visual completion from rebuilding BUILD #26 navigation`

Run GitHub : `32816285887` — **SUCCESS**.

Validation réelle utilisateur : IA MEMS sans crash, base IA runtime prête, 14 onglets stables, `Aperçu` disponible, navigation IA MEMS / Mode interactif / Test ECU 1.9 / Aperçu fonctionnelle, package MSVC local présent.

**Ne plus modifier cette branche. Elle sert de retour arrière x64 fiable.**

### BUILD #27 — état final

Commit de départ : `cbc1496a12a83da2d9e4ef2cda90dba3e873a166`

Premier bloc sécurité protocole : `bab88302036730089a89b597d0da2b4a5e4b242b`

Correction compilation : `a6f9b209f32b6dd77774832e8c84469c53deca47` — `Fix BUILD #27 QVariant compile error`.

Correction exacte : ajout de `#include <QVariant>` dans `memsinterface_dispatch.cpp`.

Run final : `32832192437` — **SUCCESS COMPLET**.

Job : `97753152749` — **SUCCESS**.

Artefact testé : `ECU-MEMS-Manager-x64-BUILD-27-v1.0.27`

Artifact ID : `9557374893`

SHA256 : `d7dce4235ee2c6c23465f3399eed27f8d5d379cab341d803253dd7509fe8da9f`

### BUILD #28 — état CI actuel

Base exacte : HEAD final du BUILD #27 `a6f9b209f32b6dd77774832e8c84469c53deca47`.

Commits du lot #28 :

1. `81da01af8f0682851f45f5886fb5940b7a1ee113` — `BUILD #28 responsive sidebar and hot idle display fix` ;
2. `2e6cb0c100b24f7e6f382827a2522181b37968e6` — `BUILD #28 cancel reconnect on user disconnect` ;
3. `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8` — `Start BUILD #28 v1.0.28 validation`.

Comparaison `a6f9b209...` → `0533adaf...` : **3 commits, 3 fichiers modifiés seulement** :

- `.github/workflows/memsx64.yml` ;
- `memsinterface_dispatch.cpp` ;
- `visualcompletion.cpp`.

Aucune modification du 32 bits.

Run BUILD #28 : **`32842049458` — SUCCESS COMPLET**.

Job principal : **`97783505796` — SUCCESS**.

Contrôles passés :

- navigation déterministe 14/14 ;
- garde responsive BUILD #28 ;
- garde signe ralenti chaud BUILD #28 ;
- garde annulation déconnexion/reconnexion BUILD #28 ;
- protections protocole historiques ;
- configuration x64 ;
- compilation application + DLL protocole ;
- self-test ABI ;
- self-test SQLite sémantique ;
- assemblage package ;
- inventaire PE AMD64 ;
- ressources QSQLITE/Qt/MSVC ;
- smoke launch ;
- rapport des 15 contrôles ;
- upload artefact.

Artefact : **`ECU-MEMS-Manager-x64-BUILD-28-v1.0.28`**

Artifact ID : **`9561033224`**

Taille : `92 876 315` octets.

SHA256 : **`50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`**

Expiration GitHub annoncée : `23 novembre 2026`.

**Important : CI vert signifie que le BUILD #28 compile et passe les contrôles automatisés. Les trois corrections doivent encore être validées matériellement par l’utilisateur avant d’être déclarées définitivement corrigées.**

---

## 2. VALIDATION PC / VÉHICULE DU BUILD #27

### Validation PC sans véhicule

- `v1.0.27 / BUILD #27` affiché ;
- 14 onglets présents ;
- IA MEMS ne plante pas ;
- question date répond ;
- message `Moteur llama.cpp local absent du dossier IA` affiché proprement ;
- plusieurs fermetures/réouvertures sans crash.

### Validation réelle véhicule — 25 août 2026

ECU / véhicule de référence :

- ECU : **AANMP002** ;
- référence affichée : **MNE101150** ;
- port : **COM3** ;
- interface : **FTDI FT232** ;
- chemin protocole : **ROSCO 1.3/1.6** ;
- D0 : `98 00 02 02` ;
- D1 : ASCII `AANMP002` répété ;
- polling `7D/80` fonctionnel et stable ;
- logging fonctionnel.

Mesures observées :

- ralenti ≈ 1160–1200 tr/min au début de la séquence ;
- batterie ≈ 13,8 V moteur en marche ;
- MAP ≈ 31–34 kPa au ralenti ;
- variations régime/MAP/TPS cohérentes pendant les sollicitations.

### Injection RAM Mode 4 validée sur AANMP002

- entrée Mode 4 réussie ;
- `0x03C8 = 1314 ticks` ;
- `0x026E = 0 ticks` ;
- `0x0280 = 0` ;
- base injection ≈ **2,63 ms** ;
- injection finale ≈ **2,63 ms** ;
- programme resté actif ;
- retour vers les mesures diagnostiques observé.

**Conclusion #27 : connexion réelle, identification, polling 7D/80, logging et lecture RAM Injection Mode 4 validés sur AANMP002.**

---

## 3. DÉFAUTS REPRODUITS SUR BUILD #27 ET CORRECTIONS BUILD #28

### 3.1 Responsive sidebar — `Aperçu` sort du viewport

Observation utilisateur décisive :

- sur un ordinateur testé le matin avec une taille/résolution permettant davantage de hauteur, `Aperçu` ne disparaît pas ;
- sur le PC utilisé avec le véhicule, le menu latéral est physiquement **coincé entre le bandeau/bouton supérieur et la barre d’outils inférieure** ;
- quand l’utilisateur clique sur le dernier onglet **`Test ECU 1.9`**, Qt fait défiler le `QListWidget` pour garder la ligne sélectionnée visible ;
- `Aperçu` sort alors du haut du viewport ;
- il ne s’agit pas d’une suppression du modèle de navigation.

**Diagnostic retenu : problème responsive de hauteur de sidebar. Le responsive doit dépendre de la taille réelle de la fenêtre et de la hauteur réellement disponible, pas de la résolution écran comme règle directe.**

Correction BUILD #28 dans `visualcompletion.cpp` :

- calcul de la hauteur des lignes à partir de `nav->viewport()->height()` ;
- prise en compte du nombre réel d’onglets ;
- adaptation au redimensionnement de la fenêtre ;
- scrollbars du menu désactivées afin d’éviter le défilement automatique masquant la première ligne ;
- taille des 14 lignes recalculée pour l’espace réellement disponible entre les barres supérieure et inférieure.

À valider sur PC : réduire/agrandir la fenêtre puis cliquer `Test ECU 1.9` ; les 14 entrées doivent rester visibles et `Aperçu` ne doit pas sortir du menu.

### 3.2 Signe incorrect du ralenti chaud

Règle projet obligatoire :

`idle_error_hot_corrected = raw - 32768 - correction`

La correction est la valeur réellement réglée dans l’onglet Réglages ; ne jamais coder `-3` en dur.

Exemple véhicule réel #27 : raw `32772`, correction `-3` ; #27 affichait `1` avec l’ancienne addition. La règle projet donne **`7`**.

Correction BUILD #28 : calcul d’affichage corrigé selon la formule obligatoire, sans modifier la valeur réelle de réglage ECU.

À valider sur véhicule : vérifier simultanément brut `7D14-15`, correction réglée et valeur corrigée. Pour raw `32772` / correction `-3`, attendu `7`.

### 3.3 Déconnexion volontaire / reconnexion résiduelle

Séquence utilisateur réelle #27 :

1. moteur arrêté ;
2. contact coupé ;
3. clic `Déconnecter` ;
4. bouton `Connecter` ne redevient pas immédiatement orange/actif ;
5. débranchement physique USB → `Connecter` redevient orange ;
6. impression de clignotements/tentatives de reconnexion résiduelles.

Cause de code identifiée : le clic `Déconnecter` arrêtait bien le timer de reconnexion, mais une tentative de reconnexion déjà engagée pouvait continuer dans le chemin ROSCO puis dans le réveil lent MEMS 1.9, car ces chemins surveillaient le shutdown programme mais pas systématiquement `m_disconnectRequested`.

Correction BUILD #28 dans `memsinterface_dispatch.cpp` :

- `tryRoscoConnect()` surveille aussi `m_disconnectRequested` ;
- le réveil MEMS 1.9 surveille la demande de déconnexion avant et pendant les étapes lentes ;
- les boucles de recherche d’interface s’arrêtent sur déconnexion volontaire ;
- une connexion obtenue alors qu’une déconnexion vient d’être demandée est refermée ;
- une tentative annulée volontairement aboutit à `disconnected()` et non à un nouvel échec/reconnect automatique ;
- distinction interne `shutdown-requested` / `disconnect-requested`.

À valider sur véhicule : **laisser le câble USB branché**, arrêter moteur, couper contact, cliquer `Déconnecter`. Le bouton `Connecter` doit redevenir orange/actif et aucune tentative de reconnexion/clignotement ne doit continuer.

---

## 4. RÈGLE BUILD / VERSION ET MÉTHODE

Le numéro de BUILD est le numéro de version :

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
9. ne pas empiler de workflows temporaires ;
10. ne pas neutraliser une fonction pour masquer un crash ;
11. identifier une cause concrète avant modification ;
12. ne pas revenir sur une piste éliminée sans nouvelle preuve ;
13. CI vert signifie compilation/tests automatisés verts, pas validation PC/ECU réel.

Commits historiques de discipline :

- `522fae53cb1573a956ce50941d5a185a4d245e66` — nettoyage CI x64 ;
- `3c4102eca34a2426970ee03e01830a6317b9db07` — BUILD/version explicite, suppression dépendance à `GITHUB_RUN_NUMBER`.

---

## 5. INCIDENT IA X64 BUILD #26 — CORRIGÉ

Symptôme : clic IA MEMS → fermeture immédiate du programme.

Donnée Windows décisive : `MSVCP140.dll`, exception `0xc0000005`, offset `0x12EB0`.

Le package n’embarquait pas `MSVCP140.dll`, `VCRUNTIME140.dll`, `VCRUNTIME140_1.dll` et utilisait le runtime système.

Correction : `66fe69db556d83df56aa6ddd968cb48129cbcf95` — `Package MSVC x64 runtime in BUILD #26`.

Run : `32814736178` — **SUCCESS**.

Validation réelle : plus de crash IA, base IA recréée, fallback fonctionnel, absence llama signalée proprement.

Pistes déjà éliminées sans nouvelle preuve : bloc 1600, ancien cache SQLite, simple absence du dossier IA, ExpertRuntimeDatabase x64 génériquement cassé, QSQLITE seul, UTF-8, wrapper IA seul.

### Méthode de diagnostic PowerShell à conserver

Lors de l’ancien incident IA, `llama-server.exe` a également été lancé directement dans une fenêtre PowerShell afin de séparer un crash du moteur local d’un crash provoqué par son intégration dans MEMS Manager. Ce test avait montré que le serveur était trouvé/lancé puis s’arrêtait pendant son démarrage. Le correctif d’intégration qui a suivi a imposé le dossier du serveur comme dossier de travail et ajouté ce dossier au `PATH` du processus. Cette logique doit rester présente dans `LocalAiClient`.

**Pour tout nouveau crash à l’ouverture de l’onglet IA : commencer par `llama-server.exe --version` depuis le dossier `ai` du package testé, avant de modifier le code ou le modèle.** Si `--version` plante, le problème est dans le runtime/exécutable ou son environnement ; si `--version` fonctionne, lancer ensuite le serveur avec le modèle pour distinguer runtime seul / chargement GGUF / intégration QProcess.

---

## 6. NAVIGATION 14 ONGLETS

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

Commit historique : `a005501c94d7e5c949926a7fa527690804994154` — mapping direct sidebar ↔ QTabWidget.

Ancien bug BUILD #26 : `visualcompletion.cpp::syncNavigation()` reconstruisait `uiRebuildNav`. Corrigé par `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`.

**BUILD #28 ne change pas l’ordre ou l’identité des 14 onglets : il corrige uniquement l’ajustement vertical de la liste à la taille réelle de la fenêtre.**

---

## 7. SÉCURITÉ PROTOCOLE — BLOC BUILD #27 CONSERVÉ

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

`mappedinjection_runtime_part4.inc` publie les transitions `Transition`, `Normal`, `Mode4`, `Unknown` et restaure la session normale après la lecture RAM quand possible.

Commits principaux du bloc :

- `9b1e0e793ca1f18119d728a3741ed5c5dbbc7abe`
- `e6367ff43ef38b445c3d6c12d8915ca062fc8b3e`
- `522c7115cc444656421fb1e6d7138dfed02f49b8`
- `2ca67aba4e530188c4c41b1c739da29a64d16746`
- `66a2e88f8f544022892ad469465d4ebfec6e1f1b`
- `4978e7ce43a5deb299eac4bc1cb73f320467f1da`
- `f01f2a47f89d24e0ac1f4fcb380038981355cdc3`
- `87c40f3faeb4a8d5c0bc6be8fedccfce05c8a086`
- `bab88302036730089a89b597d0da2b4a5e4b242b`
- `a6f9b209f32b6dd77774832e8c84469c53deca47`

Le BUILD #28 conserve ces protections ; les CI #28 et #29 ont repassé les garde-fous historiques avec succès.

---

## 8. DLL PROTOCOLE X64 / 15 CONTRÔLES

DLL : `mems_manager_x64.dll`. Sources : `librosco-x64/`.

ABI à préserver : `frame80=28`, `frame7d=32`, `mems_data=60`, 22 exports historiques exacts.

Commandes historiques : init `CA 75 F4 D0`, polling `0x80/0x7D`, IAC `0xFB/FD/FE`, clear faults `0xCC`, heartbeat `0xF4`, reset ECU `0xFA`, reset adjustments `0x0F`.

Principe : **la signification d’un octet n’est jamais universelle ; famille + mode sont nécessaires.**

Le workflow x64 vérifie application/DLL AMD64, Qt/plugins, QSQLITE/SerialPort, import DLL x64, absence DLL x86, 22 exports, ABI, gardes protocole, SQLite, runtime MSVC, smoke launch et inventaire PE.

BUILD #28 ajoute des gardes statiques ciblées pour le responsive sidebar, le signe du ralenti chaud et l’annulation de reconnexion sur déconnexion volontaire.

---

## 9. BLOQUEURS SÉCURITÉ ENCORE NO-GO

### Injecteurs 1.9/MPI

`MEMS_TestInjectors = 0xF7` ; `MEMS_TestInjectorsMPi = 0xEF`. Ne pas tester avant mapping exact famille/sous-type.

### 7D/80 MEMS1.9

Parser historique tailles fixes 28/32 ; traiter longueur réelle/per-famille avant lecture 1.9 validée.

### W4 ISO 9141 / 5 bauds

Le wake-up actuel répond trop immédiatement au key byte. ISO W4 attendu : **25–50 ms**. Ajouter délai monotone explicite + test CI avant validation 1.9.

### Reconnexion MEMS1.9

Prévoir stratégie dédiée ; après session cassée une coupure contact ~15 s peut être nécessaire. Ne pas considérer 450 ms/3 s comme universel.

### Actionneurs

Ajouter timeout global/failsafe, suivi ON/OFF et perte communication. `F4` ne doit pas être traité comme OFF universel hors contexte prouvé.

### Commandes altérant l’ECU

Clear faults, resets, trims, réglages et actionneurs doivent garder contexte + action utilisateur claire. Clear faults doit recevoir confirmation UI appropriée.

### Ports série

Le dispatcher peut encore parcourir plusieurs ports détectés. **Avant test MEMS 1.9, ne jamais envoyer de séquence ECU à un périphérique série arbitraire.**

### Profils RAM 1.9

164 profils corrélés ne signifient pas 164 profils testés matériellement. Conserver statuts distincts.

---

## 10. RÉFÉRENCE MATÉRIELLE 32 BITS À CONSERVER

Branche : `lab-expert-engine`.

BUILD #14 — v1.0.14, retesté le 24 août 2026 sur ECU réel : COM3, AANMP002/MNE101150, communication verte, polling 7D/80, navigation et IA stables.

Injection RAM Mode 4 : transition 7D/80 → Mode4 fonctionnelle, retour normal fonctionnel, injection exemple ≈2,47 ms, `0x03C8=1233`, `0x026E=0`, `0x0280=0`.

Trace : `2026-08-24_18.14.txt`.

**Ne pas modifier cette branche pour résoudre le x64.**

---

## 11. INJECTION / RAM — RÈGLES

- Ne pas calculer le temps d’injection depuis `0x7D/0x80`.
- Adresses : `0x03C8`, `0x026E`, `0x0280`.
- Lecteur Mode4 dédié read-only.
- Sélection bloc `DC`, offsets `0x00–0x7F`.
- Ne pas utiliser calibration `0x80–0xBF`.
- Firmware sans profil exact => refus avant Mode4.
- Échec restauration normal => polling arrêté.
- Injection entre Aperçu et Réglages.
- Dwell reste dans Injection.
- Sous-vues Injection partagent le même mode de lecture.

Validation x64 réelle #27 : `0x03C8=1314`, `0x026E=0`, `0x0280=0`, injection ≈2,63 ms.

---

## 12. BASE MEMS / IA

Lot : `database/reference/research_enrichment_1600.qz64`, environ 33,3 Mo compressés.

Ne pas charger automatiquement sans mesurer RAM/temps/utilité. ExpertRuntimeDatabase l’exclut de l’import cache IA.

Base : `<appdir>/database/ecu_mems_manager.sqlite`.

Cache IA : `%LOCALAPPDATA%\ECU Mems Manager\ECU Mems Manager\ia-mems\ia_mems_reference_r20.sqlite`.

Runtime IA actuel en BUILD #29 : llama.cpp b10516, `llama-server.exe` Windows x64 CPU recompilé en profil statique conservateur, Qwen3-0.6B-Q8_0 GGUF, HTTP local `127.0.0.1:18089`.

Le modèle Qwen GGUF n’est pas spécifique à Windows ou Linux. La plateforme dépend du moteur qui l’exécute. Pour MEMS Manager, `llama-server.exe` est construit spécifiquement pour Windows x64.

Améliorations IA #29 : réponses déterministes ciblées pour batterie, état moteur, diagnostic, captures, suivi de la date/heure de dernière mesure ECU et routage multilingue `fr/en/es/it/pt/de`.

---

## 13. DÉTECTION AUTOMATIQUE CÂBLE / COM — OBJECTIF

Exigence générale : détecter interface + COM, afficher COM/type, différencier interfaces, valider interface/protocole au clic Connecter, messages clairs en cas d’incompatibilité/absence ECU.

Résultat réel #27 : **COM3 / FTDI FT232 détecté et connexion ROSCO réussie sur AANMP002.**

Avant test MEMS1.9 : **ne pas envoyer de séquences ECU sur tous les ports série arbitraires.**

---

## 14. PLAN TEST MATÉRIEL — ÉTAT

### Phase A — lecture seule

**VALIDÉE sur AANMP002/MNE101150 avec BUILD #27** : interface/port, connexion, identification, polling normal, logging.

### Phase B — comparaison / stabilité

Mesures cohérentes validées sur #27. La correction de déconnexion du #28 doit maintenant être retestée.

### Phase C — Injection RAM Mode4

**VALIDÉE sur AANMP002 avec BUILD #27** : entrée Mode4, lecture 03C8/026E/0280, injection ≈2,63 ms, retour diagnostique observé.

### Phase D — MEMS1.9

Toujours NO-GO complet : W4, parser longueur, sélection port/interface, reconnexion dédiée.

### Phase E — actionneurs

Toujours NO-GO : famille exacte + mode exact + commande exacte + timeout/failsafe.

---

## 15. PROCHAINE ACTION EXACTE

**La voie active est désormais BUILD #29 / v1.0.29 COMPAT IA. Le nouvel artefact du commit `1bbe9923a7a655dcf51d7cd73f1b0e46ad8f1fd0` est compilé vert mais n’est pas encore testé sur le PC utilisateur. Ne pas créer BUILD #30 sans demande explicite.**

Ordre du prochain test IA :

1. télécharger/extrait le nouvel artefact `ECU-MEMS-Manager-x64-BUILD-29-v1.0.29-COMPAT-IA` provenant du run `32878926411` ;
2. ouvrir PowerShell dans le dossier `ai` ;
3. exécuter `./llama-server.exe --version` puis `$LASTEXITCODE` ;
4. attendu pour ce nouvel artefact : sortie normale de version et code `0` ;
5. seulement si ce test est bon, lancer MEMS Manager puis ouvrir l’onglet IA ;
6. vérifier que le statut passe à **IA locale prête** et que l’application ne se ferme pas ;
7. si `--version` échoue encore, relever exactement le nouveau code avant toute autre modification ;
8. si `--version` fonctionne mais que MEMS Manager plante à l’ouverture de l’onglet, concentrer alors le diagnostic sur l’intégration `QProcess`/cycle de vie IA ;
9. ne pas modifier le protocole ECU, le 32 bits, Qwen3-0.6B-Q8_0 ou créer BUILD #30 pendant cette validation.

---

## PRINCIPE DIRECTEUR

**BUILD #26 reste la base x64 figée. BUILD #27 / v1.0.27 est le premier build x64 réellement testé sur AANMP002 avec connexion, polling 7D/80 et Injection RAM Mode4 fonctionnels. BUILD #28 / v1.0.28 reste le rollback x64 stable avant l’intégration complète de l’IA. BUILD #29 / v1.0.29 conserve Qwen3-0.6B-Q8_0 et llama.cpp b10516 mais utilise maintenant un runtime Windows x64 statique de compatibilité sans OpenSSL, après diagnostic PowerShell d’un `0xC0000135`. Le nouveau CI #29 est entièrement vert mais le nouvel artefact doit encore être testé sur le PC utilisateur avant toute déclaration de correction réelle. Toute commande susceptible de modifier l’ECU reste fail-closed si famille ou mode ne sont pas prouvés. La référence 32 bits reste intacte.**

---

## 16. BUILD #29 / v1.0.29 — IA LOCALE QWEN EMBARQUÉE — ÉTAT AU 25 AOÛT 2026

### Objectif autorisé

BUILD #29 ajoute l’IA locale complète en conservant :

- **Qwen3-0.6B-Q8_0** ;
- **llama.cpp b10516** ;
- fonctionnement local/offline sur `127.0.0.1:18089` ;
- langues `fr/en/es/it/pt/de` ;
- réponses déterministes ciblées pour batterie, état moteur, diagnostic et captures ;
- aucune modification volontaire du protocole ECU par ce lot IA.

Ne pas créer BUILD #30 sans autorisation explicite utilisateur.

### Self-test IA et CI principal

Un premier self-test IA a échoué sur `spanish targeted answer`. Cause identifiée : le self-test n’héritait pas de `/utf-8` sous MSVC. Correction minimale appliquée uniquement au self-test. Commit : `e65b67fea31ec8848ec70ec904e8f36e7eb7a005`. Le run suivant est devenu vert.

### Premier test PC réel du runtime officiel

Symptôme utilisateur : ouverture de l’onglet IA → statut `erreur IA locale`, message `Impossible de démarrer llama-server`, `QProcess: Process crashed`, code `-1073741819 / 0xC0000005`.

Ce résultat invalide la validation fonctionnelle locale malgré le CI vert. Les autres tests IA ont été arrêtés car ils n’avaient aucun intérêt tant que le serveur local ne démarrait pas réellement sur le PC utilisateur.

**Important : l’utilisateur a explicitement signalé que ce n’est pas à traiter comme un nouveau problème de DLL MSVC.** Le précédent BUILD #26 avec `0xC0000005` avait effectivement été résolu par packaging du runtime MSVC, mais le BUILD #29 embarque déjà le package x64 et la cause actuelle doit être prouvée séparément.

### Tentative backend CPU générique

Commit `83a2b922a5cfc38da5bfb467e17ef4775ac88d57` — `BUILD #29 force generic llama CPU backend`.

Le package officiel llama.cpp b10516 charge plusieurs backends CPU. Une tentative a conservé uniquement `ggml-cpu-x64.dll`. CI vert, mais **test utilisateur : crash identique `0xC0000005`**. Cette piste seule est donc insuffisante et ne doit pas être répétée sans nouvelle preuve.

### Runtime de compatibilité recompilé depuis llama.cpp b10516

Décision : conserver exactement llama.cpp b10516 et Qwen3-0.6B-Q8_0, mais recompiler un runtime CPU x64 conservateur :

- `BUILD_SHARED_LIBS=OFF` ;
- `GGML_NATIVE=OFF` ;
- `GGML_BACKEND_DL=OFF` ;
- `GGML_CPU_ALL_VARIANTS=OFF` ;
- `GGML_OPENMP=OFF` ;
- SSE4.2 / AVX / AVX2 / AVX_VNNI / BMI2 / AVX512 désactivés ;
- runtime statique, sans sélection dynamique de backend CPU.

Commit de création du workflow compat : `68c32f4fd76b3df3b3f9620b6a034f8307da9c31`.

Première erreur workflow : appel d’un `llama-server.vcxproj` supposé et inexistant (`MSB1009`). Correction commit `c95f7b26386cfa9f769ec5f2ce16cebf78a5661a`.

Deuxième erreur : compilation globale entraînant `llama-app`, qui échouait sur `build-info.h` et `arg.h`. Vérification du CMake officiel b10516 : `tools/server` n’est ajouté que lorsque `LLAMA_BUILD_TOOLS=ON`, tandis que `LLAMA_BUILD_APP` est une option séparée. Correction exacte :

- `LLAMA_BUILD_TOOLS=ON` ;
- `LLAMA_BUILD_SERVER=ON` ;
- `LLAMA_BUILD_APP=OFF` ;
- compilation ciblée `--target llama-server`.

Commit : **`6b0df9c34a456b7c59af00bcab0acf82abcd7b80`** — `BUILD #29 build only llama-server compatibility target`.

Run compat : **`32874029233` — SUCCESS COMPLET**.

Job : **`97887567623` — SUCCESS**.

Artefact : **`ECU-MEMS-Manager-x64-BUILD-29-v1.0.29-COMPAT-IA`**.

Artifact ID : **`9573723320`**.

Taille : **`743 102 004` octets**.

SHA256 : **`73ef0fd55d700608696623aa3960879cab4cbc17f75d6bd2244eac25e3f6b977`**.

### Test utilisateur PowerShell décisif sur ce premier package COMPAT

Le package COMPAT précédent a ensuite été testé manuellement depuis PowerShell, directement dans son dossier `ai`.

Commande :

```powershell
.\llama-server.exe --version
```

Observation réelle : **aucune sortie**, retour immédiat à l’invite PowerShell.

Commande suivante :

```powershell
$LASTEXITCODE
```

Résultat réel utilisateur :

```text
-1073741515
```

Conversion Windows : **`0xC0000135` = STATUS_DLL_NOT_FOUND / DLL requise introuvable**.

Cette observation est différente du premier `0xC0000005` vu via QProcess. Le test direct PowerShell permet de conclure que, sur ce package, **`llama-server.exe` échoue déjà seul avant le chargement de Qwen et avant l’intégration MEMS Manager**.

Ce test doit être conservé comme méthode standard de diagnostic des runtimes Windows de l’IA.

### Corrélation avec llama.cpp upstream

Recherche dans le dépôt officiel `ggml-org/llama.cpp` : issue **#19236**, intitulée `llama-server and llama-cli exit immediately without any output or error message`.

Éléments concordants :

- Windows ;
- `llama-server.exe --version` ou `--help` se ferme sans sortie ;
- un utilisateur rapporte exactement **`0xC0000135 (-1073741515)`** ;
- le premier mauvais commit identifié dans le ticket est lié au changement vers OpenSSL ;
- un utilisateur confirme que la présence des DLL OpenSSL corrige le problème dans sa configuration.

Le CMake de llama.cpp b10516 confirme que l’option réelle est :

`LLAMA_OPENSSL` — **ON par défaut**.

Le workflow COMPAT utilisait auparavant `LLAMA_BUILD_BORINGSSL=OFF`, option qui ne supprimait pas cette dépendance OpenSSL dans b10516.

### Correction appliquée au runtime COMPAT

Commit : **`1bbe9923a7a655dcf51d7cd73f1b0e46ad8f1fd0`** — `BUILD #29 make COMPAT IA runtime self-contained`.

Modification principale :

```text
-DLLAMA_OPENSSL=OFF
```

Justification : MEMS Manager utilise uniquement le serveur en HTTP local sur `127.0.0.1:18089`. Le support HTTPS/OpenSSL n’est donc pas nécessaire pour ce fonctionnement local.

Contrôles ajoutés :

- vérification `LLAMA_OPENSSL:BOOL=OFF` dans `CMakeCache.txt` ;
- inspection des imports PE de `llama-server.exe` ;
- rejet si import `libssl`, `libcrypto`, `ggml-*`, OpenMP ou runtime VC externe non empaqueté ;
- affichage explicite des imports directs de `llama-server.exe` dans le CI ;
- lancement de **l’exécutable réellement placé dans le package final** ;
- test `llama-server.exe --version` avec `PATH` réduit à Windows (`System32` + `SystemRoot`) afin d’éviter qu’une DLL installée sur le runner masque une dépendance non livrée ;
- le test exige un code de sortie `0` ;
- démarrage ensuite du serveur avec le Qwen emballé, `/health` et requête `/v1/chat/completions` ;
- smoke launch de MEMS Manager.

### Nouveau CI après suppression OpenSSL

Run : **`32878926411` — SUCCESS COMPLET**.

Job : **`97903478715` — SUCCESS**.

Toutes les étapes sont vertes, notamment :

- protections protocole ;
- compilation MEMS Manager x64 ;
- self-test IA déterministe ;
- ABI protocole ;
- base SQLite ;
- compilation statique conservatrice de llama.cpp b10516 ;
- téléchargement et SHA du Qwen3-0.6B-Q8_0 ;
- assemblage package ;
- **validation architecture/imports** ;
- **`llama-server.exe --version` du package avec PATH isolé** ;
- **démarrage réel du serveur et réponse Qwen locale** ;
- smoke launch MEMS Manager ;
- upload artefact.

Nouvel artefact : **`ECU-MEMS-Manager-x64-BUILD-29-v1.0.29-COMPAT-IA`**.

Artifact ID : **`9575488580`**.

Taille : **`743 059 094` octets**.

SHA256 : **`8c821b90e676507ca0e3d7cc53950fca5c2a3668fa9e05ee394094a91d8f78aa`**.

Expiration GitHub : `23 novembre 2026`.

### ÉTAT DE VALIDATION — NE PAS SURINTERPRÉTER LE CI VERT

**Le nouvel artefact `9575488580` n’a pas encore été testé sur le PC utilisateur.**

Le CI prouve que :

- le serveur final empaqueté n’a plus besoin d’OpenSSL ;
- il démarre avec un PATH Windows minimal sur le runner ;
- il charge Qwen3-0.6B-Q8_0 ;
- il répond localement ;
- MEMS Manager démarre en smoke test.

Le CI **ne prouve pas encore** que l’ouverture réelle de l’onglet IA fonctionne sur le PC utilisateur.

Verdict actuel : **BUILD #29 COMPAT IA / commit `1bbe9923...` = CI GO, validation PC réelle EN ATTENTE.**

---

## 17. MÉTHODE WINDOWS / POWERSHELL À UTILISER PLUS SYSTÉMATIQUEMENT

Le diagnostic BUILD #29 montre que PowerShell est particulièrement utile pour distinguer rapidement :

1. crash de l’exécutable runtime lui-même ;
2. dépendance Windows manquante ;
3. problème au chargement du modèle GGUF ;
4. problème d’intégration dans MEMS Manager/QProcess.

Séquence standard recommandée pour un futur incident IA Windows :

```powershell
.\llama-server.exe --version
$LASTEXITCODE
```

Interprétation actuelle connue :

- code `0` : runtime seul démarre correctement ;
- `-1073741515 / 0xC0000135` : dépendance DLL manquante ;
- autre code : relever exactement avant toute modification.

Seulement après un `--version` réussi : lancer le serveur avec le modèle et ses arguments réels. Seulement après ce deuxième test réussi : investiguer MEMS Manager/QProcess.

Cette méthode ne cible pas un PC particulier : elle sert à vérifier qu’un package Windows x64 est autonome et ne dépend pas silencieusement de logiciels installés sur la machine de compilation.

**Prochain verdict attendu : test du nouvel artefact `9575488580` sur le PC utilisateur. Tant que ce test n’est pas fait, ne pas écrire dans le rapport que le crash IA est corrigé matériellement.**
