# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.

Dernière mise à jour : **25 août 2026 — BUILD #26 FIGÉ ET VALIDÉ ; BUILD #27 v1.0.27 ACTIF ET VALIDÉ COMME BASE ; PREMIER VERROU PROTOCOLE FAMILLE/MODE EN COURS DE VALIDATION CI.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Branche de sauvegarde x64 validée : **`MEMSX64-BUILD26-BASE`**.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- Référence x64 UI/IA/navigation figée : **BUILD #26 — v1.0.26**.
- **BUILD x64 actif : BUILD #27 — v1.0.27.**
- Ne pas créer BUILD #28 pour corriger #27 : #27 reste le build actif jusqu’à validation de ses travaux protocole.
- Le numéro BUILD = version logiciel ; ne jamais incrémenter pour un simple essai/correctif.
- Workflow x64 unique : `.github/workflows/memsx64.yml`.
- Le workflow compile le commit exact qui le déclenche.
- `GITHUB_RUN_NUMBER` ne décide pas de la version logicielle.

### BUILD #26 figé

Branche :

`MEMSX64-BUILD26-BASE`

Commit figé :

`12fef48c68807bc59d2f45f9cd8d86d2a42856ca`

Message :

`Stop visual completion from rebuilding BUILD #26 navigation`

Run GitHub :

`32816285887` — **SUCCESS**.

Validation réelle utilisateur :

- IA MEMS ne plante plus ;
- base IA runtime prête ;
- navigation 14 onglets stable ;
- `Aperçu` reste visible ;
- passage IA MEMS / Mode interactif / Test ECU 1.9 / Aperçu validé ;
- package MSVC local présent.

**Ne plus modifier cette branche. Elle sert de retour arrière x64 fiable.**

### BUILD #27 actif — base validée avant protocole

Commit de départ :

`cbc1496a12a83da2d9e4ef2cda90dba3e873a166`

Message :

`Start BUILD #27 from validated BUILD #26 base`

Run GitHub :

`32822623214` — **SUCCESS**.

Artefact :

`ECU-MEMS-Manager-x64-BUILD-27-v1.0.27`

SHA256 :

`b65b2c5af2c5a022ba35b945e5e3257257acea77eb01a308c656c8f0ca92a301`

Validation utilisateur réelle le 25 août 2026 sur un autre PC :

- `v1.0.27` affiché ;
- les 14 onglets sont présents et ouvrent les bonnes pages ;
- `Aperçu` reste visible ;
- sidebar réduite et développée fonctionnelles ;
- IA MEMS s’ouvre et répond sans crash ;
- question test « quel jour sommes nous ? » répond correctement ;
- absence de llama local signalée proprement ;
- Aperçu, Injection, Réglages, Toutes les données, IA MEMS et Test ECU 1.9 vérifiés par captures.

Des détails visuels restent à revoir, notamment du texte serré/coupé dans certaines cartes Injection. **Ils sont volontairement différés : priorité actuelle au protocole et à la sécurité ECU.**

---

## 2. RÈGLE BUILD / VERSION ET MÉTHODE

Le numéro de BUILD est le numéro de version :

- BUILD #14 = v1.0.14
- BUILD #26 = v1.0.26
- BUILD #27 = v1.0.27
- BUILD #100 = v1.1.0
- BUILD #588 = v1.5.88
- BUILD #662 = v1.6.62

Formule : `1.(build / 100).(build % 100)`.

Règles :

1. un seul BUILD actif de développement à la fois ;
2. un build rouge se corrige sans changer de numéro ;
3. un rerun GitHub n’est pas un nouveau BUILD ;
4. ne jamais utiliser `GITHUB_RUN_NUMBER` comme version ;
5. x64 uniquement sur `MEMSX64` ;
6. rapport uniquement sur `RAPPORT` ;
7. ne pas modifier `lab-expert-engine` pour résoudre un problème x64 ;
8. ne pas empiler de workflows temporaires ;
9. ne pas neutraliser une fonction pour masquer un crash ;
10. identifier une cause concrète avant modification ;
11. ne pas revenir sur une piste éliminée sans nouvelle preuve ;
12. CI vert signifie compilation/tests automatisés verts, pas validation ECU réel.

Commits de discipline historiques :

- `522fae53cb1573a956ce50941d5a185a4d245e66` — nettoyage CI x64 vers un seul workflow ;
- `3c4102eca34a2426970ee03e01830a6317b9db07` — BUILD/version explicite, suppression de la dépendance à `GITHUB_RUN_NUMBER`.

