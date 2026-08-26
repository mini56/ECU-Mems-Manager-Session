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
- HEAD qualité courant : **`e20eb3ac94807e8a65e901f1413621e4dbec7fd8`**.
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

Constaté sur les captures :
- application lancée ;
- onglet **IA MEMS** ouvert ;
- statut **base prête** ;
- statut **IA locale prête** ;
- réponse déterministe IAC correcte et immédiate : `IAC signifie Idle Air Control...` ;
- moteur ONNX/Qwen opérationnel dans l'application réelle.

Défauts de qualité encore visibles :
- fuite de marqueur **`<think>`** dans l'affichage ;
- fuite de consignes internes / prompt système (`Langage obligatoire`, règles de réponse, méta-discours) ;
- certaines sorties Qwen répondent au contexte/prompt au lieu de répondre à la question ; exemple **« C'EST QUOI LA BOBINE ? »** qui produit du méta-discours au lieu d'une définition ;
- nettoyage de sortie insuffisant ;
- réponse date observée avec formulation/troncature anormale dans l'historique affiché.

**Conclusion : l'intégration ONNX fonctionne. Ne plus changer de moteur. Le travail actif devient l'optimisation de la qualité des réponses.**

## ÉTAPE EN COURS — OPTIMISATION QUALITÉ DES RÉPONSES IA

Autorisation utilisateur : **oui — « donc on travaille là-dessus »**, puis **« OK fait ça ensuite optimise les réponses »**.

Objectif :
1. empêcher toute fuite de `<think>` / balises Qwen / prompt interne ;
2. faire répondre Qwen à la question au lieu de commenter les instructions ;
3. conserver les réponses déterministes immédiates fiables ;
4. améliorer la couverture déterministe des notions sûres ;
5. conserver `/no_think` et les budgets #81 ;
6. améliorer la pertinence et la présentation des réponses issues de la base r20 ;
7. ajouter des tests de non-régression ;
8. aucun changement protocole ECU, UI, 32 bits, base r20 ou BUILD.

### DIAGNOSTIC RACINE — LocalAiClient #81

- directives internes encore concaténées dans le message `user` envoyé à Qwen ;
- `cleanModelReply()` ne supprimait que `<think>...</think>` fermé : un `<think>` tronqué restait affiché ;
- aucun filtre de fuite explicite ;
- définition simple de bobine absente ; seulement le cas valeur/référence dwell ;
- détection date insuffisamment tolérante à `QUELLE JOURS SOMME NOUS?`.

### CORRECTIONS DÉJÀ POUSSÉES — EN VALIDATION

Commit **`a53fd13c6249d0c0711e88842d0b3e00a5167773`** — `BUILD #30 improve native IA response quality` :
- message utilisateur Qwen simplifié ;
- consignes principales déplacées/concentrées dans le system prompt ;
- interdiction de révéler prompt/raisonnement ;
- nettoyage `<think>` fermé **et tronqué** + balises ChatML ;
- filtre des marqueurs de fuite interne ;
- définition déterministe de la bobine ;
- date tolérante aux fautes `jours/somme` ;
- latence #81 conservée (128/192 tokens, `/no_think`).

Commit **`e20eb3ac94807e8a65e901f1413621e4dbec7fd8`** — `BUILD #30 test IA response quality regressions` :
- self-test production étendu : définition bobine ; date avec faute ; génération ONNX réelle `OK` ;
- échec si `<think>`, ChatML ou directive interne apparaît.
- GitHub Actions **#83** lancé sur ce HEAD ; ne pas présenter le commit intermédiaire #82 à l'utilisateur.

### SOUS-ÉTAPE SUIVANTE — QUALITÉ DES RÉPONSES BASE r20

Diagnostic : `IaMemsService::knowledgeAnswer()` sélectionne actuellement jusqu'à **6 faits** dès qu'**un seul terme** correspond, dans l'ordre brut de la base, puis affiche systématiquement `source_key` et `notes`. Risques : faits moins pertinents en tête, bruit, réponse trop technique/brute.

