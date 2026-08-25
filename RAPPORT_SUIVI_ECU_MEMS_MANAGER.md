# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager.
> Il constitue la source de vérité de continuité du projet.
> La branche `RAPPORT` sert uniquement au suivi/transmission ; le développement x64 se fait sur `MEMSX64`.

Dernière mise à jour : **25 août 2026 — BUILD #27 / v1.0.27 compilé vert et testé sur ECU réel AANMP002/MNE101150. Connexion, 7D/80 et Injection RAM Mode 4 validées. Défauts réels à corriger dans BUILD #28 / v1.0.28 : disparition d’Aperçu après Injection/Mode4, signe de correction ralenti chaud, déconnexion volontaire après coupure contact / reconnexion résiduelle.**

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Branche de sauvegarde x64 validée : **`MEMSX64-BUILD26-BASE`**.
- Référence matérielle 32 bits : **BUILD #14 — v1.0.14**.
- Référence x64 UI/IA/navigation figée : **BUILD #26 — v1.0.26**.
- BUILD x64 testé sur véhicule : **BUILD #27 — v1.0.27**.
- **Prochain lot de correction : BUILD #28 — v1.0.28.**
- Règle utilisateur à partir de ce stade : **un BUILD = une version** ; le prochain BUILD après #27 est donc obligatoirement **#28 / v1.0.28**.
- Un rerun d’un même commit/build ne change pas la version.
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
- `Aperçu` reste visible dans les essais de navigation de base ;
- passage IA MEMS / Mode interactif / Test ECU 1.9 / Aperçu validé ;
- package MSVC local présent.

**Ne plus modifier cette branche. Elle sert de retour arrière x64 fiable.**

### BUILD #27 — état final avant BUILD #28

Commit de départ :

`cbc1496a12a83da2d9e4ef2cda90dba3e873a166`

Message :

`Start BUILD #27 from validated BUILD #26 base`

Premier bloc sécurité protocole :

`bab88302036730089a89b597d0da2b4a5e4b242b`

Ce commit a échoué uniquement à la compilation de `memsinterface_dispatch.cpp` à cause de l’utilisation de `QVariant` sans inclusion de sa définition complète.

Correction minimale du même BUILD #27 :

`a6f9b209f32b6dd77774832e8c84469c53deca47`

Message :

`Fix BUILD #27 QVariant compile error`

Correction sémantique exacte : ajout de :

`#include <QVariant>`

Run GitHub après correction :

`32832192437` — **SUCCESS COMPLET**.

Job principal :

`97753152749` — **SUCCESS**.

Tous les contrôles x64 ont passé, notamment compilation application + DLL protocole, self-test ABI, self-test protocole famille/mode, SQLite, assemblage package, contrôles PE AMD64 et smoke launch.

Artefact testé :

`ECU-MEMS-Manager-x64-BUILD-27-v1.0.27`

Artifact ID :

`9557374893`

SHA256 artefact :

`d7dce4235ee2c6c23465f3399eed27f8d5d379cab341d803253dd7509fe8da9f`

### Validation PC sans véhicule du BUILD #27

- `v1.0.27 / BUILD #27` affiché ;
- 14 onglets présents ;
- séquence IA MEMS → Mode interactif → Test ECU 1.9 → Aperçu fonctionnelle avant connexion véhicule ;
- IA MEMS ne plante pas ;
- question date répond ;
- message `Moteur llama.cpp local absent du dossier IA` affiché proprement ;
- plusieurs fermetures/réouvertures du programme sans crash.

### Validation réelle véhicule du BUILD #27 — 25 août 2026

ECU / véhicule de référence de ce test :

- ECU : **AANMP002** ;
- référence affichée : **MNE101150** ;
- port : **COM3** ;
- interface détectée : **FTDI FT232** ;
- chemin protocole : **ROSCO 1.3/1.6** ;
- D0 : `98 00 02 02` ;
- D1 : ASCII `AANMP002` répété ;
- polling normal `7D/80` fonctionnel et stable ;
- enregistrement de mesures fonctionnel.

Mesures cohérentes observées dans l’enregistrement réel :

