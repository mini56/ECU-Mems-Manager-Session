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
- HEAD x64 avant lot RAVE 1680 : **`50af0a0cd4614302794e464b9e1b4c675d1adff4`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- GitHub Actions validé : **#86 — SUCCESS**.
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

Règles :
- uniquement des faits RAVE nouveaux, utiles et vérifiables ;
- conserver source, document, variante, section/page, niveau de preuve ;
- séparer strictement SPi / MPi / familles ; ne jamais appeler automatiquement Mini MPi « MEMS 1.9 » ;
- conserver les conflits ; code 23/antidémarrage reste `preuve_insuffisante` ;
- aucun brochage inventé ;
- aucun changement de schéma/révision de base si non nécessaire ;
- aucun changement IA/protocole/UI/32 bits/BUILD.

### AUDIT 1660 / 1670 — CONFIRMÉ

- Lot **1660** : commit `2d614cf47ea9842a2c24f53ddce3c9a38fc82fd2`, `BUILD #30 add verified RAVE service facts batch 1660`.
- Lot **1670** : commit `2e63d7012c8a84765c59552e1cf615198a7da3f7`, `BUILD #30 add verified RCL0194 Mini MPI wiring facts`.
- Convention confirmée : lots documentaires numérotés par dizaines ; prochain lot = **1680**.
- Format : SQL UTF-8 → flux compatible `qCompress` → Base64 texte ; application dans l'ordre de `research_enrichment_batches` du manifeste.
- La base reste `schema_version=1`, `database_revision=20`.

Audit de la r20 exacte extraite de l'artifact #86 avant 1680 :
- `mems_rave_fact` = **46** faits RAVE ;
- 31 faits RCL0193 ;
- 15 faits RCL0194, tous MPi 97MY ;
- les faits RAVE sont miroirés dans `mems_expert_fact_external` ;
- aucun fait de brochage RCL0194 SPi Japon 97MY n'était présent.

### SOURCE / CIBLE 1680 — CONFIRMÉES

Source constructeur : **RCL0194ENG, Mini Electrical Circuit Diagrams, 3rd Edition**.

Périmètre constructeur :
- MPi à partir du VIN `SAXXNNAZEBD 134455` ;
- SPi Japon à partir du VIN **`SAXXNNAXKBD 134455`**.

Pages inspectées :
- `Engine Management System (MEMS) SPi (JAPAN) 20.3` ;
- `Engine Management System (MEMS) SPi (JAPAN) 20.4` ;
- `Cooling Fan SPi (JAPAN) 39.3`.

Variante conservée dans chaque fait : **`SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`**. Ne pas déduire une génération MEMS précise du seul schéma RCL0194.

### RÉSULTAT — LOT 1680 CONSTRUIT ET VALIDÉ AVANT PUSH

Le fichier `research_enrichment_1680.qz64` a été construit localement avec **14 faits constructeur**, chacun inséré à la fois dans `mems_rave_fact` et dans `mems_expert_fact_external`.

Les 14 faits couvrent :
1. périmètre SPi Japon 97MY/VIN ;
2. module relais MEMS 693 ;
3. sonde O2 : C159-7, C159-18, C159-29 et commande chauffage C159-36 ;
4. pompe : commande relais C159-20 + interrupteur inertie C123 + pompe C205 ;
5. injecteur C522 : C159-1 / C159-24 ;
6. purge C152 : commande C159-21 ;
7. IAC C177 : quatre phases vers C159-3 / 27 / 22 / 2 ;
8. ECT C165 : signal C159-33 + masse capteurs C159-30 ;
9. TPS C175 : C159-8 / C159-9 + masse capteurs C159-30 ;
10. IAT C174 : C159-16 + masse capteurs C159-30 ;
11. masse capteurs commune C159-30 via SJ5 ;
12. prise diagnostic C549 : C159-10 / C159-15 ;
13. chauffage collecteur C224 + relais C269, sans attribuer de broche ECU ambiguë ;
14. ventilateur SPi Japon : relais C019 commandé par thermocontact haute température C370/C371, sans liaison MEMS montrée sur le circuit 39.3.

Validation locale sur une **copie exacte de `ia_mems_reference_r20.sqlite` issue de #86** :
- SQL appliqué sans erreur ;
- nouveaux `RAVE-WIR-SPIJ-*` dans `mems_rave_fact` : **14** ;
- miroir `RAVE-WIR-SPIJ-*` dans `mems_expert_fact_external` : **14** ;
- tous les niveaux = **`verifie_constructeur`** ;
- une seule variante = `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455` ;
- total RAVE après application : **60** ;
- total faits experts après application : **72** ;
- SQL non compressé : **17 194 octets** ;
- QZ64 Base64 : **3 048 caractères utiles** (+ fin de ligne) ;
- round-trip QZ64 validé : longueur déclarée 17 194 = longueur SQL décompressée 17 194 ;
- SHA-256 du SQL décompressé : `dec464ad3d5446a78886cfc995a1dc18553aab4f86ac2676803a56ac62d916dd`.

Aucun changement de `database_revision` ou de schéma n'est nécessaire.

## CONNAISSANCES RAVE À PRÉSERVER

- ralenti SPi 1993–96 : 850 ±25 tr/min ; SPi 1997+ et MPi : 900 ±50 ;
- pression carburant SPi ~1 bar ; MPi 3,0 ±0,2 bar ;
- ventilateur MPi97 : 105/98 °C ; SPi Japon : 98/93 °C ;
- ne pas régler le ralenti par la vis de butée ;
- conflits de résistance bobine conservés sans arbitrage ;
- Rover distingue MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi ;
- DTC fortement supportés : 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ;
- code 23 = preuve insuffisante ;
- brochage RCL0194 MPi97 du lot 1670 à conserver séparément du nouveau SPi Japon.

## SÉCURITÉ / NO-GO — INCHANGÉS

Ne pas modifier pendant BUILD #30 : sécurité protocole existante, MEMS1.9 F7/EF, tailles 7D/80, W4 25–50 ms, reconnexion 1.9, failsafe actionneurs, ports arbitraires, profils RAM non validés, reset/clear faults/trims/écritures. Conserver `onProtocolCommandRequested(quint8)` et la formule ralenti chaud `raw-32768-correction`.

## UI / IA — INCHANGÉES

UI officielle dark/responsive inchangée. Architecture IA ONNX native inchangée. Aucun `QProcess`, `llama-server`, HTTP localhost ou port 18089. Base r20 lecture seule ; aucune mutation ECU accessible au LLM.

## PROCHAINE ACTION EXACTE

**Pousser atomiquement sur `MEMSX64` le lot `research_enrichment_1680.qz64`, son ajout au manifeste r20 (révision 20 inchangée) et un audit de provenance RCL0194ENG. Puis suivre GitHub Actions jusqu'à validation de la base/package et vérifier que la r20 produite contient 60 faits RAVE et 14 `RAVE-WIR-SPIJ-*`. #86 reste le build utilisateur testé en parallèle. Aucun BUILD #31.**