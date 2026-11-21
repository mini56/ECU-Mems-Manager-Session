# SESSION — Étude préparatoire RC5 / antidémarrage / programmation des clés

**21 novembre 2026 — recherche et architecture uniquement.**
**Référence d'interface : SESSION 186** (page « Autres calculateurs », trois colonnes réservées).
**Branche conservée :** `fix-documentary-additive-session-161-20261118`.
**Statut :** aucune prise en charge RC5, 5AS ou apprentissage de clés ajoutée au moteur d'exécution par ce rapport. Ne pas interpréter un protocole trouvé comme un essai physique réussi.

## 1. Synthèse — trois familles d'opérations distinctes

1. **Airbag / SRS — RC5 (parfois indiqué EC5/RC5)** : diagnostic d'un calculateur de retenue, lecture des défauts présents et mémorisés, effacement demandé explicitement.
2. **Antidémarrage / alarme** : exemple documenté **Lucas 5AS** ; des familles **Lucas 10AS**, **Pektron SCU** et autres existent aussi. Aucune hypothèse de protocole commun.
3. **Clés / télécommandes** : l'apprentissage des **PLIP** sur 5AS constitue une fonction différente du jumelage entre boîtier antidémarrage et calculateur moteur. Ne pas confondre télécommande radio, transpondeur et lame mécanique de clé.

La page SESSION 186 réserve **trois colonnes égales**, sans commandes réelles. Elle n'est pas une preuve de prise en charge des systèmes.

## 2. RC5 — données établies par code archivé et recoupement indépendant

**Source de référence exacte** : `research/mems/lecture-ecu-19-2j-3/rovermems-reference/i386-20230128/ecu-rc5.go`, révision RoverMEMS Agent `ed9ec4dd3181e60dcb7e4bcb340016fcb6bf20f3`, GPLv3. L'application RoverMEMS Agent présente bien RC5 comme un calculateur **SRS**, avec lecture/effacement des défauts. L'application Faultmate SM040 est une vérification externe pour les fonctions de lecture, effacement et identification.

### 2.1 Transport et contrôle de réponse

- Port série **2 400 bauds, 8 bits, aucune parité, 1 arrêt (8N1)**.
- Réveil par une succession de BREAK/relâchements propre au RC5 ; ordre/durées dans la source archivées : relâché 2 000 ms, BREAK 200 ms, relâché 400 ms, BREAK 400 ms, relâché 400 ms, BREAK 400 ms, relâché 200 ms.
- Message de réveil attendu **`55 06 3B`**. Après réveil, le programme de référence attend **700 ms au total** avant la première commande.
- Ping `82 00 7D`, réponse attendue `C2 00 3D`.
- Lecture de défauts `82 33 4A`. Les réponses de défauts comportent l'identifiant `33` et des codes de **16 bits**.
- Effacement explicite `82 C3 BA`, réponse attendue `C2 C3 7A`.

**Propriété technique importante :** les trois octets des demandes/réponses fixes ci-dessus totalisent **0xFF modulo 256**, soit une somme de contrôle additive, **pas le XOR du MEMS 3**. Exemple `0x82 + 0x33 + 0x4A = 0xFF` ; `0xC2 + 0xC3 + 0x7A = 0x1FF`, octet faible `0xFF`. Étudier et valider ce contrôle pour toute longueur de trame avant intégration. Il faut isoler l'écho TX éventuel du câble.

La source Go déduit la longueur de la réponse à partir du premier octet : **`octet_0 - 0xC0 + 1`**. Pour la future implémentation, n'extraire et décoder que **la trame complète validée**, non l'intégralité d'un tampon pouvant contenir plusieurs trames. `rc5ParseFaults(buffer)` dans la référence d'origine retire seulement deux octets et ne borne pas son analyse à la trame reçue ; ne pas reprendre ce défaut de fiabilité.

### 2.2 Table de défauts

Le source archivé donne **24 entrées « présentes »** et **24 entrées « historiques »** portant sur :
- Airbag conducteur : court-circuit au positif/à la masse, résistances anormales, circuit d'allumeur.
- Airbag passager : mêmes familles de défauts, suivant variantes.
- Prétensionneurs droite et gauche.
- Témoin d'avertissement SRS.

Les codes observés se répartissent notamment en familles **`0x15xx`/`0x16xx`** (présents) et **`0x25xx`/`0x26xx`** (historiques), selon cette table ; l'état reste à établir selon la réponse ECU réelle. Le code `0x0000` est annoté « inconnu » dans RoverMEMS et ne prouve pas l'absence de défaut.

**Limite :** Faultmate décrit jusqu'à **28 types de problèmes** détectables sur ce système et propose en outre des informations d'identification (« Ident ID 1–14 », type système, code-barres). Le seul lecteur RoverMEMS archivé ne démontre **pas** les commandes nécessaires à la lecture de ces identifiants ; ne pas inventer cette fonction. Garder l'affichage hexadécimal et les codes inconnus.

### 2.3 Véhicules et prise diagnostic