- ralenti généralement ≈ 1160–1200 tr/min au début de la séquence ;
- batterie ≈ 13,8 V moteur en marche ;
- MAP ≈ 31–34 kPa au ralenti ;
- variations régime/MAP/TPS cohérentes pendant les sollicitations.

Injection RAM Mode 4 réellement validée sur AANMP002 :

- entrée Mode 4 réussie ;
- `0x03C8 = 1314 ticks` ;
- `0x026E = 0 ticks` ;
- `0x0280 = 0` ;
- base injection affichée ≈ **2,63 ms** ;
- injection finale affichée ≈ **2,63 ms** ;
- programme resté actif, communication revenue utilisable.

**Conclusion BUILD #27 : connexion réelle, identification, polling 7D/80, logging et lecture RAM Injection Mode 4 sont validés sur AANMP002. Le BUILD #27 n’est toutefois pas considéré final à cause des défauts ci-dessous.**

### Défauts réels constatés pendant le test véhicule #27

1. **Aperçu disparaît de la sidebar après passage Injection / Mode 4.**
   - Avant Injection/Mode4, Aperçu est présent.
   - Après ouverture/usage Injection, la première ligne Aperçu disparaît et la sidebar commence par Injection.
   - Le défaut persiste ensuite sur les autres pages.
   - Il faut préserver les 14 entrées en permanence ; `navigationorderpatch.cpp` doit rester l’unique propriétaire de la navigation officielle.

2. **Signe incorrect dans le calcul de l’erreur de ralenti chaud corrigée.**
   - Règle projet obligatoire : `idle_error_hot_corrected = raw - 32768 - correction`.
   - La correction est la valeur réellement réglée dans l’onglet Réglages ; elle ne doit jamais être figée à `-3`.
   - Exemple réel : raw `32772`, correction `-3` ; le logiciel affiche `1`, ce qui correspond à une addition de la correction. La règle projet donne `7`.
   - Même comportement observé avec raw `32777` affiché `6`.

3. **Déconnexion volontaire imparfaite quand le contact ECU est déjà coupé.**
   - Séquence utilisateur réelle : moteur arrêté → contact coupé → clic `Déconnecter`.
   - Le bouton `Connecter` ne redevient pas orange/actif immédiatement.
   - Après débranchement physique du câble USB, `Connecter` redevient orange.
   - Impression utilisateur de clignotements/tentatives de reconnexion après déconnexion.
   - Le code #27 ne ferme pas directement la session au clic : `disconnectFromECU()` pose un drapeau et attend que la boucle worker sorte de `mems_read()`.
   - Le logiciel possède en plus un timer de reconnexion automatique de 3 s ; une course entre erreur de lecture, reconnexion déjà planifiée et clic volontaire doit être éliminée.
   - Exigence : une déconnexion volontaire doit terminer proprement même contact coupé, sans obliger à débrancher le câble, et ne doit jamais relancer automatiquement une connexion après cette demande volontaire.

4. **Qualité IA à améliorer, non bloquante pour le protocole.**
   - `ETAT MOTEUR ?` / `DIAGNOSTIQUE ?` peut répondre qu’il manque des éléments alors que les mesures sont disponibles.
   - `TENSION BATTERIE ?` retourne toutes les mesures au lieu d’une réponse ciblée.
   - À traiter après les trois défauts fonctionnels prioritaires ci-dessus, sauf correction locale sans risque.

Des détails visuels restent à revoir, notamment du texte serré/coupé dans certaines cartes Injection. **Ils restent différés.**

---

## 2. RÈGLE BUILD / VERSION ET MÉTHODE

Le numéro de BUILD est le numéro de version :

- BUILD #14 = v1.0.14
- BUILD #26 = v1.0.26
- BUILD #27 = v1.0.27
- **BUILD #28 = v1.0.28**
- BUILD #100 = v1.1.0
- BUILD #588 = v1.5.88
- BUILD #662 = v1.6.62

Formule : `1.(build / 100).(build % 100)`.

Règles actuelles :

