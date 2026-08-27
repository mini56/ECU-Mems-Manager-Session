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
- HEAD x64 courant après lot RAVE 1680 : **`d156469af53b4ba20e084439e2d62b691c9e199b`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Dernier GitHub Actions validé avant 1680 : **#86 — SUCCESS**.
- L'utilisateur teste **#86** en usage réel pendant la reprise RAVE.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` : **NE PAS TOUCHER**.
- Rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.
- Aucun changement protocole ECU, UI ou moteur IA pendant l'enrichissement documentaire.

## BUILD QUALITÉ #86 — À PRÉSERVER

### ECU MEMS Manager x64 #86 — Commit `50af0a0`

- Run `33004859269`, job `98296030990`, conclusion **SUCCESS**.
- Artifact `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9620325660`.
- Taille `386 747 323` octets.
- Digest `sha256:1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`.
- Compilation, protections protocole, tests déterministes, base r20, Qwen, vrai `LocalAiClient`, package, self-test depuis package, smoke launch, manifeste et upload : VERTS.

Lot qualité #81→#86 à préserver : prompt Qwen simplifié et anti-fuite ; nettoyage `<think>`/ChatML ; bobine/date fautive ; base r20 classée par pertinence/preuve ; distinction définition ≠ mesure live ; latence `/no_think`, 128 tokens normal / 192 diagnostic.

## ÉTAPE EN COURS — RAVE / ENRICHISSEMENT BASE

Autorisation utilisateur du 27 août 2026 : **« OK je vais tester #86 tu peux continuer sur RAVE et enrichir la base de donnée »**.

Règles : uniquement faits RAVE nouveaux et vérifiables ; conserver source/document/variante/section/preuve ; séparer SPi/MPi/familles ; ne jamais appeler automatiquement Mini MPi « MEMS 1.9 » ; conserver les conflits ; code 23 reste `preuve_insuffisante` ; aucun brochage inventé ; aucun changement IA/protocole/UI/32 bits/BUILD.

### AUDIT 1660 / 1670

- 1660 : `2d614cf47ea9842a2c24f53ddce3c9a38fc82fd2`, faits service RAVE.
- 1670 : `2e63d7012c8a84765c59552e1cf615198a7da3f7`, brochage RCL0194 Mini MPi.
- Convention confirmée : lots par dizaines ; 1680 = prochain lot.
- Format moderne : SQL UTF-8 → flux compatible `qCompress` → Base64 texte ; ordre donné par `research_enrichment_batches`.
- `schema_version=1`, `database_revision=20` conservés.

Audit de la r20 #86 avant 1680 :
- `mems_rave_fact` = 46 ; 31 RCL0193 + 15 RCL0194 MPi ;
- `mems_expert_fact_external` = 58 ;
- aucun `RAVE-WIR-SPIJ-*`.

### SOURCE / CIBLE 1680

Source constructeur : **RCL0194ENG, Mini Electrical Circuit Diagrams, 3rd Edition**.

Périmètre : SPi Japon Mini 97MY à partir du VIN **`SAXXNNAXKBD 134455`**. Pages inspectées : **20.3**, **20.4**, **39.3**. Variante enregistrée : `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`. Aucune génération MEMS déduite du seul schéma.

### LOT 1680 — VALIDATION AVANT PUSH

`research_enrichment_1680.qz64` contient **14 faits constructeur**, miroirés dans `mems_rave_fact` et `mems_expert_fact_external` : périmètre/VIN ; module relais 693 ; O2 ; pompe ; injecteur ; purge ; IAC 4 phases ; ECT ; TPS ; IAT ; masse capteurs C159-30 ; prise diagnostic ; chauffage collecteur ; ventilateur SPi Japon par thermocontact/relais.

Validation sur copie exacte de la r20 #86 :
- SQL appliqué sans erreur ;
- 14 `RAVE-WIR-SPIJ-*` dans la table RAVE + 14 dans la table experte ;
- tous `verifie_constructeur` ;
- total RAVE après lot = **60** ; total expert = **72** ;
- SQL 17 194 octets ; round-trip QZ64 valide ;
- SHA-256 SQL : `dec464ad3d5446a78886cfc995a1dc18553aab4f86ac2676803a56ac62d916dd` ;
- révision r20 inchangée.

### PUSH 1680 — FAIT

Commit atomique sur `MEMSX64` : **`d156469af53b4ba20e084439e2d62b691c9e199b`** — `BUILD #30 add verified RCL0194 SPi Japan wiring facts batch 1680`.

Le commit contient uniquement :
1. `database/reference/research_enrichment_1680.qz64` ;
2. `database/reference/manifest.json` avec ajout de 1680 après 1670, révision 20 inchangée ;
3. `database/reference/audits/rave_1680_audit.md` avec provenance, périmètre et validation.

Parent exact : #86 `50af0a0cd4614302794e464b9e1b4c675d1adff4`. Aucun fichier IA, protocole, UI ou 32 bits modifié.

## CONNAISSANCES RAVE À PRÉSERVER

- ralenti SPi 1993–96 : 850 ±25 tr/min ; SPi 1997+ et MPi : 900 ±50 ;
- pression carburant SPi ~1 bar ; MPi 3,0 ±0,2 bar ;
- ventilateur MPi97 : 105/98 °C ; SPi Japon : 98/93 °C ;
- pas de réglage ralenti par butée ; conflits résistance bobine conservés ;
- Rover distingue MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi ;
- DTC fortement supportés : 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante ;
- brochages MPi97 (1670) et SPi Japon 97MY (1680) doivent rester strictement séparés.

## SÉCURITÉ / NO-GO — INCHANGÉS

Ne pas modifier pendant BUILD #30 : sécurité protocole, MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion1.9, failsafe actionneurs, ports arbitraires, profils RAM non validés, reset/clear faults/trims/écritures. Conserver `onProtocolCommandRequested(quint8)` et `raw-32768-correction`.

## UI / IA — INCHANGÉES

UI officielle dark/responsive inchangée. IA ONNX native inchangée. Aucun `QProcess`, `llama-server`, HTTP localhost ou port 18089. Base r20 lecture seule ; aucune mutation ECU accessible au LLM.

## PROCHAINE ACTION EXACTE

**Suivre GitHub Actions du commit `d156469...`. Vérifier en priorité la génération/validation de la r20, puis la chaîne package complète. Si VERT, inspecter l'artifact produit et confirmer 60 faits RAVE, 72 faits experts et 14 `RAVE-WIR-SPIJ-*` dans les deux tables. Enregistrer le résultat dans ce rapport avant tout lot RAVE suivant. #86 reste le build utilisateur testé en parallèle. Aucun BUILD #31.**