# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` est le numéro GitHub Actions, pas le BUILD logiciel.

## ÉTAT ACTUEL

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD courant : **`634fce02bd92b3048cd402c147ce3e9cc84a2103`**.
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

### ECU MEMS Manager x64 #66 — Commit `87cb4cd` — ROUGE

- Run GitHub : **`32948276603`** ; job : **`98113701995`**.
- Étapes 1 à 16 : **VERTES**.
- Échec unique : étape 17 `Validate packaged adaptive llama server model and API`.
- `llama-server.exe --version` fonctionne : `0.1.2-dev (build 1, commit b95502b)`, MSVC x64.
- Au lancement avec Qwen, le processus quitte pendant le chargement du modèle avec **`-1073740791`**, soit **`0xC0000409`** sous Windows.
- Le crash intervient donc après démarrage du serveur et avant disponibilité `/health` ; ce n’est pas un échec CMake, ni une absence simple de `llama-server.exe`.
- Étapes 18 à 20 sautées ; aucun artefact #66.
- Le changement nouveau par rapport au runtime stable #63 est le runtime partagé + chargeur de backends CPU multi-variantes.

### Étape 4 — isolement runtime x64 partagé — POUSSÉE

Autorisation utilisateur : **`OK TU POUSSE SUR GITHUB`**.

Commit **`634fce02bd92b3048cd402c147ce3e9cc84a2103`** — `BUILD #30 isolate shared llama x64 backend`.

Correction limitée à `.github/workflows/memsx64.yml` :
- la compilation conserve `BUILD_SHARED_LIBS=ON`, `GGML_BACKEND_DL=ON` et `GGML_CPU_ALL_VARIANTS=ON` afin de ne pas modifier le profil de build #66 ;
- CI vérifie toujours que plusieurs variantes ont bien été produites ;
- le dossier runtime ne reçoit plus que `llama.dll`, `ggml.dll`, `ggml-base.dll` et **`ggml-cpu-x64.dll`** ;
- aucune DLL CPU `haswell`, `skylakex`, `sandybridge`, etc. n’est exposée au chargeur pendant ce test ;
- validations package, manifest, hashes et smoke imposent exactement un backend CPU : `ggml-cpu-x64.dll` ;
- le modèle Qwen, l’application, la base, l’UI, le protocole ECU, le 32 bits et le numéro BUILD restent inchangés.

### ECU MEMS Manager x64 #67 — Commit `634fce0` — EN COURS

- Run GitHub : **`32955046246`**.
- État au premier contrôle : **queued**.
- Objectif du run : déterminer si Qwen charge correctement avec le runtime partagé quand **seul le backend x64 de base** est disponible.

## AUDIT IA DES QUESTIONS POSSIBLES — DÉMARRÉ EN LECTURE SEULE

Autorisation utilisateur reçue pendant l’exécution #66 : **commencer immédiatement l’audit sans modifier le code**.

Principe retenu : **toute connaissance certaine déjà présente dans le logiciel, ses libellés/aides, son décodage ou la base experte doit produire une réponse immédiate, sans appel Qwen**. Qwen est réservé au croisement de plusieurs faits, au raisonnement diagnostic, aux questions générales ou aux cas où la réponse n’existe pas sous forme déterministe fiable.

### Phase A — inventaire initial constaté

**Aperçu / cadrans actuellement présents** : régime moteur, température liquide, MAP, position papillon, tension batterie, correction carburant court terme, tension lambda, temps injecteur, température air admission, position IAC, avance allumage et état système. L’ancien UI contient aussi l’état contact de ralenti, boucle fermée et fonctions d’enregistrement.

