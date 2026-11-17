# Recherche approfondie des trames ECU — 1.6, 1.9, 2J et 3
Date : 17 novembre 2026. **Étude des sources seulement**, aucune compilation, écriture ECU ni modification du programme.

## Sources examinées directement

- Colin Bourassa, MEMS 1.6, tableaux de commandes, champs et offsets : https://colinbourassa.github.io/car_stuff/mems_interface/
- RoverMEMS, protocole 1.3/1.6/1.9 : https://rovermems.com/diagnostics/technical/
- RoverMEMS, réveil 1.9 : https://rovermems.com/mems-1.9/index.html
- RoverMEMS, protocole 2J : https://rovermems.com/mems-2j/index.html
- RoverMEMS, protocole 3 : https://rovermems.com/mems-3/index.html
- Code applicatif source tiers : https://github.com/james-portman/rover-mems-agent (fichiers `ecu-2j.go`, `ecu-2j-parse.go`, `ecu-2j-faults.go`, `ecu-3.go`)
- Pscan diagnostic MEMS 1.6 : https://pscan.uk/features/mems16.html ; MEMS 3 : https://pscan.uk/features/mems3.html
- **Comparaison du vrai dépôt SESSION** sur branche de recherche : `rosco.h`, `memsinterface_dispatch.cpp`, `mems19testtab.h`.

## 1. MEMS 1.6 : cartographie fine des trames ROSCO

Initialisation source : `CA 75 D0`, réponse écho avec 4 octets d'identité. Série TTL 5 V, 9600 8N1 avec RX/TX distincts. Attention au câblage physique, différent d'une K-Line ordinaire.

### Trame `0x80` (offset relatif au premier octet de longueur, sans écho)

| Offset | Donnée | Conversion selon sources | Statut |
|---|---|---|---|
| 01–02 | Régime | entier 16 bits big-endian, tr/min | SOURCES_CONVERGENTES |
| 03 | Température eau | brut - 55 °C avec particularité overflow 8 bits | A_VALIDER_SUR_CAPTURES |
| 04 | Ambiante calculée | même représentation | VARIANTE_CAPTEUR |
| 05 | Température admission | même représentation | SOURCES_CONVERGENTES |
| 06 | Température carburant | non équipée sur Mini SPI, souvent FF | VARIANTE_CAPTEUR |
| 07 | MAP | kPa, 1 unité brute par kPa | SOURCES_CONVERGENTES |
| 08 | Batterie | brut / 10 V | SOURCES_CONVERGENTES |
| 09 | Tension potentiomètre papillon | brut * 0,02 V | SOURCES_CONVERGENTES |
| 0A | Contact ralenti | bit 4 d'après Colin Bourassa | VARIANTE |
| 0C | Park/neutral | état logique ; autre interprétation possible A/C | CONFLIT_A_VERIFIER |
| 0D–0E | Drapeaux de défaut | masques selon variantes SPI/MPI | CONFLIT_A_VERIFIER |
| 0F | Consigne ralenti | brut * 6,1 tr/min, **seulement source RoverMEMS** | A_VALIDER |
| 10 | Position ralenti chaud | unité/échelle à préciser | A_VALIDER |
| 12 | Position IAC | pas, pour Mini SPI 0–180 | VARIANTE |
| 13–14 | Erreur/écart régime ralenti | 16 bits ; signe à confirmer | A_VALIDER |
| 16 | Avance | brut * 0,5 - 24 degrés (source Colin) | A_VALIDER_VARIANTE |
| 17–18 | Dwell bobine | brut 16 bits * 0,002 ms | SOURCES_CONVERGENTES |

### Trame `0x7D`

