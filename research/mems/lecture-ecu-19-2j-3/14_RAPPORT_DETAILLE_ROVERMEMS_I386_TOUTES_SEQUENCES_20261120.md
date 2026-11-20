# Rapport détaillé RoverMEMS Windows i386 — initialisation, dialogues, données et fonctions modifiant les calculateurs

**Établi le 20 novembre 2026** · **Projet : ECU MEMS Manager SESSION** · **Nature : documentation de recherche, aucune compilation ni modification applicative.**

## 1. Une référence précise, rattachée au programme téléchargé

Le ZIP `rover-mems-windows-i386.zip` de l'utilisateur contient `rover-mems-windows-i386.exe` (**10 240 000 octets**). L'inspection directe du binaire avec `go version -m` établit **Go 1.19.5**, architecture **Windows 32 bits (386)**, code source Git à la **révision `ed9ec4dd3181e60dcb7e4bcb340016fcb6bf20f3`**, et `vcs.modified=false` (28 janvier 2023).

- **SHA-256 ZIP :** `4283c2b10064246310314f0ab5cf4600b0977c932ffc64fbdba4c022f487824d`.
- **SHA-256 EXE :** `0b7ac753299ec7feae6b1e2793ce7c994313ee8ffa58371476c8f9db08e6fb87`.
- **Auteur / dépôt :** [James Portman — RoverMEMS Agent](https://github.com/james-portman/rover-mems-agent/tree/ed9ec4dd3181e60dcb7e4bcb340016fcb6bf20f3). **Licence : GNU GPLv3**.
- **Archives :** [référence complète de ce binaire](rovermems-reference/i386-20230128/) et [catalogue de trames exploitable par machine](15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json). Les **anciennes six sources déjà archivées** en `rovermems-reference/` sont issues d'une **autre révision** et ne sont pas écrasées.

**Précision :** les commandes ci-dessous sont **présentes dans le code Go**. Les réponses sont des réponses *attendues*, pas un enregistrement provenant d'un calculateur réel. L'absence d'ECU pendant les captures explique le temps d'attente et ne valide aucun dialogue.

## 2. Tous les systèmes trouvés

| Famille | Domaine | Paramètres série | Mode d'entrée en dialogue |
|---|---|---|---|
| **MEMS 1.2, 1.3, 1.6** | Moteur, protocole commun `1.x` | **9600, 8N1** | `CA` → `75` → `F4` → `D0` |
| **MEMS 1.9** | Moteur | **9600, 8N1**, et adresse `16` à 5 bauds | Réveil ISO9141 `55 76 83` / `7C E9`, puis 1.x |
| **MEMS 2J** | Moteur | **10400, 8N1** | BREAK rapide puis `81 13 F7 81 0C` |
| **MEMS 3** | Moteur | **9600, 8E1**, parité paire | Trame `B8 13 F7 02 1A 9A DE` |
| **RC5** | **Airbag/SRS (pas moteur)** | **2400, 8N1** | BREAK spécifique, réponse `55 06 3B` |

`8N1` = 8 bits, sans parité, 1 arrêt ; `8E1` = 8 bits, parité **paire**, 1 arrêt. Le fichier `main.go` de l'agent propose bien cinq chemins distincts : `1.x`, `1.9`, `2J`, `3`, `rc5`. **Les 1.2/1.3/1.6 partagent un même code ; il n'existe pas trois pilotes indépendants dans ce projet.**

## 3. MEMS 1.2 / 1.3 / 1.6 : connexion, échanges et mesures

Sources : `ecu-1x.go` et `ecu-1x-shared.go`.

| TX à émettre | Réponse attendue | Suite |
|---|---|---|
| `CA` | `CA` | `75` |
| `75` | `75` | `F4` |
| `F4` | `F4 00` | `D0` |
| `D0` | `D0` et **4 octets d'identification** | Entrée en lecture `80` |
| `80` | `80`, 1 octet de longueur puis données | Décoder, passer à `7D` |
| `7D` | `7D`, 1 octet de longueur puis données | Décoder, revenir à `80` |

Chaque commande de cette famille est **un octet envoyé séparément**. Le code Go sait éliminer **l'écho local K-Line** d'une commande ; ne pas compter cet écho comme une réponse du calculateur. Une remarque de l'auteur signale que certains **1.2 ne sauraient pas traiter `7D`** : à vérifier selon la version.

**Trame `80` :** régime, température liquide, température ambiante, air admis, carburant, pression MAP, tension batterie, potentiomètre papillon, interrupteur de ralenti, contact boîte/point mort, défauts, consigne et position ralenti, décalage régime, avance, temps de bobine.

**Trame `7D` :** contact, angle papillon, rapport air/carburant, relais chauffage lambda, synchronisation et commande de ventilateurs, tension et état lambda, boucle fermée, corrections carburant, purge canister, position de ralenti, états injecteurs et compteurs. Plusieurs échelles/positions sont **incertaines dans le Go** ; conserver les conversions ROSCO de Manager qui sont déjà vérifiées plutôt que les remplacer sans essai.

**Compatibilité :** le moteur `librosco` de SESSION pour 1.2/1.3/1.6 **reste la référence en production**. Les constantes de RoverMEMS servent de comparaison, d'inventaire des fonctions et de couverture de variantes.

## 4. MEMS 1.9 : initialisation lente et poursuite de dialogue

Source : `ecu-19.go`, puis `ecu-1x-shared.go`.

1. RoverMEMS lance initialement un essai `ecu1xLoop` (premier octet `CA`), **avant** de réaliser le réveil lent. Ce pré-essai peut échouer sans remettre en cause la séquence suivante.
2. Ouverture du port à **9600 8N1** ; BREAK relâché **2 000 ms**.
3. Émission de **l'adresse `0x16` à 5 bauds** : un bit de début, les huit bits en ordre **LSB first**, puis un bit d'arrêt ; chaque bit dure **200 ms**.
4. Réponse ECU typique **`55 76 83`** (synchronisation + deux octets-clés) ; **attente 50 ms** ; renvoi du **complément du second octet-clé**, soit **`7C`** pour `83`.
5. Le Go attend **`7C E9`** (avec écho de `7C` et confirmation ECU `E9`, complément de l'adresse `16`), puis poursuit sur la séquence 1.x **`CA`, `75`, `F4`, `D0`**.
6. L'identification vient des 4 octets après `D0` ; les mesures et défauts proviennent de `80` et `7D`.

La réponse `55 76 83` est un exemple typique ; **Manager calcule le complément de la clé effectivement reçue**, et non seulement `7C` en dur. Point physique important à valider : impulsions BREAK du câble KKL, échos supplémentaires, délais, fragmentations et **maintien de la connexion après le réveil**. L'absence d'ECU produit normalement une expiration de délai et **aucun ID inventé**.

## 5. MEMS 2J : séquence complète, format, mesures et défauts

Sources : `ecu-2j.go`, `ecu-2j-parse.go`, `ecu-2j-faults.go`, `ecu-auth.go`.

### Réveil, identification par dialogue et maintien

- **10400 bauds, 8N1**. Repos 200 ms ; BREAK bas **25 ms** ; retour haut **25 ms**, puis **50 ms** additionnels.
- TX **`81 13 F7 81 0C`** (trame initiale spécifique). Réponse de réveil attendue : payload **`C1 D5 8F`** (forme reconstituée **`03 C1 D5 8F 28`**). Le câble peut renvoyer **son propre écho** de la trame envoyée.
- Ouvrir session : payload **`10 A0` → `50`**.
- Défi : **`27 01` → `67 01` + seed 16 bits** ; calculer la clé par **`generateKey(seed)`** dans `ecu-auth.go` ; **`27 02` + clé 16 bits → `67 02`**. Ce handshake ECU est une composante du protocole, **pas un verrou ajouté par SESSION**. La source traite le cas d'un seed nul.
- Présence session : **`3E` → `7E`**, avec cas de refus **`7F 3E 10`** ; la source tente alors une nouvelle demande de seed.
- DTC : **`21 19` → `61 19`** ; mesures : **`21 XX` → `61 XX`**, où XX dépend du champ demandé.

**Enveloppe du dialogue après initialisation :** `[LONGUEUR payload][PAYLOAD][SOMME modulo 256]`. Pour les contrôles arithmétiques sans ECU :

| Fonction | Payload | Trame complète calculée |
|---|---|---|
| Démarrage session | `10 A0` | `02 10 A0 B2` |
| Demande seed | `27 01` | `02 27 01 2A` |
| Ping | `3E` | `01 3E 3F` |
| Régime | `21 09` | `02 21 09 2C` |
| Défauts | `21 19` | `02 21 19 3C` |
| Réveil réponse | `C1 D5 8F` | `03 C1 D5 8F 28` |

Ces exemples ne sont **pas** des captures d'un ECU ; ils servent aux tests de construction/décodage des trames.

### Inventaire de TOUTES les lectures `21 XX` définies dans RoverMEMS

| Requête | Donnée relevée dans le parseur | Incertitude éventuelle |
|---|---|---|
| `21 00` | Paquet connu, valeur non exploitée | Conserver brut |
| `21 01` | Température liquide | `(U16 - 2732)/10` °C dans le Go |
| `21 02` | Température huile | Même méthode |
| `21 03` | Température air admis | Même méthode |
| `21 05` | Température carburant | Valeur brute, unité non prouvée |
| `21 06` | Paquet connu, non décodé | Potentiel de recherche |
| `21 07` | MAP | `U16/100` kPa |
| `21 08` | Angle papillon | `U16/100` degrés |
| `21 09` | Régime moteur | `U16` tr/min |
| `21 0A` | Correction carburant, sonde O2 | `feedback/100`, O2 mV ; estimation AFR non universelle |
| `21 0B` | Charge des deux bobines | Conversion/échelle à confirmer |
| `21 0C` | Impulsions des quatre injecteurs | Unités brutes non établies |
| `21 0D` | Vitesse véhicule | Valeur brute |
| `21 0F` | Contact, papillon, climatisation | Bits 0, 1 et 3 |
| `21 10` | Tension batterie | Mot 16 bits, `/1000` volts, peut être refusé |
| `21 11` | Synchronisation capteur primaire/secondaire | Source interprète les bits inversés |
| `21 12` | Pas moteur ralenti | `U16/2` |
| `21 13` | État boucle fermée | Bit 0 |
| `21 19` | **Masques de défauts** | Décodeur Mini MPi, variantes non prouvées |
| `21 21` | Erreur de régime | Conversion signée **à corriger dans le Go** |
| `21 25` | Indicateur de came | Le Go indique un **refus sur Mini MPI** |
| `21 3A` | Avance de ralenti, régulateur RPM | Deux mots U16, première valeur `/10` |

**Défauts `61 19` :** sous/surtensions sondes et alimentation, état présent/historique de MAP, TPS, températures, batterie, lambda, vitesse, boîte auto et correction richesse. Le Go attribue le **même bit `buffer[12]` bit 2 à deux défauts différents** et le marque lui-même comme une probable erreur : ne pas copier cette ambiguïté dans Manager comme une vérité.

**Réponses négatives :** `7F [service] [code]` ; `78` = attente. Une mesure refusée/absente sur une variante ne doit pas interdire les autres mesures ni la connexion déjà prouvée. Le Go contient des faiblesses : checksum des réponses non systématiquement vérifié, tailles/indices parfois fragiles, lecture série répétée sur reconnexions ; **ne pas recopier ses défauts de robustesse**.

## 6. MEMS 3 : un protocole complet supplémentaire

Sources : `ecu-3.go`, `helpers.go` et `ecu-auth.go`.

**Communication `9600 8E1` (parité PAIRE)**. Les requêtes ont une autre enveloppe : **`B8 13 F7` + `[LONGUEUR]` + `[PAYLOAD]` + `[XOR de tous les octets précédents]`**.

| Fonction | Payload | Réponse attendue |
|---|---|---|
| Réveil | `1A 9A`, soit TX complet **`B8 13 F7 02 1A 9A DE`** | `5A 9A` |
| Session diagnostic | `10 A0` | `50` |
| Demande seed | `27 01` | `67 01` + seed |
| Validation clé | `27 02` + clé | `67 02` |
| Ping | `3E` | `7E` |
| Lire défauts | `18 00 00 00` | `58` + codes défaut |
| Eau, huile, air | `21 00` | `61 00` |
| MAP, papillon, régime | `21 06` | `61 06` |
| Carburant, lambda | `21 0A` | `61 0A` |
| Temps de bobines | `21 0B` | `61 0B` |
| Écart régime | `21 21` | `61 21` |

Le Go contient une table de codes `DTC` (pompe, lampe moteur, ventilation, lambda, ratés combustion, relais, MAP, etc.) avec des états « présent », « historique » et « test inachevé ». Les valeurs sont exploitées par `ecu-3.go` mais la compatibilité par variante reste à vérifier.

**Découverte importante : SESSION n'a pas encore de famille MEMS 3 dans `protocolcontext.h`.** Il faudra un module **indépendant** avec son format 8E1 / XOR, pas un simple changement de vitesse du pilote 2J. Ce rapport ne le raccorde pas au logiciel.

## 7. RC5 : protocole AIRBAG/SRS trouvé, à isoler du moteur

Source : `ecu-rc5.go`.

- Port **2400 8N1** ; repos BREAK 2000 ms ; actif 200 ms ; repos 400 ms ; actif 400 ms ; repos 400 ms ; actif 400 ms ; repos 200 ms.
- Réponse réveil **`55 06 3B`** ; attente totale 700 ms avant première commande.
- `82 00 7D` → `C2 00 3D` : ping ; `82 33 4A` : lecture défauts, avec DTC **sur 2 octets**.
- Les libellés des codes RC5 concernent airbags, prétensionneurs et témoin SRS ; ils sont archivés dans le code d'origine et le JSON, **pas des défauts moteur**.

RC5 est une mine d'information **annexe**, mais ne doit **pas** entrer dans la recherche automatique d'un calculateur moteur.

## 8. Toutes les fonctions identifiées qui MODIFIENT les calculateurs

Les fonctions ci-dessous ne doivent **jamais être déclenchées pendant la simple connexion**. La présence dans le code source **ne garantit pas** leur acceptation par toutes les variantes ECU.

### 8.1 MEMS 1.x et, après réveil, fonctions potentiellement applicables au 1.9

| Fonction demandable dans `ecu1xUserCommands` | Octet(s) Go | Nature |
|---|---|---|
| Effacement des défauts | `CC` | Mémoire défauts |
| Test compte-tours | `6B` | Sortie compteur |
| Chauffage lambda ON / OFF | `19` / `09` | Test de relais |
| Climatisation ON / OFF | `13` / `03` | Test d'embrayage |
| Pompe essence ON / OFF | `11` / `01` | **Actionneur physique** |
| Ventilateur 1 ON / OFF | `1D` / `0D` | **Actionneur physique** |
| Purge canister ON / OFF | `18` / `08` | Test électrovanne |
| Ralenti augmenter / diminuer | `91` / `92` | Réglage ECU |
| Avance allumage augmenter / diminuer | `93` / `94` | Réglage ECU |
| Correction carburant 1 augmenter / diminuer | `79` / `7A` | Réglage ECU |
| Correction carburant 2 augmenter / diminuer | `7B` / `7C` | Réglage ECU |

**Attention à l'octet `7C` :** il désigne le **complément de clé pendant le réveil 1.9**, mais dans le **dialogue moteur établi** il peut désigner une commande de correction carburant. Ne surtout pas confondre les deux étapes.

### 8.2 MEMS 2J — effacement, antidémarrage et services de recherche

| Requête trouvée | Usage/commentaire du développeur Go | Exploitabilité prudente |
|---|---|---|
| `31 CB` + zéros | Effacement de défauts ; réponse `71 CB` | **Commande d'écriture** explicitement référencée |
| `31 D0` + zéros | Apprentissage de code antidémarrage ; `71 D0` | **Fonction sensible / modification de configuration** ; ne pas relier automatiquement |
| `31 D5` | Service expérimental autour de la calibration | Effet **non suffisamment établi** |
| `33 D5, 33 C0, 33 C8, 33 D2, 33 D4, 33 DA, 33 C1, 33 D7` | Sondages de champs RAM et état antidémarrage selon annotations | **Usage exploratoire**, à ne pas prendre pour une lecture universellement sûre |
| `23 00 07 22 01` | Demande de données à une adresse | Lecture décrite, disponibilité variable |
| `13` | Service non qualifié | Insuffisamment documenté |
| `31 D6 / 31 D7` | Mentionnées **uniquement en commentaire** (calibration et état antidémarrage) | **Pas une fonction implémentée dans le chemin actif** ; ne pas automatiser |
| Lecture ROM à partir d'une adresse | Déclarations **commentées** | Projet expérimental, **pas un téléchargement ROM opérationnel** |

Le `27 01 / 27 02` du dialogue courant est une **authentification ECU par seed/key**, non une fonction qui modifie les réglages. Les opérations `31` et `33` exploratoires ne doivent pas être confondues avec ce handshake.

### 8.3 MEMS 3 et RC5

| Famille | Fonction | Trame ou payload Go | Réponse attendue |
|---|---|---|---|
| **MEMS 3** | Effacer les défauts moteur | Payload **`14 00 00`**, encadré par `B8 13 F7/LEN/XOR` | `54 00 00` |
| **RC5 / SRS** | Effacer les défauts airbag | **`82 C3 BA`** | `C2 C3 7A` |

Le code MEMS 3 de cette révision n'expose **pas** l'ensemble des réglages et actionneurs 1.x ; ne pas les supposer compatibles. Le RC5 n'est pas un ECU d'injection et sa commande d'effacement est propre au système **airbag**.

### 8.4 Classification à conserver dans Manager

**Lectures / consultation :** identification `D0` ; données `80 / 7D / 21 XX`, défauts `21 19 / 18 00 00 00 / 82 33 4A`.

**Échanges nécessaires à la session :** réveils, `10 A0`, `27 01`, `27 02`, `3E` ; **ne sont pas des tests actionneurs**.

**Commandes avec effet :** effacement DTC, actionneurs (pompe, ventilateur, lambda, purge, climatisation, compte-tours), corrections du ralenti/de l'avance/de la richesse, commandes de calibration, antidémarrage et SRS. **Archivées mais non activées dans ce travail**.

## 9. Décodeurs, variantes, erreurs et données additionnelles

- `ecu-1x-shared.go` : conversions des deux trames `80` et `7D`, longueur annoncée et bits DTC ; certaines conversions incomplètes ou qualifiées `TODO`.
- `ecu-2j-parse.go` : conversion des vingt-et-un identifiants de lecture, températures, régime, batterie, lambda, bobines, injecteurs, synchronisation, papillon, vitesse et ralenti ; erreur de conversion **signée `21 21`** (retrait de 65535 au lieu de 65536 dans la source).
- `ecu-2j-faults.go` : masques DTC issus en partie de **Mini MPi**, ne prouve pas que VVC/2J partagent tous les bits. Le **même bit du champ `buffer[12]` bit 2** a deux interprétations incompatibles signalées par un commentaire source.
- `ecu-3.go` : décodeurs des paquets `61 00`, `61 06`, `61 0A`, `61 0B`, `61 21` et table de DTC moteur.
- `ecu-rc5.go` : table de DTC SRS, avec codes présents / historiques en 16 bits.
- `ecu-auth.go` : routine `generateKey(seed)` de 16 bits utilisée par 2J et 3 ; à conserver comme source de référence, pas à exécuter pendant une détection sans dialogue ECU valide.
- `main.go` / `serialReadWrite.go` : sélection de la famille et lecture série ; ils n'apportent pas de preuve d'une communication réelle.

**Erreurs connues du logiciel de référence :** certains checksums entrants non validés ; indexation sans garde de longueur ; traitement imparfait des réponses négatives `7F` ; relances de goroutine série pouvant rester actives. **Ne pas importer ces comportements dans le programme C++**.

## 10. Exploitation future pour ECU MEMS Manager SESSION

1. **Préserver les pilotes 1.2/1.3/1.6, 1.9 et 2J déjà présents**, sans les remplacer pour la seule raison que RoverMEMS fait autrement.
2. Ajouter plus tard **MEMS 3** dans un pilote strictement séparé (9600 **8E1**, format `B8 13 F7`, XOR, séquence session/seed-key, mesures et défauts).
3. Conserver **un seul bouton « Connecter »** ; Manager choisit le câble et essaie les protocoles adaptés ; **seules les réponses du calculateur établissent la réussite**, pas le VID/PID seul ni la présence du câble.
4. Suivre la réponse de chaque phase : port, BREAK, transmission, réponse valide, handshake, ID ou mesure ; échos TX/RX, timeout et trame négative doivent être explicites.
5. Les accès **modifiants** ne font partie **ni du réveil, ni de la reconnaissance automatique**. Les maintenir séparés, avec leur génération d'ECU et un modèle de réponse propre, lors d'une éventuelle future prise en charge demandée.
6. **Ne pas fusionner RC5 avec l'injection**. C'est une piste documentaire SRS indépendante.
7. Le [catalogue JSON](15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json) conserve profils, séquences, constantes hexadécimales, noms de mesures, variantes et DTC RC5 pour développement et tests de non-régression. Les [sources originales archivées](rovermems-reference/i386-20230128/) permettent de vérifier chaque information.
8. La réussite du build **SESSION 182** ne prouve aucune validation K-Line physique ; ces essais restent à faire avec ECU branché.

**Livraison : recherche et archivage uniquement, sur la branche existante. Aucun code applicatif, base documentaire distribuée, Aide, workflow ou Setup n'est modifié.**
