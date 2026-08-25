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
- Budget de réponse réel porté à 1024 tokens pour laisser de la place au raisonnement puis à la réponse visible.
- Le serveur réel utilise `--no-webui --offline`; le `--reasoning off` reste exclusivement dans le smoke-test CI court.
- IA reste lecture seule vis-à-vis de l’ECU : elle reçoit uniquement les mesures déjà acquises par `MEMSInterface` et n’a aucune autorité de commande/mutation.
- Formule ralenti chaud conservée selon la règle validée : `raw - 32768 - correction`.
- Aucun changement du protocole ECU, de la branche 32 bits, du rollback, du modèle Qwen ou du style dark/responsive.

### Push final reconstruction propre

- HEAD final `MEMSX64` : **`776fc647a874564c932bf09e8871cb771a0ed258`** — `BUILD #30 rebuild IA on clean application service`.
- Les commits intermédiaires produits pendant la reconstruction restent des commits du même BUILD #30 et leurs Actions ont été annulées par `cancel-in-progress`; ils ne sont pas des BUILD logiciels supplémentaires.
- Run final : **ECU MEMS Manager x64 #59 — Commit `776fc64`**.
- Run ID : **`32898631148`** ; job : **`97967053552`**.

### ECU MEMS Manager x64 #59 — Commit `776fc64` — VERT

- Résultat GitHub : **SUCCESS / VERT**.
- Les 20 étapes fonctionnelles sont vertes : protections protocole, configuration/compilation x64, application + désinstalleur + self-tests, réponses IA déterministes, ABI protocole, base de référence, génération SQLite experte r20, runtime llama.cpp b10516, Qwen3-0.6B-Q8_0, assemblage package, validation architecture x64, API Qwen empaquetée, smoke launch application, hashes et upload.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9582674702`**.
- Taille : **668863270 octets**.
- SHA-256 archive GitHub : **`ee7f12e933a17c884c6ca081c583a542f45569f556a68a50f124b58b5f13fcff`**.
- Créé le **25 août 2026 à 21:15:31Z** ; expiration prévue le **8 septembre 2026 à 21:15:04Z**.
- Cet artefact correspond exactement au HEAD `776fc647a874564c932bf09e8871cb771a0ed258`.

### Test PC réel — ECU MEMS Manager x64 #59 — Commit `776fc64`

Résultat utilisateur constaté sur les captures du 25 août 2026 :

- **le crash à l’ouverture de l’onglet IA MEMS est supprimé** ;
- statut visible : `ECU non connecté • base prête • IA locale prête` ; la base r20 et Qwen sont donc réellement démarrés sur le PC ;
- défaut restant : l’IA ne fournit pas de réponse utile. Exemple exact : question utilisateur `quel jour somme nous?` puis réponse IA `Quel jour sommes-nous ?`, c’est-à-dire une simple reformulation/correction de la question ;
- l’utilisateur indique que le même comportement non-répondant est observé sur les questions testées.

### Isolation du défaut question/réponse après reconstruction

Le câblage propre `IaMemsTab -> IaMemsService -> LocalAiClient -> responseReady -> IaMemsTab` est vérifié : la question part bien et une chaîne Qwen revient bien dans la vue. Le défaut n’est donc plus un problème de signal Qt ni de démarrage du sidecar.

Deux défauts concrets ont été identifiés avant correction :

1. le nettoyage de l’ancien `iamemsqualitypatch.cpp` a retiré avec lui le traitement déterministe de la date courante ; `IaMemsService::groundingFor()` ne fournit actuellement aucune date locale pour une question comme « quel jour sommes-nous ? », et Qwen seul ne possède pas l’heure/date runtime du PC ;
2. le raisonnement Qwen3 est maintenant actif, mais `LocalAiClient` utilise encore `temperature=0.25`, `top_p=0.9` et aucun `top_k`. La documentation officielle Qwen3 recommande en thinking `temperature=0.6`, `top_p=0.95`, `top_k=20`, `min_p=0` et déconseille les réglages favorisant les répétitions. Ces paramètres doivent être appliqués au mode raisonnement réel.

Garde-fou également requis : une sortie qui n’est qu’une reformulation normalisée de la question ne doit plus être acceptée comme réponse valide. Si un contexte déterministe fiable existe, il doit servir de repli ; sinon l’échec doit être explicite plutôt que d’afficher un écho.

### Correction question/réponse poussée — BUILD #30

- Nouveau HEAD `MEMSX64` : **`be4916a53321e36573729e123b14c2cf120fd734`** — `BUILD #30 fix Qwen thinking responses`.
- Modification ciblée sur `expert/LocalAiClient.cpp` uniquement.
- Paramètres de génération du mode thinking alignés sur Qwen3 : `temperature=0.6`, `top_p=0.95`, `top_k=20`, `min_p=0`.
- La date/heure locale du PC est désormais injectée dans le contexte runtime afin qu’une question de date ne repose pas sur la mémoire figée du modèle.
- Un garde-fou rejette les sorties qui ne font que reformuler/faire écho à la question ; le contexte déterministe sert alors de repli lorsqu’il existe.
- Le raisonnement Qwen reste actif dans l’application ; `--reasoning off` reste réservé au smoke-test CI.
- Aucun changement protocole ECU, UI, base experte r20, modèle Qwen ou architecture propre.

