# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager. Il constitue la source de vérité de continuité du projet.
>
> Branche rapport : `RAPPORT`. Branche de développement x64 : `MEMSX64`.
>
> **RÈGLE DE SUIVI IMMÉDIAT : AVANT CHAQUE NOUVELLE ÉTAPE, inscrire ici l’étape exacte et son objectif. Dès que cette étape produit un résultat, inscrire immédiatement ce résultat AVANT de commencer l’étape suivante. Ne jamais attendre la fin d’un lot, d’un build ou d’une discussion.**

## JOURNAL IMMÉDIAT — 25 août 2026

### Étape terminée : recherche externe sur l’architecture propre d’une IA locale dans ECU MEMS Manager x64

- Documentation et implémentations comparées : `llama.cpp` / `llama-server`, Qt `QProcess`, `QNetworkAccessManager` et SQL/threading, Jan, LM Studio, Ollama, Windows process isolation / Job Objects, SQLite read-only, Qwen3.
- `llama-server` est conçu comme serveur HTTP local séparé. `/health` retourne 503 pendant le chargement puis 200 une fois prêt.
- Jan, LM Studio et Ollama convergent vers la même séparation GUI ↔ moteur local.
- Qt convient à ce schéma avec `QProcess` + `QNetworkAccessManager` asynchrones.
- Une connexion `QSqlDatabase` doit rester dans le thread qui l’a créée.
- Les architectures RAG robustes séparent ingestion/indexation et question/réponse.
- Windows isole les espaces mémoire des processus : un `llama-server.exe` séparé ne doit pas pouvoir corrompre directement la mémoire de `ecu_mems_manager.exe`.
- Le `LocalAiClient` actuel suit déjà le bon principe HTTP/QProcess, mais l’onglet IA possède encore le cycle de vie du serveur et son `showEvent()` déclenche en parallèle `ExpertRuntimeDatabase::buildOrOpen()` et l’initialisation/chargement Qwen.

### Architecture IA x64 retenue

1. **Moteur hors processus** : conserver `llama-server.exe` comme sidecar x64 séparé. Ne pas lier `libllama/ggml` directement dans `ecu_mems_manager.exe`.
2. **Service applicatif unique** : un gestionnaire de type `AiServiceManager`/`LocalAiService`, possédé au niveau application/MainWindow, lance et supervise un seul serveur. L’onglet `IA MEMS` devient une vue/client et ne possède plus le moteur.
3. **Cycle d’état explicite** : `Stopped -> Starting -> Loading -> Ready -> Busy -> Error -> Stopping`. Démarrage idempotent, jamais deux serveurs concurrents.
4. **Loopback privé** : `127.0.0.1` uniquement, pas `0.0.0.0`, serveur sans WebUI/outils/agents/MCP.
5. **Authentification et identité** : clé API de session ; après `/health`, vérifier `/v1/models` et l’alias `ia-mems` avant de déclarer le service prêt. Ne jamais adopter implicitement un processus inconnu présent sur le port.
6. **Confinement Windows** : attacher le sidecar possédé à un Job Object avec `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` pour éviter tout `llama-server.exe` orphelin si MEMS Manager quitte brutalement.
7. **Journal IA séparé** : journal moteur sous les données locales pour distinguer erreurs moteur, HTTP, base experte et GUI.
8. **Base experte de production = OPEN ONLY** : construire/valider `ia_mems_reference_r20.sqlite` dans GitHub Actions, l’emballer dans `database/expert/`, puis au runtime l’ouvrir seulement avec `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`. Aucune décompression QZ64 ni reconstruction SQL lourde au clic IA.
9. **RAG borné** : `mesures ECU en lecture seule -> ExpertEngine/ExpertKnowledgeReader -> faits pertinents -> contexte -> Qwen -> texte`. Le LLM ne reçoit jamais de capacité de commande ECU.
10. **Qwen3** : conserver Qwen3-0.6B-Q8_0. Après stabilisation, préférer `chat_template_kwargs: {"enable_thinking": false}` au texte `/no_think`.
11. **Package x64** : `ecu_mems_manager.exe`, `mems_manager_x64.dll`, Qt + `sqldrivers/qsqlite.dll`, `ai/llama-server.exe` et ses dépendances x64 nécessaires, `ai/models/ia-mems.gguf`, manifeste IA/hashes, `database/expert/ia_mems_reference_r20.sqlite`.
12. **Validation CI** : base r20 générée et intègre, fichiers/hashes, `llama-server --version`, chargement Qwen, `/health`, `/v1/models=ia-mems`, vraie completion, smoke GUI, crash volontaire serveur sans crash GUI, fermeture GUI sans sidecar orphelin, aucune reconstruction DB en production.

