# Archive recherches MEMS — build #500 / v1.5.0

Cette archive accompagne `database/reference/research_enrichment_500.qz64`. Les lignes SQL sont chargées en complément du lot #499 ; elles ne remplacent pas les données déjà présentes.

## Règles de validation

- `verifie_constructeur` : documentation constructeur identifiable.
- `recoupee` : information soutenue par plusieurs éléments techniques cohérents ou une bibliothèque firmware fortement documentée.
- `source_externe` : source technique identifiable mais pas encore recoupée au niveau constructeur.
- `non_verifie` : donnée explicitement incertaine ou marquée Unconfirmed.
- `conflit_a_verifier` : sources contradictoires ; aucune valeur n'est forcée.
- MEMS FCR n'est jamais utilisé comme source finale d'une valeur sans source d'origine vérifiable.
- Les couleurs de fils et brochages restent spécifiques au véhicule/faisceau ; pas de transposition automatique d'un modèle à l'autre.

## Volume du lot #500

117 nouvelles lignes actives :

- 14 sources documentées ;
- 13 faits protocole / architecture ;
- 16 capacités par génération ;
- 62 affectations véhicule / calibration / référence ECU ;
- 10 références de diagnostic ou valeurs atelier ;
- 2 conflits documentés.

Le moteur de recherche global indexe automatiquement toutes les tables et toutes leurs colonnes, donc ces données deviennent recherchables après reconstruction du package de référence.

## Sources principales archivées

### Andrew Revill — familles MEMS

Source : `https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm`

Données retenues : capacités des MEMS 1.2/1.3/1.6/1.9, lecture/sauvegarde ROM, distinction des transports, et brochage diagnostic 3 broches pour les familles anciennes : masse signal ECU 14, ECU TX 10, ECU RX 15.

### Andrew Revill — MEMS 1.9 architecture

Source : `https://andrewrevill.co.uk/MEMSMapperMems19.htm`

Données retenues : Intel AN87C196KD, mots multi-octets little-endian dans le firmware, tables 8 bits, espace ROM monolithique 32 KiB, particularités par rapport aux MEMS 1.6 plus anciens, possibilité de lire la ROM stock mais pas de l'écrire normalement sans modification matérielle.

### Bibliothèques firmware Andrew Revill

- MEMS 1.3 : `https://andrewrevill.co.uk/MapFirmwareLibrary/Rover%20MEMS%201.3/`
- MEMS 1.6 atmosphérique : `https://andrewrevill.co.uk/MapFirmwareLibrary/Rover%20MEMS%201.6/NA/`
- MEMS 1.6 turbo : `https://andrewrevill.co.uk/MapFirmwareLibrary/Rover%20MEMS%201.6/Turbo/`
- MEMS 1.9 : `https://andrewrevill.co.uk/MapFirmwareLibrary/Rover%20MEMS%201.9/`

Les noms de fichiers permettent de relier véhicule, moteur, calibration et référence ECU. Le lot #500 archive notamment des applications Mini, Metro/Rover 100, Rover 200/400/600/800, MGF, Freelander et Caterham.

Conflit conservé : `MNE10027` apparaît dans la bibliothèque MEMS 1.3 comme Mini 1300 calibration `ABEMR002`, alors que certaines classifications externes le donnent autrement. Il reste `conflit_a_verifier` et n'est pas forcé en MEMS 1.2.

### Original Technical Publications

Source : `https://store.otpubs.com/mini-1959-to-2000/`

Références constructeur archivées comme cibles documentaires :

- `AKM6799` — Mini Single Point Injection 1991–1996 ;
- `AKM7169` — Mini All Models 1993 on Service Manual ;
- `RCL0193ENG` — supplément 1997–2000 ;
- `RCL0424ENG` — electrical library 1997–2000.

### Rover Group AKM7169FRE

Valeurs Mini SPi archivées avec portée spécifique au véhicule : pression carburant 1,0 bar, chute maximale 0,7 bar pendant la première minute du contrôle, purge canister sous conditions température/régime/MAP documentées, résistance primaire bobine 0,71–0,81 ohm à 20 °C, description de cartographie richesse 10 régimes × 8 densités.

### Rimmer Bros

Catalogues utilisés uniquement pour les correspondances pièce/application/VIN. Lorsqu'une génération MEMS n'est pas indépendamment recoupée, le champ `mems_version` reste `A_DETERMINER`. Les métadonnées d'année manifestement incohérentes ne sont pas reprises comme vérité.

### MGF MEMS 1.9

La documentation miroir `https://www.mgfcar.de/mems1.9/mems1.9.htm` fournit des informations détaillées sur le connecteur C0159 36 broches et plusieurs stratégies de fonctionnement. Elles restent `source_externe` tant qu'elles ne sont pas recoupées page par page avec la documentation Rover d'origine.

## Attribution ECU MEMS Manager

Les fonctions déjà présentes dans ECU MEMS Manager ne sont pas réattribuées à des projets externes. En particulier, le décodage du champ 7D octets 14–15 / `uk10` et sa correction de ralenti chaud restent référencés ECU MEMS Manager.

Les dépôts `james-portman/rover-mems-agent` et `andrewdjackson/librosco` servent principalement de recoupement lorsque leurs commandes correspondent déjà à celles du programme.
