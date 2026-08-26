# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` est le numéro GitHub Actions, pas le BUILD logiciel.

## ÉTAT ACTUEL

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD courant : **`87cb4cdfd2f16ed98f0787454f7f9aec4f041e1f`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.

## ARCHITECTURE IA PROPRE RETENUE

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService (service application) -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> llama-server.exe -> Qwen3`

- Sidecar llama.cpp hors processus ; durée de vie IA application.
- Base experte r20 préconstruite, lecture seule.
- Mesures ECU read-only ; aucune commande ou mutation accessible au LLM.
- Qwen3-0.6B-Q8_0 + llama.cpp b10516.
- Ne pas réintroduire `iamemstab_clean.cpp`, `iamemsqualitypatch.cpp`, `iaresponsecontextpatch.cpp`.

## HISTORIQUE BUILD #30 / IA

- `600b8ef...` base BUILD #30 ; `879077a...` correction chemin base r20.
- #44 rouge au smoke Qwen ; `414ea52...` corrige uniquement le smoke CI ; #45 VERT mais crash PC à l’ouverture IA.
- `776fc64...` reconstruction propre ; #59 VERT ; test PC : crash supprimé, base + IA prêtes.
- `be4916a...` date/anti-écho/thinking ; #60 VERT ; test PC : date correcte mais contamination/hallucination/latence.
- `126cc63...` routage rapide ; #61 VERT ; test PC : vitesse un peu meilleure mais défauts langue/domaine/relances/navigation.
- `8a793a7...` français/domaine Rover-LUCAS/réponses contrôlées ; `7a8085c...` navigation responsive.
- #63 VERT : run `32936048218`, job `98077364424`, artifact `9595181353`, SHA-256 `995efda2b49768457ad3ec8f2f31b137c671b722c80465831b2ef00d47c559d4`.

## TEST PC RÉEL #63 — 26 AOÛT 2026

Validé :
- aucun crash d’ouverture IA ; `base prête` + `IA locale prête` ;
- 14 onglets visibles simultanément ; `Aperçu` reste visible avec `Test ECU 1.9` ;
- domaine Rover/Lucas MEMS mieux verrouillé.

Défauts :
- **latence ~30 s à 2 min** pour certaines réponses Qwen ;
- MAP confondu avec une mesure live ;
- injecteur défini à tort comme injection d’huile ;
- SPI halluciné alors que Rover/MEMS SPI = **Single Point Injection**.

## CAUSE PERFORMANCE ISOLÉE

Le runtime #63 force le CPU x64 de base : `GGML_NATIVE=OFF`, `GGML_BACKEND_DL=OFF`, `GGML_CPU_ALL_VARIANTS=OFF`, SSE4.2/AVX/AVX2/BMI2/AVX512 désactivés.

llama.cpp b10516 supporte officiellement `GGML_BACKEND_DL=ON` + `GGML_CPU_ALL_VARIANTS=ON` + `GGML_NATIVE=OFF`, produisant plusieurs backends (`x64`, `sse42`, `sandybridge`, `haswell`, `skylakex`, etc.) et choisissant automatiquement le meilleur compatible, avec `x64` comme repli. **Ne pas imposer AVX2 à tous les PC.**

## ÉTAPE AUTORISÉE — PERFORMANCE IA + DÉFINITIONS

Autorisation utilisateur : **GO**.

### Étape 1 — définitions contrôlées — TERMINÉE

Commit **`f860749313447e224f63b99801f7e7d6a1839a49`** — `BUILD #30 add controlled MAP injector SPI answers`.

- MAP : `Manifold Absolute Pressure`, pression absolue du collecteur, rôle charge moteur ;
- injecteur : électrovanne essence commandée ECU, distinction SPI/MPI, exclusion injection d’huile ;
- SPI : `Single Point Injection`, injection monopoint Rover/Mini MEMS ;
- réponses interceptées avant Qwen ; `map`, `injecteur`, `spi` ajoutés au domaine MEMS ;
- aucun changement protocole ECU/UI/32 bits.

### Étape 2 — runtime multi-variantes — PREMIÈRE TENTATIVE #65 ROUGE

HEAD **`20772b8ef5571cc0d0063c1e0d9b6f7e2f0866ef`** — `BUILD #30 enable adaptive llama CPU backends`.

