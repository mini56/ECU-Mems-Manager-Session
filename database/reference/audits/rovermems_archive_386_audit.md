# Audit exact du ZIP RoverMEMS 386 fourni par l’utilisateur

## Archive auditée

- Fichier : `rover-mems-386(1).zip`
- SHA-256 : `8f1272530c2f850461d270ab41dfdfd5ddea9bc0469782eea0d6bc2d1b8aba7a`
- Entrées ZIP : **29**
- Fichiers : **27**
- Taille décompressée totale : **10 295 700 octets**
- Interface incluse : **App version 1.2.0**
- Exécutable : `rover-mems_386.exe`
- SHA-256 EXE : `18bf3d4b1eea047d785f50fae9c1b2ffdcec8a1b19f38e2300d94c9b340e2fd9`
- Taille EXE : **7 151 616 octets**
- Timestamp ZIP EXE : **14/03/2021 17:42:54**
- Chaîne imprimable trouvée dans l’EXE : `1.2.015`

Niveau de vérification de la source : `source_externe`.

## Identification de la génération du code

L’archive correspond très fortement à `james-portman/rover-mems-agent` au commit
`bf48bac0d43dd11894f22213b6cb69dbd4884e0c` du **14/03/2021 17:42:47 UTC**.

Preuve utilisée :
- **11 fichiers texte `web-static`** du ZIP donnent exactement le même Git blob SHA-1 que ce commit après normalisation CRLF → LF ;
- le timestamp de l’EXE du ZIP est seulement **7 secondes** après le commit ;
- le parseur de mesures associé est `ecu-1x-shared.go`, blob `792eb8f4eb5603ef8121a2972f46dd32ce3c0b97`.

C’est une **preuve forte de génération/source**, mais pas une preuve cryptographique que chaque octet de l’EXE a été compilé depuis chaque fichier de ce commit.

## Mesures live intégrées depuis la génération exacte du ZIP

Lot : `research_enrichment_1620.qz64`

| Données structurées | Lignes |
|---|---:|
| Snapshot archive | 1 |
| Fichiers de l’archive | 27 |
| Fichiers web recoupés exactement avec le commit | 11 |
| Mesures scalaires `0x80` / `0x7D` | **31** |
| Bits d’état / défauts | **24** |
| Octets explicitement inconnus | **19** |
| Différences de révision tracées | 4 |

### Trame `0x80`

La génération exacte du ZIP expose notamment :
RPM, température liquide, température ambiante candidate, température admission,
température carburant candidate, MAP, tension batterie, tension TPS, état ralenti,
park/neutral, consigne ralenti, HotDB candidate, position IAC, déviation ralenti,
offset avance candidat, **avance brute**, et dwell bobine.

Les mots 16 bits `RPM`, déviation ralenti et dwell sont décodés **high-byte puis low-byte**.

### Trame `0x7D`

La génération exacte du ZIP expose notamment :
contact, angle papillon, A/F candidate, états lambda, lambda, boucle fermée,
`long_term_trim`, `short_term_trim_percent`, purge canister candidate,
synchro primaire, position de base ralenti candidate, erreur ralenti candidate,
drivers injecteurs et compteur vilebrequin candidat.

`0x7D:0x0B` reste `long_term_trim` dans cette génération et **ne doit pas être utilisé comme temps d’injection total**.

## Différence importante par rapport au code RoverMEMS plus récent utilisé lors du premier audit 1610

Dans **ton ZIP de 2021**, `0x80:0x16` est seulement publié sous :
`ignition_advance_raw = raw`.

Le **25/05/2022**, le commit `befb6e6ef5332deb96885ef32b7fb80f1b24baef`
a ajouté une nouvelle sortie calculée :
`ignition_advance = raw/2`.

Cette conversion postérieure n’est donc **pas attribuée à ton archive**.
Elle reste séparée dans la base comme évolution ultérieure.

## Conflits et incertitudes conservés

- `0x80:0x09` TPS : l’implémentation fait `raw/200`, alors que le commentaire adjacent annonce `0.02 V/LSB`.
- `0x80:0x0A` idle switch : le code masque `0x1000` sur un octet 8 bits ; le commentaire indique bit 4.
- `0x7D:0x06` lambda : la génération exacte du ZIP utilise `raw*5`; la documentation Rover externe utilisée dans `1610` fournit une échelle contradictoire.
- `0x80:0x16` avance : la génération du ZIP ne fournit qu’une valeur brute ; son commentaire propose une conversion candidate différente.
- Les **19 octets inconnus** restent inconnus : aucune signification n’est inventée.

## Relation avec le lot 1610

`1610` = synthèse RoverMEMS/documentation avec provenance externe et conflits.

`1620` = **snapshot exact de l’archive utilisateur de mars 2021**, avec empreintes et comportement du parseur correspondant.

`1620` **n’écrase pas** `1610`. Le futur moteur expert pourra choisir la provenance, la date et le niveau de confiance.

Aucun code protocolaire ou UI n’est modifié par ce lot.
`ui-rebuild` ne doit pas être touchée.
