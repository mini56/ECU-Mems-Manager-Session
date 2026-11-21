# Recherche complémentaire — Lucas 5AS, télécommandes PLIP, Pektron SCU et 10AS

**21 novembre 2026 — ECU MEMS Manager SESSION**  
**Portée : recherche pour les deuxième et troisième colonnes « Antidémarrage » et « Programmation des clés ».**  
**Aucune modification du programme, de l'interface, des protocoles ou du workflow ; pas de nouvelle compilation.**  
Branche existante : `fix-documentary-additive-session-161-20261118`.

## 1. Résultats exploitables immédiatement

Il faut distinguer trois opérations incompatibles entre elles :

1. **Diagnostic et états** du boîtier antidémarrage, sans programmation.
2. **Apprentissage des télécommandes PLIP** par le boîtier correspondant.
3. **Association entre antidémarrage et ECU moteur**, différente de l'apprentissage PLIP et de la reproduction radio.

Une télécommande à **433,92 MHz** n'est pas automatiquement compatible avec une autre télécommande sur 433,92 MHz : il faut vérifier la génération, les caractéristiques radio et le protocole d'appairage. La copie d'un signal enregistré une fois ne remplace pas le mécanisme de synchronisation à code tournant.

## 2. Lucas 5AS — des fonctions de lecture effectivement documentées

La documentation technique de James Portman explique deux séquences d'initialisation spécifiques et différents services de diagnostic. Son application 5AS publiée indique les champs suivants, sans prouver que chaque variante du boîtier les expose :

- Identification du module / numéro de série et version.
- Contacteur de capot, coffre, portes et état du contact (allumage).
- Compteurs de quatre emplacements de télécommandes.
- Paramètres de verrouillage, armement passif, superverrouillage, temporisations et témoins.
- Données mémoire contenant notamment des identifiants PLIP ; **les informations d'accès d'urgence EKA sont sensibles et ne doivent pas être consignées dans un journal brut ni exposées par défaut**.
- Fonctions de test sortie (avertisseur, DEL, clignotants sur certaines variantes) documentées ; leur existence ne constitue pas une autorisation de les activer automatiquement.

**Données de protocole publiées (pour analyse technique ultérieure, pas intégrées dans SESSION) :**

- Réveil à **5 bauds**, mode normal : octet `1C`, réponse de démarrage `55 83 76`.
- Mode d'accès mémoire distinct, qui ne doit pas être activé par défaut.
- Communication généralement à **9 600 bauds** après initialisation (la source évoque aussi 1 200 bauds dans certains cas).
- Demande de données en mode normal : service `83`. Une partie du paquet est documentée, **de nombreux octets restant inconnus**.
- Les commandes d'apprentissage des télécommandes et les commandes de modification des paramètres sont décrites dans la référence, mais la séquence complète dépend du boîtier ; elles ne font **pas** partie du simple module de lecture.

**Attention :** la documentation « Lucas 5AS Tools » d'Andrew Revill précise que certains identifiants n'existent que sur les modèles **98MY et suivants**. Elle précise aussi que certains octets des compteurs PLIP sont **transitoires** : leurs valeurs ne sont interprétables qu'immédiatement après une pression sur la télécommande. Ne pas considérer ces octets comme des compteurs permanents ou un inventaire fiable à tout instant.

**Conséquence pour Manager :** prévoir d'abord, côté Antidémarrage, une session 5AS indépendante des protocoles moteur et des autres familles ; afficher version/identification et états réellement documentés, marquer « non disponible » pour les valeurs impossibles à déterminer.

### 2.1 Une contradiction utile à conserver : historique des alarmes

- Faultmate SM034 annonce une mémoire des **cinq derniers déclenchements d'alarme**.
- Sur le forum Pscan, le développeur précise que **Pscan ne propose pas la lecture de cet historique**, seulement les états de plusieurs entrées. Un participant affirme même que le 5AS n'a pas d'historique accessible, ce qui **contredit** la fiche Faultmate.
- Il ne faut donc pas promettre la fonction « Historique des 5 alarmes » dans Manager tant que la compatibilité par variante et les commandes de lecture n'ont pas été démontrées.

Cette distinction évite de confondre *capacité revendiquée par un outil professionnel*, *capacité de Pscan*, *présence effective dans la variante du véhicule* et *protocole disponible pour Manager*.

## 3. Familles précises de 5AS : génération initiale versus 98MY

Les témoignages techniques et la documentation des boîtiers distinguent :

| Variante | Indice physique | Fonctionnalités / restrictions |
|---|---|---|
| **5AS ancien** | Principalement **une seule fiche grise** | Identification et certaines fonctions plus limitées |
| **5AS 98MY et suivant** | Souvent **deux fiches, grise et blanche** | Fonctions supplémentaires selon câblage : clignotants, anneau d'armement passif, éclairage intérieur |

**Important :** c'est un *indice de génération*, non un critère logiciel suffisant. Seule une réponse d'identification fiable doit définir les capacités proposées.