- L'équipement **EC5/RC5** est cité pour Rover Mini, Rover 200/400 et MG MGF (et certaines MG TF) par Faultmate ; la compatibilité exacte, variantes et années restent à établir avant toute liste fermée.
- **Mini MPI avec OBD 16 broches :** le circuit SRS RC5 est indiqué sur la **broche OBD 13**, le MEMS 2J moteur sur **OBD 7**, selon les documentations RoverMEMS et pscan.
- Attention : « câble OBD standard » chez un fabricant de valise multimarque **ne prouve pas** qu'un adaptateur USB KKL générique, limité à OBD 7, puisse accéder à OBD 13.
- Certains véhicules japonais ont un autre câblage/connecteur. Il faut documenter ces variantes plutôt que supposer l'OBD 16 broches universel.

### 2.4 Ce que pourra contenir la colonne RC5, après validation

1. Statut de la liaison, port/câble reconnus, réponse du calculateur.
2. Bouton **Connecter** propre au module et commande **Déconnecter**, sans inclure le RC5 dans la détection moteur MEMS.
3. **Lire les défauts** ; tableau « Code / État / Organe / Description », défilement interne si nécessaire ; messages d'erreur et codes bruts.
4. **Effacer les défauts**, commande séparée, jamais envoyée au réveil ou pendant la reconnaissance ; résultat vérifié par réponse ECU et nouvelle lecture.
5. **Identification du boîtier** uniquement si un service/une réponse fiable est trouvé pour cette variante.

Pas d'actionneur SRS, de commande pyrotechnique ou de programmation de boîtier supposée à partir des simples informations de défaut.

## 3. Lucas 5AS — découverte structurante pour antidémarrage et clés

Deux références indépendantes : documentation technique James Portman et compte-rendu de **MEMS Mapper / Lucas 5AS Tools** par Andrew Revill. La documentation de Portman distingue le mode diagnostic normal et le mode d'édition de mémoire. Des fonctions d'identification et de lecture des états (portes, capot, contact), des informations de télécommandes et des opérations d'apprentissage existent. Les descriptions du paquet restent incomplètes pour plusieurs octets.

**Caractéristiques de la liaison** :
- Initialisation **5 bauds** puis communication nominale **9 600 bauds** décrite par James Portman ; ne pas présumer du format électrique en lisant seulement la vitesse.
- Sur prise OBD 16 voies, **broche 3** émission vers 5AS et **broche 1** réception depuis 5AS, suivant les documents sur ce montage.
- **Deux conducteurs diagnostiques et une réception inversée**, différent du K-Line moteur simple. Les signaux diagnostiques du 5AS peuvent être de l'ordre de **0 à 12 V**, pas du RS232 PC classique ni du TTL 5 V.
- Le connecteur ancien **vert à trois broches** pour immobilisation ne doit pas être confondu avec le connecteur ECU moteur **blanc/crème** ; un câble moteur inadapté risque d'être endommagé.
- Andrew Revill confirme qu'un câble 5AS spécifique est nécessaire. **Le VID/PID USB identifie la puce série, pas la présence physique des broches et de l'inversion électrique requises.** Manager ne peut pas corriger un câblage matériel inexistant par logiciel.

