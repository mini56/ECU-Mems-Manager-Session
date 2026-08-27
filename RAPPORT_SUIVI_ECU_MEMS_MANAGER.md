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
- Branche active : **`MEMSX64`**.
- HEAD x64 courant : **`fab2e4cfddb6507345049c915e2eaa4f7f583a8a`**.
- BUILD logiciel : **#30 / v1.0.30**. Aucun BUILD #31 sans demande explicite.
- Run RAVE 1700 : **#90 — SUCCESS**, run **`33044945315`**, commit `fab2e4cf`.
- Artefact #90 : ID **`9635406628`**, taille **386 768 840** octets, SHA-256 **`125e27658bba577efdf5d22a7cb3fa26670cddadd8383e8f4e7d907d765f6d6d`**.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## RAVE 1690 — VALIDÉ DANS #89

Source : Rover `RCL0194ENG`, SPi Japan 97MY from VIN `SAXXNNAXKBD 134455`, page 20.4 + légende couleurs.

Faits couleur : diagnostic WY/BG, ECT KG, masse capteurs KB, TPS YG/YP, IAT GB.

Artefact #89 inspecté directement : **67 faits RAVE / 79 faits experts**, dont 7 faits 1690 + 7 miroirs ; SQLite integrity `ok`, user_version 20.

## RAVE 1700 — #90 VALIDÉ DIRECTEMENT DANS L’ARTEFACT

Source identique, page 20.4.

8 faits couleur : injecteur SU/YN, purge NK/BW, IAC phases OS/OU/OG/KU.

Validation locale avant push : **8 faits + 8 miroirs**, integrity `ok`, r20 inchangé.

Validation réelle après build :
- #90 SUCCESS de bout en bout : compilation, tests déterministes, base r20, Qwen, LocalAiClient natif et packagé, package, smoke launch, manifeste, upload ;
- artefact #90 téléchargé par l’assistant et base `database/expert/ia_mems_reference_r20.sqlite` ouverte directement ;
- total `mems_rave_fact` : **75** ;
- total `mems_expert_fact_external` : **87** ;
- faits 1700 `RAVE-COLOR-SPIJ-008` à `RAVE-COLOR-SPIJ-015` : **8 RAVE + 8 miroirs experts** ;
- tous les faits 1700 : **`verifie_constructeur`** ;
- variante unique 1700 : `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455` ;
- `PRAGMA integrity_check` : **ok** ;
- `PRAGMA user_version` : **20**.

- SQL 1700 : 9917 octets, SHA-256 `29987cea579fbd7987e19eaf7655e3634df76353a3b42d07f09cb9977a0768a9`.
- qz64 1700 : 1461 octets, SHA-256 `d3469e14460d5fdea3cfa4a0f6ae84d92bb69dcbb1bf98bcba1a2c301f0cfe06`.

## RAVE 1710 — BRANCHE TEMPORAIRE PRÊTE

Source primaire : PDF Rover complet `RCL0194ENG`, page **20.3**, SPi Japan 97MY.

11 faits constructeur :
1. C159-4 MAIN RELAY CONTROL : WK = blanc/rose.
2. C159-35 DUAL PRESSURE SWITCH : GW = vert/blanc.
3. C159-36 OXYGEN SENSOR RELAY : BG = noir/vert.
4. C159-14 AUTOMATIC INHIBITOR : WLG = blanc/vert clair.
5. C159-29 SCREEN GROUND sonde oxygène : B = noir.
6. C159-7 OXYGEN SENSOR +VE : S = gris.
7. C159-18 OXYGEN SENSOR -VE : LGS = vert clair/gris.
8. C159-32 CKP -VE, **boîte manuelle** : WU = blanc/bleu.
9. C159-31 CKP +VE, **boîte manuelle** : UP = bleu/violet.
10. C159-20 FUEL PUMP RELAY : BP = noir/violet.
11. C159-11 IGNITION SENSE : W = blanc.

Validation locale : +11 RAVE +11 experts, integrity `ok`, user_version20, tous `verifie_constructeur`. Après 1700 + 1710 attendu : **86 RAVE / 98 experts**.

- SQL 1710 : 12903 octets, SHA-256 `a1d1283c0f38a1b3f65994abd7f8d2ce5c34d9e1f0798c447c5862114316c6de`.
- qz64 1710 : 1945 octets, SHA-256 `fa9ec2dbdf2178c30f47a1f6cc356c97035c46b780d1e5adced765335af412dc`.
- Branche `tmp-rave-1710`, HEAD **`897b51c8382513e15f236c18446d1cffc2352c31`**.
- Comparaison avec `MEMSX64` : ahead 3, behind 0, exactement trois fichiers : payload 1710, audit 1710, ajout du batch 1710 au manifeste.
- Aucun code IA, protocole, UI, packaging, 32 bits ou BUILD modifié.

## RAVE 1720 — RECHERCHE SANS MODIFICATION

Candidats à confirmer dans RCL0194 avant intégration : signal jauge température C159-5 GU, bobine C159-25 WB / C161-18 WS, ligne A/C C159-19 RW. N’intégrer qu’après preuve exacte fonction + connecteur + variante ; ne pas inférer C161 ni la sémantique A/C depuis une source secondaire.

## SÉCURITÉ À PRÉSERVER

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé. 32 bits inchangé.

## PROCHAINE ACTION EXACTE

**#90 est désormais vérifié directement. Fast-forward `MEMSX64` sur `tmp-rave-1710` HEAD `897b51c8382513e15f236c18446d1cffc2352c31`, identifier #91 et suivre jusqu’au package. Ensuite vérifier directement la base de l’artefact #91 (86 RAVE / 98 experts, 11 faits 1710 + 11 miroirs) et poursuivre RAVE 1720.**