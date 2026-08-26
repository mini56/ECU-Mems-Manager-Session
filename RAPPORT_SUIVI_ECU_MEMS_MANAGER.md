# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager. Il constitue la source de vérité de continuité du projet.
>
> Branche rapport : `RAPPORT`. Branche de développement x64 : `MEMSX64`.
>
> **RÈGLE DE SUIVI IMMÉDIAT : AVANT CHAQUE NOUVELLE ÉTAPE, inscrire ici l’étape exacte et son objectif. Dès que cette étape produit un résultat, inscrire immédiatement ce résultat AVANT de commencer l’étape suivante. Ne jamais attendre la fin d’un lot, d’un build ou d’une discussion.**

> **RÈGLE DE NOMMAGE GITHUB POUR LES ÉCHANGES AVEC L’UTILISATEUR : annoncer une exécution sous la forme `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. Le `#NN` affiché par GitHub est le numéro d’exécution du workflow, pas le numéro de BUILD logiciel. Éviter les formulations ambiguës du type « build #44 » pour parler d’une Action GitHub.**

## JOURNAL IMMÉDIAT — 25/26 août 2026

### Architecture IA x64 retenue

- `llama-server.exe` sidecar x64 hors processus ; ne pas lier libllama/ggml dans l’exécutable ECU.
- Durée de vie IA application, pas widget ; IA MEMS reste une vue/client.
- Loopback local ; base experte r20 préconstruite en CI puis lue `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`.
- RAG : mesures ECU read-only -> moteur expert -> faits pertinents -> Qwen -> texte ; aucun accès LLM aux commandes ECU.
- Qwen3-0.6B-Q8_0 + llama.cpp b10516 conservés.

### BUILD #30 ouvert explicitement par l’utilisateur

- BUILD actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- `lab-expert-engine` et `MEMSX64-BUILD26-BASE` restent intacts.
- Aucun changement protocole ECU autorisé pendant la stabilisation IA.

### Désinstallation BUILD #30

- `uninstaller.cpp` existe et CMake crée `ecu_mems_uninstaller`.
- #30 doit livrer `ecu_mems_uninstaller.exe` + `install_manifest.txt` complet.
- Le code existant refuse si l’app tourne, conserve le profil par défaut, supprime les données locales seulement sur choix explicite, et préserve les fichiers étrangers.

### BUILD #30 avant reconstruction propre

- HEAD initial #30 : `600b8ef8607eb3dc7d591f675e9f33be0cdb0911` — `BUILD #30 clean x64 IA package and uninstaller`.
- Correction chemin base r20 : `879077a678f4c203124907dabdeb42b532c9d337`.
- ECU MEMS Manager x64 #44 / run `32890600398` : base r20, runtime, Qwen et package valides ; rouge uniquement au smoke API car `message.content` était vide avec une enveloppe courte.
- Correction smoke-test CI : `414ea52970e02fb6077c94ca2aa7aec3e92d7383`, `--reasoning off` uniquement pour le serveur du test GitHub.
- ECU MEMS Manager x64 #45 / run `32893817192` : **SUCCESS / VERT** jusqu’à l’upload.
- Artifact #45 : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9580850077`, taille `668890922`, archive SHA-256 `1db3438593c65f7f77176910e55e9de0a428208f9c3a7732b74d6e35290ed3d0`.

### Test PC réel — ECU MEMS Manager x64 #45 — Commit `414ea52`

- Résultat utilisateur : **CRASH immédiat à l’ouverture de l’onglet IA MEMS**, identique au crash précédemment observé.
- Le smoke launch CI n’ouvrait pas l’onglet IA et ne pouvait donc pas détecter ce défaut.
- Les validations séparées de `llama-server.exe`, Qwen, SQLite r20 et du package restaient vertes.

### Constat architectural après le crash PC

L’utilisateur rappelle qu’il avait demandé de **refaire l’IA sur une architecture propre**, et non d’empiler les anciens correctifs. Inspection effectuée :

