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
- HEAD x64 courant avant optimisation qualité : **`8671275bc77eb1fdbdefc0b0158254efdf86df5c`**.
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

Autorisation utilisateur : **oui — « donc on travaille là-dessus »**.

Objectif exact avant toute autre évolution :
1. empêcher toute fuite de `<think>` / balises Qwen / prompt interne / consignes système ;
2. faire répondre Qwen à la question utilisateur, pas commenter les instructions de contexte ;
3. conserver les réponses déterministes immédiates déjà fiables ;
4. améliorer la couverture déterministe des notions MEMS déjà connues quand la base/logiciel contient une réponse certaine (ex. bobine) ;
5. conserver le mode rapide #81 (`/no_think`, budgets courts) ;
6. ajouter des tests de non-régression sur ces défauts avant packaging ;
7. aucun changement protocole ECU, UI, 32 bits, base r20 ou numéro BUILD pendant cette correction.

### DIAGNOSTIC RACINE AVANT MODIFICATION

Inspection du `LocalAiClient` de #81 :
- les directives internes `LANGUE OBLIGATOIRE`, `DOMAINE OBLIGATOIRE`, contexte candidat et format diagnostic sont encore concaténées dans le **message user** envoyé à Qwen ; cela augmente le risque que le petit modèle les répète au lieu de répondre ;
- `cleanModelReply()` ne supprime que la forme **fermée** `<think>...</think>` ; une sortie tronquée commençant par `<think>` sans `</think>` reste donc visible telle quelle ;
- aucun filtre ne rejette actuellement une réponse qui recopie explicitement les directives internes ;
- la définition simple de **bobine** n'est pas interceptée : seule une question demandant la valeur/référence/normal du dwell reçoit la réponse déterministe ;
- la détection de date ne couvre pas certaines fautes simples vues au test, par exemple `QUELLE JOURS SOMME NOUS?` (`jours/somme`).

Correction retenue : simplifier le message utilisateur transmis à Qwen, renforcer le nettoyage et la détection de fuite, ajouter les réponses déterministes sûres manquantes (bobine/date tolérante aux fautes), puis étendre le self-test production sans modifier moteur, runtime, protocole, UI ou 32 bits.

## ARCHITECTURE IA COURANTE — À CONSERVER