### ECU MEMS Manager x64 #60 — Commit `be4916a` — VERT

- Run GitHub : **`32901653203`** ; job : **`97976653225`**.
- Résultat final : **SUCCESS / VERT**.
- Les 20 étapes fonctionnelles sont vertes, y compris compilation application + désinstalleur + self-tests, base experte r20, runtime llama.cpp b10516, Qwen3-0.6B-Q8_0, API empaquetée, smoke launch, hashes et upload.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9583795907`**.
- Taille : **668866141 octets**.
- SHA-256 archive GitHub : **`b03616086bc7bba254b8b089bff474ce275f28cc92b19a30db7300aeb6be9f4a`**.
- Créé le **25 août 2026 à 21:49:42Z** ; expiration prévue le **8 septembre 2026 à 21:49:13Z**.
- Cet artefact correspond exactement au HEAD `be4916a53321e36573729e123b14c2cf120fd734`.

### Test PC réel — ECU MEMS Manager x64 #60 — Commit `be4916a`

- Le crash reste supprimé ; `base prête` et `IA locale prête` sont atteints.
- La question de date répond désormais correctement.
- Défaut de contexte : après la date, la question générale `C'est quoi un moteur 4 temps ?` reçoit encore la réponse précédente sur la date. Le petit modèle reçoit les anciens tours et se laisse contaminer par un sujet sans rapport.
- Défaut de connaissance/routage : `C'est quoi l'IAC ?` produit `Intégration Automatique Contrôlée`, réponse fausse/hallucinée. Dans le contexte automobile visé, IAC signifie `Idle Air Control`, système de régulation d'air de ralenti ; une définition technique doit venir de la base/glossaire contrôlé avant de laisser Qwen improviser.
- Latence jugée trop longue par l'utilisateur ; l'interface reste longtemps sur `IA locale en réponse`.
- Cause de latence identifiée : le chemin actuel autorise jusqu'à 1536 tokens en thinking même pour une question triviale, ce qui est disproportionné pour Qwen3-0.6B local sur CPU.

### Étape autorisée — routage IA rapide et raisonnement ciblé

Objectif exact autorisé par l'utilisateur (`GO`) : corriger ces trois défauts dans le même BUILD #30, sans réintroduire les anciens patches et sans toucher au protocole ECU.

Architecture de réponse à appliquer :
- faits déterministes possédés par l'application (date/heure, états runtime) : réponse immédiate sans génération LLM ;
- questions générales simples et définitions courantes : Qwen en mode rapide non-thinking, budget court ;
- définitions techniques MEMS connues : base experte/glossaire contrôlé prioritaire, puis formulation courte ; ne jamais laisser Qwen inventer le développement d'un acronyme MEMS ;
- diagnostic, analyse de mesures, hypothèses et questions réellement complexes : Qwen thinking actif avec base + moteur expert + mesures ECU ;
- ne plus injecter automatiquement tout l'historique à chaque requête ; n'utiliser le tour précédent que lorsqu'une question est clairement une relance dépendante du contexte ;
- conserver le raisonnement Qwen disponible et actif pour les tâches qui le nécessitent ; le routage rapide n'est pas une désactivation globale du raisonnement.

---

## ÉTAT DE RÉFÉRENCE

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD actuel avant routage rapide : **`be4916a53321e36573729e123b14c2cf120fd734`**.
- Rapport : `RAPPORT`.
- 32 bits : `lab-expert-engine`, ne pas toucher.
- Rollback x64 : `MEMSX64-BUILD26-BASE`, ne pas toucher.
- BUILD actif : #30 / v1.0.30.

### Références historiques

- #26 `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, run `32816285887` SUCCESS ; navigation/onglet IA PC stables sans runtime Qwen emballé.
- #27 `a6f9b209f32b6dd77774832e8c84469c53deca47`, run `32832192437` SUCCESS ; AANMP002/MNE101150, COM3 FTDI, ROSCO 1.3/1.6, Injection RAM Mode4 ≈2,63 ms.
- #28 `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`, run `32842049458` SUCCESS, artifact `9561033224`, SHA256 `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.
- #29 final `fee195e88d3615613b8f92de83209da2cf8247c2`; runtime COMPAT validé séparément ; Qwen avait atteint `IA locale prête` sur PC.
- #30 pré-reconstruction `414ea52970e02fb6077c94ca2aa7aec3e92d7383`, ECU MEMS Manager x64 #45 SUCCESS CI mais crash PC réel à l'ouverture IA.
- #30 reconstruction propre : HEAD `776fc647a874564c932bf09e8871cb771a0ed258`, ECU MEMS Manager x64 #59 SUCCESS ; crash d'ouverture corrigé sur PC.
- #30 correction réponses Qwen : HEAD `be4916a53321e36573729e123b14c2cf120fd734`, ECU MEMS Manager x64 #60 SUCCESS ; date corrigée, mais latence, contamination d'historique et hallucination IAC confirmées sur PC.

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

**Implémenter sur `MEMSX64` le routage IA autorisé : réponse immédiate pour les faits runtime, mode rapide sans thinking pour les questions simples, priorité base/glossaire pour les définitions MEMS, thinking réservé au diagnostic/analyse complexe, et historique uniquement pour les vraies relances. Vérifier compilation/self-tests puis pousser et suivre la nouvelle Action GitHub du BUILD #30.**