**Connaissances déjà intégrées à l’UI** à exploiter en réponse immédiate :
- lambda : 0–200 mV indiqué comme mélange pauvre ; 700–900 mV comme mélange riche ;
- MAP : moteur arrêté ~100 kPa ; ralenti ~25–40 kPa ; valeur anormale → vérifier notamment les durites/dépressions ;
- affichage Aperçu : plages visuelles déjà codées pour batterie, liquide, air admission, avance et régime ; ces plages doivent être distinguées des spécifications constructeur lorsqu’elles ne sont que des seuils/repères MEMS Manager.

**Réglages réels présents** : correction carburant, position ralenti chaud, vitesse de ralenti, correction d’avance, remise à zéro réglages et reset ECU. Aides déjà codées :
- vitesse ralenti réglable par pas de 50 tr/min ; `0` = valeur MEMS d’origine ; exemple documenté A+ SPi chaud = 850 tr/min ; agit sur ralenti froid et chaud ;
- correction carburant : agit sur les émissions au ralenti si non régulé lambda ; les moteurs régulés lambda réadaptent cette valeur ;
- position ralenti chaud : MEMS la réapprend avec le temps, donc une correction manuelle n’est pas nécessairement permanente ;
- avance : cartes historiques adaptées à des carburants 91/95 RON ; trop d’avance peut endommager le moteur ; ce réglage n’agit pas sur l’avance au ralenti ;
- aide écran : régime jusqu’à 2000 tr/min pour réglage fin du ralenti.

**Actionneurs réels présents** : chauffage collecteur/PTC, pompe à carburant, chauffage sonde O2/lambda, électrovanne purge canister, embrayage de climatisation, électrovanne de pression/boost, ventilateurs 1/2/3, injecteur, bobine d’allumage, moteur IAC et remise à zéro de tous les actionneurs. IAC : déplacement demandé par pas de 25 %. Avertissement déjà codé pour le chauffage lambda : durée limitée et ne pas démarrer immédiatement le moteur après chauffage avant refroidissement suffisant.

**Erreurs mémorisées affichées** :
01 température liquide ; 02 température air admission ; 03 non documenté dans l’UI ; 04 pression boost élevée ; 05 température air ambiant ; 06 température carburant ; 07 cliquetis détecté ; 08 non documenté ; 09 jauge température ; 10 circuit pompe carburant ; 11 non documenté ; 12 commande embrayage climatisation ; 13 vanne purge ; 14 capteur MAP ; 15 vanne de contrôle boost ; 16 circuit capteur position papillon ; 17/18/19 non documentés ; 20 alimentation chauffage lambda ; 21 synchronisation vilebrequin ; 22 commande ventilateur 1 ; 23 commande antidémarrage ; 24 commande ventilateur 2.

**Anomalies live déjà documentées** :
- signal régime / capteur vilebrequin : indicateur attendu dès que le volant moteur tourne ;
- erreur signal lambda ;
- lambda anormalement haute : peut notamment orienter vers sonde usée ou câblage signal mal blindé, particulièrement près alternateur/HT ;
- lambda anormalement basse ;
- IAC en position minimale : peut indiquer que l’ECU peine à obtenir un ralenti assez bas ; vérifier réglage câble papillon et prises d’air admission ;
- compteur Jack au maximum : retour à zéro uniquement par reset ECU complet, puis surveillance dans Toutes les mesures.

**Réponses immédiates déjà codées aujourd’hui** : batterie, régime, température liquide, MAP, lambda, avance, dwell, ralenti/IAC, papillon, état moteur et diagnostic de cohérence. Le service sait aussi fournir valeurs courantes/historique et rechercher des faits dans la base experte.

### Premier écart majeur identifié

L’IA immédiate ne couvre actuellement qu’une **fraction** de ce que MEMS Manager sait déjà. Sont notamment absents comme familles déterministes complètes : correction carburant court terme, temps injecteur, température air admission, états boucle fermée/contact ralenti, explication de tous les réglages, explication de chaque actionneur, et couche structurée par erreur/DTC avec `signification → rôle → symptômes → causes → contrôles`.

### Grille de questions à construire pour chaque élément

