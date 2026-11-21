# SESSION — Étude technique 20 : lecture diagnostique Lucas 5AS et limites de compatibilité 10AS

**21 novembre 2026 — Recherche documentaire, après validation de la version SESSION 187.**  
**Projet :** ECU-Mems-Manager-Session, page **Autres calculateurs**, deuxième colonne **Antidémarrage**, troisième colonne **Programmation des clés**.  
**Branche conservée :** `fix-documentary-additive-session-161-20261118`.  
**Statut : documentation uniquement ; ni modification de la version 187, ni compilation, ni publication de Setup.**

## 1. Résultat principal : la lecture du Lucas 5AS peut être étudiée indépendamment de toute programmation

Les documents publics de **James Portman / RoverMEMS** décrivent une vraie session de diagnostic **5AS**. La présentation du logiciel **Lucas 5AS Tools** d'**Andrew Revill** en confirme la réalisation : identification, entrées dynamiques et activités des télécommandes.

La première étape fonctionnelle du futur bloc **Antidémarrage** pourra donc être **diagnostic normal et lecture seule**. Il ne faut **pas** assimiler cette connexion aux sessions moteur MEMS 1.9, MEMS 2J, MEMS 3, à l'interface RC5 Airbag ou au 10AS.

### 1.1 Réveil et mode normal : informations présentes dans la documentation source

**Source exacte consultée :**  
`https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md`  
(blob public GitHub vérifié, SHA `79290af8887cdc1f069d4d86e6f613ccc9e4a978`).

| Élément | Information figurant dans la source | Niveau de preuve |
|---|---|---|
| Initialisation normale | Transmission d'un octet `1C` par réveil **5 bauds** | Documenté par Portman |
| Réponse d'initialisation | `55 83 76` | Documenté |
| Communication après initialisation | Généralement **9 600 bauds**, mention distincte d'un réglage possible à 1 200 bauds | 9 600 documenté ; nécessité et sélection du 1 200 non démontrées |
| Lecture d'un paquet dynamique en mode normal | Service `83` | Documenté |
| Fin de communication diagnostique | Service `F6` (« reset/disconnect from diagnostics ») | Documenté |
| Mode spécial de mémoire | Initialisation distincte du mode normal | Documenté, **ne fait pas partie de la lecture normale** |

**Important :** le contenu du fichier est un relevé de recherche, pas une spécification automobile complète et certifiée. Il ne donne **pas**, pour toutes les variantes, la séquence complète de contrôle d'écho, les délais garantis, ni une capture horodatée de dialogue 5AS. Ne pas les inventer.

### 1.2 Trame de lecture du service normal : champs publiés et incertitudes

Portman décrit une réponse dont **l'octet 0 contient la taille de paquet, octet de taille compris**. Les positions ci-dessous sont des **indices de l'ensemble de la réponse**, pas des offsets relatifs à la seule charge utile.

| Indice réponse | Champ indiqué par Portman | Certitude / remarque |
|---|---|---|
| 0 | Longueur du paquet | Règle de bornage de trame |
| 1 | Contacteur capot (bit 7, valeurs `00`/`80`) | Indiqué comme capot |
| 6 | Contact mis / ignition (bit 7) | Indiqué comme contact |
| 9 | Contacteurs de portes (bit 7) | Source regroupe plusieurs portes |
| 11 | Contacteur coffre possible (bit 7) | **Incertain :** source écrit `boot(?)` |
| 16, 17, 18, 19 | Quatre compteurs de réception des télécommandes | Source les nomme ; interprétation temporelle limitée |
| 20, 21, 22 | Numéro de série en trois octets, faible en premier | Documenté |
| 35 à 48 | Chaîne ASCII de description du boîtier, exemple **`AP3.005AS-98MY`** | Exemple d'une variante, pas universel |
| 2–5, 7–8, 10, 12–15, 23–34 | Champs inconnus ou non publiés | **Rester bruts, sans libellé inventé** |

**Lien avec Andrew Revill :** il confirme que l'identification et le numéro de série ne sont pas disponibles de la même manière sur tous les boîtiers, et que les compteurs de PLIP peuvent utiliser leurs octets à d'autres fins entre deux réceptions.

**Règle future pour Manager :** conserver une réponse brute, vérifier sa longueur effective, n'afficher que les champs réellement contenus dans le paquet, et utiliser « non disponible / à confirmer » si le modèle n'expose pas l'information. Les champs absents ne sont pas des valeurs zéro et ne constituent pas un « capteur défectueux ».

## 2. Problème matériel confirmé : le câble 5AS n'est pas le câble moteur ordinaire

