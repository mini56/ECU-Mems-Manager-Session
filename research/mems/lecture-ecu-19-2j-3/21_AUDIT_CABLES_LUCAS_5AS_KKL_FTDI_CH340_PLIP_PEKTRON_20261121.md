# Recherche 21 — Câbles Lucas 5AS : adaptateurs KKL, FTDI, CH340, et télécommandes Pektron

**Date : 21 novembre 2026. Projet : ECU MEMS Manager SESSION.**  
**Objet :** compléter les informations des aides « câble » des deuxième et troisième blocs, et préparer une future lecture du Lucas 5AS.  
**Branche GitHub conservée :** `fix-documentary-additive-session-161-20261118`.  
**Nature : recherche/documentation seulement, aucun changement au logiciel, aux traductions du BUILD 188, aux modules moteur ou au protocole RC5. Aucun run lancé pour cette étude.**

## 1. Découverte majeure : une solution de câble 5AS existe déjà, avec schéma de conversion documenté

Andrew Revill, auteur de l'outil **Lucas 5AS Tools / MEMS Mapper**, a publié :
- une méthode d'adaptation des câbles **VAG-COM KKL 409.1** équipés d'un **véritable FTDI FT232R** ;
- une autre méthode pour ceux équipés d'un **CH340G** ;
- la conversion des **broches OBD 7/15 vers 3/1** nécessaire au 5AS ;
- et une **offre de câble 5AS déjà modifié avec adaptateur**, ce qui confirme qu'une fabrication personnelle n'est pas systématiquement nécessaire.

**Référence du développeur et schémas photographiques :**  
https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm  
**Catalogue des câbles moteur et 5AS distincts :**  
https://andrewrevill.co.uk/shop.htm

### 1.1 Ce que transporte chaque broche sur les véhicules 5AS utilisant une prise OBD 16 voies

| Canal de l'interface | Sur un KKL moteur typique | Pour le Lucas 5AS documenté |
|---|---|---|
| Émission sur la ligne K | Broche OBD **7** | Broche OBD **3** |
| Réception / ligne L | Broche OBD **15** | Broche OBD **1** |
| Polarité de la réponse | Réception normale attendue par le KKL | **Réception logiquement inversée** sur L |
| Adaptation électrique | Circuit automobile du câble KKL selon modèle | **Utiliser l'étage automobile adapté** ; les signaux 5AS sont de l'ordre de 0–12 V, pas une sortie TTL à raccorder directement |

Cette répartition explique pourquoi un convertisseur USB–COM détecté par Windows ne suffit pas : même si Manager sait envoyer une initialisation à 5 bauds, le 5AS **ne répondra pas sur le canal que le câble normal écoute**.

**Concordance avec la documentation Portman :**
- Broche **OBD 1** ↔ **5AS connecteur C61 broche 24** : transmission 5AS, réception interface.
- Broche **OBD 3** ↔ **5AS connecteur C61 broche 16** : réception 5AS, émission interface.
- Sources :
  - https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md
  - https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md

**Précision :** l'appellation « broche 15 » ne désigne ici que le contact OBD utilisé par un KKL standard. Les broches C61-16/C61-24 sont les **contacts du boîtier 5AS**, et non les contacts 16/24 de la prise OBD (qui n'a que 16 voies).

### 1.2 Adaptation de la réception KKL à l'inversion du signal 5AS

#### Variante FTDI FT232R authentique

Dans le cas **d'un composant FTDI original dont la configuration est modifiable**, Andrew Revill indique que le paramètre **Invert RXD** peut être réglé à l'aide de l'utilitaire officiel **FT_PROG** ; il faut débrancher/rebrancher l'adaptateur pour prendre en compte les nouvelles options.

**Cela ne change ni le brochage OBD ni la capacité électrique du câble.** La permutation de voies décrite plus bas reste nécessaire.

**Réserve majeure de l'auteur :** de nombreux câbles bon marché marqués FT232R contiennent des imitations non reprogrammables. Une commande de configuration apparemment acceptée ne signifie pas que l'inversion a bien été appliquée. Seule la lecture de configuration et un essai réel peuvent le confirmer.