**Fonctions reconnues en lecture** :
- Connexion et identification de module, avec limitations éventuelles sur les modèles antérieurs à MY98.
- Lecture de certains états d'entrées et sortie/alarme ; informations de télécommandes enregistrées.
- Diagnostic journalisé (requêtes et réponses, détails d'erreurs), sans affichage de secrets dans les journaux par défaut.

**Fonctions modifiantes recensées, NON implémentées dans SESSION** :
- Modification de paramètres de configuration du boîtier 5AS.
- Association et apprentissage des télécommandes (PLIP).
- Jumelage entre antidémarrage et calculateur moteur : **opération distincte** de la programmation PLIP.

**Point déterminant pour la future page :** selon Andrew Revill, **l'apprentissage de nouvelles télécommandes remplace/efface les télécommandes précédemment associées** : il faut réunir toutes les télécommandes autorisées et prévoir une vérification finale. Le 5AS accepte **jusqu'à quatre** télécommandes dans cette source. Cette capacité ne doit pas être extrapolée aux 10AS et Pektron.

La même source précise que des écritures invalides peuvent rendre un 5AS inutilisable et que les échanges mémoire n'ont pas nécessairement de somme de contrôle. **Les fonctions d'écriture et de jumelage ne sont pas prêtes à être câblées à l'interface.**

## 4. Lucas 10AS, Pektron SCU : familles distinctes, compatibilité non héritée

- **10AS** : utilisé sur des Land Rover (notamment Defender/Discovery 1). Les outils professionnels distinguent diagnostic, codage et apprentissage ; les familles de boîtiers/versions ne sont pas interchangeables.
- **Pektron SCU** : monté sur certains MG Rover plus récents ; sa logique de diagnostic et de télécommandes n'est pas celle d'un 5AS.
- Un aspect visuel identique de télécommande n'assure ni la compatibilité du modèle ni celle de son apprentissage.

Le futur module Antidémarrage peut partager avec la colonne Clés une **session Lucas 5AS** quand cette famille est réellement identifiée, mais les vues et les commandes doivent rester distinctes. N'ajouter 10AS et Pektron qu'après collecte de protocoles spécifiques et validation matérielle.

## 5. Matrice : confirmé / restant à vérifier

| Sujet | État documentaire | Vérification suivante |
|---|---|---|
| RC5 2400 8N1, réveil, ping | Source de protocole archivée | Échange sur boîtier réel + comportement d'écho |
| RC5 lecture de défauts et codes 16 bits | Source + outils tiers | Plusieurs réponses longues/multiples, checksum, défauts historiques |
| RC5 effacement à la demande | Commande et acquittement connus | Confirmer le retour et relire les défauts après réparation |
| RC5 identification/type/codes-barres | Capacité signalée par Faultmate | Identifier commandes réelles, variantes et provenance |
| Connexion OBD 13 RC5 Mini MPI | Recoupement RoverMEMS/pscan | Vérifier câble et brochage véhicule, pas seulement VID/PID |
| 5AS connexion 5 bauds/9600 et deux lignes | Documentation Portman/Revill | Compatibilité exacte du câble spécifique, inversion RX |
| 5AS états et identifiants | Sources techniques | Captures réelles sur variantes, avant/après MY98 |
| 5AS apprentissage PLIP | Fonction documentée, jusqu'à 4 | Préconditions, sauvegarde/inventaire et tests de non-régression avec consentement explicite |
| 5AS jumelage antidémarrage/ECU | Fonction distincte reconnue | Protocole précis, configurations compatibles et preuve matérielle |
| 10AS / Pektron | Existence/fonctions générales confirmées | Études de protocole indépendantes, pas de reprise des trames 5AS |

## 6. Conséquences pour SESSION : architecture retenue

- **Une seizième page, trois colonnes égales** : RC5, Antidémarrage, Programmation des clés. La version 186 est uniquement la coquille de navigation et d'affichage.
- Les logiciels de protocole devront être **indépendants du pilote moteur** ; surtout ne pas tenter l'initialisation RC5 pendant `Connecter` sur la page moteur.
- Un **gestionnaire d'accès exclusif au port COM** doit empêcher que deux modules accèdent au même port en parallèle, sans restreindre arbitrairement les calculateurs moteur déjà opérationnels.
- Identifier la connexion **par réponse authentique du module**, jamais par seul VID/PID.
- Pour la suite, commencer par **RC5 ping / lecture de codes** dans la colonne Airbag. L'effacement est une commande explicitement séparée.
- Pour 5AS, documenter d'abord le **câble adapté, la connexion, l'identification et les données en lecture**. Toute programmation ou association sera un chantier distinct.
- Le texte de l'interface doit suivre les **six traductions installées** ; style clair/sombre existant et comportement à 1200/1366/1920 pixels ; aucune modification non demandée aux 15 anciennes pages ou à MEMS 1.2/1.3/1.6/1.9/2J/3.
- Le module de programmation des clés n'est **pas** une promesse d'une fonction universelle ; la compatibilité sera documentée **par famille et véhicule**.

## 7. Sources vérifiables

1. **Code d'origine RoverMEMS Agent** : `rovermems-reference/i386-20230128/ecu-rc5.go` (dans ce dépôt), GPLv3, archivé depuis la révision `ed9ec4dd3181e60dcb7e4bcb340016fcb6bf20f3`.
2. **Rapport antérieur** : `14_RAPPORT_DETAILLE_ROVERMEMS_I386_TOUTES_SEQUENCES_20261120.md`.
3. **Catalogue machine antérieur** : `15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json`.
4. James Portman, **RC5 SRS** : https://github.com/james-portman/rover-mems-documentation/blob/master/SRS/RC5/RC5-SRS-airbag-diagnostics.md
5. James Portman, **Lucas 5AS communication** : https://rovermems.com/5as/index.html
6. James Portman, **câble 5AS** : https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md
7. James Portman, **câble 3 broches** : https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md
8. Andrew Revill, **Lucas 5AS Tools et contraintes de câble, programmation** : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
9. Blackbox Solutions, **Faultmate EC5/RC5 SM040** : https://www.blackbox-solutions.com/site/product?sku=SM040
10. Pscan, **Mini ECU families** : https://pscan.uk/features/mini.html
11. Pscan, **distinction OBD 7 / OBD 13 Mini MPI** : https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/96517-mini-mpi-k-line-fault
12. Pscan, **familles de calculateurs Rover/MG** : https://www.pscan.co.uk/features/eculist.html
13. RoverMEMS Agent, **commandes RC5 annoncées** : https://rovermems.com/agent/
14. Blackbox Solutions, **Lucas 10AS distinct** : https://www.blackbox-solutions.com/site/product?sku=SM031
15. Tec[h]nozen, **Pektron SCU, variantes** : https://www.technozen-electronics.co.uk/mg-rover-pektron-scu-information/

**Livraison de cette étape :** documentation de recherche uniquement. Aucun nouveau workflow, aucun Setup, aucune commande opérationnelle airbag/antidémarrage/clé créée par ce document. Les informations électriques doivent être confrontées à la documentation exacte du véhicule avant une liaison réelle.