### Décision utilisateur : ouverture officielle BUILD #30

- **L’utilisateur a explicitement demandé le 25 août 2026 de créer BUILD #30 pour réaliser cette intégration propre.**
- BUILD actif devient **BUILD #30 / v1.0.30**.
- BUILD #29 reste l’historique expérimental qui a permis d’isoler les problèmes et de fixer l’architecture ; il n’est plus le build cible de production IA.
- Base de départ code pour BUILD #30 : `MEMSX64` HEAD `fee195e88d3615613b8f92de83209da2cf8247c2` tant que le premier commit #30 n’a pas encore été poussé.
- Ne pas toucher à `lab-expert-engine` ni à `MEMSX64-BUILD26-BASE`.
- Ne pas modifier le protocole ECU dans le lot IA #30.

### Nouvelle exigence utilisateur BUILD #30 : désinstallation propre

- L’utilisateur signale que les tests s’empilent sur le PC et qu’il ne voit plus de désinstallation disponible.
- Le source `uninstaller.cpp` existe encore et CMake déclare déjà `ecu_mems_uninstaller` sous Windows.
- BUILD #30 doit **livrer réellement `ecu_mems_uninstaller.exe` dans l’artefact**, avec `install_manifest.txt` cohérent.
- Le désinstalleur doit supprimer les fichiers installés par le package et le raccourci, préserver par défaut le profil/données utilisateur, et ne supprimer les données locales que si l’utilisateur le choisit explicitement.
- Il doit refuser de désinstaller si `ecu_mems_manager.exe` est encore actif.
- Les fichiers étrangers ajoutés manuellement dans le dossier d’installation doivent rester préservés ; le dossier racine ne doit être supprimé que s’il devient vide.

### Étape terminée : inspection exacte du HEAD #29 avant modification BUILD #30

- **Résultat packaging base :** le checkout contient la vraie source experte sous `database/reference/` avec `manifest.json`, `mems_reference_seed_*.qz64` et les `research_enrichment_*.qz64`. Le chemin `expert/base_connaissance_ia` utilisé par le workflow COMPAT #29 n’existe pas. Le rouge #29 est donc bien un défaut de chemin de staging/packaging.
- **Résultat désinstalleur :** `uninstaller.cpp` existe et le CMake crée déjà la cible Windows `ecu_mems_uninstaller`. Le workflow #29 construit explicitement `ecu_mems_manager` et `rosco_abi_selftest`, mais pas `ecu_mems_uninstaller`, et l’assemblage du package ne copie pas ce binaire. Le défaut utilisateur vient du packaging, pas d’une disparition du code de désinstallation.
- **Résultat comportement désinstalleur :** le code existant repose sur `install_manifest.txt`, refuse d’agir si `ecu_mems_manager.exe` est actif, préserve le profil par défaut, peut supprimer les données locales sur choix explicite, supprime seulement les fichiers listés et ne retire les dossiers que s’ils deviennent vides. Il correspond donc au comportement demandé et doit surtout être construit/emballé avec un manifeste complet.
- **Résultat IA :** `IaMemsTab` contient encore `LocalAiClient *m_localAi` et crée le client avec le widget comme parent ; `showEvent()` programme simultanément `startKnowledgeLoad()` et `LocalAiClient::initialize()` ; `startKnowledgeLoad()` crée un `QThread` qui appelle `ExpertRuntimeDatabase::buildOrOpen()`. C’est exactement la divergence architecturale identifiée par la recherche externe.
- **Résultat base runtime :** `ExpertRuntimeDatabase::buildOrOpen()` préfère déjà `database/expert/ia_mems_reference_r<revision>.sqlite`. Si cette base est réellement présente dans le package, il n’effectue pas la reconstruction lourde. Le BUILD #30 doit donc garantir cette présence et vérifier qu’aucun chemin de fallback n’est déclenché dans le package final.
- **Résultat lecteur expert :** `ExpertKnowledgeReader` ouvre déjà SQLite avec `QSQLITE_OPEN_READONLY` puis `PRAGMA query_only=ON`; cette partie est conforme et doit être conservée.
- **Résultat runtime IA :** le workflow COMPAT #29 contient le profil llama.cpp b10516 le plus conservateur déjà validé : statique, `LLAMA_OPENSSL=OFF`, sans backends dynamiques/OpenMP/AVX2 etc. Ce profil est la base à retenir pour l’artefact #30, plutôt que de revenir au runtime officiel qui avait déjà produit un crash sur le PC réel.

