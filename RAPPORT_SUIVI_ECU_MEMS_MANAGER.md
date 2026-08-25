# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager. Il constitue la source de vérité de continuité du projet.
>
> Branche rapport : `RAPPORT`. Branche de développement x64 : `MEMSX64`.
>
> **RÈGLE DE SUIVI IMMÉDIAT : AVANT CHAQUE NOUVELLE ÉTAPE, inscrire ici l’étape exacte et son objectif. Dès que cette étape produit un résultat, inscrire immédiatement ce résultat AVANT de commencer l’étape suivante. Ne jamais attendre la fin d’un lot, d’un build ou d’une discussion.**

> **RÈGLE DE NOMMAGE GITHUB POUR LES ÉCHANGES AVEC L’UTILISATEUR : annoncer une exécution sous la forme `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. Le `#NN` affiché par GitHub est le numéro d’exécution du workflow, pas le numéro de BUILD logiciel. Éviter les formulations ambiguës du type « build #44 » pour parler d’une Action GitHub.**

## JOURNAL IMMÉDIAT — 25 août 2026

### Architecture IA x64 retenue

- `llama-server.exe` sidecar x64 hors processus ; ne pas lier libllama/ggml dans l’exécutable ECU.
- Durée de vie IA application, pas widget ; IA MEMS reste une vue/client.
- Loopback local ; base experte r20 préconstruite en CI puis lue `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`.
- RAG : mesures ECU read-only -> moteur expert -> faits pertinents -> Qwen -> texte ; aucun accès LLM aux commandes ECU.
- Qwen3-0.6B-Q8_0 + llama.cpp b10516 conservés.

### BUILD #30 ouvert explicitement par l’utilisateur

- BUILD actif : **#30 / v1.0.30**.
- BUILD #29 reste historique.
- Aucun BUILD #31 sans demande explicite.
- `lab-expert-engine` et `MEMSX64-BUILD26-BASE` intacts ; aucun changement protocole ECU dans le lot IA.

### Désinstallation BUILD #30

- `uninstaller.cpp` existe et CMake crée `ecu_mems_uninstaller`.
- Le manque #29 venait du packaging.
- #30 doit livrer `ecu_mems_uninstaller.exe` + `install_manifest.txt` complet.
- Le code existant refuse si l’app tourne, conserve le profil par défaut, supprime les données locales seulement sur choix explicite, et préserve les fichiers étrangers.

### Inspection pré-#30

- Source experte réelle : `database/reference/`; `expert/base_connaissance_ia` n’existe pas.
- `ExpertRuntimeDatabase::buildOrOpen()` préfère déjà la base emballée `database/expert/ia_mems_reference_r<revision>.sqlite` et évite alors la reconstruction lourde.
- `ExpertKnowledgeReader` est déjà read-only.
- Runtime retenu : COMPAT statique b10516, `LLAMA_OPENSSL=OFF`, sans backend dynamique/OpenMP/AVX2, déjà observé chargeant Qwen sur PC.

### Premier push BUILD #30

- HEAD initial `MEMSX64` #30 : **`600b8ef8607eb3dc7d591f675e9f33be0cdb0911`** — `BUILD #30 clean x64 IA package and uninstaller`.
- Commit intermédiaire `2fd0ad91ec905690b07aefc23104be1571f2eeb1` = marqueur temporaire supersédé, pas un build distinct.
- Workflow expérimental COMPAT supprimé ; `.github/workflows/memsx64.yml` devient le workflow unique #30.
- `iamemstab_clean.cpp` reparente `LocalAiClient` vers `QApplication` avant `showEvent()` : le sidecar n’est plus possédé par le widget.
- Aucun fichier protocole, ABI ROSCO ou ordre d’onglets modifié.

### GitHub Actions BUILD #30 — run `32889430143`, job `97937586271`

Résultats **VERTS** avant l’échec :
- checkout exact BUILD #30 ;
- Python/outils ;
- Qt 5.15.2 MSVC x64 ;
- protections protocole ;
- configuration CMake ;
- **compilation application + `ecu_mems_uninstaller.exe` + self-tests** ;
- réponses IA déterministes ;
- ABI protocole ;
- base de référence / recherche sémantique.