| Offset | Donnée | Réserve |
|---|---|---|
| 02 | angle papillon | Colin : angle? ; RoverMEMS : brut/2 ; note de recherche plus ancienne : brut*0,6. **CONFLIT SUR FACTEUR, à ne pas changer sans capture/firmware** |
| 04 | rapport air/carburant supposé | souvent FF constant, ne pas afficher comme mesure fiable |
| 06 | tension lambda | Colin/RoverMEMS `0,5 mV par unité` ; l'ancien journal `ETAT_RECHERCHE.md` retient `5 mV` : **CONFLIT CRITIQUE**, facteur 10 |
| 07–09 | activation/fréquence/duty/statut lambda | interprétation partiellement incertaine |
| 0A | boucle fermée | zéro = ouverte, non-zéro = fermée |
| 0B–0C | adaptations/richesse | libellé et sens exact incertains selon firmware |
| 0D | purge canister | hypothèse de duty cycle |
| 0F | position de base ralenti | source RoverMEMS |
| 14 | erreur ralenti? | A_VALIDER |
| 05,0E,11,16 | bits de défaut | tables divergentes ; ne pas traiter comme universelles |

### Comparaison réelle avec SESSION

- `rosco.h` **contient déjà** la structure `mems_data_frame_80` avec régime, température, MAP, batterie, papillon, contact ralenti, défauts, IAC, correction ralenti, avance, temps bobine.
- `rosco.h` **contient déjà** `mems_data_frame_7d` avec lambda, trims, position ralenti et erreurs. Il s'agit d'un inventaire et d'un contrôle de précision, **pas de nouveaux champs à ajouter automatiquement**.
- Les commandes `0x7D`, `0x80`, `0xCC`, `0xF4`, `0xFB`, `0x0F`, `0xFA` sont **déjà définies** dans `rosco.h`. Les commandes d'actionneurs sont également déjà présentes. Les commandes mutantes ne font pas partie d'une campagne de lecture.
- **Point non trivial** : la fonction `MEMSInterface::connectToECU()` positionne `MemsEcuFamily::Unknown` après succès du chemin ROSCO, tant qu'aucune preuve d'identité 1.3/1.6 n'est établie. Il faut étudier l'identification précise **sans présumer que tout ROSCO = 1.6**.

## 2. MEMS 1.9 — à confronter à l'existant, pas à réinventer

- Réveil ISO9141 à **5 bauds**, adresse `16`, puis 9600 8N1, echo K-Line et initialisation ROSCO `CA 75 D0`. `memsinterface_dispatch.cpp` implémente déjà un réveil et effectue un essai ROSCO après succès.
- Même famille de demandes `7D`/`80` et identifiants `D0`/`D1`, état sécurité `D2`; taille de réponse explicitement **variable**.
- `mems19testtab.h` réalise des demandes D1, D2, F0 ; aucun second pilote parallèle n'est justifié.
- Les champs d'injection RAM dépendent de la cartographie exacte D1. Ne pas substituer les données de polling `7D`/`80` aux données d'injection.
- Nouveau contrôle à approfondir : à quel moment et sur quelle preuve technique attribuer `Mems19` vs génération historique et comment éviter tout envoi prématuré d'initialisation 1.9 sur un autre ECU.

## 3. MEMS 2J — inventaire enrichi par `ecu-2j-parse.go`

Transport : K-Line 10400 8N1, initialisation rapide 25 ms/25 ms, message unique `81 13 F7 81 0C`, réponse documentée `03 C1 D5 8F 28`, puis paquets KWP2000, contrôle par **somme modulo 256**. Présence d'un écho à éliminer.