### Étape en cours — implémentation et push BUILD #30

**Objectif exact :** produire un seul artefact x64 BUILD #30 / v1.0.30 fondé sur le runtime COMPAT déjà éprouvé, avec base experte r20 générée depuis `database/reference`, désinstalleur + manifeste installés, et cycle de vie IA nettoyé sans toucher au protocole ECU. Puis pousser sur `MEMSX64` et contrôler la CI. Aucun BUILD #31 sans demande explicite.

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 avant premier commit BUILD #30 : **`fee195e88d3615613b8f92de83209da2cf8247c2`** — dernier commit BUILD #29.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Branche de sauvegarde x64 validée : **`MEMSX64-BUILD26-BASE`**, à laisser intacte.
- BUILD actif : **BUILD #30 — v1.0.30**.
- Règle : **un BUILD = une version**. Un rerun d’un même commit ne change pas la version.
- L’utilisateur compile/teste via **GitHub Actions uniquement** et n’utilise pas Qt Creator.

## 2. BUILDS DE RÉFÉRENCE

### BUILD #26 — référence x64 UI/IA/navigation figée

- Commit : `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`.
- Run : `32816285887` — SUCCESS.
- Validation PC réelle : IA sans crash dans ce package, base runtime prête, 14 onglets stables, `Aperçu` reste visible, navigation IA / Interactif / Test ECU 1.9 / Aperçu fonctionnelle.
- Branche figée : `MEMSX64-BUILD26-BASE`.

### BUILD #27 — premier x64 validé sur véhicule

- Commit final : `a6f9b209f32b6dd77774832e8c84469c53deca47`.
- Run : `32832192437` — SUCCESS.
- ECU réel : AANMP002 / MNE101150, COM3, FTDI FT232, ROSCO 1.3/1.6.
- Polling 7D/80 stable, logging fonctionnel.
- Injection RAM Mode4 validée : `0x03C8=1314`, `0x026E=0`, `0x0280=0`, injection ≈ 2,63 ms, retour diagnostic normal réussi.

### BUILD #28 — rollback x64 stable avant le lot IA

- HEAD : `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`.
- Run : `32842049458` — SUCCESS.
- Artefact : `ECU-MEMS-Manager-x64-BUILD-28-v1.0.28`.
- Artifact ID : `9561033224`.
- SHA256 : `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.

### BUILD #29 — historique d’intégration IA

- HEAD final avant #30 : `fee195e88d3615613b8f92de83209da2cf8247c2`.
- Qwen3-0.6B-Q8_0 conservé ; llama.cpp b10516 conservé.
- Ancien crash `0xC0000135` lié aux dépendances runtime corrigé par runtime COMPAT self-contained / `LLAMA_OPENSSL=OFF`.
- Commit runtime COMPAT : `1bbe9923a7a655dcf51d7cd73f1b0e46ad8f1fd0` ; run `32878926411` vert avec imports, `--version`, Qwen, `/health`, chat completion et smoke launch.
- Test PC plus récent : Qwen charge réellement et MEMS Manager atteint `IA locale prête`, mais le processus principal peut encore se fermer dans le chemin d’initialisation de l’onglet.
- Workflow principal : run `32883089997`, commit `209da29d3870fe34aec21c29f1b4182e2cb9372e`, SUCCESS.
- Workflow COMPAT final #29 : run `32883199177`, HEAD `fee195e8...`, rouge uniquement au packaging de base experte : tentative de copie du chemin absent `expert/base_connaissance_ia`.
- Validation source avant cet échec : **2134 entrées / 433 concepts sémantiques**.

## 3. VERSIONNAGE

- BUILD #26 = v1.0.26
- BUILD #27 = v1.0.27
- BUILD #28 = v1.0.28
- BUILD #29 = v1.0.29
- **BUILD #30 = v1.0.30**
- BUILD #100 = v1.1.0
- BUILD #588 = v1.5.88
- BUILD #662 = v1.6.62

Formule : `1.(build / 100).(build % 100)`.

Règles :

1. un seul BUILD actif ;
2. un BUILD = une version ;
3. un rerun du même commit/build n’est pas un nouveau BUILD ;
4. ne jamais utiliser `GITHUB_RUN_NUMBER` comme version ;
5. x64 uniquement sur `MEMSX64` ;
6. rapport uniquement sur `RAPPORT` ;
7. ne pas modifier `lab-expert-engine` ;
8. ne pas modifier `MEMSX64-BUILD26-BASE` ;
9. ne pas neutraliser une fonction pour masquer un crash ;
10. identifier une cause concrète avant modification ;
11. CI vert = automatisation verte, pas validation PC/ECU ;
12. **BUILD #30 est explicitement autorisé par l’utilisateur ; ne pas créer BUILD #31 sans demande explicite** ;
13. avant chaque étape, écrire objectif + résultat dans ce rapport.

## 4. NAVIGATION / UI À PRÉSERVER

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

Contraintes : style dark actuel, responsive, aucune refonte graphique opportuniste. `Injection` reste entre `Aperçu` et `Réglages`.

## 5. SÉCURITÉ PROTOCOLE À PRÉSERVER

Contexte central :

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ;
- `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.