**Premier échec concret :**
- étape `Generate and validate packaged expert database r20` : **ROUGE** ;
- toutes les étapes suivantes (build llama, Qwen, packaging, API, smoke, upload) ont été sautées.

### Diagnostic exact du rouge r20

Le log du run `32889430143` montre que `expert_runtime_selftest.exe` **génère correctement** la base :
- `EXPERT_RUNTIME_REVISION=20` ;
- `EXPERT_RUNTIME_TABLES=63` ;
- chemin réel produit : `C:/Users/runneradmin/AppData/Local/ECU Mems Manager/ECU Mems Manager/ia-mems/ia_mems_reference_r20.sqlite`.

L’échec survient seulement après cette génération : le workflow cherche ensuite `ia_mems_reference_r20.sqlite` sous `${{ github.workspace }}\\expert-runtime-cache`, mais Qt `QStandardPaths` n’a pas suivi cette redirection comme supposé et a conservé le LocalAppData Windows réel. Le générateur n’est donc pas en cause ; **seule la récupération du fichier utilise le mauvais chemin**.

### Correction du chemin r20 poussée

- Nouveau HEAD `MEMSX64` : **`879077a678f4c203124907dabdeb42b532c9d337`** — `BUILD #30 fix expert database output path`.
- Modification limitée à `.github/workflows/memsx64.yml`.
- Le workflow ne devine plus l’emplacement SQLite : il capture la sortie de `expert_runtime_selftest.exe`, extrait la valeur annoncée par `EXPERT_RUNTIME_DATABASE=...`, vérifie que ce fichier existe et n’est pas vide, puis l’utilise pour `integrity_check`, révision r20 et packaging.
- Aucun changement protocole ECU, Qwen, llama.cpp ou UI.

### ECU MEMS Manager x64 #44 — Commit `879077a`

Résultat exact du run GitHub `32890600398`, job `97941378322` : **ROUGE** à l’étape 17 `Validate packaged llama server model and API`.

Toutes les étapes 1 à 16 sont **VERTES**, notamment :
- protections protocole ;
- compilation application + désinstalleur + self-tests ;
- réponses IA déterministes ;
- ABI protocole ;
- base de référence ;
- génération et validation de la base experte r20 ;
- compilation du runtime llama.cpp b10516 ;
- téléchargement/vérification Qwen3-0.6B-Q8_0 ;
- assemblage du package ;
- validation architecture du package.

Erreur exacte du test API empaqueté :
`Empty chat completion from packaged Qwen`.
Le serveur empaqueté démarre (`version: 0.1.2-dev`, commit llama.cpp `b95502b`, MSVC x64) et la requête atteint l’API, mais le contrôle de `choices[0].message.content` obtient une chaîne vide. Le test impose une enveloppe de génération courte alors que le raisonnement Qwen3 est actif par défaut ; le texte visible peut donc rester vide dans cette enveloppe.

### Étape autorisée — correction minimale du smoke-test Qwen

Objectif exact : modifier **uniquement** le lancement de `llama-server.exe` dans le smoke-test GitHub empaqueté afin d’y ajouter `--reasoning off`, puis relancer la validation. Cette option est réservée au serveur lancé par le CI : **le serveur lancé par ECU MEMS Manager et la fonction de raisonnement de l’IA réelle restent inchangés et actifs**. Aucun changement protocole ECU, UI, base experte, modèle Qwen ou code `LocalAiClient` n’est autorisé dans cette correction.

### Correction smoke-test Qwen poussée

- Nouveau HEAD `MEMSX64` : **`414ea52970e02fb6077c94ca2aa7aec3e92d7383`** — `BUILD #30 fix packaged Qwen smoke reasoning`.
- Modification unique : dans l’étape GitHub `Validate packaged llama server model and API`, les arguments du `llama-server.exe` de test incluent maintenant `--reasoning off`.
- Le code de l’application n’a pas été modifié : `LocalAiClient` et le serveur réellement lancé par ECU MEMS Manager ne reçoivent pas cette option. **Le raisonnement de l’IA MEMS reste actif.**
- Aucun changement protocole ECU, UI, base experte, modèle Qwen ou packaging hors argument du smoke-test.