`navigationorderpatch.cpp -> IaMemsTab -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

- moteur génératif dans le processus MEMS Manager ;
- aucun `QProcess` IA ;
- aucun `llama-server.exe` ;
- aucun HTTP localhost / port 18089 ;
- chargement/génération hors thread UI ;
- base experte r20 préconstruite, lecture seule ;
- mesures ECU read-only dans le contexte ;
- aucune commande/mutation ECU accessible au LLM ;
- Qwen3-0.6B ONNX INT4 CPU ;
- ONNX Runtime GenAI 0.14.0 + ONNX Runtime app-local.

## OPTIMISATION LATENCE #81 — À PRÉSERVER

Commit `8671275bc77eb1fdbdefc0b0158254efdf86df5c` :
- `/no_think` ;
- réponses normales : **128 tokens max** ;
- diagnostics génératifs : **192 tokens max** ;
- réponses déterministes/base experte servies sans appel Qwen lorsqu'elles sont connues.

## HISTORIQUE ONNX — NE PAS REFAIRE

- #74 : modèle chargé/génération OK ; rouge uniquement `UnicodeEncodeError` console CP1252.
- #75 : probe ONNX Windows x64 VERT, runtime officiel et Qwen snapshot/hashes vérifiés ; artefact 659 octets = probe seulement.
- #79 (`9e105623...`) : compilation/tests/base/modèle VERTS, échec unique du self-test production `LocalAiClient`; aucun package final.
- #80 (`e460586c140d2f87cbcbbe45740ef666c7923395`) : runtime ONNX app-local corrigé, chaîne complète VERT.
- #81 (`8671275...`) : optimisation latence, chaîne complète VERT + test utilisateur fonctionnel.

## HISTORIQUE LLAMA — VOIE ABANDONNÉE

Ne pas revenir à llama.cpp sans décision explicite utilisateur.

- #63 : fonctionnel mais Qwen 30 s à 2 min ; défauts MAP/injecteur/SPI ensuite corrigés.
- `f860749...` : réponses contrôlées MAP / injecteur / SPI.
- #65 : `GGML_BACKEND_DL requires BUILD_SHARED_LIBS`.
- #66 : crash modèle `0xC0000409`.
- #67 : staging incomplet `0xC0000135 STATUS_DLL_NOT_FOUND`.
- #68 et #72 : CI verte mais `QProcess 0 / FailedToStart` sur test réel.
- #73 : nouveau `0xC0000409`.
- décision : remplacement par ONNX natif.

## PRINCIPE IA / BASE EXPERTE

**Toute connaissance certaine déjà présente dans le logiciel, ses aides/décodages ou la base experte doit produire une réponse immédiate sans modèle génératif.**

Qwen est réservé au croisement de faits, au raisonnement diagnostic, aux questions générales MEMS et aux cas sans réponse déterministe fiable.

Réponses immédiates à préserver : batterie, régime, température liquide, MAP, lambda, avance, dwell, ralenti/IAC, papillon, état moteur, cohérence, MAP/injecteur/SPI.

Définitions contrôlées :
- MAP = `Manifold Absolute Pressure`, pression absolue collecteur / charge ;
- injecteur = électrovanne essence commandée ECU, SPI/MPI, jamais injection d'huile ;
- SPI = `Single Point Injection`, injection monopoint Rover/Mini MEMS.

## CONNAISSANCES / AUDIT À PRÉSERVER

Aperçu : RPM, LDR, MAP, TPS, batterie, correction carburant, lambda, temps injecteur, IAT, IAC, avance, état système. Repères : lambda 0–200 mV pauvre, 700–900 mV riche ; MAP moteur arrêté ~100 kPa, ralenti ~25–40 kPa.

Réglages : correction carburant, ralenti chaud, vitesse ralenti, correction avance, remises à zéro.

Actionneurs : PTC/collecteur, pompe, chauffage lambda, purge, clim, pression/boost, ventilateurs 1/2/3, injecteur, bobine, IAC, reset actionneurs.

Codes : 01–24 ; code 23/antidémarrage reste non prouvé.

### RAVE / MINI SPi / MPi
- ralenti SPi 1993–96 : 850 ±25 tr/min ; SPi 1997+ et MPi : 900 ±50 tr/min ;
- pression carburant SPi ~1 bar ; MPi 3,0 ±0,2 bar ;
- ventilateur MPi 97MY 105 °C ON / 98 °C OFF ; SPi Japon 98/93 °C ;
- ne pas régler ralenti par vis de butée papillon ;
- conflits résistance bobine conservés sans arbitrage non prouvé ;
- lots base 1660/1670 déjà poussés ; r20 régénérée sans changement de schéma ;
- Rover distingue MEMS 1.3 SPi, 1.6 SPi, 2J MPi ; ne pas appeler automatiquement MPi « MEMS 1.9 » ;
- DTC fortement supportés : `0x80` code 1 ECT, 2 IAT, 10 pompe, 16 TPS ; `0x7D:0x05` 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ;
- code 23 / bit6 = preuve insuffisante ;
- RCL0194 MPi 97MY : MAP C159-8, retour capteurs C159-13, IAT C159-14, ECT C159-36, pompe C159-30, antidémarrage C159-17.

## SÉCURITÉ PROTOCOLE — NE PAS MODIFIER PENDANT BUILD #30 IA

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}` ; `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}` ;
- D0/D1/D2 normal uniquement ; D1 bloqué Mode4 ; D3/F3/F4/F5 bloqués interface générique ;
- mutations uniquement Rosco13_16 prouvé + Normal ; Unknown fail-closed ; MEMS1.9 mutations bloquées ; F7/EF bloqués sans sous-type ; transaction RAM bloque commandes génériques ;
- conserver `void onProtocolCommandRequested(quint8 command);` ;
- traces D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00` ;
- ralenti chaud `raw - 32768 - correction` réelle ; jamais -3 hardcodé ;
- dwell repère ~1,9–3,1 ms vers 14 V ;
- aucune mutation ECU pendant BUILD #30.

## BLOQUEURS NO-GO

MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports série arbitraires, profils RAM non validés, reset/clear faults/trims/écritures pendant BUILD #30.

## UI OFFICIELLE À PRÉSERVER

Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9. Style dark/responsive inchangé. Injection reste entre Aperçu et Réglages.

## EXIGENCES IA FUTURES — PAS MAINTENANT

Après stabilisation qualité moteur : CSV/TXT local read-only (drag/drop, `+`, filtre, suppression du fichier sélectionné) et nouveau composeur dark/responsive. Aucune commande ECU.

## DÉSINSTALLATION BUILD #30

`ecu_mems_uninstaller.exe` + `install_manifest.txt` ; refus si app active ; profil conservé par défaut ; données locales supprimées seulement sur choix explicite ; fichiers étrangers préservés.

## PROCHAINE ACTION EXACTE

**Reprendre sur `MEMSX64`, BUILD logiciel #30/v1.0.30, base stable #81 `8671275bc77eb1fdbdefc0b0158254efdf86df5c`. L'intégration ONNX est validée en CI et fonctionne dans le test utilisateur. Travailler uniquement sur la QUALITÉ DES RÉPONSES : supprimer les fuites `<think>`/prompt interne, empêcher le méta-discours, améliorer le nettoyage et ajouter les réponses déterministes sûres déjà connues (notamment bobine) sans dégrader la latence #81. Ajouter des tests de non-régression, pousser sur `MEMSX64`, suivre GitHub Actions jusqu'au package complet. Aucun BUILD #31, aucun changement protocole ECU, 32 bits ou UI.**