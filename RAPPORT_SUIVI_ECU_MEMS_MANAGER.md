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
2. **Service applicatif unique** : le moteur doit avoir une durée de vie application et non widget. L’onglet `IA MEMS` devient une vue/client.
3. **Loopback privé** : `127.0.0.1` uniquement ; WebUI/outils/agents/MCP non nécessaires.
4. **Base experte préconstruite** : construire/valider `ia_mems_reference_r20.sqlite` dans GitHub Actions, l’emballer dans `database/expert/`, puis l’ouvrir avec le lecteur déjà `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`.
5. **RAG borné** : `mesures ECU en lecture seule -> ExpertEngine/ExpertKnowledgeReader -> faits pertinents -> contexte -> Qwen -> texte`. Le LLM ne reçoit aucune capacité de commande ECU.
6. **Qwen3** : conserver Qwen3-0.6B-Q8_0 et llama.cpp b10516.
7. **Package x64** : application, DLL protocole, Qt/qsqlite, runtime IA, modèle, base experte, manifeste IA, désinstalleur et manifeste d’installation.

### Décision utilisateur : ouverture officielle BUILD #30

- L’utilisateur a explicitement demandé le 25 août 2026 de créer **BUILD #30 / v1.0.30** pour réaliser cette intégration propre.
- BUILD #29 reste l’historique expérimental ; BUILD #30 est le build actif.
- Ne pas toucher à `lab-expert-engine` ni à `MEMSX64-BUILD26-BASE`.
- Ne pas modifier le protocole ECU dans ce lot IA.
- Aucun BUILD #31 sans demande explicite utilisateur.

### Exigence utilisateur BUILD #30 : désinstallation propre

- Le source `uninstaller.cpp` existe encore et CMake déclare déjà `ecu_mems_uninstaller` sous Windows.
- Le manque observé provenait du packaging : la cible n’était pas explicitement construite/emballée dans le workflow #29.
- BUILD #30 doit livrer `ecu_mems_uninstaller.exe` avec `install_manifest.txt` complet.
- Le désinstalleur existant refuse d’agir si `ecu_mems_manager.exe` tourne, conserve le profil par défaut, peut supprimer les données locales sur choix explicite, ne supprime que les fichiers du manifeste et préserve les fichiers étrangers.

### Étape terminée : inspection exacte du HEAD #29 avant modification BUILD #30

- La vraie source experte est sous `database/reference/` avec `manifest.json`, `mems_reference_seed_*.qz64` et `research_enrichment_*.qz64`; `expert/base_connaissance_ia` n’existe pas.
- `IaMemsTab` crée encore `LocalAiClient` avec le widget comme parent et `showEvent()` lance à la fois la préparation de base et l’initialisation Qwen.
- `ExpertRuntimeDatabase::buildOrOpen()` préfère déjà `database/expert/ia_mems_reference_r<revision>.sqlite`; avec une base r20 réellement emballée il retourne immédiatement et évite la reconstruction lourde.
- `ExpertKnowledgeReader` utilise déjà `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`.
- Le runtime COMPAT #29 statique (`LLAMA_OPENSSL=OFF`, pas de backends dynamiques/OpenMP/AVX2 etc.) est le runtime retenu pour #30 parce qu’il a déjà chargé Qwen sur le PC réel.

### Étape terminée : premier push BUILD #30