- `navigationorderpatch.cpp` crée déjà directement `IaMemsTab` à la position officielle n°7 ; l’ancien installateur IA global était redondant.
- `iamemstab_clean.cpp` réincluait `iamemstab.cpp` puis ajoutait un `eventFilter` global qui reparente `LocalAiClient` pendant `QEvent::Show`.
- `iamemsqualitypatch.cpp` et `iaresponsecontextpatch.cpp` ajoutaient encore d’autres filtres globaux autour de l’IA.
- L’application mélangeait donc une vue IA, des wrappers, plusieurs hooks de démarrage et des patches globaux.
- Le code de `LocalAiClient` injectait encore `/no_think` dans les requêtes et dans le prompt système : le raisonnement Qwen n’était donc pas réellement actif dans l’application, contrairement à l’intention annoncée.
- Le BUILD #26 PC stable n’embarquait pas le vrai runtime Qwen ; sa stabilité ne validait pas le chemin sidecar complet actuel.

### Reconstruction IA propre — effectuée sur `MEMSX64`

Architecture désormais appliquée :

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService (service application) -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> llama-server.exe -> Qwen3`

Changements réalisés :

- `navigationorderpatch.cpp` reste l’unique créateur/intégrateur de l’onglet IA ; ordre officiel des 14 onglets inchangé.
- `iamemstab.cpp` est compilé directement et devient une vue simple : aucun installateur global, aucun wrapper, aucun `eventFilter` IA, aucun propriétaire direct de `LocalAiClient`, aucun thread de reconstruction SQLite.
- Nouveau `expert/IaMemsService.h/.cpp` : service IA unique, parenté à l’application dès sa création ; il possède `LocalAiClient`, `ExpertEngine`, `ExpertKnowledgeReader`, contexte et historique.
- La base de production utilisée par l’application est directement `database/expert/ia_mems_reference_r20.sqlite` du package, ouverte en lecture seule. `ExpertRuntimeDatabase` reste uniquement dans le self-test/générateur CI.
- L’ouverture de l’onglet active le service : ouverture read-only de la base empaquetée puis démarrage du sidecar ; aucun reparentage pendant `Show`.
- `iamemstab_clean.cpp` supprimé du dépôt.
- `iamemsqualitypatch.cpp` supprimé du dépôt ; la barre de défilement du transcript est configurée directement dans la vraie vue.
- `iaresponsecontextpatch.cpp` supprimé du dépôt ; l’horodatage des mesures est traité directement dans le service.
- `CMakeLists.txt` ne compile plus aucun de ces anciens patches/wrappers IA.
- `/no_think` supprimé des requêtes et du prompt de l’application ; Qwen garde son raisonnement natif actif dans MEMS Manager.
- Le serveur réel utilise `--no-webui --offline`; le `--reasoning off` reste exclusivement dans le smoke-test CI court.
- IA reste lecture seule vis-à-vis de l’ECU : elle reçoit uniquement les mesures déjà acquises par `MEMSInterface` et n’a aucune autorité de commande/mutation.
- Formule ralenti chaud conservée selon la règle validée : `raw - 32768 - correction`.
- Aucun changement du protocole ECU, de la branche 32 bits, du rollback, du modèle Qwen ou du style dark/responsive.

### Push final reconstruction propre

- HEAD final reconstruction : **`776fc647a874564c932bf09e8871cb771a0ed258`** — `BUILD #30 rebuild IA on clean application service`.
- Run : **ECU MEMS Manager x64 #59 — Commit `776fc64`** / `32898631148`.
- Résultat : **VERT** ; artifact `9582674702` ; SHA-256 `ee7f12e933a17c884c6ca081c583a542f45569f556a68a50f124b58b5f13fcff`.
- Test PC : crash d’ouverture IA supprimé ; `base prête` + `IA locale prête` atteints.

### Correction question/réponse — #60

- HEAD : **`be4916a53321e36573729e123b14c2cf120fd734`** — `BUILD #30 fix Qwen thinking responses`.
- Date locale injectée ; anti-écho ; paramètres thinking Qwen3.
- **ECU MEMS Manager x64 #60 — Commit `be4916a` — VERT** / run `32901653203` / artifact `9583795907` / SHA-256 `b03616086bc7bba254b8b089bff474ce275f28cc92b19a30db7300aeb6be9f4a`.
- Test PC : date correcte, mais latence, contamination d’historique et hallucination IAC confirmées.

### Routage IA rapide — #61

