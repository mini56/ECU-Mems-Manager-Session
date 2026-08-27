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

## TEST UTILISATEUR #86 — DÉFAUTS OBSERVÉS

#86 fonctionne réellement, IA/base prêtes, réponses plus rapides, mais les captures montrent des défauts de ciblage : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, liste des cadrans dans `L4ONGLET APERCU`, ainsi que d'anciennes fuites `<think>`/consignes internes.

## LOT QUALITÉ #88 — `5e707530...`

Diff validé avant push : exactement 3 fichiers IA, aucun protocole/UI/RAVE/packaging/32 bits/BUILD.

Corrections incluses :
- normalisation étroite `C4EST`→`C EST`, `L4ONGLET`→`L ONGLET`, sans conversion globale du chiffre 4 ;
- intentions `General/WireColor/Pinout` avant recherche experte ;
- une question de couleur de fil ne peut plus être satisfaite par une procédure de dépose ou un fait sans information couleur/fil ;
- si aucune couleur vérifiée n'existe, l'IA le dit sans en inventer ;
- une demande de brochage exige un fait câblage/connecteur/broche ;
- `ECU 1.3` / `OBD 1.9` peuvent fixer le contexte famille ;
- la définition générique MEMS 1.x ne détourne plus une demande de broche ;
- `TPM`→`RPM` uniquement en contexte cadran/régime ;
- `quel cadran dans l'onglet Aperçu` répond depuis le code réel : 11 cadrans + indicateur état système ;
- self-test exact avec les formulations utilisateur.

### Validation #88 complète

Compilation, protections protocole, tests déterministes, base r20/RAVE 1680, modèle Qwen, vrai `LocalAiClient` natif et packagé, package, smoke launch, manifeste et artefact : **SUCCESS**. Le self-test natif prend 7 s, le packagé 8 s.

## RAVE / BASE — LOT 1680 VALIDÉ

- 1660 : faits service RAVE ; 1670 : RCL0194 Mini MPi wiring ; 1680 : RCL0194ENG SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`, pages 20.3/20.4/39.3.
- 1680 ajoute 14 faits constructeur + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur`.
- Brochages vérifiés : O2, pompe, injecteur, purge, IAC, ECT, TPS, IAT, masse capteurs, prise diagnostic, etc.

## RAVE 1690 — OBJECTIF EN COURS AVANT MODIFICATION

Une recherche dédiée aux couleurs de fils a été lancée après les captures #86.

### Source primaire retrouvée
- **Rover Technical Communication — MINI Electrical Circuit Diagrams, publication RCL0194ENG, 3rd Edition**.
- Périmètre du schéma retenu : **SPi (Japan), MINI 97 Model Year from VIN `SAXXNNAXKBD 134455`**.
- La page de légende `COLOUR CODES / CODES DES COULEURS DES FILS` donne notamment : B=noir, G=vert, K=rose, LG=vert clair, N=brun, O=orange, P=violet, R=rouge, S=gris ardoise, U=bleu, W=blanc, Y=jaune.
- Page **20.4** associe directement code couleur + broche + fonction pour plusieurs signaux ; ces associations sont assez précises pour `verifie_constructeur`.

### Faits candidats 1690 retenus uniquement lorsqu'ils sont directement lisibles
- `C159-10` / prise diagnostic `C549-2` : **WY = blanc/jaune**.
- `C159-15` / prise diagnostic `C549-3` : **BG = noir/vert**.
- `C159-33` / capteur ECT : **KG = rose/vert**.
- `C159-30` / masse capteurs : **KB = rose/noir**.
- `C159-8` / TPS : **YG = jaune/vert**.
- `C159-9` / TPS : **YP = jaune/violet**.
- `C159-16` / IAT : **GB = vert/noir**.

### Règle de prudence 1690
Ne pas déduire les autres couleurs simplement à partir de l'ordre du texte extrait du schéma. N'intégrer que les relations où le code couleur est visiblement rattaché au connecteur/fonction dans RCL0194 20.4. Ne pas généraliser ces couleurs aux MEMS 1.2/1.3/1.6 hors cette variante SPi Japon 97MY.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Construire et valider localement un lot documentaire RAVE 1690 contenant uniquement les 7 associations de couleur SPi Japon 97MY directement vérifiées dans RCL0194ENG 20.4, les miroiter dans la table experte, mettre à jour manifeste + audit de provenance, vérifier absence de doublons/conflits, puis pousser un seul commit documentaire sur `MEMSX64`. Aucun BUILD #31.**