- **HEAD `MEMSX64` BUILD #30 : `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`** — `BUILD #30 clean x64 IA package and uninstaller`.
- Un commit intermédiaire `2fd0ad91ec905690b07aefc23104be1571f2eeb1` n’a ajouté qu’un marqueur temporaire pendant la préparation atomique ; il est immédiatement supersédé par `600b8ef8...` et ne constitue pas un build distinct.
- Le workflow expérimental `memsx64_compat_ai.yml` a été supprimé du HEAD #30 afin qu’il n’y ait plus deux artefacts concurrents.
- `.github/workflows/memsx64.yml` est maintenant le **workflow unique BUILD #30**, avec `MEMS_BUILD_NUMBER=30`, `MEMS_VERSION=1.0.30` et artefact prévu `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`.
- Le workflow #30 construit explicitement : `ecu_mems_manager`, `ecu_mems_uninstaller`, `rosco_abi_selftest`, `ia_response_selftest`, `mems_search_selftest`, `expert_runtime_selftest`.
- La génération de base experte utilise explicitement **`database/reference`**, force un cache CI propre, génère `ia_mems_reference_r20.sqlite`, puis valide `PRAGMA integrity_check`, `user_version=20` et la présence de tables.
- Le runtime IA officiel de BUILD #30 est le profil statique conservateur llama.cpp b10516 déjà éprouvé : `BUILD_SHARED_LIBS=OFF`, runtime MSVC statique, `GGML_NATIVE=OFF`, backends dynamiques OFF, OpenMP OFF, SSE4.2/AVX/AVX2/BMI2/AVX512 OFF, `LLAMA_OPENSSL=OFF`.
- Qwen3-0.6B-Q8_0 est téléchargé avec SHA256 attendu `9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.
- Le package assemble `ecu_mems_manager.exe`, `mems_manager_x64.dll`, `ecu_mems_uninstaller.exe`, Qt, traductions, base référence, base experte r20, `ai/llama-server.exe`, `ai/models/ia-mems.gguf`, manifeste IA et licences.
- `install_manifest.txt` est généré à partir des fichiers réellement présents dans le package et inclut le désinstalleur, le modèle, le serveur et la base experte.
- La CI vérifie l’architecture AMD64, `qsqlite.dll`, le manifeste, l’intégrité SQLite, les dépendances interdites du sidecar, `llama-server --version`, chargement réel Qwen, `/health`, `/v1/models` avec alias `ia-mems`, une vraie `/v1/chat/completions` et un smoke launch de MEMS Manager.
- `iamemstab_clean.cpp` conserve la suppression de l’ancien installer IA à timers et ajoute un filtre BUILD #30 qui, au premier affichage de l’onglet, **reparente `LocalAiClient` vers `QApplication` avant `showEvent()`**. Le pointeur/signaux de l’onglet restent identiques, mais le sidecar n’est plus possédé par le widget ; son destructeur/shutdown s’exécute à la fin de l’application.
- La présence garantie de la base r20 signifie que `startKnowledgeLoad()` ne doit plus reconstruire la base : son `buildOrOpen()` doit prendre immédiatement le chemin préconstruit puis `ExpertKnowledgeReader` l’ouvre en lecture seule.
- Aucun fichier protocole ECU, aucune ABI ROSCO et aucun ordre d’onglets n’ont été modifiés dans ce push.

### Étape en cours — contrôle GitHub Actions BUILD #30

**Objectif exact :** vérifier le run déclenché par `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`. Si la CI échoue, corriger uniquement la cause exacte dans le même BUILD #30, inscrire le résultat ici avant la correction suivante, puis repush. Si elle est verte, relever run, artifact ID et hash avant de donner le package à tester sur PC.

---

## 1. ÉTAT ACTUEL À RETENIR EN PREMIER

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 : **`600b8ef8607eb3dc7d591f675e9f33be0cdb0911`**.
- Branche rapport : **`RAPPORT`**.
- Branche 32 bits de référence : **`lab-expert-engine`**, à laisser intacte.
- Branche de sauvegarde x64 validée : **`MEMSX64-BUILD26-BASE`**, à laisser intacte.
- BUILD actif : **BUILD #30 — v1.0.30**.
- L’utilisateur compile/teste via GitHub Actions uniquement.

## 2. BUILDS DE RÉFÉRENCE

### BUILD #26 — référence x64 UI/IA/navigation figée
- Commit `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, run `32816285887` SUCCESS.
- Validation PC : IA sans crash dans ce package, base prête, 14 onglets stables, navigation stable.