Sources : Andrew Revill (différences 98MY), forum Rover 200/400 (reconnaissance des deux générations), forum Pscan.

## 4. Câble Lucas 5AS : transport très différent du KKL moteur et du RC5

Documentation électrique originale de James Portman pour certaines prises 16 voies :
- **Broche OBD 1** → transmission depuis 5AS vers l'outil (réception outil).
- **Broche OBD 3** → réception 5AS depuis l'outil (émission outil).
- Signaux diagnostiques d'ordre **0–12 V**, de logique spécifique, réception du 5AS **inversée**.
- **Ce n'est pas du RS232 PC classique ni une ligne moteur KKL 5 V.**
- Les anciens montages à **prise verte 3 broches** doivent être distingués du connecteur blanc des anciens calculateurs moteur ; les tensions diffèrent.

**Conclusion :** le fait de détecter un convertisseur USB–série COM n'indique ni que les bons contacts sont reliés ni que le câble réalise l'adaptation électrique nécessaire. Ne jamais envoyer une séquence de diagnostic 5AS ou RC5 sur un port supposé équivalent parce qu'il possède le même VID/PID USB.

La prise d'aspect OBD ne signifie d'ailleurs pas que tous ces systèmes parlent le protocole OBD2 réglementaire.

## 5. Télécommandes Lucas 5AS — types et radio

Types régulièrement mentionnés : **3TXA**, **3TXB**, **3TXC**, **3TXD**, **17TN**. Les fréquentes familles RF sont **433,92 MHz** et **315 MHz**, selon le marché et le boîtier.

Le développeur Pscan écrit qu'il a rencontré plusieurs types de PLIP, mais que **la compatibilité exacte entre les variantes et les versions de 5AS n'est pas parfaitement établie**. Des propriétaires signalent notamment des problèmes de compatibilité des **3TXA avec certains 5AS récents**. Un autre contributeur avance une matrice partiellement différente : ne surtout pas transformer ces discussions en règle absolue sans validation.

La seule mention « compatible Rover » ou « 433 MHz » dans une annonce de vente ne prouve rien. Des spécialistes recommandent de vérifier le résonateur RF et les indications du boîtier : la couleur ou le marquage de coque peut avoir été modifié.

**Jalon de la future colonne Clés :**
- Afficher d'abord l'identification du 5AS et sa génération **si l'information est accessible**.
- Faire distinguer **type de PLIP** (3TXA/3TXB/…), **fréquence vérifiée** et **nombre d'emplacements déclarés** ; aucun appairage automatique pendant la connexion.
- L'apprentissage est une **opération sur la mémoire du boîtier**, pas un « clonage » radio universel.
- Pouvoir reconnaître une télécommande existante selon les données effectivement accessibles et le contexte d'une pression ; ne pas inventer un compteur de réception permanent.
- Conserver l'indépendance de l'association 5AS ↔ ECU moteur, du diagnostic et de l'apprentissage PLIP.

**Les PLIP Lucas d'occasion compatibles semblent pouvoir être réappris** ; cette possibilité n'est pas transposable à Pektron.

## 6. Pektron SCU — différences majeures découvertes

Le spécialiste Technozen décrit l'arrivée du **Pektron SCU en 2003** sur différents modèles MG Rover, en remplacement de plusieurs boîtiers précédents. C'est une *autre famille de calculateur*, pas une déclinaison logiciel 5AS.

- **Jusqu'à 10 télécommandes** pour certaines SCU Pektron (contre **4** pour 5AS selon Faultmate).
- Télécommandes neuves : la programmation usuelle requiert un **code fourni avec la télécommande**, souvent décrit comme un **code-barres de 22 caractères**.
- Une télécommande d'occasion Pektron ne peut généralement **pas** être présentée comme réinscriptible avec une simple procédure véhicule. Certaines interventions spécialisées de migration de données de SCU existent, mais ce n'est pas de la programmation usuelle et doit être étudié séparément.
- **La SCU Pektron armée refuse certaines communications diagnostiques**. Le comportement « port accessible mais pas de réponse » n'a donc pas la même signification que sur 5AS.
- Des variantes/références de SCU existent (`YWC001540`, `YWC001541`, `YWC001551`), avec particularités à ne pas généraliser.
- Une seconde fréquence **315 MHz** existe également pour certains marchés, distincte des versions **433 MHz**.

**Seuils de production indiqués par Technozen (à confirmer par VIN et boîtier sur chaque véhicule) :**
- Rover 25 / MG ZR / Streetwise : à partir de **VIN RF732814** ;
- Rover 45 / MG ZS : à partir de **VIN RT637209** ;
- MG TF : à partir de **VIN RD620459**.

Ces bornes désignent la transition **d'antidémarrage/boîtier habitacle**, **pas** la transition des boîtiers airbag RC5 vers un autre SRS. La colonne RC5 ne doit pas interpréter ces VIN Pektron comme une règle airbag.

### 6.1 Différence importante entre logiciels de diagnostic

