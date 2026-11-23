# MEMS 1.3 / 1.6 — vérification des conversions lambda et temps bobine

**Étude du 23 novembre 2026. DOCUMENTATION UNIQUEMENT — aucune modification du runtime, des seuils, de la DLL, des packs de données ou du Setup.**  
Voir également [l'audit initial de Colin Bourassa](COLIN_BOURASSA_MEMS16_PROTOCOL_COMPARE_20261123.md) et [l'audit des diagnostics](AUDIT_DIAGNOSTIC_AUTOMATIQUE_2026-11-22.md).

## 1. Tension lambda : conflit de facteurs ×0,5 et ×5

**Champ :** commande 0x7D, position 0x06, **un octet non signé**, de 0 à 255.

| Source | Facteur | Observation |
|---|---:|---|
| [Colin Bourassa, table 4](https://colinbourassa.github.io/car_stuff/mems_interface/) | 0,5 mV/LSB | Documentation issue de quelques calculateurs MEMS 1.6 SPi |
| [RoverMEMS Technical info](https://rovermems.com/diagnostics/technical/) | 0,5 mV/LSB | Formulation très proche de Bourassa : ne pas la compter comme preuve indépendante |
| [MemsFCR diagnostics, 7dx06](https://memsfcr.co.uk/memsfcr-diagnostics/) | 5 mV/LSB | Décodage publié pour les MEMS 1.3/1.6 |
| `research/mems/lecture-ecu-19-2j-3/rovermems-reference/i386-20230128/ecu-1x-shared.go` | 5 mV/LSB | Code conservé : `globalDataOutput["lambda_mv"] = float32(data[6]) * 5` |
| [Bosch, documentation sonde oxygène](https://aa-bosch-ecat-ap.resource.bosch.com/sao_ecat/downloads/bosch%20australia%20oxygen%20sensor%20catalogue%202013.pdf) | Plage usuelle env. 100–900 mV | Confirme seulement l'ordre de grandeur d'une sonde zirconium à bande étroite, pas l'encodage MEMS |

### Test physique et mathématique

Un octet représente au maximum **255**. Avec `×0,5 mV`, la tension maximale représentable est **127,5 mV**, bien trop basse pour couvrir une sonde étroite chaude oscillant typiquement vers 100–900 mV. Avec `×5 mV`, la plage est **0–1275 mV**, compatible avec le signal.

| Brut reçu | Hypothèse ×0,5 | Hypothèse ×5 |
|---|---:|---:|
| 0x14 = 20 | 10 mV | 100 mV |
| 0x5A = 90 | 45 mV | 450 mV |
| 0xB4 = 180 | 90 mV | 900 mV |
| 0xFF = 255 | 127,5 mV | 1275 mV |

**Résultat : la conversion ×5 mV est fortement privilégiée sur les trames MEMS 1.x concernées, mais le conflit documentaire 0,5/5 doit rester explicite faute de mesures réelles appariées à l'ECU.** Aucune généralisation au 1.9, 2J, 3.

**État réel de Manager :** `librosco-x64/rosco.h` conserve `lambda_voltage` comme octet brut ; `diagnosticpanel.cpp` affiche ce brut dans `U=%1` sans multiplication par 5. La condition d'« anomalie lambda » y dépend de `(d.dtc2 & 0x04) || (d.dtc2 & 0x08)`, **pas de la valeur de tension convertie**. Les masques DTC restent à vérifier par famille avant tout diagnostic.

## 2. Bobine : conversion concordante, seuils de panne non démontrés

**Champ :** commande 0x80, positions 0x17 et 0x18, mot **16 bits big-endian**.  
Formule : `raw = (octet17 << 8) | octet18` ; `dwell_ms = raw × 0,002` (soit 2 µs par pas).

Les sources concordent :
- [Colin Bourassa, table 5](https://colinbourassa.github.io/car_stuff/mems_interface/) : **0,002 milliseconde/LSB**.
- [MemsFCR Diagnostics](https://memsfcr.co.uk/memsfcr-diagnostics/) : **valeur ×0,002** ; environ **2–3 ms** pour une alimentation d'environ **14 V** ; temps élevé pouvant orienter vers le circuit primaire.
- Code RoverMEMS Go archivé dans `ecu-1x-shared.go` : `coil_time_microseconds = coil_time * 2`, équivalent à ×0,002 ms.
- [Manuel Rover MEMS MPi/SPi — chapitre allumage, page 14•4](https://james-portman.github.io/rover-mems/ECU_Rover_MEMS.pdf) : durée généralement **environ 3,0 à 3,5 ms** avec régulation électronique, selon vitesse moteur, tension et configuration.

### Cas numériques de référence (non issus d'un ECU réel)

| Octets bruts | Décimal | Temps |
|---|---:|---:|
| 03 E8 | 1000 | 2,00 ms |
| 05 DC | 1500 | 3,00 ms |
| 06 D6 | 1750 | 3,50 ms |
| 07 D0 | 2000 | 4,00 ms |

**État réel de Manager, code vérifié :**
- `librosco-x64/protocol.c` reconstitue `coil_time` depuis les deux octets, sans conversion prématurée.
- `diagnosticpanel.cpp` et `overviewramlive.cpp` utilisent **déjà correctement `coil_time ×0,002`**.
- `DiagnosticPanel::rebuild` n'évalue le temps que **moteur tournant** et avec **batterie entre 13,5 et 14,5 V**. À ces conditions : **OK de 2,0 à 3,5 ms**, **surveillance sous 2 ms ou entre >3,5 et 4 ms**, **anomalie possible au-dessus de 4 ms**.
- Le programme **incrémente le compteur d'anomalies** si le temps dépasse 4 ms. C'est un indicateur logiciel, **pas un seuil constructeur certifié prouvant une bobine défaillante**. Les sources consultées n'établissent aucun seuil universel de panne à 4 ms ; elles décrivent surtout les durées usuelles. Plusieurs valeurs prises dans des conditions identifiées et les autres signes électriques sont nécessaires.
- La **résistance primaire 0,71–0,81 ohm à 20 °C** citée dans l'audit interne AKM7169 pour une variante ne doit jamais être confondue avec une durée exprimée en millisecondes.

**Résultat : formule ×0,002 ms confirmée ; aucune correction de conversion nécessaire dans Manager. La qualification « anomalie possible >4 ms » reste à valider et ne doit jamais être présentée comme « bobine défaillante ».**

## 3. Protocole de vérification sans changement du programme

1. Identifier la **génération, référence ECU et moteur** ; archiver les octets bruts des trames complètes 0x7D et 0x80.
2. Relever simultanément **0x7D:06**, état lambda et boucle fermée (**0x7D:09–0A**), **0x80:17–18**, régime (**0x80:01–02**), tension batterie (**0x80:08**) et température moteur.
3. **Lambda** : sur un moteur chaud et une sonde active, comparer la forme d'onde et l'amplitude obtenues par `brut ×5 mV` à une mesure indépendante fiable du même instant. Ne pas conclure sur un octet isolé ni sur une sonde froide.
4. **Bobine** : relever plusieurs points à tension stable et régimes variés ; confronter aux caractéristiques de **l'ECU précis**, aux défauts réellement lus et au circuit primaire. La seule durée ne prouve pas une panne.
5. Après corroboration, proposer séparément toute correction éventuelle de l'affichage de l'unité lambda et toute évolution des règles du diagnostic, avec tests 6 langues et non-régression.

**Statut documentaire au 23/11/2026 :** lambda **×5 très probable, divergence ouverte** ; bobine **×0,002 confirmé, code actuel cohérent** ; **>4 ms non validé comme limite de panne**. Aucun fichier exécutable, protocole, seuil ou pack de base de données modifié.
