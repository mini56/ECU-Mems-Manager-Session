# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` est le numéro GitHub Actions, pas le BUILD logiciel.

## ÉTAT ACTUEL

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD courant après correction définitions : **`f860749313447e224f63b99801f7e7d6a1839a49`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.

## ARCHITECTURE IA PROPRE RETENUE

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService (service application) -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> llama-server.exe -> Qwen3`

- `llama-server.exe` sidecar x64 hors processus ; ne pas lier libllama/ggml dans `ecu_mems_manager.exe`.
- Durée de vie IA application, pas onglet.
- Base experte r20 préconstruite en CI et ouverte en lecture seule.
- Mesures ECU transmises en lecture seule ; aucune autorité LLM sur commandes/mutations ECU.
- Qwen3-0.6B-Q8_0 + llama.cpp b10516 conservés.
- Ancien empilement supprimé : `iamemstab_clean.cpp`, `iamemsqualitypatch.cpp`, `iaresponsecontextpatch.cpp` ne doivent pas revenir.

## HISTORIQUE BUILD #30 / IA

### Pré-reconstruction
- `600b8ef8607eb3dc7d591f675e9f33be0cdb0911` : base BUILD #30.
- `879077a678f4c203124907dabdeb42b532c9d337` : correction chemin base r20.
- #44 / run `32890600398` : rouge au smoke Qwen (`Empty chat completion from packaged Qwen`).
- `414ea52970e02fb6077c94ca2aa7aec3e92d7383` : `--reasoning off` uniquement pour le smoke CI.
- #45 — VERT ; artifact `9580850077`, SHA-256 `1db3438593c65f7f77176910e55e9de0a428208f9c3a7732b74d6e35290ed3d0`.
- Test PC #45 : crash immédiat ouverture IA.

### Reconstruction propre
- `776fc647a874564c932bf09e8871cb771a0ed258` : service IA unique application, vue simple, base r20 directe read-only, suppression wrappers/patches globaux.
- #59 — VERT ; run `32898631148`, artifact `9582674702`, SHA-256 `ee7f12e933a17c884c6ca081c583a542f45569f556a68a50f124b58b5f13fcff`.
- Test PC #59 : crash supprimé ; `base prête` + `IA locale prête`.

### Réponses/routage
- `be4916a53321e36573729e123b14c2cf120fd734` : date locale, anti-écho, thinking Qwen3 ; #60 VERT, artifact `9583795907`.
- Test PC #60 : date correcte, contamination historique, hallucination IAC, latence forte.
- `126cc638d584975a78d0101430d61bdc435c5879` : routage rapide ; date immédiate ; IAC contrôlé ; simple `/no_think` 256 tokens ; diagnostic `/think` 768 ; #61 VERT, artifact `9584843179`.
- Test PC #61 : vitesse un peu meilleure ; IAC correct ; défauts langue/domaine/relances/navigation.
- `8a793a7f9d660a729e12cf32c2f888161cad6598` : français/domaine Rover-LUCAS, réponses contrôlées ECU/MEMS/dwell.
- `7a8085cef236e00091d8a053cdb97293568d97d0` : navigation responsive 14 onglets.
- #63 — VERT ; run `32936048218`, job `98077364424`, artifact `9595181353`, SHA-256 `995efda2b49768457ad3ec8f2f31b137c671b722c80465831b2ef00d47c559d4`.

## TEST PC RÉEL #63 — 26 AOÛT 2026

Validé :
- aucun crash d’ouverture IA ;
- `base prête` + `IA locale prête` ;
- 14 onglets visibles simultanément, `Aperçu` reste visible avec `Test ECU 1.9` ;
- domaine Rover/Lucas MEMS mieux verrouillé.

Défauts :
- **latence ~30 s à 2 min** pour certaines réponses Qwen ;
- MAP confondu avec mesure live ;
- injecteur défini à tort comme injection d’huile ;
- SPI halluciné alors que Rover/MEMS SPI = **Single Point Injection**.

## CAUSE PERFORMANCE ISOLÉE

Le runtime #63 utilise : `GGML_NATIVE=OFF`, `GGML_BACKEND_DL=OFF`, `GGML_CPU_ALL_VARIANTS=OFF`, `GGML_SSE42=OFF`, `GGML_AVX=OFF`, `GGML_AVX2=OFF`, `GGML_BMI2=OFF`, AVX512 OFF. Il force donc le backend x64 de base sans SIMD avancé.

llama.cpp b10516 fournit le mode multi-variantes officiel :
- `GGML_NATIVE=OFF`
- `GGML_BACKEND_DL=ON`
- `GGML_CPU_ALL_VARIANTS=ON`

Il construit des backends `x64`, `sse42`, `sandybridge`, `haswell`, `skylakex`, etc. Le chargeur sélectionne automatiquement le meilleur compatible et conserve `x64` comme repli. **Ne pas imposer AVX2 à tous les PC.**

## ÉTAPE AUTORISÉE — PERFORMANCE IA + DÉFINITIONS

Autorisation utilisateur : **GO**.

### Résultat étape 1 — définitions contrôlées

Commit **`f860749313447e224f63b99801f7e7d6a1839a49`** — `BUILD #30 add controlled MAP injector SPI answers`.

`LocalAiClient.cpp` :
- MAP : réponse immédiate contrôlée, `Manifold Absolute Pressure`, pression absolue collecteur, rôle charge moteur ;
- injecteur : électrovanne essence commandée ECU, distinction SPI/MPI, exclusion explicite de l’injection d’huile ;
- SPI : `Single Point Injection`, injection monopoint Rover/Mini MEMS, distinction MPI ;
- ces réponses sont interceptées avant l’appel Qwen ;
- `map`, `injecteur`, `spi` ajoutés au garde-fou domaine MEMS ;
- aucun changement protocole ECU, UI ou 32 bits.

### Étape 2 à exécuter

Modifier `.github/workflows/memsx64.yml` pour :
1. runtime llama.cpp multi-variantes (`GGML_BACKEND_DL=ON`, `GGML_CPU_ALL_VARIANTS=ON`, `GGML_NATIVE=OFF`) ;
2. empaqueter `llama-server.exe` + toutes DLL ggml/CPU nécessaires ;
3. vérifier présence backend `x64` de repli et variantes optimisées ;
4. valider démarrage, `/health`, `/v1/models`, chat Qwen dans le package ;
5. garder BUILD #30, aucun changement protocole ECU.

## DÉSINSTALLATION BUILD #30

- `ecu_mems_uninstaller.exe` + `install_manifest.txt` requis.
- Refuse si app active, conserve profil par défaut, données locales supprimées seulement sur choix explicite, fichiers étrangers préservés.

## UI OFFICIELLE À PRÉSERVER

Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9. Style dark/responsive inchangé.

## SÉCURITÉ PROTOCOLE À PRÉSERVER

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 normal seulement ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique.
- Mutations : Rosco13_16 prouvé + Normal uniquement ; unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués sans sous-type ; transaction RAM bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);`.
- D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction` avec correction Réglages réelle ; jamais -3 hardcodé.
- Dwell référence ~1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant BUILD #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports série arbitraires, profils RAM non validés, reset/clear faults/trims/écritures pendant BUILD #30.

## PROCHAINE ACTION EXACTE

**Modifier maintenant `.github/workflows/memsx64.yml` pour le runtime multi-variantes, packaging et validations. Pousser sur `MEMSX64`, consigner le HEAD final puis suivre uniquement son Action GitHub. Aucun BUILD #31.**