Objectif avant modification :
- classer les faits par nombre/qualité de correspondances avec la question ;
- favoriser contexte famille/firmware et niveau de preuve élevé ;
- limiter la réponse aux meilleurs faits ;
- conserver le niveau de preuve visible mais n'afficher la source détaillée que si l'utilisateur la demande ;
- ne jamais faire passer cette amélioration par Qwen : réponse base doit rester immédiate ;
- nettoyer le fallback générique afin qu'il ne contienne plus une instruction adressée au modèle.

## ARCHITECTURE IA COURANTE — À CONSERVER

`navigationorderpatch.cpp -> IaMemsTab -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

- moteur génératif dans le processus ; aucun `QProcess`, `llama-server`, HTTP localhost ou port 18089 ;
- génération hors thread UI ;
- base r20 read-only ; mesures ECU read-only ; aucune commande/mutation accessible au LLM ;
- Qwen3-0.6B ONNX INT4 CPU ; ONNX Runtime GenAI 0.14.0 + runtime app-local.

## OPTIMISATION LATENCE #81 — À PRÉSERVER

- `/no_think` ;
- réponses normales : **128 tokens max** ;
- diagnostics : **192 tokens max** ;
- réponses déterministes/base experte sans Qwen quand elles sont connues.

## HISTORIQUE ONNX — NE PAS REFAIRE

- #74 : modèle/génération OK ; rouge uniquement `UnicodeEncodeError` CP1252.
- #75 : probe ONNX Windows x64 VERT ; artefact 659 octets = probe seulement.
- #79 (`9e105623...`) : compilation/tests/base/modèle VERTS, échec self-test production ; aucun package.
- #80 (`e460586...`) : runtime app-local corrigé, chaîne complète VERT.
- #81 (`8671275...`) : optimisation latence, chaîne complète VERT + test utilisateur fonctionnel.

## HISTORIQUE LLAMA — VOIE ABANDONNÉE

Ne pas revenir à llama.cpp sans décision explicite utilisateur. #63 lent 30 s–2 min ; #65 CMake ; #66 `0xC0000409` ; #67 `0xC0000135`; #68/#72 `QProcess FailedToStart` réel ; #73 `0xC0000409`. Décision : ONNX natif.

## PRINCIPE IA / BASE EXPERTE

**Toute connaissance certaine déjà présente dans le logiciel, aides/décodages ou base experte doit produire une réponse immédiate sans Qwen.** Qwen = croisement de faits, diagnostic, questions générales MEMS, cas sans réponse déterministe fiable.

Définitions contrôlées : MAP = Manifold Absolute Pressure ; injecteur = électrovanne essence ; SPI = Single Point Injection ; bobine = allumage/haute tension, dwell = temps de charge primaire.

## CONNAISSANCES / AUDIT À PRÉSERVER

Aperçu : RPM, LDR, MAP, TPS, batterie, correction carburant, lambda, temps injecteur, IAT, IAC, avance, état système. Repères : lambda 0–200 mV pauvre, 700–900 mV riche ; MAP moteur arrêté ~100 kPa, ralenti ~25–40 kPa.

RAVE : ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 ; pas de réglage ralenti par butée ; conflits résistance bobine non arbitrés ; lots 1660/1670 dans r20 ; MEMS1.3 SPi / 1.6 SPi / 2J MPi distingués ; DTC supportés 1 ECT,2 IAT,10 pompe,16 TPS,20 lambda heater,21 synchro,22 fan1,24 fan2 ; code23 preuve insuffisante ; pins RCL0194 conservés.

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

**Reprendre sur `MEMSX64`, BUILD logiciel #30/v1.0.30. Base stable validée : #81 `8671275...`. Travail qualité courant : HEAD `e20eb3a...`, Actions #83 en validation. Après validation du filtre/prompt/self-test, poursuivre immédiatement l'optimisation `IaMemsService::knowledgeAnswer()` : classement de pertinence, preuve concise, sources uniquement sur demande, fallback générique propre. Aucun BUILD #31, changement protocole, 32 bits ou UI. Puis suivre le dernier run jusqu'au package complet avant tout nouveau test utilisateur.**