- HEAD `MEMSX64` : **`126cc638d584975a78d0101430d61bdc435c5879`** — `BUILD #30 route IA fast and reasoning responses`.
- Modification unique : `expert/LocalAiClient.cpp`.
- Date courante : réponse immédiate sans Qwen.
- IAC : réponse contrôlée `Idle Air Control`.
- Question générale simple : `/no_think`, budget 256 tokens.
- Diagnostic/analyse : `/think`, budget 768 tokens.
- Historique : dernier tour uniquement pour une vraie relance détectée.
- Raisonnement Qwen conservé pour les tâches complexes.

### ECU MEMS Manager x64 #61 — Commit `126cc63` — VERT

- Run GitHub : **`32904830665`** ; job : **`97986482796`**.
- Les 20 étapes fonctionnelles sont vertes, jusqu’au smoke launch, hashes et upload.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9584843179`**.
- Taille : **668869675 octets**.
- SHA-256 archive GitHub : **`cc9cd0e2f299d563e469cac08c2d86bfe7ab52f998c7f4568d86ca3fe53aa207`**.

### Test PC réel — ECU MEMS Manager x64 #61 — Commit `126cc63`

Résultat utilisateur sur PC réel, captures du 26 août 2026 :

- **Les réponses simples sont un peu plus rapides.** Le routage rapide apporte donc un gain perceptible, mais encore insuffisant pour considérer l’IA finalisée.
- IAC : la définition contrôlée est maintenant correcte et en français : `Idle Air Control`, régulation de l’air de ralenti.
- Relance `OU IL EST PLACÉ ?` après IAC : réponse inutile `IL EST PLACÉ.` ; le petit modèle ne traite pas correctement cette relance. Le routage des suivis doit utiliser le fait précédent/base plutôt que laisser Qwen improviser sans information de localisation.
- Question `QUEL VALEUR POUR LA BOBINE ?` : réponse `Je n'ai encore reçu aucune mesure ECU.`. Cause isolée dans `IaMemsService::groundingFor()` : le mot `valeur` déclenche `currentValuesAnswer()` avant la recherche de connaissance. Une demande de **valeur de référence** est donc confondue avec une demande de **mesure actuelle**. La référence validée dwell bobine ≈ **1,9–3,1 ms vers 14 V** doit être routée comme connaissance/référence, pas comme mesure live.
- Date : réponse immédiate correcte (`2026-08-26`).
- Question générale `C'EST QUOI UN MOTEUR 4 TEMPS ?` : réponse en français et plus rapide, mais formulation médiocre (`temps fourni`) ; qualité générale limitée du Qwen3 0.6B confirmée.
- Question `EXPLIQUE MOI LE ROLE DE L'ECU ?` : réponse française mais contient des fonctions automobiles inventées/hors sujet (`gestion des voies`, `auto-stop`, `auto-accélération`, `auto-remise de la clé`). Il faut donc davantage de réponses contrôlées/base pour le domaine automobile au lieu de confier ces définitions au modèle général.
- `QUEL TYPE D'ECU MEMS EXISTE ?` : réponse trop vague et non exploitable.
- `1.6` seul : le routeur produit le fallback générique puis le modèle ne produit pas de réponse exploitable.
- `MEMS 1.6` : **réponse en anglais et techniquement fausse pour notre domaine**, Qwen interprétant MEMS comme `Micro-Electro-Mechanical System`. C’est un défaut critique de routage de domaine : dans IA MEMS, `MEMS` doit signifier en priorité le système de gestion moteur Rover/Lucas MEMS, jamais le domaine générique des microsystèmes, sauf demande explicite de l’utilisateur.
- `REPOND EN FRANCAIS` : le modèle n’arrive pas à se recaler de façon fiable et finit par un échec exploitable. La langue ne doit pas dépendre seulement d’une consigne système en anglais au petit modèle ; le français doit être **imposé par le routeur** lorsque l’interface est française.
- **Défaut navigation responsive sur ce PC** : lorsque l’utilisateur sélectionne `Test ECU 1.9`, `Aperçu` disparaît visuellement du haut de la barre latérale. Inspection de `navigationorderpatch.cpp` : `tabs.currentChanged` appelle `nav->setCurrentRow(index)` ; `QListWidget` fait alors défiler automatiquement l’élément courant dans sa fenêtre. Sur ce PC, la hauteur disponible n’affiche pas simultanément les 14 lignes, donc la sélection de la ligne 14 décale la liste d’une ligne et masque `Aperçu`. **Aperçu n’est pas supprimé ; la liste est simplement scrollée.** Ce comportement viole l’exigence responsive : les 14 onglets doivent rester visibles dans le panneau latéral quand la résolution le permet, sans faire disparaître le premier onglet lors de la sélection du dernier.