Principes obligatoires :

- D0/D1/D2 seulement session normale ;
- D1 bloqué Mode4 ;
- D3/F3/F4/F5 bloqués depuis interface générique ;
- toute mutation exige famille prouvée Rosco13_16 + mode Normal ;
- famille inconnue = fail closed ;
- MEMS1.9 mutations bloquées ;
- F7/EF injecteurs bloqués sans sous-type prouvé ;
- transactions RAM Injection bloquent les commandes génériques concurrentes.

`memsinterface.h` doit conserver : `void onProtocolCommandRequested(quint8 command);`

Réponses de référence :

- D0 → `D0 98 00 02 02`
- D1 → ASCII `AANMP002`
- F0 → `F0 50`
- D2 → `D2 00 01`
- F4 → `F4 00`

Ralenti chaud : `idle_error_hot_corrected = raw - 32768 - correction`, avec la correction réellement réglée dans l’onglet Réglages ; ne jamais hardcoder `-3`. Exemple : raw 32772, correction -3 => 7.

Dwell primaire attendu ≈ 1,9–3,1 ms vers 14 V.

Aucune commande mutante ECU pendant la validation IA BUILD #30.

## 6. DLL / ABI X64

- DLL protocole : `mems_manager_x64.dll`, sources `librosco-x64/`.
- ABI à préserver : `frame80=28`, `frame7d=32`, `mems_data=60`, 22 exports historiques.
- Init historique `CA 75 F4 D0`, polling `0x80/0x7D`, IAC `0xFB/FD/FE`, clear faults `0xCC`, heartbeat `0xF4`, reset ECU `0xFA`, reset adjustments `0x0F`.

## 7. INJECTION / RAM

- Ne pas calculer l’injection depuis polling 7D/80.
- Adresses RAM : `0x03C8`, `0x026E`, `0x0280`.
- Lecteur Mode4 dédié read-only ; sélection bloc `DC`, offsets `0x00–0x7F` ; pas de calibration `0x80–0xBF`.
- Firmware sans profil exact => refus avant Mode4.
- Échec restauration normal => polling arrêté.
- Validation réelle BUILD #27 : injection ≈ 2,63 ms.

## 8. BLOQUEURS SÉCURITÉ TOUJOURS NO-GO

- MEMS1.9 injecteurs F7/EF : pas de test sans mapping exact.
- Parser 7D/80 MEMS1.9 : tailles à valider par famille.
- Wake-up ISO 9141 5 bauds : W4 25–50 ms à sécuriser.
- Reconnexion MEMS1.9 : stratégie dédiée ; coupure contact peut être requise.
- Actionneurs : timeout global/failsafe encore à finaliser.
- Ports série : ne jamais envoyer de séquence ECU à un périphérique série arbitraire.
- 164 profils RAM 1.9 corrélés ≠ 164 profils validés matériellement.
- Actionneurs, reset, clear faults, trims et écritures restent NO-GO pendant validation #30.

