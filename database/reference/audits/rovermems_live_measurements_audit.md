# Audit RoverMEMS / James Portman — mesures live MEMS 1.x

## Périmètre

Source structurée : `rovermems_james_portman_live_measurements`  
Niveau de vérification : `source_externe`  
Familles : MEMS 1.2 / 1.3 / 1.6 / 1.9

Sources auditées :
- `james-portman/rover-mems-documentation` commit `d028fba1af34aaee99f335f979819a2a72321207`, fichier `ECUs/1.x-ECU-all/diagnostics`, blob `3b94b1b57611c62d8df37a658a328400f89e07d7`
- `james-portman/rover-mems-agent` commit `b381af39b2031c80303c5262055aa0c0121dbdd4`, fichier `ecu-1x-shared.go`, blob `a923a72325bd5d8541917ac03f0e7a677243234c`
- contrôle de cohérence uniquement : `rosco.h` du projet, blob `4c39acadc2e5c37b4d208ea0459d10e90bc7a0a4`

Aucun fait RoverMEMS n'est promu automatiquement en `decoded_by_project`. Les mesures ECU réellement validées par le projet restent prioritaires.

## Résultat d'intégration

| Jeu structuré | Lignes |
|---|---:|
| Disponibilité des paquets par famille | 8 |
| Champs de mesure 0x7D / 0x80 | 57 |
| Bits DTC / états | 26 |
| Conflits explicites | 5 |
| Alias de champs inconnus | 16 |

Répartition des 57 champs :
- `documented` : 22
- `uncertain` : 13
- `unknown` : 18
- `conflict` : 4

## Compatibilité par famille

- MEMS 1.2 : `0x80` disponible ; `0x7D` explicitement marqué `unavailable_single_plug`.
- MEMS 1.3 : `0x80` + `0x7D`.
- MEMS 1.6 : `0x80` + `0x7D`.
- MEMS 1.9 : `0x80` + `0x7D`.
- Longueur des trames : variable selon génération ; le premier octet du payload indique la taille.

## Mesures principales structurées

### 0x80
RPM, températures liquide/ambiante/admission/carburant, MAP, batterie, tension TPS, état ralenti, park/neutral, deux octets de défauts, consigne ralenti, HotBDPos, IAC, déviation ralenti, avance, dwell bobine, plus les octets inconnus conservés.

Les mots 16 bits normaux de `0x80` documentés/implémentés ici (`RPM`, `idle deviation`, `dwell`) sont **high-byte puis low-byte** dans la trame. Cela ne doit pas être confondu avec le **little-endian Mode 4 RAM** validé séparément par le projet.

### 0x7D
Contact, angle papillon, états lambda, boucle ouverte/fermée, correction carburant, positions/erreurs de ralenti candidates, états/DTC, octets inconnus et candidat compteur vilebrequin.

`0x7D:0x0B` reste explicitement **incertain** (`long term trim? / injector dead time?`) et **n'est pas un temps d'injection total**.

## Conflits conservés, non masqués

1. `0x7D:0x06` lambda : documentation `0.5 mV/LSB`, agent `raw*5`.
2. `0x80:0x09` TPS : documentation `0.02 V/LSB`, agent `raw/200` (= `0.005 V/LSB`).
3. `0x80:0x0A` idle switch : documentation bit 4 ; l'agent masque `0x1000` sur un octet 8 bits.
4. `0x80:0x16` avance : documentation `raw*0.5 - 24°` ; code agent `raw/2` sans retrait de 24.
5. `0x80:0x17-0x18` dwell : l'échelle `0.002 ms/LSB` est documentée, mais le maximum écrit en microsecondes dans la source est incohérent avec un mot 16 bits.

Ces points sont enregistrés en `conflit_a_verifier`; le moteur expert ne devra pas choisir automatiquement une variante.

## DTC / états

26 définitions de bits sont conservées séparément des mesures scalaires, notamment :
- chauffe lambda, synchro vilebrequin/arbre à cames, ventilateurs ;
- défauts température, MAP, TPS, pompe, purge, boost, knock ;
- drivers injecteurs, cruise, hill assist et ventilation compartiment moteur ;
- idle switch fermé sur `0x80:0x0A bit 4`.

## Relation avec le code actuel

Le `rosco.h` existant contient déjà les structures brutes `mems_data_frame_7d` et `mems_data_frame_80`. Le lot 1610 ne modifie **aucun code protocolaire** : il ajoute à la base les offsets, sens, conversions proposées, incertitudes et conflits avec provenance.

## Règles pour le moteur expert

- Les lignes `source_externe` peuvent soutenir une hypothèse ou proposer un contrôle, jamais à elles seules une conclusion forte.
- Les champs `unknown` restent exploitables comme valeurs brutes pour corrélation future mais n'ont aucune sémantique inventée.
- Les champs `uncertain` doivent conserver leur point d'interrogation logique.
- Les champs en `conflit_a_verifier` ne doivent pas être utilisés comme conversion certaine avant recoupement projet/ECU.
- Ne jamais assimiler les offsets `0x7D/0x80` aux adresses RAM Mode 4.
