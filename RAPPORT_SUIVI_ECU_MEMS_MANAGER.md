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
- Run RAVE 1700 : **#90**, run **`33044945315`**, en cours.
- Dernier run validé : **#89 — SUCCESS**, commit `84e677a8`.
- Artefact #89 : ID `9635185747`, taille `386 764 960`, SHA-256 `1923f090fd3fdf6f13dcf9a089cc9588ac10c0f67605ef1bcfed6825b21b154c`.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## RAVE 1690 — VALIDÉ DANS #89

Source : Rover `RCL0194ENG`, SPi Japan 97MY from VIN `SAXXNNAXKBD 134455`, page 20.4 + légende couleurs.

Faits couleur : diagnostic WY/BG, ECT KG, masse capteurs KB, TPS YG/YP, IAT GB.

Artefact #89 inspecté directement : **67 faits RAVE / 79 faits experts**, dont 7 faits 1690 + 7 miroirs ; SQLite integrity `ok`, user_version 20.

## RAVE 1700 — POUSSÉ, RUN #90 EN COURS

Source identique, page 20.4.

8 faits couleur : injecteur SU/YN, purge NK/BW, IAC phases OS/OU/OG/KU.

Validation locale : **8 faits + 8 miroirs**, total prévu après 1700 **75 RAVE / 87 experts**, integrity `ok`, r20 inchangé.

- SQL : 9917 octets, SHA-256 `29987cea579fbd7987e19eaf7655e3634df76353a3b42d07f09cb9977a0768a9`.
- qz64 : 1461 octets, SHA-256 `d3469e14460d5fdea3cfa4a0f6ae84d92bb69dcbb1bf98bcba1a2c301f0cfe06`.
- Push `MEMSX64` : **`fab2e4cfddb6507345049c915e2eaa4f7f583a8a`**.
- Diff : exactement payload 1700 + audit 1700 + manifeste ; aucun code IA/protocole/UI/packaging/32 bits.

## RAVE 1710 — VALIDÉ LOCALEMENT AVANT BRANCHE

Source primaire vérifiée visuellement : PDF Rover complet `RCL0194ENG`, page **20.3**, SPi Japan 97MY. Les 11 associations suivantes sont directement lisibles sur le schéma et la légende Rover :

1. C159-4 MAIN RELAY CONTROL : **WK = blanc/rose**.
2. C159-35 DUAL PRESSURE SWITCH : **GW = vert/blanc**.
3. C159-36 OXYGEN SENSOR RELAY : **BG = noir/vert**.
4. C159-14 AUTOMATIC INHIBITOR : **WLG = blanc/vert clair**.
5. C159-29 SCREEN GROUND sonde oxygène : **B = noir**.
6. C159-7 OXYGEN SENSOR +VE : **S = gris**.
7. C159-18 OXYGEN SENSOR -VE : **LGS = vert clair/gris**.
8. C159-32 CKP -VE, boîte manuelle : **WU = blanc/bleu**.
9. C159-31 CKP +VE, boîte manuelle : **UP = bleu/violet**.
10. C159-20 FUEL PUMP RELAY : **BP = noir/violet**.
11. C159-11 IGNITION SENSE : **W = blanc**.

Règle : ne pas mélanger les variantes CKP automatique/manuelle ; les deux faits CKP ci-dessus sont explicitement qualifiés **boîte manuelle**.

Validation sur la vraie base r20 de l'artefact #89 :
- **11 faits RAVE + 11 miroirs experts** ;
- total sur base #89 seule : **78 RAVE / 90 experts** ; après 1700 attendu : **86 RAVE / 98 experts** ;
- integrity `ok`, user_version 20 ;
- tous `verifie_constructeur`.
- SQL 1710 : **12903 octets**, SHA-256 **`a1d1283c0f38a1b3f65994abd7f8d2ce5c34d9e1f0798c447c5862114316c6de`**.
- qz64 1710 : **1945 octets**, SHA-256 **`fa9ec2dbdf2178c30f47a1f6cc356c97035c46b780d1e5adced765335af412dc`**.

## SÉCURITÉ À PRÉSERVER

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé. 32 bits inchangé.

## PROCHAINE ACTION EXACTE

**Créer `tmp-rave-1710` depuis le HEAD 1700, ajouter uniquement payload 1710 + audit + manifeste, comparer le diff. Ne fast-forward `MEMSX64` sur 1710 qu'après validation complète #90 et vérification de 75 RAVE / 87 experts dans son artefact. Ensuite poursuivre RAVE 1720.**