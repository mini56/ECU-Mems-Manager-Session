# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager. Il constitue la source de vérité de continuité du projet.
>
> Branche rapport : `RAPPORT`. Branche de développement x64 : `MEMSX64`.
>
> **RÈGLE DE SUIVI IMMÉDIAT : AVANT CHAQUE NOUVELLE ÉTAPE, inscrire ici l’étape exacte et son objectif. Dès que cette étape produit un résultat, inscrire immédiatement ce résultat AVANT de commencer l’étape suivante. Ne jamais attendre la fin d’un lot, d’un build ou d’une discussion.**

## JOURNAL IMMÉDIAT — 25 août 2026

### Architecture IA x64 retenue après recherche externe

- Moteur hors processus : conserver `llama-server.exe` sidecar x64, ne pas lier `libllama/ggml` dans `ecu_mems_manager.exe`.
- Durée de vie IA application, pas widget ; `IA MEMS` reste une vue/client.
- Loopback `127.0.0.1` uniquement ; pas de WebUI/outils/agents/MCP.
- Base experte r20 construite/validée dans GitHub Actions, emballée sous `database/expert/`, lue via `QSQLITE_OPEN_READONLY` + `PRAGMA query_only=ON`.
- RAG borné : mesures ECU read-only -> moteur expert -> faits pertinents -> Qwen -> texte ; aucune capacité de commande ECU donnée au LLM.
- Qwen3-0.6B-Q8_0 et llama.cpp b10516 conservés.
- Package : application, DLL protocole, Qt/qsqlite, runtime IA, modèle, base experte, manifeste IA, désinstalleur et manifeste d’installation.

### BUILD #30 officiellement ouvert par demande utilisateur

- BUILD actif : **BUILD #30 / v1.0.30**.
- BUILD #29 reste historique expérimental.
- Aucun BUILD #31 sans demande explicite.
- `lab-expert-engine` et `MEMSX64-BUILD26-BASE` restent intacts.
- Aucun changement protocole ECU dans le lot IA #30.

### Désinstallation BUILD #30

- `uninstaller.cpp` existe et CMake crée déjà `ecu_mems_uninstaller` sous Windows.
- Le défaut #29 était le packaging : la cible n’était pas construite/emballée.
- BUILD #30 livre `ecu_mems_uninstaller.exe` + `install_manifest.txt` complet.
- Le désinstalleur refuse d’agir si l’application tourne, conserve le profil par défaut, ne supprime les données locales que sur choix explicite, ne supprime que les fichiers du manifeste et préserve les fichiers étrangers.

### Inspection pré-BUILD #30 — résultats

- Vraie source experte : `database/reference/` ; `expert/base_connaissance_ia` n’existe pas.
- `IaMemsTab` créait encore `LocalAiClient` avec le widget comme parent et `showEvent()` lançait préparation DB + Qwen.
- `ExpertRuntimeDatabase::buildOrOpen()` préfère déjà `database/expert/ia_mems_reference_r<revision>.sqlite`; une base emballée évite la reconstruction lourde.
- `ExpertKnowledgeReader` est déjà read-only (`QSQLITE_OPEN_READONLY`, `PRAGMA query_only=ON`).
- Runtime retenu : profil COMPAT statique b10516 (`LLAMA_OPENSSL=OFF`, pas de backends dynamiques/OpenMP/AVX2 etc.), déjà observé chargeant Qwen sur PC réel.

### Premier push BUILD #30 — terminé

- HEAD `MEMSX64` : **`600b8ef8607eb3dc7d591f675e9f33be0cdb0911`** — `BUILD #30 clean x64 IA package and uninstaller`.
- Commit intermédiaire `2fd0ad91ec905690b07aefc23104be1571f2eeb1` : simple marqueur temporaire, immédiatement supersédé ; pas un build distinct.
- Le workflow expérimental `memsx64_compat_ai.yml` a été retiré ; un seul workflow x64 reste.
- `.github/workflows/memsx64.yml` est BUILD #30 / v1.0.30 ; artefact prévu `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`.
- Workflow : application + désinstalleur + self-tests, expert r20 depuis `database/reference`, runtime llama statique conservateur, Qwen vérifié SHA256, package complet + manifeste.
- `iamemstab_clean.cpp` reparente `LocalAiClient` vers `QApplication` avant le `showEvent()` ; le sidecar n’est plus possédé par le widget.
- Aucun fichier protocole ECU, ABI ROSCO ou ordre d’onglets modifié.