| Requête payload | Donnée (réponse `61 xx`) | Conversion source Go | Réserve |
|---|---|---|---|
| `21 01` | eau | (U16 - 2732)/10 °C | source Go |
| `21 02` | huile | même formule | équipement variable |
| `21 03` | air | même formule | source Go |
| `21 05` | température carburant? | U16 brut | **unité non confirmée** |
| `21 07` | MAP | U16 /100 kPa | source Go |
| `21 08` | papillon | U16 /100 degrés | source Go |
| `21 09` | régime | U16 tr/min | source Go |
| `21 0A` | correction essence / lambda | pourcentage U16/100 ; millivolts U16 | estimation AFR du Go non validée |
| `21 0B` | temps de charge bobines 1 et 2 | deux octets /1000, unité source ambiguë | **A_VALIDER** |
| `21 0C` | quatre injecteurs | 4 octets bruts | **pas de conversion ms vérifiée** |
| `21 0D` | vitesse véhicule | brut | unités à confirmer |
| `21 0F` | états papillon/ignition/clim | masques bit 0,1,3 | source Go |
| `21 10` | batterie | octets 4–5, U16/1000 V | peut être refusée sur certaines Mini MPI |
| `21 11` | synchronisation déclencheurs primaire/secondaire | bits 0/1 (0 signifie OK selon source) | à recouper ECU réel |
| `21 12` | position ralenti | U16/2 | unité à confirmer |
| `21 13` | boucle lambda fermée | bit 0 | source Go |
| `21 19` | défauts | tableau de bits multiples | **conflit sur même bit** consigné dans l'audit 04 |
| `21 21` | écart régime | U16 avec signe | conversion signée du Go probablement erronée à -65535 ; à corriger dans futur décodeur, pas dans SESSION présent |
| `21 25` | déphasage arbre à cames | U16 brut | **rejeté sur Mini MPI** d'après source |
| `21 3A` | adaptation ralenti/avance | offsets/décodage à analyser | source Go, non validée |

**Information nouvelle** par rapport au tableau de recherche 03–06 : `21 0D`, `21 0F`, `21 11`, `21 13` et `21 3A`. Ne pas croire que toutes les requêtes sont supportées par toutes les variantes 2J.

## 4. MEMS 3 — inventaire des offsets du parseur tiers

Transport étudié : 9600 8E1, K-Line, en-tête requête `B8 13 F7`, réponse `13 B8 F7`, longueur et contrôle en fonction des trames. Pscan signale les modes **BMW** et **ISO**, BMW plus fiable dans ses essais.

| Requête | Champs du payload positif | Conversion relevée |
|---|---|---|
| `21 00` | eau octets 2–3, huile 6–7, admission 10–11 ; valeurs d'entrée capteurs intercalées | (U16 - 2730) /10 °C |
| `21 06` | MAP 2–3, signal MAP 4–5, tension papillon 8–9 mV, régime 10–11 | MAP U16/100 kPa ; régime U16 |
| `21 0A` | feedback carburant 2–3 ; lambda avant catalyseur 4–5 ; données post-catalyseur à confirmer | correction U16/100 % ; lambda U16 mV |
| `21 0B` | temps charge bobines 1 et 2, octets 2–3 et 4–5 | U16, µs d'après libellé du Go |
| `21 21` | déviation régime, octets 2–3 | U16 brut, signe et unité à confirmer |
| `18 00 00 00` | défauts `58` puis groupes (identifiant 16 bits + statut) | code/description par table, variations possibles |

**Réserve supplémentaire** : le parseur source Go contient plusieurs manipulations sur octets non préalablement convertis en entier large. Ne pas reproduire mot pour mot les calculs : tests de limites et conversions U16 indépendantes indispensables.

## 5. Politique d'implémentation future — pas de compilation maintenant

1. Déterminer le calculateur d'après **une preuve d'identification**, non d'après un nom commercial de câble.
2. Respecter les voies électriques et les initialisations propres au 1.6, 1.9, 2J et 3.
3. Décoder d'abord les paquets bruts avec **tests hors ligne sur trames de référence**, validation des longueurs, échos, checksum, échecs `7F`, offsets et mesures manquantes.
4. Maintenir séparés : *information publiée dans une source*, *champ présent dans SESSION*, *unité vérifiée*, *test matériel réussi*.
5. S'interdire toute écriture ou effacement accidentel, les actions d'antidémarrage restant hors lecture diagnostic.
6. Les projets externes RoverMEMS et autres logiciels ont leurs licences : recopier uniquement des faits protocolaires vérifiables, pas le code sous licence.