---

## 3. INCIDENT IA X64 BUILD #26 — DIAGNOSTIC ET CORRECTION

### Symptôme réel

Sur PC utilisateur : clic sur IA MEMS => fermeture immédiate du programme.

Avant correction :

- aucun nouveau `ia_mems_reference_r20.sqlite` ;
- aucun `.tmp` ;
- aucun `.lock` ;
- le crash survenait avant le démarrage utile du cache IA.

Artefact ayant reproduit le défaut :

`sha256:0e78c77bf9b7171412e7986e5a50542d261aa41f9203c9758610e86d82a0e636`

Il correspondait bien au commit `eb762546584567b705cd7beb88c13b5da18008ed` : ce n’était pas un ancien ZIP.

### Pistes éliminées

Ne pas recommencer sans nouvelle donnée :

- bloc 1600 ;
- cache utilisateur `.sqlite` ancien ;
- absence du dossier `ai` ;
- ExpertRuntimeDatabase x64 génériquement cassé ;
- simple conflit QSQLITE ;
- UTF-8 ;
- wrapper IA seul.

Self-test ExpertRuntimeDatabase x64 :

- run `32721284999` ;
- job `97413054422` ;
- `FIRST_BUILD=PASS` en 1,89 s ;
- `REUSE_CACHE=PASS` en 0,04 s ;
- lecteur de connaissances + ExpertEngine également exercés.

Le bloc `database/reference/research_enrichment_1600.qz64` est physiquement livré mais explicitement exclu de l’import runtime IA. Il n’est pas la cause du crash.

Test cache utilisateur : ancien `ia_mems_reference_r20.sqlite` renommé en `.OLD` ; crash inchangé ; aucun nouveau cache créé avant correction runtime.

### Donnée décisive Windows

Journal Windows, plusieurs crashs identiques :

- module fautif : **`MSVCP140.dll`** ;
- exception : **`0xc0000005`** ;
- violation d’accès mémoire ;
- offset : **`0x12EB0`**.

Le package ne contenait pas :

- `MSVCP140.dll` ;
- `VCRUNTIME140.dll` ;
- `VCRUNTIME140_1.dll`.

Windows utilisait donc le runtime de `C:\Windows\System32`.

### Correction retenue

Commit :

`66fe69db556d83df56aa6ddd968cb48129cbcf95`

Message :

`Package MSVC x64 runtime in BUILD #26`

Le workflow copie maintenant explicitement le runtime MSVC x64 du toolchain Visual Studio utilisé par le runner et vérifie la présence/architecture des DLL.

Run : `32814736178` — **SUCCESS**.

Résultat PC réel :

- plus de crash IA ;
- base prête en lecture seule ;
- absence llama proprement signalée ;
- nouveau `ia_mems_reference_r20.sqlite` recréé avec succès ;
- ancien `.OLD` supprimable.

**Verdict : crash IA x64 corrigé et validé.**

---

## 4. NAVIGATION 14 ONGLETS — VALIDÉE

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

Commit `a005501c94d7e5c949926a7fa527690804994154` : mapping direct ligne sidebar ↔ index fixe du QTabWidget, clé MEMS1.9 corrigée vers `19000`.

Cause de la disparition d’Aperçu : `visualcompletion.cpp::syncNavigation()` vidait/recréait `uiRebuildNav` pendant que `navigationorderpatch.cpp` en était déjà le propriétaire.

Correction finale :

`12fef48c68807bc59d2f45f9cd8d86d2a42856ca`

`Stop visual completion from rebuilding BUILD #26 navigation`

`visualcompletion.cpp` ne reconstruit plus la sidebar ; `navigationorderpatch.cpp` est le seul propriétaire des 14 entrées.

Run `32816285887` : **SUCCESS**.

Validation réelle #26 puis #27 : **Aperçu reste visible et navigation fonctionnelle.**

---

## 5. PREMIER BLOC SÉCURITÉ PROTOCOLE — BUILD #27

### Objectif

Mettre en place un contexte central :

`{ famille ECU, mode diagnostic }`

avant toute commande qui peut modifier l’ECU.

Raison : un même octet change de sens selon le mode/famille.

Exemples critiques :

- `D1` : identification en normal ; programmation RAM calibration vers ROM en Mode 4 ;
- `D3` : recodage / programmation calibration selon contexte ;
- `F7` : injecteur SPi ou fonction calibration Mode 4 ;
- `F8` : bobine en normal, écriture calibration complète en Mode 4 ;
- `F4` : heartbeat/all-actuators-off ou transition de mode selon contexte.