### ECU MEMS Manager x64 #65 — Commit `20772b8` — ROUGE

- Run GitHub : **`32946087349`** ; job : **`98107022158`**.
- Étapes 1 à 12 : **VERTES**.
- Échec unique : étape 13 `Build adaptive multi-variant llama.cpp b10516 runtime`.
- Erreur CMake exacte : **`GGML_BACKEND_DL requires BUILD_SHARED_LIBS`**.
- Cause : combinaison invalide `GGML_BACKEND_DL=ON` + `GGML_CPU_ALL_VARIANTS=ON` + `BUILD_SHARED_LIBS=OFF`.
- Étapes 14 à 20 sautées ; aucun artefact #65.
- Aucun défaut constaté dans l’application, `LocalAiClient.cpp`, les réponses MAP/injecteur/SPI, le protocole ou la base.

### Étape 3 — correction runtime partagé — POUSSÉE

HEAD **`87cb4cdfd2f16ed98f0787454f7f9aec4f041e1f`** — `BUILD #30 fix shared adaptive llama runtime`.

Correction limitée à `.github/workflows/memsx64.yml` :
- `BUILD_SHARED_LIBS=ON` ; `GGML_NATIVE=OFF` ; `GGML_BACKEND_DL=ON` ; `GGML_CPU_ALL_VARIANTS=ON` ; `GGML_OPENMP=OFF` ; `LLAMA_OPENSSL=OFF` ;
- création d’un dossier CI autonome `llama-runtime-stage` ;
- copie dans ce dossier de `llama-server.exe`, `llama.dll`, `ggml.dll`, `ggml-base.dll` et de toutes les DLL produites en Release, dont `ggml-cpu-*.dll` ;
- validation obligatoire de `ggml-cpu-x64.dll` comme repli et `ggml-cpu-haswell.dll` comme variante optimisée ;
- `llama-server --version` exécuté depuis le dossier autonome avant packaging ;
- packaging du dossier runtime complet sous `ai/` ;
- validation PE de toutes les DLL runtime x64 ; OpenMP/OpenSSL restent interdits ;
- `install_manifest.txt` et hashes étendus aux DLL cœur et aux backends ;
- smoke Qwen conservé : chargement backend dynamique, `/health`, `/v1/models`, chat, puis smoke application.
- Aucun changement métier, UI, modèle Qwen, protocole ECU ou 32 bits.

### ECU MEMS Manager x64 #66 — Commit `87cb4cd` — EN COURS

- Run GitHub : **`32948276603`** ; job : **`98113701995`**.
- Checkout, Python, outils, Qt, protections protocole et configuration application sont **VERTS**.
- Compilation application/self-tests en cours au dernier contrôle.
- Étape critique à valider ensuite : `Build adaptive shared multi-variant llama.cpp b10516 runtime`, puis package/PE/Qwen/smoke/upload.

## AUDIT IA À FAIRE APRÈS VALIDATION RUNTIME

L’utilisateur demande un audit complet des questions pouvant être posées à IA MEMS à partir de tout ce que contient déjà ECU MEMS Manager.

Principe retenu : **toute connaissance certaine déjà présente dans le logiciel ou la base doit produire une réponse immédiate, sans appel Qwen**. Qwen est réservé aux questions nécessitant croisement, raisonnement ou diagnostic multi-facteurs.

Périmètre à couvrir au minimum :
- tous les cadrans/mesures : ce que la valeur mesure, comment elle est obtenue/décodée, son unité, son rôle, ce que signifie une valeur trop haute/trop basse, causes possibles et conséquences ;
- onglet Réglages : fonction de chaque réglage, valeur d’origine/référence lorsque connue et validée, effet d’une augmentation ou diminution, risques/limites et contexte d’application ;
- tests Actionneurs : rôle de chaque actionneur, ce que le test commande, résultat attendu, conséquences possibles si l’actionneur ne fonctionne pas, contrôles électriques/mécaniques pertinents ;
- Erreurs/DTC : signification de chaque erreur connue, organe concerné, rôle, symptômes possibles, stratégie ECU/fallback si connue, causes possibles et ordre de contrôle ;
- familles MEMS/ECU/ROSCO, capteurs, actionneurs, protocoles, valeurs de référence, fonctions du logiciel et autres données déjà documentées.

