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
- GitHub Actions RAVE 1690 : **#89**, run **`33044364773`**, commit `84e677a8`, état au dernier contrôle : **in_progress**.
- Dernier GitHub Actions entièrement validé : **#88 — SUCCESS**, run `33042796844`.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## QUALITÉ IA #88 — VALIDÉE

#88 corrige le ciblage observé sur les captures utilisateur et valide compilation, base r20/RAVE 1680, Qwen, vrai `LocalAiClient` natif et packagé, package et smoke launch. Aucun changement protocole/UI/32 bits.

## RAVE 1690 — COULEURS DE FILS SPi JAPON 97MY — POUSSÉ

Source : Rover Technical Communication `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition, SPi Japan 97MY from VIN `SAXXNNAXKBD 134455`, page 20.4 + légende constructeur.

Faits ajoutés :
1. C549-2 ↔ C159-10 : WY = blanc/jaune.
2. C549-3 ↔ C159-15 : BG = noir/vert.
3. ECT C165-2 ↔ C159-33 : KG = rose/vert.
4. masse capteurs ↔ C159-30 : KB = rose/noir.
5. TPS C175-2 ↔ C159-8 : YG = jaune/vert.
6. TPS C175-3 ↔ C159-9 : YP = jaune/violet.
7. IAT C174-2 ↔ C159-16 : GB = vert/noir.

Validation locale : 7 faits + 7 miroirs, total prévu 67 RAVE / 79 experts, integrity `ok`, r20 inchangé. SQL SHA-256 `aef8a4c9d808267188c76c25c53dac254cba57f434a16badf7791023ffcfa97a`, qz64 SHA-256 `7288e46b99caf41e14751ab5690a7bf17fd35941e1e17527af472bf8bad3ec19`.

Push : `MEMSX64` fast-forward sur **`84e677a87067ae43054df7fc534d84a70b6908b7`**, exactement 3 fichiers documentaires/base. Run #89 déclenché.

## RAVE 1700 — VALIDATION LOCALE AVANT PUSH

Même source primaire et même variante SPi Japon 97MY, page 20.4. La légende Rover RCL 0145 est utilisée pour développer les codes couleur.

### 8 faits retenus
1. Injecteur C522-1 ↔ C159-1 : **SU = gris/bleu**.
2. Injecteur C522-2 ↔ C159-24 : **YN = jaune/brun**.
3. Purge C152-1 alimentation : **NK = brun/rose**.
4. Purge C152-2 ↔ C159-21 : **BW = noir/blanc**.
5. IAC phase 1 C177-1 ↔ C159-3 : **OS = orange/gris**.
6. IAC phase 4 C177-4 ↔ C159-27 : **OU = orange/bleu**.
7. IAC phase 3 C177-6 ↔ C159-22 : **OG = orange/vert**.
8. IAC phase 2 C177-3 ↔ C159-2 : **KU = rose/bleu**.

La bobine n'est pas intégrée dans 1700 : le texte extrait ne donne pas encore une association broche/couleur aussi sûre que pour les huit liaisons retenues.

### Validation locale
Le SQL 1700 a été appliqué après le lot 1690 sur une copie de la base r20.

- 8 faits RAVE + 8 miroirs experts.
- total RAVE : **67 → 75**.
- total expert : **79 → 87**.
- SQLite integrity : **ok**.
- user_version : **20**.
- tous `verifie_constructeur` ; variante unique SPi Japon 97MY.
- SQL : **9917 octets**, SHA-256 **`29987cea579fbd7987e19eaf7655e3634df76353a3b42d07f09cb9977a0768a9`**.
- qz64 : **1461 octets**, SHA-256 **`d3469e14460d5fdea3cfa4a0f6ae84d92bb69dcbb1bf98bcba1a2c301f0cfe06`**.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé. 32 bits inchangé. Aucun BUILD #31.

## PROCHAINE ACTION EXACTE

**Pendant que #89 termine, préparer `tmp-rave-1700` depuis le HEAD 1690 avec uniquement payload 1700 + audit + manifeste. Comparer le diff mais ne pousser `MEMSX64` qu'après verdict #89. Ensuite poursuivre RAVE 1710 sur d'autres circuits seulement si les liaisons sont directement vérifiables.**