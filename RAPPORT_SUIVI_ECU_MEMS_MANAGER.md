# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` est le numéro GitHub Actions, pas le BUILD logiciel.

## ÉTAT ACTUEL

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD validé avant optimisation performance : `7a8085cef236e00091d8a053cdb97293568d97d0`.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.

## ARCHITECTURE IA PROPRE RETENUE

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService (service application) -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> llama-server.exe -> Qwen3`

Règles :
- `llama-server.exe` sidecar x64 hors processus ; ne pas lier libllama/ggml dans `ecu_mems_manager.exe`.
- Durée de vie IA application, pas onglet.
- Base experte r20 préconstruite en CI et ouverte `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`.
- Mesures ECU transmises en lecture seule ; aucune autorité LLM sur commandes ou mutations ECU.
- Qwen3-0.6B-Q8_0 + llama.cpp b10516 conservés.
- Ancien empilement supprimé : `iamemstab_clean.cpp`, `iamemsqualitypatch.cpp`, `iaresponsecontextpatch.cpp` ne doivent pas revenir.

## DÉSINSTALLATION BUILD #30

- `ecu_mems_uninstaller.exe` doit être compilé et empaqueté.
- `install_manifest.txt` complet requis.
- Refuse l’uninstall si l’app tourne, conserve le profil par défaut, supprime les données locales seulement sur choix explicite, préserve les fichiers étrangers.

## HISTORIQUE BUILD #30 / IA

### Pré-reconstruction
- HEAD initial : `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`.
- Correction chemin base r20 : `879077a678f4c203124907dabdeb42b532c9d337`.
- **ECU MEMS Manager x64 #44** / run `32890600398` : rouge uniquement au smoke Qwen, `Empty chat completion from packaged Qwen`.
- Correction smoke CI uniquement : `414ea52970e02fb6077c94ca2aa7aec3e92d7383`, ajout `--reasoning off` au serveur du smoke GitHub seulement.
- **#45 — `414ea52` — VERT**, artifact `9580850077`, SHA-256 `1db3438593c65f7f77176910e55e9de0a428208f9c3a7732b74d6e35290ed3d0`.
- Test PC #45 : crash immédiat à l’ouverture IA ; CI ne testait pas l’ouverture de l’onglet.

### Reconstruction propre
- HEAD : `776fc647a874564c932bf09e8871cb771a0ed258`.
- `IaMemsTab` devient une vue simple ; `IaMemsService` service unique application ; base r20 lue directement ; plus de reparentage lors de `Show`, plus de wrappers/patches globaux.
- **#59 — `776fc64` — VERT**, run `32898631148`, artifact `9582674702`, SHA-256 `ee7f12e933a17c884c6ca081c583a542f45569f556a68a50f124b58b5f13fcff`.
- Test PC #59 : crash d’ouverture supprimé ; `base prête` + `IA locale prête`.

### Réponses Qwen / routage
- `be4916a53321e36573729e123b14c2cf120fd734` : date locale, anti-écho, paramètres thinking Qwen3.
- **#60 — VERT**, run `32901653203`, artifact `9583795907`, SHA-256 `b03616086bc7bba254b8b089bff474ce275f28cc92b19a30db7300aeb6be9f4a`.
- Test PC #60 : date correcte mais historique contaminant, hallucination IAC, latence forte.

- `126cc638d584975a78d0101430d61bdc435c5879` : routage rapide ; date immédiate ; IAC contrôlé ; simple `/no_think` max 256 tokens ; diagnostic `/think` max 768 ; historique limité aux vraies relances.
- **#61 — VERT**, run `32904830665`, artifact `9584843179`, SHA-256 `cc9cd0e2f299d563e469cac08c2d86bfe7ab52f998c7f4568d86ca3fe53aa207`.
- Test PC #61 : réponses simples un peu plus rapides ; IAC correct ; relance IAC mauvaise ; bobine confondue avec mesure live ; rôle ECU halluciné ; MEMS 1.6 interprété hors domaine et parfois en anglais ; navigation masquait `Aperçu` quand `Test ECU 1.9` sélectionné.

### Langue / domaine / navigation
- `8a793a7f9d660a729e12cf32c2f888161cad6598` : langue active imposée, domaine Rover/Lucas MEMS verrouillé, réponses contrôlées moteur 4 temps / ECU / familles MEMS / dwell, relance IAC.
- HEAD final : `7a8085cef236e00091d8a053cdb97293568d97d0` : sidebar responsive, 14 entrées visibles, liste recalée en haut.
- **ECU MEMS Manager x64 #63 — `7a8085c` — VERT** ; run `32936048218`, job `98077364424`.
- Artifact : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9595181353`, taille `668875356`, SHA-256 `995efda2b49768457ad3ec8f2f31b137c671b722c80465831b2ef00d47c559d4`.

