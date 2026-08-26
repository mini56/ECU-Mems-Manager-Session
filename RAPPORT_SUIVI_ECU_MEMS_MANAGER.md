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
- HEAD courant avant intégration native ONNX : **`bf7c6dde6f74daaf59a80f4c121870d6d8ea995e`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.
- Décision IA du 26 août 2026 : **abandonner llama.cpp comme moteur x64 actif et intégrer ONNX Runtime GenAI 0.14.0 directement dans MEMS Manager**, après validation du probe Windows #75.

## ARCHITECTURE IA CIBLE — INTÉGRATION EN COURS

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

- Moteur génératif **dans le processus MEMS Manager**, sans `QProcess`, sans `llama-server.exe`, sans serveur HTTP local et sans port localhost.
- Chargement/génération à maintenir hors thread UI afin de préserver la réactivité.
- Base experte r20 préconstruite, lecture seule.
- Mesures ECU read-only ; aucune commande ou mutation accessible au LLM.
- Modèle de test retenu : Qwen3-0.6B ONNX INT4 CPU, snapshot et hashes figés par la CI.
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

## CAUSE PERFORMANCE ISOLÉE — HISTORIQUE LLAMA

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

- Le dossier runtime ne reçoit plus que `llama.dll`, `ggml.dll`, `ggml-base.dll` et **`ggml-cpu-x64.dll`** côté CPU.
- Cette voie d’isolement est **ABANDONNÉE** : elle constitue un contournement matériel et ne répond pas à l’exigence d’un MEMS Manager x64 général pour des PC Intel/AMD différents.

### ECU MEMS Manager x64 #67 — Commit `634fce0` — ROUGE

- Run GitHub : **`32955046246`** ; job : **`98134767273`**.
- `llama-server.exe --version` échoue après staging avec **`0xC0000135 = STATUS_DLL_NOT_FOUND`**.
- Cause : staging incomplet des DLL partagées non-CPU (`llama-common.dll`, `mtmd.dll`, `llama-server-impl.dll`, etc.).
- Aucun artefact #67.

### Décision utilisateur — runtime officiel llama.cpp b10516 — HISTORIQUE

Décision utilisateur du 26 août 2026 : **prendre l’officiel et ne plus appliquer de rustine**.

- archive officielle `llama-b10516-bin-win-cpu-x64.zip` ;
- SHA-256 `fbbbc55e0eb2e1b07f9dcb9488616c98ed47d9003b90e15e7c8c7812c4307cd3` ;
- runtime complet et multi-variantes conservé intact.

### ECU MEMS Manager x64 #68 — Commit `c7164e4` — VERT CI, ÉCHEC PC IA

- Run GitHub : **`32962220202`** ; job : **`98156880259`**.
- Artifact : **`9598541996`** — `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`.
- Taille artifact : **147 504 305 octets**.
- Artifact digest SHA-256 : **`39507831b6a5ceba22d315617938093fe24250789986c30ff208fc2dc1f4494b`**.
- CI : `llama-server --version`, chargement Qwen, `/health`, `/v1/models`, chat API et smoke app VERTS.
- **Test PC réel : `QProcess 0 / FailedToStart` à l’ouverture de IA MEMS.** La CI ne reproduit donc pas le comportement du PC utilisateur.

### #72 — dépendances VC++ app-local — VERT CI, ÉCHEC PC IDENTIQUE

- Commit **`6284a2ecd02d0b5b191faac9cfcf733b5e463233`**.
- Déploiement local des dépendances Microsoft VC++ v14 détectées par imports PE.
- CI VERT.
- **Test PC réel : toujours `QProcess 0 / FailedToStart`.**

### #73 — retour source-build llama — ROUGE, VOIE ARRÊTÉE

- Le serveur démarre et `--version` fonctionne, mais Qwen replante au chargement avec **`-1073740791 = 0xC0000409`**.
- Cette tentative a réintroduit une reconstruction llama déjà abandonnée ; ne plus poursuivre cette boucle.

## BASCULE ONNX RUNTIME GENAI — DÉCISION UTILISATEUR

Après constat que les variantes llama ramènent soit `QProcess 0` sur le PC réel soit `0xC0000409` au chargement Qwen, l’utilisateur autorise explicitement le remplacement du moteur : **`OK ALORS CHANGONS`**, puis **`ok go`**.