### ECU MEMS Manager x64 #45 — Commit `414ea52` — VERT

- Run GitHub : **`32893817192`** ; job : **`97951667159`**.
- Commit exact : **`414ea52970e02fb6077c94ca2aa7aec3e92d7383`**.
- Résultat final : **SUCCESS / VERT**.
- Les étapes 1 à 20 sont toutes vertes, notamment : protections protocole, compilation x64, `ecu_mems_uninstaller.exe`, self-tests IA/ABI/base, génération SQLite experte r20, llama.cpp b10516, vérification Qwen3-0.6B-Q8_0, assemblage package, architecture x64, API Qwen empaquetée, smoke launch de l’application, hashes et upload.
- Le correctif `--reasoning off` n’existe que dans le smoke-test CI ; **le raisonnement de l’IA MEMS réelle reste actif**.

### Artefact officiel BUILD #30 / v1.0.30

- Nom : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9580850077`**.
- Taille : **668890922 octets**.
- SHA-256 de l’archive GitHub : **`1db3438593c65f7f77176910e55e9de0a428208f9c3a7732b74d6e35290ed3d0`**.
- Créé le 25 août 2026 à 20:22:33Z ; expiration prévue le 8 septembre 2026.
- Cet artefact correspond exactement au HEAD `MEMSX64` `414ea52970e02fb6077c94ca2aa7aec3e92d7383`.

### Test PC réel — ECU MEMS Manager x64 #45 — Commit `414ea52`

- Résultat utilisateur : **CRASH immédiat à l’ouverture de l’onglet IA MEMS**, identique au crash observé précédemment.
- Le succès CI #45 ne valide donc pas ce chemin d’exécution réel.
- L’étape GitHub `Smoke launch ECU MEMS Manager` démarre l’application et vérifie qu’elle reste ouverte, mais **n’ouvre pas l’onglet IA MEMS** ; elle ne pouvait donc pas détecter ce crash.
- Les validations séparées du package, de `llama-server.exe`, de Qwen et de la base experte restent vertes. Le défaut à isoler est désormais l’intégration déclenchée par l’ouverture de l’onglet IA sur le PC réel.

### Étape en cours — isolation du crash à l’ouverture IA

Objectif exact : comparer le chemin d’ouverture IA du commit `414ea52970e02fb6077c94ca2aa7aec3e92d7383` avec le dernier état confirmé sans crash sur PC (`12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, BUILD #26), puis isoler l’objet, callback ou thread responsable **avant toute correction**. Ne modifier ni protocole ECU, ni UI, ni modèle Qwen, ni fonction de raisonnement ; ne pas créer BUILD #31.

### Décision utilisateur — reconstruction IA réellement propre

L’utilisateur confirme explicitement qu’il avait demandé de **refaire l’IA sur une architecture propre** et autorise maintenant la reconstruction immédiate puis le push sur `MEMSX64`, toujours dans BUILD #30 / v1.0.30.

Constats techniques avant modification :
- `navigationorderpatch.cpp` crée déjà directement `IaMemsTab` et garantit sa place officielle n°7 ; l’ancien installateur `Q_COREAPP_STARTUP_FUNCTION` contenu dans `iamemstab.cpp` est donc redondant.
- `iamemstab_clean.cpp` ne constitue pas une implémentation propre : il réinclut `iamemstab.cpp` puis ajoute un `eventFilter` global qui reparente `LocalAiClient` pendant l’événement `Show`.
- `iamemsqualitypatch.cpp` et `iaresponsecontextpatch.cpp` ajoutent encore d’autres `eventFilter` globaux autour de l’IA ; cette superposition doit disparaître.
- Le BUILD #26 stable n’embarquait pas le vrai runtime Qwen ; son succès PC ne validait donc pas le chemin de démarrage du sidecar actuel.
- Le code actuel de `LocalAiClient` contient encore `/no_think` dans la requête et dans le prompt système ; contrairement à l’intention annoncée, cela désactive le raisonnement Qwen dans l’application. Cette incohérence doit être supprimée dans la reconstruction propre.