Référence FTDI : https://ftdichip.com/utilities/#ft_prog  
Documentation de la procédure et des limites : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm

#### Variante CH340G

Andrew Revill décrit une adaptation **matérielle de l'inversion RXD sur certaines cartes KKL à CH340G** : son montage relie **les broches 15 et 16 du circuit intégré CH340G**, après vérification que la broche 15 est libre. Ce sont les numéros **des pattes du composant CH340G**, **pas ceux du connecteur OBD**.

**Ne pas généraliser cette modification à tout câble comportant la marque CH340/CH341.** Le schéma de la carte, la référence exacte du composant et l'usage des pattes peuvent différer. Il s'agit d'une manipulation électronique délicate, pas d'un réglage Windows. Le branchement électrique doit être vérifié hors tension et par une personne compétente. Le document source comporte des photos montrant l'orientation du composant et l'emplacement concernés.

**Conclusion pratique :** une aide pour Manager peut donner ce principe documenté et renvoyer aux illustrations originales, mais elle ne doit **jamais** promettre « relier les broches 15 et 16 du connecteur OBD » : ce serait une confusion de numérotation potentiellement dommageable.

Source : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm

### 1.3 Adaptation des contacts OBD via une rallonge à deux sorties

Andrew Revill décrit un adaptateur réalisé à partir d'une **rallonge / répartiteur OBD**, dont une sortie reste directe pour le **câble moteur**, tandis que l'autre inverse les connexions **7 avec 3** et **15 avec 1** pour le **câble 5AS déjà adapté électriquement**.

| Fonction | Sortie moteur (directe) | Sortie 5AS (permutée) |
|---|---|---|
| Émission KKL standard | 7 → 7 moteur | 7 du câble → 3 véhicule (5AS) |
| Réception L du KKL | 15 → 15 véhicule | 15 du câble → 1 véhicule (5AS) |
| Inversion réception dans le KKL | Non | Oui, adaptation spécifique FTDI/CH340G comme ci-dessus |

**Il s'agit de deux opérations distinctes :** adaptation de la polarité de réception **et** adaptation des broches physiques. L'une sans l'autre ne garantit pas une communication 5AS.

L'auteur recommande de **marquer distinctement** les prises destinées au moteur et au 5AS afin de ne pas les confondre.

**Ne pas brancher ou commander deux interfaces simultanément sur le même système.** La présence de deux connecteurs sur le répartiteur ne prouve pas que deux logiciels indépendants peuvent occuper les mêmes voies à tout instant. Ne pas déduire la compatibilité à partir de la seule fiche OBD.

Le site de l'auteur décrit aussi un câble CH340 5AS **prêt à l'emploi**, livré avec le répartiteur modifié, et précise que le câble destiné aux calculateurs moteurs **n'est pas interchangeable** avec le câble 5AS. La page propose donc deux options concrètes pour la future aide : **câble spécialisé** ou **adaptation documentée d'un câble compatible**. Ne pas interpréter la présence d'un article sur le site comme une confirmation des stocks à une date donnée.

Sources :
- https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
- https://andrewrevill.co.uk/shop.htm

### 1.4 Réserve par type de véhicule

La disposition **OBD 1/3** est documentée pour les montages 5AS concernés ; les **Mini SPI avec connecteur vert à trois voies** sont un cas différent. Pscan indique disposer d'une interface adaptée et d'un câble trois broches. Ne pas transformer la permutation OBD décrite ici en règle universelle pour tous les modèles 5AS.

Sources :
- https://pscan.uk/features/mini.html
- https://pscan.uk/buy/
- https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md

## 2. Nouvelle confirmation : le protocole 5AS est effectivement exploité dans un appareil moderne

Une publication d'**Andrew Revill du 11 décembre 2025** décrit son appareil autonome **OBDII Pairing Pod** :

