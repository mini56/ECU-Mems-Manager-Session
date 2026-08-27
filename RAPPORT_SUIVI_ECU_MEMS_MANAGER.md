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
- HEAD x64 courant : **`5e707530352f5e4da3a04e832f62acfa7054ef2f`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- GitHub Actions qualité : **#88 — SUCCESS**, run **`33042796844`**, job `98419961592`.
- Artefact #88 : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID **`9634616803`**, taille **386 761 108** octets, SHA-256 **`bf646347db27b0406483196f3057c0828d5a1a7645d4c3eaeee06dcb854c825a`**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## QUALITÉ IA #88 — VALIDÉE

#88 corrige le ciblage observé sur les captures utilisateur : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, et liste des cadrans dans `L4ONGLET APERCU`.

Validation complète #88 : compilation, protections protocole, tests déterministes, base r20/RAVE 1680, Qwen, vrai `LocalAiClient` natif et packagé, package, smoke launch, manifeste et artefact : **SUCCESS**. Aucun changement protocole/UI/32 bits.

## RAVE / BASE — LOT 1680 VALIDÉ

- 1660 : faits service RAVE ; 1670 : RCL0194 Mini MPi wiring ; 1680 : RCL0194ENG SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`, pages 20.3/20.4/39.3.
- 1680 ajoute 14 faits constructeur + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur`.
- Brochages vérifiés : O2, pompe, injecteur, purge, IAC, ECT, TPS, IAT, masse capteurs, prise diagnostic, etc.

## RAVE 1690 — COULEURS DE FILS SPi JAPON 97MY

### Source primaire
- **Rover Technical Communication — MINI Electrical Circuit Diagrams, publication RCL0194ENG, 3rd Edition**.
- Périmètre retenu : **SPi (Japan), MINI 97 Model Year from VIN `SAXXNNAXKBD 134455`**.
- La légende constructeur donne les codes : B=noir, G=vert, K=rose, LG=vert clair, N=brun, O=orange, P=violet, R=rouge, S=gris ardoise, U=bleu, W=blanc, Y=jaune.
- Page **20.4** : associations directes code couleur + connecteur + fonction.

### 7 faits retenus
1. Diagnostic C549-2 ↔ C159-10 : **WY = blanc/jaune**.
2. Diagnostic C549-3 ↔ C159-15 : **BG = noir/vert**.
3. ECT C165-2 ↔ C159-33 : **KG = rose/vert**.
4. Masse capteurs commune ↔ C159-30 : **KB = rose/noir** ; ECT/TPS/IAT via SJ5.
5. TPS C175-2 ↔ C159-8 : **YG = jaune/vert**.
6. TPS C175-3 ↔ C159-9 : **YP = jaune/violet**.
7. IAT C174-2 ↔ C159-16 : **GB = vert/noir**.

### Validation locale avant push
Le SQL 1690 a été appliqué sur une copie de la base experte r20 issue de l’artefact #88.

- `RAVE-COLOR-SPIJ-*` dans `mems_rave_fact` : **7**.
- miroirs `RAVE-COLOR-SPIJ-*` dans `mems_expert_fact_external` : **7**.
- total RAVE : **60 → 67**.
- total expert : **72 → 79**.
- `PRAGMA integrity_check` : **ok**.
- `PRAGMA user_version` : **20**.
- tous les faits 1690 : `verifie_constructeur`.
- variante unique : `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`.
- SQL décompressé : **8708 octets**.
- SHA-256 SQL : **`aef8a4c9d808267188c76c25c53dac254cba57f434a16badf7791023ffcfa97a`**.
- fichier qz64 : **1397 octets**.
- SHA-256 qz64 : **`7288e46b99caf41e14751ab5690a7bf17fd35941e1e17527af472bf8bad3ec19`**.

### Règle de prudence
Ne pas généraliser ces couleurs aux autres SPi/MEMS ou au MPi. Ne pas déduire une couleur à partir d’un ordre de texte ambigu. Seules les liaisons explicitement rattachées sur RCL0194 20.4 entrent en 1690.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Créer un lot documentaire atomique 1690 sur une branche temporaire depuis `MEMSX64` : ajouter `research_enrichment_1690.qz64`, mettre à jour `database/reference/manifest.json`, ajouter l’audit de provenance 1690, comparer le diff, puis fast-forward `MEMSX64` une seule fois. Suivre ensuite le GitHub Actions jusqu’à la base/package, puis préparer le lot RAVE suivant. Aucun BUILD #31.**