Architecture à appliquer maintenant :
- `navigationorderpatch.cpp` reste l’unique créateur/intégrateur visuel de l’onglet IA ;
- `iamemstab.cpp` devient l’unique implémentation de la vue IA et est compilé directement ; aucun wrapper ni installateur IA global ;
- `LocalAiClient` devient un **service unique à durée de vie application dès sa création**, sans reparentage au `Show` ;
- ouverture IA : chargement/connexion de la base experte read-only, puis démarrage du sidecar Qwen ; pas de double démarrage concurrent au premier `Show` ;
- suppression de `iamemstab_clean.cpp`, `iamemsqualitypatch.cpp` et `iaresponsecontextpatch.cpp` de la cible x64 ; les comportements nécessaires sont intégrés directement dans les composants réels, pas interceptés globalement ;
- raisonnement Qwen actif dans l’application : suppression de `/no_think` côté application ; le `--reasoning off` reste uniquement dans le smoke-test CI rapide ;
- aucun changement protocole ECU, ordre des 14 onglets, style dark/responsive, base experte r20, modèle Qwen, branche 32 bits ou rollback.

---

## ÉTAT DE RÉFÉRENCE

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD avant reconstruction propre : `414ea52970e02fb6077c94ca2aa7aec3e92d7383`.
- Rapport : `RAPPORT`.
- 32 bits : `lab-expert-engine`, ne pas toucher.
- Rollback x64 : `MEMSX64-BUILD26-BASE`, ne pas toucher.
- BUILD actif : #30 / v1.0.30.

### Références historiques

- #26 `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, run `32816285887` SUCCESS, IA/navigation PC validées sans runtime Qwen emballé.
- #27 `a6f9b209f32b6dd77774832e8c84469c53deca47`, run `32832192437` SUCCESS, AANMP002/MNE101150, COM3 FTDI, ROSCO 1.3/1.6, Injection RAM Mode4 ≈2,63 ms.
- #28 `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`, run `32842049458` SUCCESS, artifact `9561033224`, SHA256 `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.
- #29 final `fee195e88d3615613b8f92de83209da2cf8247c2`; runtime COMPAT run `32878926411` vert ; Qwen chargé sur PC et statut `IA locale prête` atteint ; dernier rouge #29 limité au packaging expert.
- #30 `414ea52970e02fb6077c94ca2aa7aec3e92d7383`, ECU MEMS Manager x64 #45 / run `32893817192` SUCCESS, artifact `9580850077`, archive SHA-256 `1db3438593c65f7f77176910e55e9de0a428208f9c3a7732b74d6e35290ed3d0`, mais crash PC réel à l’ouverture de l’onglet IA MEMS.

## VERSIONNAGE

- #29 = v1.0.29 ; #30 = v1.0.30 ; #100 = v1.1.0.
- Formule `1.(build / 100).(build % 100)`.
- Un rerun ou correctif du même BUILD #30 ne devient pas BUILD #31.

## UI OFFICIELLE À PRÉSERVER

Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9. Style dark/responsive inchangé.

## SÉCURITÉ PROTOCOLE À PRÉSERVER

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 normal seulement ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique ; mutations Rosco13_16 prouvé + Normal ; unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués sans sous-type ; RAM transaction bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);`.
- D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud `raw - 32768 - correction` avec correction Réglages réelle ; jamais -3 hardcodé.
- Dwell ≈1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant validation #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports série arbitraires, profils RAM non validés, reset/clear faults/trims/écritures pendant #30.

## PROCHAINE ACTION EXACTE

**Reconstruire maintenant l’intégration IA propre sur `MEMSX64` : compiler directement `iamemstab.cpp`, retirer les wrappers/patches IA globaux, rendre `LocalAiClient` application-scoped dès sa création, séquencer base experte puis Qwen, supprimer `/no_think` côté application, vérifier que le protocole et les 14 onglets restent intacts, puis pousser et suivre la nouvelle exécution GitHub du BUILD #30.**