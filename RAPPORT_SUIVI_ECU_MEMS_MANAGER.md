# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions, pas le BUILD logiciel.

## ÉTAT COURANT — 27 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 courant : **`50af0a0cd4614302794e464b9e1b4c675d1adff4`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- GitHub Actions courant validé : **#86 — SUCCESS**.
- L'utilisateur teste **#86** en usage réel pendant la reprise RAVE.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant l'enrichissement documentaire.
- UI dark/responsive officielle à préserver.

## DERNIER ÉTAT RÉEL UTILISATEUR

### ECU MEMS Manager x64 #81 — Commit `8671275` — FONCTIONNEL

Le 26 août 2026, l'utilisateur confirme : **« bon ça fonctionne »**.

Captures constatées : application lancée ; IA MEMS ouverte ; `base prête` ; `IA locale prête` ; IAC correct et immédiat ; ONNX/Qwen opérationnel.

Défauts #81 traités dans #86 : fuite `<think>` ; consignes internes ; méta-discours ; bobine ; nettoyage ; date avec faute ; définition ≠ mesure.

## BUILD QUALITÉ VALIDÉ CI

### ECU MEMS Manager x64 #86 — Commit `50af0a0` — VERT COMPLET

- Run : **`33004859269`** ; job **`98296030990`** ; conclusion **SUCCESS**.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`** ; ID **`9620325660`**.
- Taille : **386 747 323 octets**.
- Digest : **`sha256:1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`**.
- Compilation, protections protocole, tests déterministes, r20, Qwen, `LocalAiClient`, package, validation depuis package, smoke launch, manifeste et upload : VERTS.

**#86 est le candidat qualité actuellement testé par l'utilisateur.**

## LOT QUALITÉ #81 → #86 — À PRÉSERVER

- `a53fd13...` : prompt Qwen simplifié, anti-fuite, nettoyage `<think>`/ChatML, bobine, date fautive, latence 128/192 + `/no_think`.
- `e20eb3a...` : self-test production bobine/date/Qwen/anti-fuite.
- `4f12bb6...` : base r20 classée par pertinence/preuve/famille-firmware, 3 faits max, sources sur demande.
- `e6a4b70...` + `50af0a0...` : distinction définition / mesure live dans `IaResponseLogic`.

## ÉTAPE EN COURS — REPRISE RAVE / ENRICHISSEMENT BASE

Autorisation utilisateur du 27 août 2026 : **« OK je vais tester #86 tu peux continuer sur RAVE et enrichir la base de donnée »**.

Objectif :
1. reprendre l'audit Rover RAVE après les lots **1660** et **1670** ;
2. intégrer uniquement des faits RAVE nouveaux, utiles et vérifiables ;
3. conserver source/document/variante/page ou repère/niveau de preuve ;
4. séparer strictement SPi / MPi et les familles ; ne jamais appeler automatiquement Mini MPi « MEMS 1.9 » ;
5. conserver les conflits ; code 23/antidémarrage reste `preuve_insuffisante` ;
6. aucun brochage SPi inventé sans schéma usine lisible ;
7. régénérer/valider r20 sans changement de schéma/révision si non nécessaire ;
8. aucun changement IA, protocole, UI, 32 bits ou BUILD.

### RÉSULTAT — AUDIT DES LOTS 1660 / 1670

Historique Git confirmé :
- **1660** : commit `2d614cf47ea9842a2c24f53ddce3c9a38fc82fd2`, message `BUILD #30 add verified RAVE service facts batch 1660` ;
- **1670** : commit `2e63d7012c8a84765c59552e1cf615198a7da3f7`, message `BUILD #30 add verified RCL0194 Mini MPI wiring facts` ;
- convention actuelle confirmée : nouveaux lots documentaires numérotés par dizaines ; **1680** est donc le prochain numéro.

Format confirmé par le code runtime : chaque `research_enrichment_XXXX.qz64` est un flux SQL compressé avec le format `qCompress`, puis stocké en Base64 texte ; les lots sont appliqués dans l'ordre du tableau `research_enrichment_batches` de `database/reference/manifest.json`. Les lots modernes peuvent contenir du SQL multiligne terminé par `;`.

Audit direct de la base **r20 réellement produite par #86** :
- table `mems_rave_fact` : **46 faits RAVE** ;
- **31** faits RCL0193 (service, capteurs, stratégies, réglages) ;
- **15** faits RCL0194, tous consacrés au **MPi 97MY** ;
- ces faits RAVE sont également miroirés dans `mems_expert_fact_external`, donc utilisables directement par IA MEMS ;
- aucun fait RCL0194 de brochage **SPi Japon 97MY** n'est encore présent.

