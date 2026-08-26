# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions, pas le BUILD logiciel.

## ÉTAT COURANT — 26 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- Base stable validée utilisateur : **#81 / `8671275bc77eb1fdbdefc0b0158254efdf86df5c`**.
- HEAD qualité courant : **`4f12bb6fa32df71bb9893865f2ad6acc568a7a48`**.
- GitHub Actions courant : **#84**, intermédiaire qui sera remplacé par le dernier HEAD après complétion des définitions déterministes.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.
- UI dark/responsive officielle à préserver.

## DERNIER BUILD COMPLET VALIDÉ

### ECU MEMS Manager x64 #81 — Commit `8671275` — VERT CI + TEST UTILISATEUR FONCTIONNEL

- Run GitHub Actions : **`33001002041`** (#81), conclusion **SUCCESS**.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9618786882`**.
- Taille : **386 739 081 octets**.
- Digest : **`sha256:17f8b0d3b81ff0a7d02b964ef265b38e5746fa70ff1cf754675d9d14ced3265e`**.

CI #81 validée : compilation MSVC x64, tests déterministes, base r20, Qwen ONNX, vrai self-test `LocalAiClient`, package complet, validation depuis package final, smoke launch, manifeste/hashes et artefact.

### TEST UTILISATEUR #81 — RÉSULTAT IMPORTANT

Le 26 août 2026, l'utilisateur confirme : **« bon ça fonctionne »**.

Constaté sur les captures : application lancée ; IA MEMS ouverte ; `base prête` ; `IA locale prête` ; réponse IAC correcte et immédiate ; moteur ONNX/Qwen opérationnel.

Défauts qualité visibles : fuite `<think>` ; fuite de consignes internes ; méta-discours ; « C'EST QUOI LA BOBINE ? » sans vraie définition ; nettoyage insuffisant ; question date avec faute mal gérée.

**Conclusion : l'intégration ONNX fonctionne. Ne plus changer de moteur. Travail actif = qualité des réponses.**

## ÉTAPE EN COURS — OPTIMISATION QUALITÉ DES RÉPONSES IA

Autorisation utilisateur : **« donc on travaille là-dessus »**, puis **« OK fait ça ensuite optimise les réponses »**.

### Diagnostic racine LocalAiClient #81

- directives internes dans le message `user` ;
- nettoyage `<think>` seulement si balise fermée ;
- aucun filtre de fuite explicite ;
- définition bobine simple absente ;
- date peu tolérante aux fautes.

### Corrections poussées

**`a53fd13c6249d0c0711e88842d0b3e00a5167773`** — qualité native : prompt utilisateur simplifié, filtre anti-fuite, nettoyage `<think>` tronqué/ChatML, définition bobine, date tolérante, latence 128/192 + `/no_think` préservée.

**`e20eb3ac94807e8a65e901f1413621e4dbec7fd8`** — self-test production : bobine, date fautive, vraie génération ONNX `OK`, rejet `<think>`/ChatML/directives internes. #83 annulé automatiquement car remplacé par HEAD plus récent.

**`4f12bb6fa32df71bb9893865f2ad6acc568a7a48`** — réponses base r20 : classement par pertinence, preuve, famille/firmware ; maximum 3 faits ; source/notes seulement sur demande ; fallback générique nettoyé. Actions #84 lancé.

### NOUVEAU SOUS-AUDIT AVANT FIGEAGE — DÉFINITIONS VS MESURES

Cause identifiée : `IaMemsService::groundingFor()` classe plusieurs termes techniques comme intentions de **mesure** via `IaResponseLogic::classify()`. Ainsi une question de définition telle que « c'est quoi la lambda ? », « c'est quoi le TPS ? », « c'est quoi le dwell ? », « c'est quoi l'avance ? » ou « c'est quoi le régime ? » peut recevoir « aucune mesure ECU disponible » au lieu d'expliquer le concept lorsque l'ECU est déconnecté.

Objectif avant le dernier HEAD : compléter `LocalAiClient::controlledTechnicalAnswer()` pour les **définitions certaines déjà connues** et uniquement lorsqu'une question demande le rôle/sens/définition : lambda/sonde O2, TPS/papillon, dwell, avance à l'allumage, régime/RPM, température liquide/ECT, température d'air/IAT et batterie/tension. Ne jamais intercepter une demande de valeur actuelle, diagnostic, normalité ou panne. Ces réponses restent instantanées sans Qwen et ne dépendent d'aucune mesure inventée.

Après ce lot : étendre le self-test déterministe sur un échantillon représentatif, laisser le dernier run complet aller jusqu'au package et ne plus pousser d'autre optimisation avant son verdict.

## ARCHITECTURE IA COURANTE — À CONSERVER

`navigationorderpatch.cpp -> IaMemsTab -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

- moteur dans le processus ; aucun `QProcess`, `llama-server`, HTTP localhost ou port 18089 ;
- génération hors thread UI ; base r20 read-only ; mesures ECU read-only ; aucune commande/mutation LLM ;
- Qwen3-0.6B ONNX INT4 CPU ; ONNX Runtime GenAI 0.14.0 + runtime app-local.

## OPTIMISATION LATENCE #81 — À PRÉSERVER

- `/no_think` ; normal **128 tokens max** ; diagnostic **192 tokens max** ;
- réponses déterministes/base experte sans Qwen quand connues.

## HISTORIQUE ONNX — NE PAS REFAIRE

- #74 : génération OK, rouge CP1252 ; #75 probe ONNX VERT ; #79 self-test production rouge ; #80 runtime app-local corrigé VERT ; #81 latence optimisée VERT + fonctionnement utilisateur confirmé.

## HISTORIQUE LLAMA — VOIE ABANDONNÉE

Ne pas revenir à llama.cpp sans décision explicite utilisateur. #63 lent 30 s–2 min ; #65 CMake ; #66 `0xC0000409` ; #67 `0xC0000135`; #68/#72 `QProcess FailedToStart` réel ; #73 `0xC0000409`. Décision : ONNX natif.

## PRINCIPE IA / BASE EXPERTE

**Toute connaissance certaine déjà présente dans le logiciel, aides/décodages ou base experte doit produire une réponse immédiate sans Qwen.** Qwen = croisement de faits, diagnostic, questions générales MEMS, cas sans réponse déterministe fiable.

Définitions contrôlées : MAP = Manifold Absolute Pressure ; injecteur = électrovanne essence ; SPI = Single Point Injection ; bobine = allumage/haute tension ; dwell = temps de charge primaire.

## CONNAISSANCES / AUDIT À PRÉSERVER

Aperçu : RPM, LDR, MAP, TPS, batterie, correction carburant, lambda, temps injecteur, IAT, IAC, avance, état système. Repères : lambda 0–200 mV pauvre, 700–900 mV riche ; MAP moteur arrêté ~100 kPa, ralenti ~25–40 kPa.

RAVE : ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 ; pas de réglage ralenti par butée ; conflits résistance bobine non arbitrés ; lots 1660/1670 r20 ; MEMS1.3 SPi / 1.6 SPi / 2J MPi distingués ; DTC supportés 1 ECT,2 IAT,10 pompe,16 TPS,20 lambda heater,21 synchro,22 fan1,24 fan2 ; code23 preuve insuffisante ; pins RCL0194 conservés.

## SÉCURITÉ PROTOCOLE — NE PAS MODIFIER PENDANT BUILD #30 IA

`MemsEcuFamily`/`MemsDiagnosticMode` existants ; D0/D1/D2 normal ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués générique ; mutations Rosco13_16 prouvé+Normal seulement ; Unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués ; transaction RAM bloque commandes ; conserver `onProtocolCommandRequested(quint8)` ; ralenti chaud `raw-32768-correction` ; dwell ~1,9–3,1 ms vers14V ; aucune mutation ECU BUILD30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion1.9, failsafe actionneurs, ports arbitraires, profils RAM non validés, reset/clear faults/trims/écritures BUILD30.

## UI OFFICIELLE À PRÉSERVER

Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9. Dark/responsive inchangé. Injection entre Aperçu et Réglages.

## EXIGENCES IA FUTURES — PAS MAINTENANT

Après stabilisation qualité : CSV/TXT local read-only + nouveau composeur dark/responsive. Aucune commande ECU.

## DÉSINSTALLATION BUILD #30

`ecu_mems_uninstaller.exe` + `install_manifest.txt` ; refus si app active ; profil conservé ; suppression données seulement explicite ; fichiers étrangers préservés.

## PROCHAINE ACTION EXACTE

**Reprendre sur `MEMSX64`, BUILD logiciel #30/v1.0.30. Base stable #81 `8671275...`. HEAD courant avant dernier lot définitions `4f12bb6...`. Compléter les définitions déterministes sûres sans intercepter les demandes de mesures/diagnostics, étendre le self-test, puis suivre uniquement le dernier Actions run jusqu'au package complet. Aucun BUILD #31, changement protocole, 32 bits, UI ou moteur ONNX.**