Pour chaque mesure/cadran, réglage, actionneur et DTC connu, l’audit doit vérifier la disponibilité d’une réponse immédiate aux formulations :
- `C’est quoi ?` / `À quoi ça sert ?`
- `Qu’est-ce que ça mesure ?` / `Comment c’est mesuré ou décodé ?`
- `Quelle unité ?` / `Quelle valeur normale ou de référence ?`
- `Pourquoi cette valeur monte/baisse ?`
- `Si c’est trop haut / trop bas, ça veut dire quoi ?`
- `Qu’est-ce que ça peut provoquer ?`
- `Quelles causes possibles ?`
- `Qu’est-ce que je contrôle en premier ?`
- pour Réglages : `si j’augmente/diminue, qu’est-ce que ça change ?`, `valeur d’origine ?`, `est-ce que MEMS la réapprend ?`, `quel risque ?` ;
- pour Actionneurs : `qu’est-ce que le test doit faire ?`, `qu’est-ce que je dois entendre/voir ?`, `si rien ne se passe, quelles causes ?`, `quelles conséquences en fonctionnement ?` ;
- pour DTC : `j’ai cette erreur, ça fait quoi ?`, `ça peut provoquer quoi ?`, `le moteur peut-il continuer ?`, `quels contrôles en premier ?`.

### Suite exacte de l’audit

1. lire l’implémentation `MEMSInterface` des réglages/actionneurs pour connaître précisément les pas, commandes et limites sans les inventer ;
2. inventorier les faits déjà présents dans `ExpertRuntimeDatabase.cpp` et la base de référence ;
3. terminer l’inventaire complet des champs `7D/80` / Toutes les mesures / Toutes les données ;
4. comparer chaque famille de question à ce qui est déjà routé par `IaMemsService` / `IaResponseLogic` ;
5. produire la matrice finale `question → source fiable → réponse immédiate possible → donnée manquante → Qwen nécessaire ou non → niveau de preuve` ;
6. **aucune implémentation de l’audit avant validation du résultat avec l’utilisateur**.

## SOUS-AUDIT DOCUMENTAIRE — RAVE / MINI SPI / MPI — DÉMARRÉ

Demande utilisateur : rechercher sur le Web les documents constructeur Rover/Mini disponibles publiquement, en priorité **RAVE**, manuels d’atelier Mini **SPi** et **MPi**, et intégrer à la base uniquement les informations techniques vérifiables utiles aux réponses IA.

Règles de classement :
- ne pas stocker un manuel complet ou du texte copyrighté dans la base ; extraire uniquement des faits techniques structurés ;
- chaque fait doit conserver `source`, `document`, `famille/variant`, `rubrique`, `page/section si disponible`, `niveau de preuve` et `notes/conflits` ;
- priorité au niveau **`verifie_constructeur`** quand la donnée vient directement d’un manuel Rover/MG/RAVE identifiable ;
- ne pas écraser les données projet déjà validées ; en cas de divergence, conserver le conflit explicitement ;
- cibles prioritaires : valeurs normales/de contrôle, valeurs d’origine de réglages, procédures de test, rôle des capteurs/actionneurs, symptômes et conséquences d’un défaut, contrôles électriques/mécaniques, conditions de test, DTC et stratégie ECU lorsqu’elle est documentée ;
- les données spécifiques SPi et MPi doivent rester séparées ; ne pas généraliser une valeur d’un système à l’autre sans preuve.

Objectif : enrichir la matrice des **réponses immédiates** avec des données constructeur fiables et traçables, sans faire dépendre ces réponses de Qwen.

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

**Suivre ECU MEMS Manager x64 #67 — Commit `634fce0`. Si rouge, consigner l’étape et l’erreur exactes avant toute nouvelle correction. Si vert, consigner l’artefact puis décider du retour contrôlé des variantes optimisées. Audit IA/RAVE reste en lecture seule. Aucun BUILD #31.**