Pour les Erreurs/DTC, IA MEMS doit pouvoir répondre immédiatement à des formulations du type : `J'ai cette erreur, ça fait quoi ?`, `Ça peut provoquer quoi ?`, `Quelles causes possibles ?`, `Qu'est-ce que je contrôle en premier ?`, puis croiser avec les mesures ECU disponibles si une analyse plus poussée est demandée.

## EXIGENCE UI IA — FICHIERS CSV/TXT

À intégrer dans l’onglet **IA MEMS** après validation du runtime, sans changer le style dark/responsive :
- accepter le **glisser-déposer** d’un fichier `.csv` ou `.txt` dans la zone de saisie / conversation IA ;
- ajouter un petit bouton **`+`** près de la zone où l’utilisateur tape le texte ;
- bulle d’aide du bouton : **`Sélectionner un fichier`** ;
- clic sur `+` : ouvrir le gestionnaire/explorateur de fichiers Windows avec filtre `Fichiers CSV/TXT (*.csv *.txt)` ;
- afficher le fichier sélectionné dans la zone de saisie sous forme compacte avec son nom et possibilité de le retirer avant analyse ;
- lecture/analyse locale en **lecture seule** ; aucun fichier importé ne doit pouvoir déclencher une commande ECU ou une mutation ;
- l’objectif est notamment d’analyser des logs, traces et exports de mesures puis de les croiser avec la base MEMS et, lorsque nécessaire, avec le raisonnement Qwen ;
- pour ce besoin, le périmètre demandé est **CSV et TXT**. Aucun autre format n’est ajouté sans demande explicite.

## EXIGENCE UI IA — NOUVELLE ZONE DE SAISIE

Refaire la zone de saisie de **IA MEMS** dans l’esprit d’un composeur de chat moderne, tout en conservant le style dark/responsive propre à MEMS Manager :
- conteneur principal **arrondi aux deux extrémités**, type capsule lorsque la saisie tient sur une ligne ;
- saisie **multiligne** : la zone grandit verticalement lorsque le texte prend plusieurs lignes, sans perdre les coins arrondis ;
- le texte reste lisible et la zone ne doit pas écraser le transcript ni casser le responsive ;
- bouton **`+`** intégré à gauche de la zone pour la sélection `.csv` / `.txt`, avec bulle **`Sélectionner un fichier`** ;
- à droite, remplacer le bouton texte `Envoyer` par un **bouton rond orange** cohérent avec les accents actuels de MEMS Manager ;
- dans ce rond orange : **flèche orientée vers le haut** pour envoyer le message ;
- le bouton d’envoi reste accessible lorsque la saisie devient multiligne ;
- conserver la possibilité d’envoyer au clavier de manière cohérente avec une saisie multiligne : `Entrée` ne doit pas empêcher l’écriture sur plusieurs lignes ; le comportement exact du raccourci d’envoi sera validé avec l’utilisateur avant codage si nécessaire ;
- aucun changement global de thème : conserver le dark, le responsive et l’identité visuelle actuelle du programme.

## DÉSINSTALLATION BUILD #30

- `ecu_mems_uninstaller.exe` + `install_manifest.txt` requis.
- Refuse si app active ; profil conservé par défaut ; données locales uniquement sur choix explicite ; fichiers étrangers préservés.

## UI OFFICIELLE À PRÉSERVER

Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9. Style dark/responsive inchangé.

## SÉCURITÉ PROTOCOLE À PRÉSERVER

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 normal seulement ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique.
- Mutations : Rosco13_16 prouvé + Normal uniquement ; unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués sans sous-type ; transaction RAM bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);`.
- D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction`, correction Réglages réelle ; jamais -3 hardcodé.
- Dwell référence ~1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant BUILD #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports série arbitraires, profils RAM non validés, reset/clear faults/trims/écritures pendant BUILD #30.

## PROCHAINE ACTION EXACTE

**Suivre uniquement ECU MEMS Manager x64 #66 — Commit `87cb4cd`. Si rouge : consigner la cause exacte avant toute correction. Si verte : consigner l’artefact puis tester sur PC MAP/injecteur/SPI et mesurer la latence Qwen avec le runtime adaptatif partagé. Aucun BUILD #31.**