### GitHub Actions BUILD #30 — résultats enregistrés au fil du run

Run : **`32889430143`** ; job : **`97937586271`** ; commit : `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`.

Résultat initial : checkout, Python, outils validation, Qt x64 et protections protocole **VERTS**.

**Nouveau résultat :**
- `Configure ECU MEMS Manager x64` : **VERT**.
- `Build application uninstaller and self-tests` : **VERT**. Cela confirme que le code BUILD #30 compile, y compris `ecu_mems_manager.exe` et **`ecu_mems_uninstaller.exe`** ainsi que les self-tests.
- `Run deterministic IA response self-test` : **VERT**.
- `Run protocol ABI self-test` : **VERT**.
- Étape maintenant en cours : `Run reference database semantic self-test`.
- Aucune correction de code n’est nécessaire à ce stade ; on poursuit le même run.

### Étape en cours — poursuivre le contrôle du run #30

Objectif : obtenir le résultat du self-test base référence, puis de la génération expert r20, du runtime llama, de Qwen, du packaging, de l’API et de l’upload. En cas de premier rouge, inscrire le log exact avant toute correction.

---

## ÉTAT DE RÉFÉRENCE

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD : `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`.
- Rapport : branche `RAPPORT`.
- 32 bits : `lab-expert-engine`, ne pas toucher.
- Rollback x64 figé : `MEMSX64-BUILD26-BASE`.
- BUILD actif : #30 / v1.0.30.

### Références historiques

- BUILD #26 : `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, run `32816285887` SUCCESS ; IA/navigation validées sur PC.
- BUILD #27 : `a6f9b209f32b6dd77774832e8c84469c53deca47`, run `32832192437` SUCCESS ; AANMP002/MNE101150, COM3 FTDI, ROSCO 1.3/1.6, polling/logging et Injection RAM Mode4 ≈2,63 ms.
- BUILD #28 : `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`, run `32842049458` SUCCESS ; rollback stable, artifact ID `9561033224`, SHA256 `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.
- BUILD #29 final : `fee195e88d3615613b8f92de83209da2cf8247c2`; runtime COMPAT run `32878926411` vert ; Qwen a chargé sur PC et MEMS Manager a atteint `IA locale prête`; dernier rouge #29 limité au packaging base experte.

## VERSIONNAGE

- BUILD #29 = v1.0.29
- BUILD #30 = v1.0.30
- BUILD #100 = v1.1.0
- Formule `1.(build / 100).(build % 100)`.
- Un rerun du même commit n’est pas un nouveau BUILD.

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
- D0/D1/D2 seulement session normale ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique.
- Mutations seulement Rosco13_16 prouvé + Normal ; unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués sans sous-type ; transaction RAM bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);` dans `memsinterface.h`.
- D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction` avec correction réelle de Réglages, jamais -3 hardcodé.
- Dwell ≈1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant validation IA #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9 dédiée, failsafe actionneurs, séquences sur ports série arbitraires, profils RAM non validés matériellement, reset/clear faults/trims/écritures pendant validation #30.

## TESTS BUILD #30 AVANT PC

1. CI BUILD #30 / v1.0.30 verte.
2. App + DLL protocole + uninstaller x64 présents.
3. `install_manifest.txt` complet.
4. Expert r20 intégrité OK, user_version 20, tables présentes.
5. Package contient `database/expert/ia_mems_reference_r20.sqlite`.
6. Runtime statique `llama-server --version` OK.
7. Qwen charge ; `/health` OK ; `/v1/models` contient `ia-mems` ; chat completion réelle.
8. Smoke launch application.
9. Puis test PC : IA prête sans fermeture, réponses base/déterministes, répétition ouverture/fermeture, désinstallation propre.

## PROCHAINE ACTION EXACTE

**Continuer le contrôle du run GitHub Actions `32889430143` sur HEAD `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`. Ne modifier aucun code sans résultat d’échec concret.**
