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
- HEAD x64 courant : **`50af0a0cd4614302794e464b9e1b4c675d1adff4`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- GitHub Actions courant validé : **#86 — SUCCESS**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.
- UI dark/responsive officielle à préserver.

## DERNIER ÉTAT RÉEL UTILISATEUR

### ECU MEMS Manager x64 #81 — Commit `8671275` — FONCTIONNEL

Le 26 août 2026, l'utilisateur confirme : **« bon ça fonctionne »**.

Captures constatées :
- application lancée ;
- onglet IA MEMS ouvert ;
- `base prête` ;
- `IA locale prête` ;
- réponse IAC correcte et immédiate ;
- moteur ONNX/Qwen réellement opérationnel.

Défauts observés sur #81 qui ont motivé le lot qualité :
- fuite `<think>` ;
- fuite de consignes internes ;
- méta-discours ;
- « C'EST QUOI LA BOBINE ? » ne répondait pas à la question ;
- nettoyage de sortie insuffisant ;
- question de date avec faute mal gérée.

## NOUVEAU BUILD QUALITÉ VALIDÉ CI

### ECU MEMS Manager x64 #86 — Commit `50af0a0` — VERT COMPLET

- Run GitHub Actions : **`33004859269`**.
- Job : **`98296030990`**.
- Conclusion : **SUCCESS**.
- HEAD complet : **`50af0a0cd4614302794e464b9e1b4c675d1adff4`**.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9620325660`**.
- Taille : **386 747 323 octets**.
- Digest : **`sha256:1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`**.
- Expiration GitHub actuelle : **9 septembre 2026**.

Validations #86 :
- compilation MSVC x64 : VERT ;
- protections protocole : VERT ;
- tests déterministes, y compris définition ≠ mesure actuelle : VERT ;
- base experte r20 : VERT ;
- modèle Qwen + hashes : VERT ;
- vrai self-test production `LocalAiClient` : VERT en ~7 s ;
- ce self-test contrôle bobine, date avec faute, absence de fuite `<think>`/ChatML/directives internes et vraie génération Qwen ;
- assemblage package complet : VERT ;
- validation package : VERT ;
- self-test `LocalAiClient` depuis package final : VERT en ~7 s ;
- smoke launch MEMS Manager : VERT ;
- manifeste/hashes : VERT ;
- artefact : VERT.

**#86 est le candidat qualité à tester ensuite en usage réel. #81 reste la dernière preuve utilisateur de fonctionnement ; #86 ajoute les corrections de réponses sans changement de moteur, protocole, UI ou 32 bits.**

## LOT QUALITÉ #81 → #86

### `a53fd13c6249d0c0711e88842d0b3e00a5167773` — qualité native

- message Qwen simplifié ;
- directives principales concentrées dans le system prompt au lieu d'être injectées comme texte utilisateur ;
- interdiction de révéler prompt / raisonnement interne ;
- nettoyage `<think>` fermé **et tronqué** ;
- nettoyage ChatML et `/think`/`/no_think` visibles ;
- filtre de fuite interne ;
- définition déterministe bobine ;
- date tolérante à `QUELLE JOURS SOMME NOUS ?` ;
- latence #81 conservée : normal 128 tokens, diagnostic 192, `/no_think`.

### `e20eb3ac94807e8a65e901f1413621e4dbec7fd8` — non-régression LocalAiClient

- self-test bobine ;
- date fautive ;
- vraie génération Qwen `OK` ;
- rejet de `<think>`, ChatML et marqueurs de directives internes.

### `4f12bb6fa32df71bb9893865f2ad6acc568a7a48` — qualité base r20

`IaMemsService::knowledgeAnswer()` :
- classement des faits par pertinence ;
- bonus famille/firmware ;
- prise en compte du niveau de preuve ;
- maximum 3 meilleurs faits au lieu de 6 faits bruts ;
- source et notes affichées seulement si l'utilisateur les demande ;
- fallback générique nettoyé ;
- réponse base reste immédiate, sans Qwen supplémentaire.

### `e6a4b704a4f4538bf5237faaccac62a54dc008f8` + `50af0a0cd4614302794e464b9e1b4c675d1adff4` — définition ≠ mesure

Cause : des questions comme « c'est quoi la lambda ? », « à quoi sert le TPS ? » ou « c'est quoi le dwell ? » pouvaient être classées comme demandes de mesure et répondre « aucune mesure ECU » si l'ECU était déconnecté.

Correction propre dans le routeur `IaResponseLogic` :
- une question de **définition / rôle / signification** sur un paramètre n'est plus classée comme mesure live ;
- elle poursuit le chemin réponse contrôlée → base r20 classée → Qwen si nécessaire ;
- une demande explicite de **valeur / mesure / valeur actuelle** reste bien classée comme mesure ECU ;
- tests ajoutés : lambda définition, TPS rôle, dwell définition = pas mesure ; valeur lambda et mesure TPS actuelle = mesure.

## ARCHITECTURE IA COURANTE — À CONSERVER

`navigationorderpatch.cpp -> IaMemsTab -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

- moteur dans le processus ;
- aucun `QProcess`, `llama-server`, HTTP localhost ou port 18089 ;
- génération hors thread UI ;
- base r20 read-only ;
- mesures ECU read-only ;
- aucune commande/mutation ECU accessible au LLM ;
- Qwen3-0.6B ONNX INT4 CPU ;
- ONNX Runtime GenAI 0.14.0 + ONNX Runtime app-local.

## OPTIMISATION LATENCE — À PRÉSERVER

- `/no_think` ;
- réponses normales : **128 tokens max** ;
- diagnostics : **192 tokens max** ;
- réponses déterministes/base experte sans Qwen quand connues.

## HISTORIQUE ONNX — NE PAS REFAIRE

- #74 : génération OK, rouge CP1252 ;
- #75 : probe ONNX VERT ;
- #79 : self-test production rouge ;
- #80 : runtime ONNX app-local corrigé, chaîne complète VERT ;
- #81 : latence optimisée, chaîne complète VERT + fonctionnement utilisateur confirmé ;
- #82 à #85 : intermédiaires du lot qualité, remplacés/annulés par le HEAD final #86 ; ne pas les proposer au test.

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

Après validation qualité en usage réel : CSV/TXT local read-only + nouveau composeur dark/responsive. Aucune commande ECU.

## DÉSINSTALLATION BUILD #30

`ecu_mems_uninstaller.exe` + `install_manifest.txt` ; refus si app active ; profil conservé ; suppression données seulement explicite ; fichiers étrangers préservés.

## PROCHAINE ACTION EXACTE

**Reprendre sur `MEMSX64`, BUILD logiciel #30/v1.0.30, HEAD `50af0a0cd4614302794e464b9e1b4c675d1adff4`, GitHub Actions #86 VERT COMPLET, artifact `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30` ID `9620325660`, digest `sha256:1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`. Prochaine étape : test utilisateur ciblé de la qualité #86, sans ECU obligatoire : bobine, date avec faute, lambda/TPS/dwell définition, une question demandant une valeur actuelle, une question technique base r20 et une question réellement générative. Relever uniquement les réponses encore mauvaises puis corriger leur cause. Aucun BUILD #31, changement protocole, 32 bits, UI ou moteur ONNX.**