1. un seul BUILD actif de développement à la fois ;
2. **à ce stade du projet : un BUILD = une version ; après #27 le prochain lot est #28 / v1.0.28** ;
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
13. CI vert signifie compilation/tests automatisés verts, pas validation ECU réel.

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

## 4. NAVIGATION 14 ONGLETS — RÉFÉRENCE ET NOUVEAU DÉFAUT #27

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

Cause historique de disparition d’Aperçu dans BUILD #26 : `visualcompletion.cpp::syncNavigation()` vidait/recréait `uiRebuildNav` pendant que `navigationorderpatch.cpp` en était déjà le propriétaire.

Correction historique :

`12fef48c68807bc59d2f45f9cd8d86d2a42856ca`

`Stop visual completion from rebuilding BUILD #26 navigation`

Le test véhicule du BUILD #27 montre toutefois un **nouveau déclenchement réel** : après passage sur Injection / Mode 4, Aperçu disparaît à nouveau de la sidebar. Ce cas doit être corrigé dans BUILD #28 sans revenir à une reconstruction concurrente de la navigation.

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

`mappedinjection_runtime_part4.inc` met explicitement le contexte central en :

- `Transition` avant changement de session ;
- `Normal` après confirmation session normale ;
- `Mode4` après entrée confirmée ;
- `Transition` avant restauration ;
- `Normal` après restauration réussie ;
- `Unknown` si restauration échoue.

Le lecteur RAM Injection reste read-only et continue d’exiger un profil firmware validé exact.

### Self-test intégré au build

`CMakeLists.txt` construit `protocol_context_selftest` et l’exécute en POST_BUILD de l’application.

### Commits du premier bloc

- `9b1e0e793ca1f18119d728a3741ed5c5dbbc7abe` — ajout politique ;
- `e6367ff43ef38b445c3d6c12d8915ca062fc8b3e` — premier self-test ;
- `522c7115cc444656421fb1e6d7138dfed02f49b8` — contexte dans MEMSInterface ;
- `2ca67aba4e530188c4c41b1c739da29a64d16746` — gardes centrales ;
- `66a2e88f8f544022892ad469465d4ebfec6e1f1b` — politique fail-closed renforcée ;
- `4978e7ce43a5deb299eac4bc1cb73f320467f1da` — test aligné ;
- `f01f2a47f89d24e0ac1f4fcb380038981355cdc3` — suivi central Mode4 Injection ;
- `87c40f3faeb4a8d5c0bc6be8fedccfce05c8a086` — self-test exécuté par le build ;
- `bab88302036730089a89b597d0da2b4a5e4b242b` — famille legacy laissée inconnue tant qu’elle n’est pas prouvée + transactions exclusives verrouillées ;
- `a6f9b209f32b6dd77774832e8c84469c53deca47` — correction compilation `QVariant` sans changement fonctionnel protocole.

Run final BUILD #27 :

`32832192437` — **SUCCESS**.

**Verdict : bloc de sécurité famille/mode compilé, self-test CI vert et utilisé pendant le test véhicule réel #27.**

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

À partir du BUILD #27, le self-test famille/mode est également exécuté pendant la compilation de l’application.

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

## 8. BLOQUEURS SÉCURITÉ ENCORE À TRAITER À PARTIR DE BUILD #28

Le test AANMP002 du #27 a permis de valider la lecture normale et l’Injection RAM, mais les points suivants restent NO-GO pour les essais concernés.

### Injecteurs 1.9/MPI

`rosco.h` distingue :

- `MEMS_TestInjectors = 0xF7` ;
- `MEMS_TestInjectorsMPi = 0xEF`.

Ne pas tester les injecteurs avant mapping exact famille/sous-type. Le verrou #27 les bloque volontairement.

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

Le test RAM AANMP002 possède sa propre whitelist et des confirmations F0 50 / F0 14 / F0 1E. Le verrou #27 empêche les commandes génériques/actionneurs de s’intercaler pendant `injectionRamTestRunning`.

Validation x64 réelle #27 : `0x03C8=1314`, `0x026E=0`, `0x0280=0`, injection ≈ 2,63 ms.

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