### Corrections requises avant nouvelle validation PC

Sans créer BUILD #31 et sans toucher au protocole ECU :

1. **Langue** : imposer la langue active de l’interface au niveau de chaque requête/réponse ; en français, une réponse anglaise non demandée doit être refusée/reformulée ou remplacée par une réponse contrôlée.
2. **Domaine MEMS** : ajouter un garde-fou fort `MEMS = Rover/Lucas engine management` dans le contexte IA MEMS et utiliser la base experte avant le savoir général ; ne jamais laisser Qwen développer MEMS comme `Micro-Electro-Mechanical System` dans ce contexte.
3. **Références techniques** : distinguer demande de valeur de référence et demande de mesure actuelle ; pour la bobine/dwell, fournir la référence validée 1,9–3,1 ms vers 14 V lorsqu’elle est demandée.
4. **Définitions automobile/MEMS** : augmenter les réponses contrôlées/base pour ECU, familles MEMS, IAC et autres notions centrales afin d’éviter les hallucinations du 0.6B.
5. **Relances** : les relances courtes (`où il est placé ?`, `et pourquoi ?`) doivent conserver le fait technique précédent pertinent, pas seulement le texte brut du dernier tour.
6. **Navigation** : adapter la hauteur/hauteur de lignes de `uiRebuildNav` à l’espace disponible pour que les 14 entrées restent visibles ; ne pas corriger en réordonnant les onglets ni en supprimant le comportement responsive.

---

## ÉTAT DE RÉFÉRENCE

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD actuel : **`126cc638d584975a78d0101430d61bdc435c5879`**.
- Rapport : `RAPPORT`.
- 32 bits : `lab-expert-engine`, ne pas toucher.
- Rollback x64 : `MEMSX64-BUILD26-BASE`, ne pas toucher.
- BUILD actif : #30 / v1.0.30.

### Références historiques

- #26 `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, run `32816285887` SUCCESS ; navigation/onglet IA PC stables sans runtime Qwen emballé.
- #27 `a6f9b209f32b6dd77774832e8c84469c53deca47`, run `32832192437` SUCCESS ; AANMP002/MNE101150, COM3 FTDI, ROSCO 1.3/1.6, Injection RAM Mode4 ≈2,63 ms.
- #28 `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`, run `32842049458` SUCCESS.
- #29 final `fee195e88d3615613b8f92de83209da2cf8247c2`.
- #30 reconstruction propre `776fc647a874564c932bf09e8871cb771a0ed258`, #59 SUCCESS.
- #30 réponses Qwen `be4916a53321e36573729e123b14c2cf120fd734`, #60 SUCCESS.
- #30 routage rapide `126cc638d584975a78d0101430d61bdc435c5879`, #61 SUCCESS ; test PC : vitesse améliorée, mais langue/domaine MEMS/qualité et navigation responsive encore à corriger.

## VERSIONNAGE

- #29 = v1.0.29 ; #30 = v1.0.30 ; #100 = v1.1.0.
- Formule `1.(build / 100).(build % 100)`.
- Les correctifs et reruns actuels restent BUILD #30 ; aucun BUILD #31 sans demande explicite.

## UI OFFICIELLE À PRÉSERVER

Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9. Style dark/responsive inchangé.

## SÉCURITÉ PROTOCOLE À PRÉSERVER

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 normal seulement ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique ; mutations Rosco13_16 prouvé + Normal ; unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués sans sous-type ; RAM transaction bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);`.
- D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction` avec correction Réglages réelle ; jamais -3 hardcodé.
- Dwell ≈1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant validation #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports série arbitraires, profils RAM non validés, reset/clear faults/trims/écritures pendant #30.

## PROCHAINE ACTION EXACTE

**Après autorisation utilisateur : corriger dans le même BUILD #30 les six points issus du test PC #61 : langue active imposée, garde-fou domaine Rover/Lucas MEMS, distinction valeur de référence/mesure live, définitions techniques contrôlées, relances contextualisées, et sidebar responsive gardant les 14 onglets visibles. Aucun changement protocole ECU et aucun BUILD #31.**