## 9. BASE MEMS / IA BUILD #30

- Base principale : `<appdir>/database/ecu_mems_manager.sqlite`.
- Base experte cible : `<appdir>/database/expert/ia_mems_reference_r20.sqlite`.
- Runtime : llama.cpp b10516 x64 CPU COMPAT, Qwen3-0.6B-Q8_0, local/offline.
- Le lot volumineux `research_enrichment_1600.qz64` ne doit pas être chargé automatiquement au runtime.
- Le package de production #30 ne doit pas reconstruire la base experte lors de l’ouverture de l’onglet.

## 10. TESTS BUILD #30 À OBTENIR EN CI AVANT TEST PC

1. Version affichée/emballée = BUILD #30 / v1.0.30.
2. Compilation `ecu_mems_manager.exe` x64 + `mems_manager_x64.dll`.
3. `ecu_mems_uninstaller.exe` compilé et présent dans le package.
4. `install_manifest.txt` présent et contient les fichiers réellement emballés, notamment application, DLL protocole, runtime Qt, IA, modèle, base experte et désinstalleur.
5. Base experte r20 générée en CI à partir du chemin source réel ; `PRAGMA user_version=20`, tables attendues et `integrity_check` OK.
6. Package contient `database/expert/ia_mems_reference_r20.sqlite`.
7. Runtime de production ne reconstruit pas la base si la base emballée manque : il doit signaler proprement l’indisponibilité et conserver le fallback conversationnel/déterministe sans construction lourde cachée.
8. `llama-server.exe --version` passe dans l’environnement package.
9. Qwen charge ; `/health` devient 200 ; `/v1/models` confirme `ia-mems`.
10. Vraie `/v1/chat/completions` réussie.
11. Ouverture/fermeture répétée de l’onglet IA ne crée pas de second serveur et ne démarre aucun thread/base builder lourd.
12. Arrêt forcé du sidecar => MEMS Manager reste vivant et affiche fallback/erreur.
13. Fermeture MEMS Manager => aucun sidecar possédé ne reste.
14. Self-tests réponses déterministes FR/EN/ES/IT/PT/DE restent verts.
15. Navigation 14 onglets et protections BUILD #28 restent intactes.

## 11. TEST PC APRÈS CI VERTE

- Extraire le nouvel artefact BUILD #30 proprement dans un dossier neuf.
- Vérifier la présence de `ecu_mems_uninstaller.exe`.
- Tester `ai/llama-server.exe --version` puis le serveur/modèle si nécessaire.
- Lancer MEMS Manager, ouvrir IA MEMS, confirmer `IA locale prête` sans fermeture de l’application.
- Poser plusieurs questions connues, dont une dépendant de la base experte préconstruite.
- Vérifier réponses déterministes batterie / état moteur / diagnostic / captures et au moins deux langues.
- Fermer/réouvrir IA et relancer l’application plusieurs fois.
- Tester ensuite le désinstalleur : application fermée, profil conservé par défaut, fichiers du package supprimés, aucun fichier étranger supprimé.
- Seulement après stabilité PC, reprendre les tests ECU non dangereux.

## 12. PROCHAINE ACTION EXACTE

**BUILD #30 / v1.0.30 est officiellement ouvert.**

Étape en cours : implémenter et pousser le package #30 à partir du HEAD `fee195e8...` en conservant le protocole/UI existants. Le premier commit #30 doit au minimum : utiliser la source `database/reference` pour générer la base experte, embarquer cette base r20, utiliser le runtime llama COMPAT validé, construire/emballer le désinstalleur et son manifeste, supprimer la concurrence lourde base+Qwen dans le chemin utilisateur, puis déclencher une CI #30 dédiée.

---

## PRINCIPE DIRECTEUR

**BUILD #26 reste la base x64 figée, BUILD #27 le premier x64 validé sur véhicule, BUILD #28 le rollback stable avant IA, BUILD #29 l’historique d’investigation IA, et BUILD #30 est désormais le build actif destiné à intégrer proprement l’IA locale x64. La cible est un `llama-server.exe` sidecar séparé et supervisé au niveau application, base experte préconstruite ouverte strictement en lecture seule, onglet IA réduit à une vue/client, aucune capacité de commande ECU donnée au LLM, packaging complet avec désinstalleur, et conservation intégrale des protections protocole/UI existantes.**