Résultat réel #27 : **COM3 / FTDI FT232 détecté et connexion ROSCO réussie sur AANMP002.**

Avant test 1.9 : minimum obligatoire = **ne pas envoyer de séquences ECU sur tous les ports série**.

---

## 13. IA LOCALE APRÈS VALIDATION DU CŒUR X64

Runtime de référence :

- llama.cpp Windows x64 CPU ;
- `llama-server.exe` x64 ;
- Qwen3-0.6B-Q8_0 GGUF ;
- HTTP `127.0.0.1:18089`.

Le package x64 #27 omet runtime/modèle lourds. L’onglet IA + moteur expert + fallback fonctionnent sans crash ; le message d’absence llama est propre.

Réintégrer llama/Qwen **après levée des bloqueurs protocole et validation cœur ECU x64**.

---

## 14. PLAN TEST MATÉRIEL X64 — ÉTAT APRÈS BUILD #27

### Phase A — lecture seule

**VALIDÉE sur AANMP002/MNE101150 avec BUILD #27 :** interface/port, connexion, identification, polling normal, logging.

### Phase B — comparaison BUILD #14

**Partiellement validée :** AANMP002/MNE101150, RPM, MAP, batterie, mesures et logging cohérents. Déconnexion reste à corriger dans #28.

### Phase C — Injection RAM Mode4

**VALIDÉE sur AANMP002 avec BUILD #27** pour profil exact et lecteur dédié read-only : entrée Mode4, lecture 03C8/026E/0280, valeur injection ≈2,63 ms. Le cas UI où Aperçu disparaît après cette séquence doit être corrigé dans #28.

### Phase D — MEMS1.9

Toujours NO-GO pour test complet : W4, parser longueur, sélection port/interface et reconnexion dédiée à traiter.

### Phase E — actionneurs

Toujours NO-GO : famille exacte + mode exact + commande exacte + timeout/failsafe à établir avant essai.

---

## 15. PROCHAINE ACTION EXACTE

**La prochaine version est BUILD #28 / v1.0.28.**

Ordre de travail immédiat autorisé le 25 août 2026 :

1. partir du HEAD BUILD #27 validé/compilé `a6f9b209f32b6dd77774832e8c84469c53deca47` sur `MEMSX64` ;
2. passer le lot suivant à **BUILD #28 / v1.0.28** conformément à la règle utilisateur « un build = une version » ;
3. corriger en priorité la **déconnexion volontaire après coupure contact** et empêcher toute reconnexion automatique résiduelle après clic Déconnecter ;
4. corriger le **signe du calcul ralenti chaud** selon `raw - 32768 - correction` en utilisant la correction réellement réglée ;
5. corriger la **disparition d’Aperçu après Injection/Mode4** sans recréer une seconde autorité de navigation ;
6. ajouter/adapter des contrôles automatisés ciblés si possible pour empêcher ces trois régressions ;
7. pousser sur `MEMSX64` et vérifier le workflow GitHub Actions ;
8. ne pas toucher `MEMSX64-BUILD26-BASE` ni `lab-expert-engine` ;
9. ne pas lancer d’essai actionneur, reset, clear faults ou écriture ECU tant que les garde-fous restants ne sont pas levés ;
10. après BUILD #28 vert en CI, fournir l’artefact pour nouveau test PC/véhicule ciblé.

Les améliorations IA de formulation et les détails purement visuels ne doivent pas détourner ce lot des trois défauts fonctionnels reproduits.

---

## PRINCIPE DIRECTEUR

**BUILD #26 reste la base x64 figée. BUILD #27 / v1.0.27 est maintenant documenté comme premier build x64 testé sur ECU réel AANMP002 avec connexion, polling 7D/80 et Injection RAM Mode4 fonctionnels. Le prochain lot est BUILD #28 / v1.0.28 et doit corriger d’abord les trois défauts reproduits : déconnexion/reconnexion résiduelle, signe ralenti chaud et disparition d’Aperçu après Injection/Mode4. Toute commande susceptible de modifier l’ECU reste fail-closed si famille ou mode ne sont pas prouvés. La référence 32 bits reste intacte.**