- électronique autour d'un microcontrôleur **ATmega32U4**, avec liaison côté véhicule et liaison USB-C distinctes ;
- trois actions séparées selon le matériel : **MEMS 1.9**, **MEMS 2J / MEMS 3**, et **Lucas 5AS** ;
- pour le 5AS, communication décrite comme un **ROSCO modifié**, avec **réception L inversée**, traitée par les circuits électroniques du pod ;
- le matériel contient les circuits nécessaires pour transmettre et recevoir sur K et recevoir la L inversée ;
- l'auteur présente le fonctionnement réel de l'appairage et un résultat par voyants et avertisseur ; ce n'est pas seulement une hypothèse théorique.

**Source primaire :**
https://www.caterhamlotus7.club/forums/topic/279789-obdii-pods-ecu-immobiliser-key-pairing-and-much-more/

**Point important pour Manager :** l'existence d'un microcontrôleur possédant une prise USB-C ne signifie **pas** que ce Pod est déjà un câble USB/COM série générique pour notre logiciel. Il contient son **propre firmware de pilotage** et des commandes propres au concepteur. Ne pas le présenter dans l'aide comme compatible Manager sans définition publiée de son interface PC.

**Usage utile pour nos recherches :** ce projet confirme indépendamment que l'association des calculateurs moteur et l'apprentissage 5AS sont des fonctions réalisables, à condition de conserver les chemins de dialogue et les câbles adaptés. La communication 5AS ne doit pas emprunter aveuglément le pilote MEMS 2J.

## 3. Données nécessaires pour le futur deuxième bloc : diagnostic 5AS normal, sans programmation

Éléments déjà documentés dans le rapport 20 et recoupés :
- Initialisation normale à **5 bauds** par adresse de réveil **0x1C**, réponse attendue **55 83 76**.
- Fonctionnement généralement autour de **9 600 bauds** après initialisation.
- Lecture d'un paquet de statut par la commande **0x83** ; informations de contacts et compteurs transitoires PLIP.
- Différences selon la génération **98MY**.
- Des fonctions séparées (apprentissage, tests de sorties, modification de paramètres) existent ; la lecture seule n'exige pas de les exécuter.

**Nouveaux critères de validation du câble à conserver :**
1. Le câble apparaît comme interface série USB, sans pour autant conclure au succès de communication.
2. Le **circuit TX du KKL arrive à l'entrée 5AS** prévue par le montage.
3. Le **circuit RX reçoit la sortie L 5AS avec inversion de polarité**, sans exposition directe de l'USB à 12 V.
4. La réponse au réveil est réellement reçue et valide.
5. Des données de statut sont renvoyées et interprétées en préservant la trame brute et les champs inconnus.

**Aucune de ces étapes ne doit déclencher une modification mémoire ou une opération de programmation de PLIP dans la session de lecture.**

## 4. Contradiction documentaire à NE PAS masquer : quatre ou huit télécommandes Lucas 5AS

Les sources ne donnent **pas** toutes le même maximum :

- **Andrew Revill**, outil Lucas 5AS Tools : « up to **4** key fobs ».
- **Blackbox Faultmate SM034** (certaines variantes 5AS) : généralement **4** PLIP.
- **Portman**, paquet de statut 5AS : **4** octets de compteurs PLIP (indices 16–19), ce qui étaye quatre emplacements observables dans ce paquet.
- Un manuel de programmation Rover/MG publié sur Manualzz affirme jusqu'à **8 PLIP** pour le Lucas 5AS, sans expliquer dans l'extrait comment sont représentées ces huit positions dans les paquets et pour quelles versions.

**Conclusion : conserver 4 comme nombre d'emplacements actuellement documentés par les sources techniques primaires utilisées, mais NE PAS annoncer qu'aucun 5AS ne peut en accepter 8.** Chercher un document constructeur de variantes ou une capture de huit identifiants avant de traiter l'écart comme résolu.

Sources :
- https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
- https://www.blackbox-solutions.com/site/product?sku=SM034
- https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md
- https://manualzz.com/doc/23028112/rover---mg-key-fob--plip--programming

## 5. Pektron : trois formats de données ne doivent pas être confondus

Les recherches ont permis d'affiner une contradiction apparente sur les télécommandes Pektron :