Cible retenue :
- **ONNX Runtime GenAI 0.14.0** officiel, Windows x64 CPU ;
- API native C/C++ ;
- Qwen3-0.6B ONNX INT4 CPU ;
- aucun service externe ; aucun serveur localhost ; aucun `QProcess` pour le moteur IA ;
- ExpertEngine, ExpertKnowledgeReader, base r20, réponses déterministes, UI et protocole conservés.

### #74 — probe ONNX — ROUGE UNIQUEMENT SUR SORTIE CONSOLE

Le modèle a chargé et généré une réponse, mais le script Python a échoué en affichant un emoji sur la console Windows CP1252 : `UnicodeEncodeError`. Ce défaut n’est pas un échec ONNX/modèle.

### #75 — probe ONNX Windows x64 — VERT

- HEAD : **`bf7c6dde6f74daaf59a80f4c121870d6d8ea995e`**.
- Run : **`32977752270`**.
- Conclusion : **SUCCESS**.
- ONNX Runtime GenAI 0.14.0 Windows x64 officiel vérifié par SHA-256 : **VERT**.
- Qwen3-0.6B ONNX INT4 CPU, snapshot figé et fichiers principaux vérifiés par SHA-256 : **VERT**.
- Chargement réel du modèle + génération réelle : **VERT**.
- Artifact `ONNX-Windows-x64-Probe-BUILD-30`, ID **`9610425335`**, taille **659 octets** : il contient seulement le rapport de probe, **ce n’est pas MEMS Manager et il ne doit pas être testé comme application**.

### ÉTAPE EN COURS — INTÉGRATION ONNX DANS LE VRAI MEMS MANAGER

Autorisation utilisateur : **oui, intégrer maintenant dans le build**.

Objectif obligatoire :
- remplacer l’intérieur de `LocalAiClient` sur la voie x64 par ONNX Runtime GenAI natif ;
- conserver l’interface/signaux existants utilisés par `IaMemsService` (`Starting`, `Ready`, `Busy`, `Error`, `responseReady`, `responseError`) ;
- charger le modèle une seule fois sur un thread dédié et exécuter la génération hors thread UI ;
- supprimer de la voie x64 l’usage de `QProcess`, `llama-server.exe`, HTTP localhost et port 18089 ;
- compiler/lier contre le package officiel ONNX Runtime GenAI 0.14.0 Windows x64 CPU vérifié ;
- empaqueter toutes les DLL runtime ONNX requises et le dossier complet du modèle Qwen ONNX ;
- ajouter un self-test natif utilisant le même `LocalAiClient` que l’application, puis smoke-launcher le package réel ;
- restaurer l’artefact complet `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30` au lieu du rapport de probe de 659 octets ;
- aucun changement protocole ECU, 32 bits, UI, base experte ou numéro BUILD.

## AUDIT IA DES QUESTIONS POSSIBLES — EN COURS

Principe retenu : **toute connaissance certaine déjà présente dans le logiciel, ses libellés/aides, son décodage ou la base experte doit produire une réponse immédiate, sans appel au modèle génératif**. Le modèle est réservé au croisement de plusieurs faits, au raisonnement diagnostic, aux questions générales ou aux cas où la réponse n’existe pas sous forme déterministe fiable.

### Phase A — inventaire initial constaté

**Aperçu / cadrans actuellement présents** : régime moteur, température liquide, MAP, position papillon, tension batterie, correction carburant court terme, tension lambda, temps injecteur, température air admission, position IAC, avance allumage et état système. L’ancien UI contient aussi l’état contact de ralenti, boucle fermée et fonctions d’enregistrement.

**Connaissances déjà intégrées à l’UI** à exploiter en réponse immédiate :
- lambda : 0–200 mV indiqué comme mélange pauvre ; 700–900 mV comme mélange riche ;
- MAP : moteur arrêté ~100 kPa ; ralenti ~25–40 kPa ; valeur anormale → vérifier notamment les durites/dépressions ;
- affichage Aperçu : plages visuelles déjà codées pour batterie, liquide, air admission, avance et régime ; distinguer ces repères des spécifications constructeur.

**Réglages réels présents** : correction carburant, position ralenti chaud, vitesse de ralenti, correction d’avance, remise à zéro réglages et reset ECU.

**Actionneurs réels présents** : chauffage collecteur/PTC, pompe à carburant, chauffage sonde O2/lambda, électrovanne purge canister, embrayage de climatisation, électrovanne de pression/boost, ventilateurs 1/2/3, injecteur, bobine d’allumage, moteur IAC et remise à zéro de tous les actionneurs.

