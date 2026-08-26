# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : ne jamais corriger un symptôme par un patch provisoire, une suppression de capacité, un contournement matériel ou une sélection arbitraire destinée uniquement à faire passer le build. Rechercher la cause réelle, utiliser en priorité l’architecture officiellement supportée par le composant amont et produire une solution propre, générale et maintenable. Quand un runtime officiel existe pour le besoin visé, il doit être préféré à une reconstruction/staging maison sauf nécessité technique démontrée et validée par l’utilisateur.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` est le numéro GitHub Actions, pas le BUILD logiciel.

## ÉTAT ACTUEL

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD courant avant probe ONNX : **`bdd7de2da64cb1308852e0900c3e20287cbf128b`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.

## ARCHITECTURE IA PROPRE RETENUE JUSQU’AU CHANGEMENT DE MOTEUR

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService (service application) -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> moteur local -> modèle local`

- Base experte r20 préconstruite, lecture seule.
- Mesures ECU read-only ; aucune commande ou mutation accessible au LLM.
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

### ECU MEMS Manager x64 #66 — Commit `87cb4cd` — ROUGE

- Run GitHub : **`32948276603`** ; job : **`98113701995`**.
- Étapes 1 à 16 : **VERTES**.
- Échec unique : étape 17 `Validate packaged adaptive llama server model and API`.
- `llama-server.exe --version` fonctionne : `0.1.2-dev (build 1, commit b95502b)`, MSVC x64.
- Au lancement avec Qwen, le processus quitte pendant le chargement du modèle avec **`-1073740791`**, soit **`0xC0000409`** sous Windows.
- Le crash intervient donc après démarrage du serveur et avant disponibilité `/health` ; ce n’est pas un échec CMake, ni une absence simple de `llama-server.exe`.
- Étapes 18 à 20 sautées ; aucun artefact #66.
- Le changement nouveau par rapport au runtime stable #63 est le runtime partagé + chargeur de backends CPU multi-variantes.

### Étape 4 — isolement runtime x64 partagé — POUSSÉE, VOIE ABANDONNÉE

Commit **`634fce02bd92b3048cd402c147ce3e9cc84a2103`** — `BUILD #30 isolate shared llama x64 backend`.

Cette voie d’isolement est **ABANDONNÉE** : elle constitue un contournement matériel temporaire et ne répond pas à l’exigence d’un MEMS Manager x64 général pour des PC Intel/AMD différents.

### ECU MEMS Manager x64 #67 — Commit `634fce0` — ROUGE

- Run GitHub : **`32955046246`** ; job : **`98134767273`**.
- `llama-server.exe --version` échoue avec **`0xC0000135` = `STATUS_DLL_NOT_FOUND`** car le staging partagé excluait des DLL non-CPU obligatoires (`llama-common.dll`, `mtmd.dll`, `llama-server-impl.dll`).

### ECU MEMS Manager x64 #68 — Commit `c7164e4` — CI VERT / TEST PC RÉEL ÉCHEC

- Run GitHub : **`32962220202`** ; job : **`98156880259`**.
- Artifact : **`9598541996`**.
- Runtime officiel `llama-b10516-bin-win-cpu-x64.zip` conservé intact en CI ; Qwen/API verts sur runner GitHub.
- **Test PC réel utilisateur : échec au démarrage de l’IA avec `QProcess 0 / FailedToStart`.**
- Conclusion : un VERT runner GitHub ne suffit pas à valider le moteur sur les PC utilisateurs réels.

### ECU MEMS Manager x64 #72 — Commit `6284a2e` — CI VERT / TEST PC RÉEL ÉCHEC

- ajout du runtime VC++ app-local autour de llama-server ;
- CI verte ;
- **test PC réel : même `QProcess 0 / FailedToStart`**.

### ECU MEMS Manager x64 #73 — Commit `bdd7de2` — ROUGE

- Run GitHub : **`32972550911`**.
- retour expérimental au runtime llama.cpp compilé depuis les sources ;
- `llama-server --version` démarre, donc le problème de DLL manquante #67 est corrigé ;
- au chargement de Qwen, crash **`-1073740791 = 0xC0000409`** ;
- cette voie réintroduit en plus une compilation/staging maison déjà jugée non souhaitable ; elle est donc **arrêtée**.

## DÉCISION UTILISATEUR — CHANGEMENT DE MOTEUR IA — 26 AOÛT 2026

Constat utilisateur : l’intégration llama.cpp fait tourner le projet en rond depuis plusieurs jours. Autorisation explicite : **changer de moteur IA et tester directement sous Windows x64**.

### Cible de probe retenue