| Information publiée | Signification documentée | Conséquence |
|---|---|---|
| **22 caractères** | Code-barres / identification fourni avec une **télécommande neuve** | Format de l'étiquette du PLIP, pas une preuve de lecture ECU |
| **20 caractères** | Un outil de programmation décrit l'emploi des 20 caractères centraux de l'étiquette 22 caractères | **Format d'entrée propre à cet outil** ; ne pas généraliser à tous les logiciels |
| **28 caractères hexadécimaux** | Données de télécommande extraites d'une SCU d'origine, utilisables par un spécialiste pour transférer une association vers une autre SCU | **Ce n'est pas** le code-barres d'une télécommande neuve |

**Sources :**
- Technozen (spécialiste SCU) : https://www.technozen-electronics.co.uk/mg-rover-pektron-scu-information/
- Manuel « Rover / MG Key fob (Plip) programming » : https://manualzz.com/doc/23028112/rover---mg-key-fob--plip--programming
- Pscan Pektron : https://pscan.uk/features/pektron.html

Un autre écart de vocabulaire doit rester explicite :
- Technozen écrit qu'un **PLIP Pektron d'occasion** ne peut généralement pas être reprogrammé par la **procédure usuelle**, mais indique qu'un spécialiste peut **transférer les données** d'une SCU à une autre.
- Le manuel indique qu'un PLIP **déjà associé à la SCU et encore fonctionnel** peut être réappris sans fournir à nouveau son code-barres.

Il ne faut donc pas confondre **réapprentissage d'un ancien PLIP légitime**, **programmation d'un PLIP d'occasion provenant d'un autre véhicule**, **création d'un nouveau PLIP** et **transfert expert de données entre boîtiers**.

### 5.1 Conditions de diagnostic Pektron

Technozen confirme qu'une SCU **armée** peut refuser les communications diagnostiques. Il mentionne aussi des défaillances matérielles du **récepteur radio** et des **relais** : toutes les défaillances de la fermeture centralisée n'impliquent pas une erreur de programmation PLIP.

**Conséquence pour Manager plus tard :** le module Pektron doit avoir une stratégie de diagnostic propre, sans déduire « calculateur absent » du simple refus d'une tentative. Il ne faut pas importer les commandes Lucas 5AS dans une SCU Pektron.

## 6. Lucas 10AS : indice de compatibilité, mais pas confirmation du même câblage

Le catalogue d'Andrew Revill désigne une application **« Lucas 5/10AS Tools »** associée au **câble spécialisé 5AS**, et ses outils ECU offrent des familles moteur **K-Series 5/10AS**. Cela indique qu'au moins certains échanges sont proches du 5AS.

**Mais** cela ne démontre PAS :
- que la prise véhicule du 10AS est brochée comme l'OBD 1/3 du 5AS ;
- que toutes les versions 10AS utilisent le même réveil **0x1C** ;
- que le 10AS ancien **vert** et le 10AS récent **noir** partagent le même logiciel diagnostique ;
- que le raccordement automatique du câble 5AS sera valide pour toutes les Defender/Discovery.

Source pour l'intitulé de la suite : https://andrewrevill.co.uk/shop.htm  
Étude des variantes 10AS : voir le rapport **20**.

**Action documentaire future :** trouver le schéma de faisceau authentique **par véhicule / millésime** et une trace de dialogue 10AS avant d'écrire une procédure de modification de câble 10AS dans le programme.

## 7. Modifications proposées au texte de l'aide câble d'une VERSION ULTÉRIEURE (non réalisées ici)

Ces informations ne sont **pas** intégrées dans le BUILD 188 actuel : il s'agit de recherches postérieures, à vérifier avant l'ajout de contenu à l'aide.