## TEST PC RÉEL #63 — 26 AOÛT 2026

Validé :
- aucun crash d’ouverture IA ; `base prête` + `IA locale prête` ;
- 14 onglets visibles simultanément ; `Aperçu` reste visible lorsque `Test ECU 1.9` est sélectionné ;
- garde-fou Rover/Lucas MEMS actif sur `MEMS` / `MEMS 1.9`.

Défauts restants :
- **latence entre ~30 secondes et 2 minutes** pour certaines réponses Qwen : inacceptable ;
- `C'EST QUOI LE MAP ?` est routé vers une mesure live absente au lieu d’une définition ;
- `C'EST QUOI L'INJECTEUR ?` produit une fausse définition d’injection d’huile ;
- `C'EST QUOI SPI SUR LES MOTEUR ROVER ?` hallucine `Signal Pulse Intensité` ; dans ce domaine SPI = **Single Point Injection**.

## CAUSE PERFORMANCE ISOLÉE

Le workflow #63 compile llama.cpp b10516 en profil CPU ultra-conservateur :
- `GGML_NATIVE=OFF`
- `GGML_BACKEND_DL=OFF`
- `GGML_CPU_ALL_VARIANTS=OFF`
- `GGML_SSE42=OFF`
- `GGML_AVX=OFF`
- `GGML_AVX2=OFF`
- `GGML_BMI2=OFF`
- variantes AVX512 OFF.

Sur x86/MSVC, ce profil force le backend x64 de base sans SIMD avancé. Il a été choisi pendant la stabilisation crash mais explique une part majeure de la lenteur.

Le même llama.cpp b10516 supporte officiellement le mode :
- `GGML_NATIVE=OFF`
- `GGML_BACKEND_DL=ON`
- `GGML_CPU_ALL_VARIANTS=ON`

Ce mode construit plusieurs backends (`x64`, `sse42`, `sandybridge`, `haswell`, `skylakex`, etc.). Le chargeur teste leur compatibilité, choisit automatiquement le meilleur backend supporté et conserve `x64` comme repli. **Ne pas imposer AVX2 à tous les PC.**

## ÉTAPE AUTORISÉE — PERFORMANCE IA + DÉFINITIONS

Autorisation utilisateur : **GO**.

Objectif exact dans le même BUILD #30 :
1. remplacer le runtime ultra-conservateur par le runtime llama.cpp **multi-variantes CPU** (`GGML_BACKEND_DL=ON`, `GGML_CPU_ALL_VARIANTS=ON`, `GGML_NATIVE=OFF`) ;
2. empaqueter `llama-server.exe` avec toutes les DLL ggml/CPU requises ;
3. conserver le backend x64 comme repli et la sélection automatique du meilleur backend compatible ;
4. ajouter des validations CI : présence des DLL variantes, démarrage serveur, `/health`, `/v1/models`, chat Qwen ;
5. ajouter des réponses contrôlées immédiates en français pour **MAP**, **injecteur essence** et **SPI = Single Point Injection**, afin d’éviter les hallucinations et les appels Qwen inutiles ;
6. aucun changement protocole ECU, aucune branche 32 bits, aucun BUILD #31.

## UI OFFICIELLE À PRÉSERVER

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

Style dark/responsive inchangé.

## SÉCURITÉ PROTOCOLE À PRÉSERVER

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 : session normale seulement ; D1 bloqué Mode4.
- D3/F3/F4/F5 bloqués interface générique.
- Mutations uniquement Rosco13_16 prouvé + mode Normal ; famille inconnue fail-closed ; mutations MEMS1.9 bloquées.
- F7/EF injecteurs bloqués sans sous-type prouvé ; transaction RAM bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);`.
- Réponses : D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction`, correction issue de Réglages, jamais `-3` hardcodé.
- Dwell référence : ~1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant validation BUILD #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports série arbitraires, profils RAM non validés, reset/clear faults/trims/écritures pendant BUILD #30.

## PROCHAINE ACTION EXACTE

**Modifier `LocalAiClient.cpp` pour MAP/injecteur/SPI puis `.github/workflows/memsx64.yml` pour le runtime llama.cpp multi-variantes et son packaging/validation. Pousser sur `MEMSX64`, suivre l’Action du HEAD final et consigner son résultat avant tout test PC. Aucun BUILD #31.**