Pscan indique pouvoir lire et programmer des fonctions 5AS et Pektron, mais les opérations sont différentes. Selon son forum, son support 5AS **ne donne pas accès à l'historique des déclenchements d'alarme** ; Faultmate SM034 le revendique pour certaines versions.

**Conséquence :** documentation et tests séparés par logiciel / variante. Une fonction disponible dans Faultmate ou un outil atelier ne signifie pas automatiquement que les trames correspondantes sont publiées et utilisables dans Manager.

## 7. Land Rover 10AS — branche de recherche encore distincte

Pscan liste le **Lucas 10AS** notamment sur Land Rover Defender et propose des fonctions de lecture/programmation spécifiques. La couverture pscan aurait été ajoutée en **septembre 2024** selon ses notes publiques. Cela montre la faisabilité générale, **pas** l'interchangeabilité des trames avec Lucas 5AS.

**Manquent encore** pour cette famille : séquence de connexion démontrée en source publique, brochages par génération, identifiant calculateur, définition des lectures et procédures de télécommandes propres au 10AS. Aucun protocole 10AS ne doit être créé par déduction à partir du 5AS.

## 8. Ordre proposé pour la recherche et le futur développement

| Étape | Module | Ce qui est réellement vérifiable |
|---|---|---|
| A | **5AS lecture uniquement** | Câble spécifique, réveil normal, réponse authentique, identification / états documentés |
| B | **5AS diagnostic complémentaire** | États d'alarme, comptage des PLIP après pression, variantes anciennes / 98MY ; historique d'alarmes uniquement s'il est démontré |
| C | **5AS apprentissage PLIP** | Identification de génération et fréquence, méthode documentée, contrôle du résultat, état des autres télécommandes |
| D | **Pektron SCU lecture** | Identification de la SCU, condition de communication quand elle est armée, états et particularités matériels |
| E | **Pektron SCU clés** | Compatibilité des PLIP neufs, codes associés et procédure distincte |
| F | **10AS** | Recherche de source/protocole spécifiques puis conception indépendante |

Ne pas ajouter de boutons 5AS/Pektron/10AS « actifs » dans SESSION sur la seule base de cette étude.

## 9. Sources vérifiées et références

1. James Portman, documentation diagnostic 5AS et commandes/format de réponse : https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md
2. James Portman, brochage OBD 16 voies 5AS : https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md
3. James Portman, prise 3 voies 5AS : https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md
4. James Portman, application 5AS et champs affichés : https://rovermems.com/5as/application.html
5. Andrew Revill, logiciel 5AS : lecture, identification, compteurs transitoires, variantes 98MY : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
6. Blackbox Solutions Faultmate SM034, capacités des variantes, cinq événements d'alarme, nombre de PLIP : https://www.blackbox-solutions.com/site/product?sku=SM034
7. Forum Pscan, compatibilité des PLIP 3TXA/3TXB/3TXC/3TXD/17TN **discutée et non totalement résolue** : https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/97206-fob-programming-5as
8. Forum Pscan, **historique des alarmes indisponible dans Pscan** malgré la fiche Faultmate : https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/98318-interrogating-lucas-5as
9. Rover 200/400 Owners Club, variante à une ou deux fiches : https://rover200.org.uk/forums/topic?p=105086
10. MGFCar, indices RF résonateur, marquages 315/433 : https://www.mgfcar.de/alarm_blipper/eblipper.htm
11. Technozen, Pektron SCU : modèles, seuils VIN, caractéristiques, jusqu'à 10 télécommandes : https://www.technozen-electronics.co.uk/mg-rover-pektron-scu-information/
12. Pscan, présentation de la couverture 5AS : https://pscan.uk/features/5as.html
13. Pscan, présentation de la couverture 10AS : https://pscan.uk/features/10as.html
14. Pscan, présentation de la couverture Pektron : https://pscan.uk/features/pektron.html
15. SP Diagnostics, manuel iKEYS, séparation programmation PLIP / jumelage ECU moteur et limites Pektron : https://spdiagnostics.com/keyscan/downloads/Documents/iKeys%2Bdocument%2B-%2Bcomplete%2B%2823.2%29.pdf

## 10. Conclusions pour ECU MEMS Manager

Nous possédons **suffisamment de connaissances publiées pour préparer proprement un pilote de lecture 5AS**, à condition de ne pas le confondre avec les trames moteur et de vérifier le câble. Les commandes d'apprentissage existent en documentation mais **leurs conditions et le comportement des variantes demandent encore un relevé précis**.

Le **Pektron SCU** et le **10AS** demandent des recherches de protocoles indépendants. Les modèles de télécommandes, leur fréquence et leur état (neuf / d'occasion) doivent être traités **par famille**.

**Aucun changement à SESSION 187 n'a été effectué pour cette étude.** Cette fiche est une référence préparatoire que nous pourrons utiliser pour le bloc Antidémarrage et la colonne Clés après validation du développement RC5.