**Sources croisées et vérifiées :**
- `https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md` (SHA `d8c9026f77ae4961fed18347a9fd830b80c86f5d`).
- `https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md` (SHA `6eb729dee697a849af86cae020ac0437700d7055`).
- `https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm`.

**Sur les configurations 5AS avec prise OBD à 16 broches :**

| Broche de la prise OBD du véhicule | Liaison | Rôle |
|---|---|---|
| **3** | 5AS connecteur **C61-16** | Réception par le 5AS : émission depuis l'outil |
| **1** | 5AS connecteur **C61-24** | Émission depuis le 5AS : réception par l'outil, **inversée** |

Ce sont les **broches de la prise OBD**, à ne pas confondre avec les broches 16 et 24 du connecteur interne du calculateur. Pour la connexion moteur KKL courante, les lignes habituelles sont différentes.

- Portman indique des niveaux autour de **0–12 V** et précise que ce **n'est pas du RS-232 PC standard**.
- Andrew Revill confirme le besoin des **deux lignes K et L**, avec **réception inversée**. Un KKL normal peut détecter un COM Windows sans pouvoir recevoir la réponse 5AS.
- Un KKL équipé d'un circuit **CH340** n'est pas automatiquement compatible avec un 5AS ; le type de puce USB ne certifie ni la modification de polarité ni le brochage réel. Certains KKL FTDI disposent de capacités de configuration mais ce n'est pas une preuve sur un câble donné.
- Les **anciens connecteurs 5AS verts à 3 broches** sont distincts des connecteurs ECU moteur **blancs** ; la source Portman signale des tensions différentes.
- Une interface adaptée et vérifiée pour le 5AS est nécessaire ; **aucun logiciel seul ne peut compenser deux broches non câblées**.

**Implication très pratique pour l'objectif utilisateur :** Manager peut inventorier les COM et détecter une réponse propre à un calculateur **lorsqu'il est physiquement relié au bon circuit**. Il ne peut pas déduire les broches et l'inversion d'une simple identification USB. La confirmation « 5AS connecté » doit reposer sur l'échange diagnostique, pas sur le seul modèle du câble.

## 3. Éléments nouveaux provenant de deux logiciels qui fonctionnent déjà

### 3.1 RoverMEMS — interface visible et catégories de fonctions

Page de référence : `https://rovermems.com/5as/application.html`.

L'interface publique RoverMEMS sépare :
- **Diagnostics** (connexion, lecture de contacts et données dynamiques).
- **Tests de sorties**, spécifiques à certains modèles (avertisseur, témoin, feux de détresse).
- **Procédures** dont l'apprentissage des PLIP.
- **Édition de paramètres et mémoire**, qui est une famille différente de commandes modifiant le calculateur.

Des intitulés visibles sont : `bonnetSwitchState`, `doorSwitchState`, `ignitionSwitchState`, `bootMaybeSwitchState`, `keyFob1PressedCounter` à `keyFob4PressedCounter`, `serialNumber`, `description`, `Voltage` et `rawData`.

**Attention :** l'existence du libellé **Voltage** dans l'interface n'est pas la démonstration de sa formule de conversion ni de son offset dans le paquet `83`. De même `bootMaybeSwitchState` confirme que le développeur garde lui-même une incertitude. Ne pas créer une conversion de tension non sourcée.

### 3.2 Andrew Revill — fonctions et limites réellement annoncées

Page `https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm`, documentation « Lucas 5AS Tools ».

L'application comporte **About 5AS**, lecture de paramètres, **Verify**, sauvegarde/ouverture, onglet hexadécimal, journal, diagnostic direct des états de contacteurs, tests des sorties et programmation de PLIP.

**Précisions particulièrement utiles :**
- L'auteur signale que les messages entre PC et 5AS **n'incluent pas de contrôle de somme de vérification** dans les opérations mémoire qu'il décrit : il utilise une fonction **Verify** pour relire et comparer.
- Il indique que des **écritures de paramètres invalides peuvent rendre le 5AS inutilisable**. Ne pas assimiler « bouton Écrire disponible dans un autre logiciel » à une capacité de programmation validée dans Manager.
- Les compteurs de touches des PLIP ont une **validité temporaire, immédiatement après pression**. Le numéro de la dernière télécommande programmée peut aussi être temporaire.
- Certaines informations d'identification ne sont disponibles que sur des boîtiers **98MY ou postérieurs**, et le numéro de série peut manquer sur les plus anciens.

Le diagnostic de base est suffisamment établi pour être envisagé dans Manager. En revanche, **les réponses longues brutes réelles et la compatibilité matérielle des différents câbles restent à relever**.

## 4. Clés Lucas 5AS — distinction des opérations

L'apprentissage d'une télécommande compatible dans un 5AS est confirmé par RoverMEMS, Faultmate et Andrew Revill.