| Fenêtre | Élément qu'il serait utile de documenter | Source |
|---|---|---|
| **Airbag RC5** | Conserver la distinction broche 13 / broche 7 et le contrôle de câblage, sans généralisation aux autres véhicules | Rapport 17 ; pscan |
| **Antidémarrage 5AS** | **Deux opérations distinctes** pour adaptation KKL : inversion RXD FTDI/CH340G et permutation OBD 7↔3, 15↔1 | Andrew Revill |
| **Antidémarrage 5AS** | Variante **câble prêt à l'emploi**, et **répartiteur à deux sorties séparées** avec identification moteur / 5AS | Catalogue Andrew Revill |
| **Clés Lucas 5AS** | Réutilisation du même câble spécialisé 5AS que pour le diagnostic ; aucune nécessité d'un clonage radio 433 MHz | Andrew Revill / RoverMEMS |
| **Clés Pektron** | Code-barres **22 caractères**, saisie **20 caractères** seulement dans un outil identifié, données SCU **28 caractères hex** : trois formats distincts | Technozen / manuel |
| **10AS / Pektron** | Indiquer **brochage non suffisamment confirmé**, plutôt que prétendre qu'un câble 5AS convient universellement | Documentation disponible |

**Contrainte permanente :** la version 188 compile les trois aides indépendantes. Le code existant, les tests RC5 et les interfaces moteur doivent rester inchangés pendant cette phase de recherche. Une modification de l'aide ne devra être envisagée qu'avec une nouvelle demande ou un lot explicite validé.

## 8. Sources consultées et niveau de preuve

**Sources primaires — auteurs des outils / matériels**
1. Andrew Revill, description illustrée de la conversion FT232R et CH340G, remappage OBD et 5AS :  
   https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
2. Andrew Revill, catalogue de câbles moteur, câble 5AS adapté et répartiteur double sortie :  
   https://andrewrevill.co.uk/shop.htm
3. Andrew Revill, OBDII Pairing Pod (publication 11/12/2025), matériel 5AS avec K émission et L réception inversée :  
   https://www.caterhamlotus7.club/forums/topic/279789-obdii-pods-ecu-immobiliser-key-pairing-and-much-more/
4. James Portman, brochage 16 voies :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md
5. James Portman, brochage 3 voies :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md
6. James Portman, protocole 5AS et service de statut :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md
7. Technozen, famille Pektron, formats PLIP et cas de panne :  
   https://www.technozen-electronics.co.uk/mg-rover-pektron-scu-information/
8. Blackbox Faultmate SM034, caractéristiques 5AS :  
   https://www.blackbox-solutions.com/site/product?sku=SM034

**Sources tierces / rapprochements à ne pas transformer en spécifications**
9. Manuel Rover/MG PLIP (reproduction, règles 22/20 et huit PLIP 5AS à confirmer) :  
   https://manualzz.com/doc/23028112/rover---mg-key-fob--plip--programming
10. pscan Mini, documentation connecteur vert 5AS :  
    https://pscan.uk/features/mini.html
11. pscan Pektron, conditions d'association :  
    https://pscan.uk/features/pektron.html
12. FTDI FT_PROG (utilitaire officiel uniquement pour circuit authentique compatible) :  
    https://ftdichip.com/utilities/#ft_prog

**Niveau de certitude :**
- **Confirmé textuellement** : paires de broches OBD 7/15 et 3/1, nécessité d'inversion RXD, adaptations citées par Revill, câble prêt à l'emploi, existence du POD, formats distincts Pektron.
- **Indications à confirmer selon l'appareil** : succès réel de la modification d'un FT232R/CH340G particulier ; disponibilité commerciale ; nombre maximal de PLIP selon variante 5AS ; brochage et interface précise du 10AS.
- **Inconnu / à ne pas inventer** : compatibilité du POD USB-C avec le protocole PC de Manager, reconnaissance universelle d'un câble USB à partir du seul VID/PID, trame de programmation 10AS/Pektron non publiée.

## 9. Conclusion opérationnelle

La prochaine fenêtre d'aide câble du 5AS peut être **beaucoup plus précise** que la simple mention « câble spécial requis » : des **adaptations publiées et photographiées** existent pour des matériels précis, ainsi qu'une **solution prête à l'emploi**. Leur fonctionnement pour **Manager** n'est toutefois pas démontré tant qu'un véritable 5AS n'a pas répondu.

Pour les télécommandes **Pektron**, trois formats d'information sont désormais distingués, ce qui évitera de demander à tort un code de 28 caractères pour une étiquette neuve de 22 caractères. Les deux familles devront rester indépendantes.  
**Documentation enregistrée uniquement ; aucun programme, aucun Setup, aucun build modifié pour cette étude.**