**Erreurs mémorisées affichées** : codes 01 à 24 ; la correspondance Code 23 / antidémarrage reste en audit et ne doit pas être promue sans preuve.

**Réponses immédiates déjà codées** : batterie, régime, température liquide, MAP, lambda, avance, dwell, ralenti/IAC, papillon, état moteur et diagnostic de cohérence.

### Grille de questions

Pour chaque mesure/cadran, réglage, actionneur et DTC : rôle, mesure/décodage, unité, valeur normale/référence, variation, trop haut/bas, symptômes, causes, contrôles prioritaires, réglage/apprentissage/risque, résultat attendu des tests actionneurs et interprétation des DTC.

## SOUS-AUDIT DOCUMENTAIRE — RAVE / MINI SPI / MPI — EN COURS

Règles :
- ne pas stocker les manuels complets ; extraire des faits techniques structurés ;
- conserver source/document/famille/page/niveau de preuve/conflits ;
- séparer strictement SPi, MPi et cas particuliers ;
- priorité aux preuves constructeur RAVE/Rover ;
- **tout fait vérifié utile doit être intégré à la base de référence**, pas seulement conservé dans l’audit.

### Faits déjà classés

- ralenti SPi 1993–96 : **850 ±25 tr/min** ; SPi 1997+ et MPi : **900 ±50 tr/min** ;
- pression carburant : SPi ~1 bar ; MPi **3,0 ±0,2 bar** ;
- IACV, TPS, MAP, ECT, IAT, CKP, CMP, lambda, injecteurs, pompe, purge, ventilateurs et stratégies de secours documentés ;
- ventilateur MPi 97MY : **105 °C ON / 98 °C OFF** ; SPi Japon : **98/93 °C** ;
- ne pas régler le ralenti par la vis de butée papillon ;
- conflits de résistance bobine conservés au lieu d’être arbitrés sans preuve.

### Enrichissements base déjà poussés

- lot **1660** : faits RAVE pratiques/constructeur ;
- lot **1670** : brochages MPi 97MY RCL0194 et faits associés ;
- base experte r20 générée avec ces enrichissements sans changement de schéma/révision.

### Phase TestBook / codes / brochages

- documentation Rover/TestBook distingue Mini MEMS 1.3 SPi, MEMS 1.6 SPi et MEMS 2J MPi ; ne pas appeler automatiquement Mini MPi « MEMS 1.9 » ;
- `0x80` confirme fortement codes 1 = ECT, 2 = IAT, 10 = pompe carburant, 16 = TPS sur Mini SPi ;
- `0x7D:0x05` recoupe 20 = chauffage lambda, 21 = synchronisation, 22 = ventilateur 1, 24 = ventilateur 2 ;
- **Code 23 / bit 6 reste `preuve_insuffisante`** : RCL0194 prouve un circuit antidémarrage sur certaines variantes mais pas la correspondance du bit DTC ; RoverMEMS/ROSCO/librosco laissent ce masque non identifié ;
- RCL0194 MPi 97MY fournit des pins exacts déjà classés, notamment MAP `C159-8`, retour capteurs `C159-13`, IAT `C159-14`, ECT `C159-36`, relais pompe `C159-30`, signal antidémarrage `C159-17` ;
- ne pas inventer les pins SPi ancien tant que le schéma constructeur lisible ne les donne pas.

## EXIGENCE UI IA — FICHIERS CSV/TXT

À intégrer après stabilisation du moteur, sans changer le style dark/responsive : glisser-déposer `.csv/.txt`, bouton `+`, filtre Windows, fichier sélectionné amovible, analyse locale read-only, aucune commande ECU.

## EXIGENCE UI IA — NOUVELLE ZONE DE SAISIE

Composeur dark/responsive arrondi, multiligne, bouton `+` à gauche, bouton rond orange avec flèche vers le haut à droite ; aucun changement global de thème.

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

**Intégrer ONNX Runtime GenAI 0.14.0 + Qwen3-0.6B ONNX INT4 CPU dans le vrai `LocalAiClient` de `MEMSX64`, sur thread dédié, sans QProcess/HTTP/llama-server ; restaurer le packaging complet BUILD #30 et produire un artefact complet testable sur le PC réel. Aucun changement protocole ECU, 32 bits ou BUILD #31. Après cette intégration/validation, reprendre l’enrichissement RAVE de la base.**