### Fichiers introduits

`protocolcontext.h`

Enums :

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ;
- `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.

Politique centrale `MemsProtocolSafety` :

- `F0` autorisé comme lecture de mode ;
- D0/D1/D2 autorisés uniquement en session normale ;
- D1 refusé en Mode 4 ;
- D3/F3/F4/F5 refusés depuis l’interface générique ;
- toute mutation exige famille prouvée `Rosco13_16` + mode `Normal` ;
- famille inconnue => mutation refusée ;
- MEMS1.9 => mutations/actionneurs refusés tant que sa table spécifique n’est pas prouvée ;
- injecteurs `F7` et `EF` restent bloqués tant que le sous-type exact n’est pas établi.

`protocolcontext_test.cpp`

Self-test couvrant notamment :

- Mode4 + D1 => refus ;
- Mode4 + D3 => refus ;
- Mode4 + F8 => refus ;
- D3 générique => refus ;
- F3/F4/F5 génériques => refus ;
- famille inconnue + actionneur => refus ;
- transition + actionneur => refus ;
- MEMS1.9 + mutation => refus ;
- F7/EF injecteur => refus tant que famille/sous-type non prouvés ;
- D1 read-only reste disponible en mode Normal pour identifier la famille.

### Intégration MEMSInterface

`memsinterface.h` conserve obligatoirement :

`void onProtocolCommandRequested(quint8 command);`

et ajoute deux états atomiques : famille ECU et mode diagnostic.

`memsinterface_dispatch.cpp` enveloppe le code historique sans modifier le transport C historique :

- appels `mems_test_actuator` passent par une garde centrale ;
- IAC passe par garde ;
- clear faults passe par garde ;
- reset adjustments passe par garde ;
- reset ECU passe par garde ;
- commande brute ECU/ROSCO passe par une whitelist read-only ;
- les transactions RAM Injection exclusives bloquent tout envoi générique/actionneur concurrent.

Une connexion réussie par le chemin ROSCO **ne suffit plus à prouver 1.3/1.6** : la famille reste `Unknown` tant qu’une identification fiable n’est pas établie. C’est volontairement fail-closed.

La voie 1.9 ayant réussi le wake-up ISO 9141 est marquée `Mems19`, mais ses mutations restent bloquées.

### Suivi Mode 4 Injection

`mappedinjection_runtime_part4.inc` met maintenant explicitement le contexte central en :

- `Transition` avant changement de session ;
- `Normal` après confirmation session normale ;
- `Mode4` après entrée confirmée ;
- `Transition` avant restauration ;
- `Normal` après restauration réussie ;
- `Unknown` si restauration échoue.

Le lecteur RAM Injection reste read-only et continue d’exiger un profil firmware validé exact.

### Self-test intégré au build

`CMakeLists.txt` construit `protocol_context_selftest` et l’exécute en POST_BUILD de l’application. Un échec de la matrice famille/mode doit donc rendre le BUILD #27 rouge.

### Commits de ce premier bloc

Historique de mise en place :

- `9b1e0e793ca1f18119d728a3741ed5c5dbbc7abe` — ajout politique ;
- `e6367ff43ef38b445c3d6c12d8915ca062fc8b3e` — premier self-test ;
- `522c7115cc444656421fb1e6d7138dfed02f49b8` — contexte dans MEMSInterface ;
- `2ca67aba4e530188c4c41b1c739da29a64d16746` — gardes centrales ;
- `66a2e88f8f544022892ad469465d4ebfec6e1f1b` — politique fail-closed renforcée ;
- `4978e7ce43a5deb299eac4bc1cb73f320467f1da` — test aligné ;
- `f01f2a47f89d24e0ac1f4fcb380038981355cdc3` — suivi central Mode4 Injection ;
- `87c40f3faeb4a8d5c0bc6be8fedccfce05c8a086` — self-test exécuté par le build ;
- `bab88302036730089a89b597d0da2b4a5e4b242b` — famille legacy laissée inconnue tant qu’elle n’est pas prouvée + transactions exclusives verrouillées.

Run associé au dernier commit :

`32828707613`

**État au moment de cette mise à jour : en cours. Ne pas déclarer ce bloc vert avant conclusion `success`.**

---

## 6. 15 CONTRÔLES X64 DE BASE

Le workflow vérifie :

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
14. SQLite + ressources + runtime MSVC ;
15. tous les PE finaux AMD64.

À partir du premier bloc #27, le self-test famille/mode est également exécuté pendant la compilation de l’application.

---

## 7. DLL PROTOCOLE X64 / CARTOGRAPHIE

DLL : `mems_manager_x64.dll`.

Sources : `librosco-x64/`.

ABI à préserver :

- `frame80 = 28` octets ;
- `frame7d = 32` octets ;
- `mems_data = 60` octets ;
- 22 exports historiques exacts.

Commandes historiques :

- init : `CA 75 F4 D0` ;
- polling : `0x80` puis `0x7D` ;
- IAC : `0xFB` / `FD` / `FE` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : `0xFA` ;
- reset adjustments : `0x0F`.

Principe : **la signification d’un octet n’est jamais considérée universelle ; famille + mode sont nécessaires.**

---

## 8. BLOQUEURS SÉCURITÉ ENCORE À TRAITER DANS BUILD #27

Même après le premier verrou famille/mode, le test ECU complet reste NO-GO tant que les points suivants ne sont pas traités.

### Injecteurs 1.9/MPI

`rosco.h` distingue :

- `MEMS_TestInjectors = 0xF7` ;
- `MEMS_TestInjectorsMPi = 0xEF`.

Ne pas tester les injecteurs avant mapping exact famille/sous-type. Le premier verrou #27 les bloque volontairement.

### 7D/80 MEMS1.9

Le parser historique utilise tailles fixes 28/32 octets. La documentation indique des longueurs pouvant varier selon génération/profil.

À corriger avant lecture 1.9 validée : parser longueur réelle/per-famille.

### W4 ISO 9141 / 5 bauds

Le wake-up actuel écrit le complément du second key byte immédiatement après réception. ISO W4 attendu : **25–50 ms**.

Ajouter délai monotone explicite + test CI.

### Reconnexion MEMS1.9

Prévoir stratégie dédiée ; après session cassée une coupure contact ~15 s peut être nécessaire. Ne pas considérer 450 ms/3 s comme universel.

### Actionneurs

Ajouter timeout global/failsafe, suivi ON/OFF et perte communication. `F4` ne doit pas être traité comme OFF universel hors contexte prouvé.

### Commandes altérant l’ECU

Clear faults, resets, trims, réglages et actionneurs doivent avoir garde contexte + action utilisateur claire. Clear faults doit recevoir une confirmation UI appropriée.

### Signaux de résultat

Séparer succès, échec et simple fin d’opération. Ne pas afficher vert sur une fin d’appel non prouvée réussie.

### Ports série

Le dispatcher actuel peut encore parcourir tous les ports détectés. **À corriger avant test 1.9 : ne jamais envoyer de séquence ECU à un périphérique série arbitraire.**

### Profils RAM 1.9

164 profils corrélés ne signifient pas 164 profils testés sur ECU réel. Conserver statuts distincts : corrélé statiquement / validé matériellement / ECU-version-date.

---

## 9. RÉFÉRENCE MATÉRIELLE 32 BITS À CONSERVER

Branche : `lab-expert-engine`.

BUILD #14 — v1.0.14, retesté le 24 août 2026 sur ECU réel :

- COM3 ;
- firmware AANMP002 ;
- ID AANMP002 / MNE101150 ;
- communication verte ;
- polling 7D/80 fonctionnel ;
- navigation stable ;
- IA stable connecté/déconnecté ;
- IA locale llama-server x64 séparé ;
- Qwen3-0.6B-Q8_0 ;
- HTTP local 18089.

Injection RAM Mode 4 :

- transition 7D/80 → Mode4 fonctionnelle ;
- retour Mode4 → diagnostic normal fonctionnel ;
- injection exemple ≈ 2,47 ms ;
- `0x03C8 = 1233` ;
- `0x026E = 0` ;
- `0x0280 = 0`.

Trace : `2026-08-24_18.14.txt`.

**Ne pas modifier cette branche pour résoudre le x64.**

---

## 10. INJECTION / RAM — RÈGLES À NE PAS PERDRE

- Ne pas calculer le temps d’injection depuis le polling normal `0x7D/0x80`.
- Adresses importantes : `0x03C8`, `0x026E`, `0x0280`.
- Lecteur Mode4 dédié read-only.
- Sélection bloc `DC`, puis offsets lecture `0x00–0x7F`.
- Ne pas utiliser les plages calibration `0x80–0xBF`.
- Firmware sans profil exact => refus avant Mode4.
- Échec restauration normal => polling arrêté.
- Injection reste entre Aperçu et Réglages.
- Dwell/temps bobine reste dans Injection.
- Sous-vues Injection doivent partager le même mode de lecture pour éviter du polling supplémentaire.

Le test RAM AANMP002 possède déjà sa propre whitelist et des confirmations F0 50 / F0 14 / F0 1E. Le premier verrou #27 empêche désormais les commandes génériques/actionneurs de s’intercaler pendant `injectionRamTestRunning`.

---

## 11. BASE MEMS / BLOC 1600

Lot : `database/reference/research_enrichment_1600.qz64`.

Environ 33,3 Mo compressés et plusieurs millions de cellules de corrélation.

Ne pas le charger automatiquement sans mesurer RAM/temps/utilité.

Point établi : **ExpertRuntimeDatabase exclut explicitement le bloc 1600 de l’import cache IA.**

Base principale :

`<appdir>/database/ecu_mems_manager.sqlite`

Cache IA runtime :

`%LOCALAPPDATA%\ECU Mems Manager\ECU Mems Manager\ia-mems\ia_mems_reference_r20.sqlite`

Ce cache a été recréé avec succès sur PC réel après correction runtime MSVC.

---

## 12. DÉTECTION AUTOMATIQUE CÂBLE / COM — OBJECTIF

Exigence générale MEMS, pas seulement 1.9 :

- détecter automatiquement interface + COM ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- plusieurs interfaces => COM + type clair ;
- au clic Connecter, valider interface/protocole ;
- messages : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Avant test 1.9 : minimum obligatoire = **ne pas envoyer de séquences ECU sur tous les ports série**.

---

## 13. IA LOCALE APRÈS VALIDATION DU CŒUR X64

Runtime de référence :

- llama.cpp Windows x64 CPU ;
- `llama-server.exe` x64 ;
- Qwen3-0.6B-Q8_0 GGUF ;
- HTTP `127.0.0.1:18089`.

Le package x64 courant omet volontairement runtime/modèle lourds. L’onglet IA + moteur expert + fallback doivent fonctionner sans eux, ce qui est maintenant validé.

Réintégrer llama/Qwen **après levée des bloqueurs protocole et validation cœur ECU x64**.

---

## 14. PLAN TEST MATÉRIEL X64 APRÈS LEVÉE DU NO-GO

### Phase A — lecture seule

- interface/port explicitement validé ;
- connexion ;
- identification ;
- polling normal ;
- aucune écriture/actionneur/reset/réglage.

### Phase B — comparaison BUILD #14

Comparer AANMP002/MNE101150, RPM, MAP, batterie, températures, logging, déconnexion.

### Phase C — Injection RAM Mode4

Seulement profil exact et lecteur dédié read-only ; retour normal obligatoire avant reprise polling.

### Phase D — MEMS1.9

Seulement après W4, parser longueur, sélection port/interface et reconnexion dédiée.

### Phase E — actionneurs

Seulement après famille exacte + mode exact + commande exacte + timeout/failsafe.

---

## 15. PROCHAINE ACTION EXACTE

1. Attendre conclusion du run `32828707613` sur le commit `bab88302036730089a89b597d0da2b4a5e4b242b`.
2. Si rouge : corriger **le même BUILD #27**, sans créer #28.
3. Si vert : consigner le résultat exact et le self-test famille/mode.
4. Ensuite établir la **classification fiable de famille ECU** à partir d’éléments prouvés, afin de ne jamais confondre un ECU 1.9 déjà réveillé avec un 1.3/1.6.
5. Puis traiter le mapping injecteur F7/EF.
6. Puis W4 25–50 ms, parser 1.9 variable, reconnexion 1.9, ports série, timeouts actionneurs et confirmations.
7. Ne pas retoucher l’UI maintenant sauf défaut bloquant.
8. Ne pas toucher `MEMSX64-BUILD26-BASE` ni `lab-expert-engine`.

---

## PRINCIPE DIRECTEUR

**BUILD #26 est une base x64 figée et validée. BUILD #27 est la branche active, validée comme copie fonctionnelle du #26 avant travaux protocole. Toute commande susceptible de modifier l’ECU doit désormais échouer fermée si famille ou mode ne sont pas prouvés. Le test ECU complet reste NO-GO tant que les autres bloqueurs 1.9/ports/actionneurs ne sont pas levés. Les défauts purement visuels sont différés.**