- **ONNX Runtime GenAI 0.14.0**, release officielle Microsoft non-prerelease ;
- package officiel Windows CPU x64 : **`onnxruntime-genai-0.14.0-win-x64.zip`** ;
- digest release officiel : **SHA-256 `8a303e52dc7be8fb2a5331929af451a25ac59774102d7fd09ef673adc85c5ebf`** ;
- premier probe **CPU x64 uniquement**, afin de couvrir les PC Intel/AMD Windows sans imposer de GPU ;
- moteur testé séparément avant toute réécriture de `IaMemsService` / `LocalAiClient` ;
- aucun `QProcess`, aucun `llama-server.exe`, aucun serveur HTTP dans ce probe ;
- modèle candidat pour le probe : **Qwen3-0.6B ONNX INT4 CPU**, révision figée `e6bf97818c142808967a48cbab4f0aef18b64621`, dossier `cpu_and_mobile/cpu-int4-rtn-block-32-acc-level-4` ;
- gros fichiers figés : `model.onnx` SHA-256 `b52cfcd91e6ed3caa34c670d4831d96cda3da5eb07ef95bc8ba06547b4f3b86f`, `model.onnx.data` SHA-256 `8539cf4fbc3b5b331fd78ca8cd5b025ce537dfc2f993451f5c474b2f9fb7cd6c`, `tokenizer.json` SHA-256 `be75606093db2094d7cd20f3c2f385c212750648bd6ea4fb2bf507a6a4c55506`.

### Probe Windows autorisé — objectif exact

1. conserver BUILD logiciel **#30 / v1.0.30** ;
2. compiler d’abord l’application et les self-tests actuels sous Windows x64 pour vérifier qu’aucune régression hors IA n’est introduite ;
3. installer **`onnxruntime-genai==0.14.0` CPU** sur le runner Windows ;
4. télécharger le modèle Qwen3-0.6B ONNX INT4 à la révision figée et vérifier les hashes connus ;
5. charger le modèle avec ONNX Runtime GenAI ;
6. générer une réponse locale à une consigne déterministe de type `Réponds uniquement : OK` ;
7. mesurer au minimum temps de chargement et temps de génération ;
8. si ce probe échoue, **ne pas modifier l’application** : diagnostiquer le moteur/modèle ;
9. si ce probe est vert, seulement alors remplacer proprement `LocalAiClient` et supprimer la dépendance llama.cpp ;
10. aucun changement protocole ECU, aucun 32 bits, aucun BUILD #31.

## AUDIT IA DES QUESTIONS POSSIBLES — DÉMARRÉ

Principe retenu : toute connaissance certaine déjà présente dans le logiciel, ses libellés/aides, son décodage ou la base experte doit produire une réponse immédiate, sans appel au modèle génératif. Le modèle est réservé au croisement de plusieurs faits, au raisonnement diagnostic, aux questions générales ou aux cas où la réponse n’existe pas sous forme déterministe fiable.

## SOUS-AUDIT DOCUMENTAIRE — RAVE / MINI SPI / MPI — EN COURS

Règles :
- ne pas stocker un manuel complet ou du texte copyrighté dans la base ; extraire uniquement des faits techniques structurés ;
- chaque fait doit conserver `source`, `document`, `famille/variant`, `rubrique`, `page/section si disponible`, `niveau de preuve` et `notes/conflits` ;
- priorité au niveau **`verifie_constructeur`** quand la donnée vient directement d’un manuel Rover/MG/RAVE identifiable ;
- ne pas écraser les données projet déjà validées ; en cas de divergence, conserver le conflit explicitement ;
- cibles prioritaires : valeurs normales/de contrôle, valeurs d’origine de réglages, procédures de test, rôle des capteurs/actionneurs, symptômes et conséquences d’un défaut, contrôles électriques/mécaniques, conditions de test, DTC et stratégie ECU lorsqu’elle est documentée ;
- les données spécifiques SPi et MPi doivent rester séparées ; ne pas généraliser une valeur d’un système à l’autre sans preuve ;
- **toute donnée RAVE vérifiée utile doit être intégrée à la base de connaissances**, pas seulement au rapport documentaire.

Lots déjà intégrés sur `MEMSX64` :
- `research_enrichment_1660.qz64` : réglage câble/papillon et premier ensemble de faits RAVE constructeur ;
- `research_enrichment_1670.qz64` : brochages Mini MPi 97MY RCL0194 exacts ;
- base experte reste en révision r20, avec lots d’enrichissement versionnés.

## EXIGENCE UI IA — FICHIERS CSV/TXT

À intégrer dans l’onglet **IA MEMS** après validation du runtime, sans changer le style dark/responsive : glisser-déposer `.csv/.txt`, bouton `+`, lecture locale read-only, aucune commande ECU issue d’un fichier importé.

## EXIGENCE UI IA — NOUVELLE ZONE DE SAISIE

Zone de saisie type composeur moderne, dark/responsive, multiligne, bouton `+` à gauche et bouton rond orange/flèche vers le haut à droite. Ne pas changer le thème global.

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

**Exécuter le probe Windows x64 ONNX Runtime GenAI 0.14.0 + Qwen3-0.6B ONNX INT4 CPU, sans modifier encore `IaMemsService`/`LocalAiClient`. Si et seulement si le chargement + génération `OK` sont verts sous Windows, remplacer ensuite proprement llama.cpp par ONNX Runtime GenAI dans l’application. Continuer RAVE après stabilisation du moteur. Aucun changement protocole ECU, aucun 32 bits et aucun BUILD #31.**