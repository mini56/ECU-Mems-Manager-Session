# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Ce fichier doit être relu avant toute modification d’ECU MEMS Manager. Il constitue la source de vérité de continuité du projet.
>
> Branche rapport : `RAPPORT`. Branche de développement x64 : `MEMSX64`.
>
> **RÈGLE DE SUIVI IMMÉDIAT : AVANT CHAQUE NOUVELLE ÉTAPE, inscrire ici l’étape exacte et son objectif. Dès que cette étape produit un résultat, inscrire immédiatement ce résultat AVANT de commencer l’étape suivante. Ne jamais attendre la fin d’un lot, d’un build ou d’une discussion.**

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

- HEAD `MEMSX64` : **`600b8ef8607eb3dc7d591f675e9f33be0cdb0911`** — `BUILD #30 clean x64 IA package and uninstaller`.
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
- toutes les étapes suivantes (build llama, Qwen, packaging, API, smoke, upload) ont été sautées ;
- aucun changement de code n’est encore entrepris.

### Étape en cours — diagnostic du seul rouge du run `32889430143`

Objectif exact : lire le log de l’étape `Generate and validate packaged expert database r20`, identifier la ligne/erreur exacte, inscrire ce diagnostic au rapport, puis corriger uniquement cette cause dans le même BUILD #30.

---

## ÉTAT DE RÉFÉRENCE

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 : `MEMSX64`.
- HEAD : `600b8ef8607eb3dc7d591f675e9f33be0cdb0911`.
- Rapport : `RAPPORT`.
- 32 bits : `lab-expert-engine`, ne pas toucher.
- Rollback x64 : `MEMSX64-BUILD26-BASE`, ne pas toucher.
- BUILD actif : #30 / v1.0.30.

### Références historiques

- #26 `12fef48c68807bc59d2f45f9cd8d86d2a42856ca`, run `32816285887` SUCCESS, IA/navigation PC validées.
- #27 `a6f9b209f32b6dd77774832e8c84469c53deca47`, run `32832192437` SUCCESS, AANMP002/MNE101150, COM3 FTDI, ROSCO 1.3/1.6, Injection RAM Mode4 ≈2,63 ms.
- #28 `0533adaf50cf2c4d62a1ba5241a0100dfa1b48e8`, run `32842049458` SUCCESS, artifact `9561033224`, SHA256 `50407002f1368be30a163714ab8765a4ea7fe283fa8fd46cb1dcbd4015025e1b`.
- #29 final `fee195e88d3615613b8f92de83209da2cf8247c2`; runtime COMPAT run `32878926411` vert ; Qwen chargé sur PC et statut `IA locale prête` atteint ; dernier rouge #29 limité au packaging expert.

## VERSIONNAGE

- #29 = v1.0.29 ; #30 = v1.0.30 ; #100 = v1.1.0.
- Formule `1.(build / 100).(build % 100)`.
- Un rerun du même commit n’est pas un nouveau BUILD.

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

**Lire le log du job `97937586271`, étape `Generate and validate packaged expert database r20`. Ne corriger qu’après avoir consigné la cause exacte.**