- Capacité : **jusqu'à quatre télécommandes** dans les conditions documentées.
- Sur la procédure d'Andrew Revill, l'ouverture d'une session d'apprentissage **efface les télécommandes associées auparavant** ; les télécommandes légitimes qui doivent continuer de fonctionner doivent être présentes pendant l'opération.
- Le récepteur et la télécommande utilisent des mécanismes de synchronisation ; avoir **la même fréquence radio** (433,92 MHz sur certaines variantes, 315 MHz sur d'autres) ne suffit pas à garantir une compatibilité.
- Les PLIP **3TXA, 3TXB, 3TXC, 3TXD, 17TN** ne doivent pas être déclarés interchangeables sans référence de version précise du 5AS, du boîtier radio et du marché.
- **Ne pas confondre** l'apprentissage des PLIP avec la synchronisation **5AS–ECU moteur**.

La lecture dynamique de la réception PLIP peut être étudiée dans l'onglet **Antidémarrage** ; l'apprentissage reste dans la troisième colonne **Programmation des clés**.

## 5. Lucas 10AS : constat indépendant, plus précis que dans les fiches précédentes

**Source professionnelle :** `https://www.blackbox-solutions.com/site/product?sku=SM031`, « LUCAS 10AS ALARM » ; document Nanocom de référence `https://www.nanocom-diagnostics.com/downloads/download/lucas-10as-alarm-nanocom-evolution`.

Le 10AS vert (notamment Land Rover Defender et Discovery I) a sa propre architecture :

- **Jusqu'à 47 types de problèmes** décrits comme détectables par le module Faultmate, une mémoire de défauts distincte, avec **cinq derniers événements déclencheurs** sur certaines versions (source : SM031).
- Informations d'entrée : contact, capot, portes, coffre, contacteur de serrure, témoins, état d'apprentissage, **quatre compteurs PLIP**, paramètres et états selon montage.
- Tests de sortie : serrure, clignotants, avertisseur, témoin de sécurité, etc. ; non pertinents pour une première lecture seule.
- Fonctions d'apprentissage PLIP et de synchronisation selon le système de motorisation (DDS, Spider, etc.) : **pas un simple copier-coller du 5AS**.
- Le connecteur **gris du boîtier 10AS**, dans le document, emploie notamment **broche 17 (« Serial comms »)** et **broche 24 (« Serial ground »)**. **Ce sont les broches du connecteur du boîtier**, PAS celles de la prise OBD du véhicule. Le document consulté n'établit pas les octets de dialogue propres au 10AS.
- La prise « OBD-II » mentionnée est une **forme de connecteur** ; elle ne prouve pas un protocole standard OBD-II.

### 5.1 Nouvelle découverte importante : deux générations « 10AS » ne communiquent pas forcément de la même manière

Le document Nanocom « Lucas 10AS Alarm » précise qu'à partir du **millésime 2014** Land Rover a installé **un boîtier d'un autre fabricant, noir**, qui garde parfois la désignation « 10AS » et une enveloppe de forme similaire, mais est **incompatible avec le diagnostic de l'ancien 10AS vert**.

La publication Nanocom originale indique une transition à **VIN FA000001**, tandis qu'une transcription HTML sur `manuals.plus` indique **VIN EA000001**. **Il y a divergence textuelle entre deux versions de la source : ne pas intégrer de seuil VIN absolu dans Manager avant examen de documents constructeur précis.** Le point ferme est la distinction **ancien Lucas vert / nouvelle génération noire** et l'absence de compatibilité diagnostique démontrée.

**Résultat pour le futur développement :** créer une famille/protocole **10AS ancien** distincte ; ne jamais conclure à la compatibilité sur le seul intitulé « 10AS ».

## 6. Pektron SCU : encore une famille différente

**Source :** `https://www.technozen-electronics.co.uk/mg-rover-pektron-scu-information/`.

- Introduit vers **2003** selon les modèles MG Rover ; il regroupe alarme, antidémarrage et plusieurs fonctions d'habitacle.
- Références connues : **YWC001540, YWC001541, YWC001551** ; jusqu'à **10 télécommandes** sur les configurations documentées.
- Différences radio **433/315 MHz** ; un PLIP neuf doit disposer de son identifiant d'origine (code-barres de **22 caractères** selon la documentation spécialisée).
- **L'état armé peut empêcher la communication diagnostique** ; cela ne signifie pas nécessairement « ECU absent ».
- Le spécialiste indique que des relais ou des défauts de réception radio peuvent produire des symptômes sans être des échecs de protocole.
- Le site Pscan `https://pscan.uk/features/pektron.html` confirme une prise en charge propre à Pektron ; il ne publie pas les trames exactes de connexion. Les protocoles Pektron ne peuvent pas être déduits de ceux du 5AS.

**Conclusion de recherche :** le 5AS est la cible la plus documentée pour une **future première réalisation en lecture**, tandis que Pektron et 10AS nécessitent encore du travail de protocole.

## 7. Spécification préparatoire réaliste de la colonne « Antidémarrage »

Ceci est un **plan de travail**, pas une réalisation :

| Ordre | Fonction future | Source disponible | État |
|---|---|---|---|
| 1 | Inventorier les ports sans changer la connexion MEMS moteur | Infrastructure SESSION existante | Faisable côté logiciel |
| 2 | Ouvrir un canal **5AS** sur câble physiquement compatible | Portman + Revill | Documenté ; test matériel à prévoir |
| 3 | Initialisation **5 bauds** puis réception du mot de réveil normal | Portman | Octets connus ; tolérances à vérifier |
| 4 | Lire une trame de statut normal et vérifier sa longueur | Portman service `83` | Trame partiellement décrite |
| 5 | Afficher **contact, capot, portes**, autres champs uniquement si confirmés | Portman + web-app RoverMEMS | Plusieurs champs exploitables, coffre incertain |
| 6 | Afficher **identification/description** quand présentes | Portman + Revill | Dépend de la version 5AS |
| 7 | Suivre la **réception des PLIP pendant l'action de la télécommande** | Revill | Valeur transitoire, pas compteur définitif |
| 8 | Terminer la session sans effet moteur ou programmation | Commande de fin documentée | À valider dans la session propre |

Pour **Pektron** et **10AS**, ne pas lancer d'échanges supposés à partir de la connaissance du 5AS. Leur cartographie matérielle, commandes et caractéristiques de trame devront être sourcées indépendamment.

### 7.1 Contrôles qui restent à réaliser avant de prétendre au fonctionnement réel

- Références exactes du boîtier et du câble compatible.
- Traces d'une **vraie réponse d'initialisation** et d'au moins une vraie réponse de statut 5AS ; les champs non documentés restent marqués inconnus.
- Gestion des sessions et de l'absence de câble, de la déconnexion et de la sortie de session en conservant les autres pages.
- Contrôle de l'affichage à **1200, 1366 et 1920 pixels**, sans modifier la disposition des trois colonnes de SESSION 187.
- Conservation séparée des tableaux et compteurs : **RC5 SRS**, **5AS**, **Pektron**, **10AS**.
- Aucune lecture de secret d'antidémarrage (EKA, codes EMS, identifiants PLIP complets) dans un journal d'erreur ou une publication de recherche.

## 8. Références exactes utilisées

1. **James Portman**, notes diagnostic 5AS avec trames de réveil, service normal et offsets de statut :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md
2. **James Portman**, connecteur OBD 16 broches 5AS :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md
3. **James Portman**, connecteur vert 3 broches 5AS :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md
4. **RoverMEMS**, interface 5AS (diagnostic, programmation, paramètres séparés) :  
   https://rovermems.com/5as/application.html
5. **Andrew Revill**, support Lucas 5AS et câble spécifique :  
   https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
6. **Blackbox Solutions**, Lucas 5AS, variantes et fonctions :  
   https://www.blackbox-solutions.com/site/product?sku=SM034
7. **Blackbox Solutions**, Lucas 10AS, 47 types de défauts et cinq événements :  
   https://www.blackbox-solutions.com/site/product?sku=SM031
8. **Nanocom**, documentation 10AS et distinction des boîtiers récents :  
   https://www.nanocom-diagnostics.com/downloads/download/lucas-10as-alarm-nanocom-evolution
9. **Transcription secondaire Nanocom** (divergence de seuil VIN signalée) :  
   https://manuals.plus/m/3174ce311931653175ad0cf47b8b5ea56bf21e2820696c3cafa221b1df1ab6e6
10. **Technozen**, informations Pektron, modèles, télécommandes et comportement armé :  
    https://www.technozen-electronics.co.uk/mg-rover-pektron-scu-information/
11. **Pscan**, support 5AS / 10AS / Pektron pour comparaison des familles :  
    https://pscan.uk/features/5as.html  
    https://pscan.uk/features/10as.html  
    https://pscan.uk/features/pektron.html

**Bilan :** les octets de réveil et de lecture, une grande partie des entrées normalisées et le câblage 5AS sont connus avec leur source. **Il reste à confirmer la réception physique des réponses du boîtier par le câble réel**, plutôt que de considérer un COM USB reconnu comme une connexion ECU. Les commandes 10AS et Pektron restent à rechercher indépendamment. **Aucun build 187, source applicative ou Release ne doit être modifié par ce rapport.**