### CIBLE CONFIRMÉE — LOT 1680

Source constructeur : **RCL0194ENG 3rd Edition**, Mini 97MY. Le document indique explicitement :
- MPi : VIN à partir de `SAXXNNAZEBD 134455` ;
- **SPi Japon : VIN à partir de `SAXXNNAXKBD 134455`**.

Pages constructeur maintenant inspectées :
- **Engine Management System (MEMS) SPi (JAPAN) 20.3** ;
- **Engine Management System (MEMS) SPi (JAPAN) 20.4** ;
- **Cooling Fan SPi (JAPAN) 39.3**.

Faits candidats 1680 vérifiés sur les schémas :
- périmètre SPi Japon/VIN distinct du MPi ;
- module relais SPi Japon : relais principal, chauffage sonde O2, pompe carburant et démarreur ;
- sonde O2 : signaux ECU et masse écran distincts, relais chauffage commandé par ECU ;
- pompe : relais commandé par ECU + interrupteur à inertie + pompe ;
- injecteur SPi C522 et ses deux voies ECU ;
- purge canister C152 et commande ECU ;
- quatre phases moteur pas à pas IAC C177 vers ECU ;
- ECT C165, TPS C175 et IAT C174 avec **masse capteurs commune C159-30** ;
- prise diagnostic C549 vers ECU ;
- ventilateur SPi Japon 39.3 commandé par **thermocontact haute température + relais**, sans liaison MEMS montrée sur ce circuit, contrairement au MPi.

Règle pour 1680 : ne saisir que les liaisons dont la lecture du schéma est non ambiguë ; ne pas déduire une génération MEMS précise du seul schéma RCL0194 ; conserver la variante explicite `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455` dans chaque fait.

## ARCHITECTURE IA COURANTE — À CONSERVER

`navigationorderpatch.cpp -> IaMemsTab -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

- aucun `QProcess`, `llama-server`, HTTP localhost ou port 18089 ; génération hors thread UI ; r20 read-only ; aucune mutation ECU accessible au LLM ; Qwen3-0.6B ONNX INT4 CPU ; ONNX Runtime GenAI 0.14.0 app-local.

## OPTIMISATION LATENCE — À PRÉSERVER

- `/no_think` ; normal **128 tokens max** ; diagnostic **192 tokens max** ; réponses déterministes/base sans Qwen quand connues.

## HISTORIQUE ONNX — NE PAS REFAIRE

#74 génération OK/CP1252 ; #75 probe VERT ; #79 self-test rouge ; #80 runtime app-local VERT ; #81 latence VERT + fonctionnement utilisateur ; #82-#85 intermédiaires remplacés par #86.

## HISTORIQUE LLAMA — VOIE ABANDONNÉE

Ne pas revenir à llama.cpp sans décision explicite. #63 lent ; #65 CMake ; #66 `0xC0000409` ; #67 `0xC0000135` ; #68/#72 `QProcess FailedToStart` réel ; #73 `0xC0000409`.

## PRINCIPE IA / BASE EXPERTE

**Toute connaissance certaine déjà présente dans le logiciel, aides/décodages ou base experte doit produire une réponse immédiate sans Qwen.**

## CONNAISSANCES / AUDIT À PRÉSERVER

Aperçu : RPM, LDR, MAP, TPS, batterie, correction carburant, lambda, temps injecteur, IAT, IAC, avance, état système. Repères : lambda 0–200 mV pauvre, 700–900 mV riche ; MAP moteur arrêté ~100 kPa, ralenti ~25–40 kPa.

RAVE existant : ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 ; pas de réglage ralenti par butée ; conflits résistance bobine non arbitrés ; MEMS1.3 SPi / 1.6 SPi / 2J MPi distingués ; DTC supportés 1 ECT,2 IAT,10 pompe,16 TPS,20 lambda heater,21 synchro,22 fan1,24 fan2 ; code23 preuve insuffisante ; pins RCL0194 MPi97 conservés.

## SÉCURITÉ PROTOCOLE — NE PAS MODIFIER PENDANT BUILD #30

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

**Créer le lot `research_enrichment_1680.qz64` à partir des pages constructeur RCL0194ENG 20.3/20.4/39.3, avec uniquement les liaisons SPi Japon 97MY non ambiguës, les insérer à la fois dans `mems_rave_fact` et `mems_expert_fact_external`, ajouter 1680 au manifeste sans changer la révision, valider le SQL contre une copie de la r20 #86, pousser sur `MEMSX64`, puis suivre la CI. #86 reste le build utilisateur testé en parallèle. Aucun BUILD #31.**