### BUILD #27 — premier x64 validé sur véhicule
- Commit `a6f9b209f32b6dd77774832e8c84469c53deca47`, run `32832192437` SUCCESS.
- AANMP002 / MNE101150, COM3, FTDI FT232, ROSCO 1.3/1.6 ; polling 7D/80 et logging stables.
- Injection RAM Mode4 : `0x03C8=1314`, `0x026E=0`, `0x0280=0`, ≈2,63 ms.

### BUILD #28 — rollback x64 stable
- HEAD `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`, run `32842049458` SUCCESS.
- Artifact ID `9561033224`, SHA256 `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.

### BUILD #29 — historique IA
- HEAD final `fee195e88d3615613b8f92de83209da2cf8247c2`.
- Runtime COMPAT self-contained validé au run `32878926411`; Qwen a chargé sur PC et MEMS Manager a atteint `IA locale prête`.
- Dernier rouge #29 localisé au packaging de base experte, pas au modèle.

## 3. VERSIONNAGE

- BUILD #29 = v1.0.29
- **BUILD #30 = v1.0.30**
- BUILD #100 = v1.1.0
- Formule : `1.(build / 100).(build % 100)`.
- Un rerun du même commit n’est pas un nouveau BUILD.
- Aucun BUILD #31 sans demande explicite.

## 4. UI À PRÉSERVER

Ordre officiel : Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9.

Style dark et responsive à préserver. Aucun redesign opportuniste.

## 5. SÉCURITÉ PROTOCOLE À PRÉSERVER

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 seulement session normale ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique.
- Toute mutation exige Rosco13_16 prouvé + mode Normal ; famille inconnue fail-closed ; mutations MEMS1.9 bloquées ; F7/EF bloqués sans sous-type ; transaction RAM bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);` dans `memsinterface.h`.
- Références : D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction` avec correction réelle de Réglages ; jamais -3 hardcodé.
- Dwell ≈1,9–3,1 ms vers 14 V.
- Aucune commande mutante pendant validation IA #30.

## 6. BLOQUEURS TOUJOURS NO-GO

MEMS1.9 F7/EF, validation tailles parser 7D/80, W4 25–50 ms, reconnexion 1.9 dédiée, failsafe actionneurs, ports série arbitraires, profils RAM non validés matériellement, reset/clear faults/trims/écritures pendant validation #30.

## 7. TESTS BUILD #30 À OBTENIR AVANT PC

1. CI v1.0.30 / BUILD #30 verte.
2. Application + DLL protocole + uninstaller x64 présents.
3. `install_manifest.txt` complet.
4. Expert r20 : intégrité OK, user_version 20, tables présentes.
5. Package contient `database/expert/ia_mems_reference_r20.sqlite`.
6. Runtime statique llama `--version` OK.
7. Qwen charge, `/health` OK, `/v1/models` contient `ia-mems`, chat completion réelle.
8. Smoke launch application.
9. Test PC ensuite : IA prête sans fermeture, réponses base/déterministes, fermeture/réouverture, désinstallation propre.

## 8. PROCHAINE ACTION EXACTE

**Contrôler GitHub Actions sur le HEAD BUILD #30 `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`.** Ne faire aucune autre modification avant d’avoir le résultat exact du run et de l’avoir inscrit dans ce rapport.

---

## PRINCIPE DIRECTEUR

**BUILD #30 est le build x64 actif destiné à fournir une IA locale fonctionnelle et propre : runtime llama.cpp b10516 statique conservateur séparé du processus ECU, Qwen3-0.6B-Q8_0, base experte r20 préconstruite et lue en lecture seule, durée de vie du client IA portée au niveau application, package unique avec désinstalleur et manifeste, tout en conservant strictement le protocole ECU, l’ABI, les 14 onglets et le style existants.**