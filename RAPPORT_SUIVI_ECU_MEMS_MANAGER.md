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
- HEAD x64 courant : **`84e677a87067ae43054df7fc534d84a70b6908b7`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Dernier GitHub Actions entièrement validé avant 1690 : **#88 — SUCCESS**, run `33042796844`.
- Artefact #88 : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9634616803`, taille `386 761 108`, SHA-256 `bf646347db27b0406483196f3057c0828d5a1a7645d4c3eaeee06dcb854c825a`.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## QUALITÉ IA #88 — VALIDÉE

#88 corrige le ciblage observé sur les captures utilisateur : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, et liste des cadrans dans `L4ONGLET APERCU`.

Validation complète #88 : compilation, protections protocole, tests déterministes, base r20/RAVE 1680, Qwen, vrai `LocalAiClient` natif et packagé, package, smoke launch, manifeste et artefact : **SUCCESS**.

## RAVE 1690 — COULEURS DE FILS SPi JAPON 97MY — POUSSÉ

### Source primaire
- Rover Technical Communication — `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition.
- Périmètre : SPi (Japan), MINI 97 Model Year from VIN `SAXXNNAXKBD 134455`.
- Page 20.4 + légende constructeur des codes couleur.

### Faits ajoutés
1. Diagnostic C549-2 ↔ C159-10 : **WY = blanc/jaune**.
2. Diagnostic C549-3 ↔ C159-15 : **BG = noir/vert**.
3. ECT C165-2 ↔ C159-33 : **KG = rose/vert**.
4. Masse capteurs commune ↔ C159-30 : **KB = rose/noir** ; ECT/TPS/IAT via SJ5.
5. TPS C175-2 ↔ C159-8 : **YG = jaune/vert**.
6. TPS C175-3 ↔ C159-9 : **YP = jaune/violet**.
7. IAT C174-2 ↔ C159-16 : **GB = vert/noir**.

### Validation locale avant push
- 7 faits RAVE + 7 miroirs experts.
- total RAVE prévu : **60 → 67** ; total expert : **72 → 79**.
- SQLite integrity `ok`, user_version 20.
- tous `verifie_constructeur`, une seule variante SPi Japon 97MY.
- SQL 8708 octets, SHA-256 `aef8a4c9d808267188c76c25c53dac254cba57f434a16badf7791023ffcfa97a`.
- qz64 1397 octets, SHA-256 `7288e46b99caf41e14751ab5690a7bf17fd35941e1e17527af472bf8bad3ec19`.

### Push
- Préparation sur `tmp-rave-1690` depuis le HEAD #88.
- Diff : exactement trois fichiers documentaires/base : payload 1690, audit 1690, manifeste.
- `MEMSX64` avancée en fast-forward sur **`84e677a87067ae43054df7fc534d84a70b6908b7`**.
- Aucun code IA, protocole, UI, packaging, 32 bits ou changement de BUILD.

### Règle de prudence
Ne pas généraliser ces couleurs aux autres SPi/MEMS ou au MPi. Ne pas déduire une couleur à partir d’un ordre de texte ambigu.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Identifier le GitHub Actions déclenché par `84e677a8`, suivre génération base r20 et package jusqu’au verdict, vérifier 67 faits RAVE / 79 faits experts. Ensuite préparer RAVE 1700 séparément avec d’autres liaisons couleur directement lisibles dans RCL0194, sans généralisation. Aucun BUILD #31.**