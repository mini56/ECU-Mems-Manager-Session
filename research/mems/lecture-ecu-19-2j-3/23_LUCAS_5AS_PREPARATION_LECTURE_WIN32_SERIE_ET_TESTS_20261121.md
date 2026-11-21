# Rapport 23 — Lucas 5AS : étude de connexion Windows, lecture du statut et premiers tests de protocole

**21 novembre 2026 — ECU-Mems-Manager-Session.**  
**Branche source conservée :** `fix-documentary-additive-session-161-20261118`.  
**Contexte :** SESSION **188** a terminé avec succès son Setup et son test d'installation. La demande suivante concerne **le second bloc « Antidémarrage »** de la page « Autres calculateurs », pour le **Lucas 5AS**. Aucun module de programmation de clé ni d'édition de mémoire ne doit être introduit dans le premier lot.

## 1. Sources réellement inspectées

**Source primaire 5AS :** James Portman, `james-portman/rover-mems-documentation`, fichier `Immobiliser/5AS/diagnostics and configuring.md`. SHA GitHub du blob vérifié : `79290af8887cdc1f069d4d86e6f613ccc9e4a978`.  
URL : https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md

**Câblage** : `Cables/16-pin-5AS-cable.md` (blob `d8c9026f77ae4961fed18347a9fd830b80c86f5d`) et `Cables/3-pin-5AS-cable.md` (blob `6eb729dee697a849af86cae020ac0437700d7055`) du même dépôt.

**Outil fonctionnel externe** : Andrew Revill, MEMS Mapper / Lucas 5AS Tools, description du diagnostic et adaptation KKL : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm

**Code de connexion 5 bauds effectivement lu** : James Portman, RoverMEMS Agent, `ecu-19.go`, SHA vérifiée `f804e098dd71c4fd37a00bc7ad2cafc6fe9834ba` : https://github.com/james-portman/rover-mems-agent/blob/master/ecu-19.go  
**Limite :** il s'agit du **MEMS 1.9**, pas d'un pilote Lucas 5AS. Son système de génération des bits est une référence d'**implémentation du transport 5 bauds**, pas une preuve qu'il faut copier son handshake tel quel.

**Code existant de Manager vérifié** :
- `memsinterface_dispatch.cpp`, fonction `MEMSInterface::performMems19Wakeup`, deux chemins : Win32 natif `SetCommBreak/ClearCommBreak` sur le handle ouvert, et Qt `QSerialPort::setBreakEnabled`. Les 10 bits sont programmés sur une horloge monotone à 200 ms par bit.
- `mems19protocol.h`, vecteur de bits pour l'adresse moteur `0x16`, validation des octets de clé ISO9141, complément de clé et confirmation adresse.
- `serialadapterdetector.h`, inventaire des ports Windows COM avec FTDI/CH340 et informations VID/PID **sans conclure à la compatibilité physique du câblage**.
- `rc5protocol.h`, travailleur de diagnostic série indépendant, QThread et gestion d'annulation, sans interférer avec le moteur.
- `othercontrolunitstab.h`, trois colonnes égales, le RC5 seul actif avant le futur module 5AS ; aides câbles déjà ajoutées par SESSION 188.

## 2. Connaissances 5AS directement établies

| Partie | Octets/donnée | Statut de preuve |
|---|---|---|
| Réveil du mode normal | Adresse **0x1C à 5 bauds** | Documenté par Portman |
| Réponse à l'initialisation | **55 83 76** | Documenté par Portman |
| Vitesse nominale de dialogue | **9 600 bauds** | Documentée, avec mention d'une variante 1 200 à élucider |
| Demande d'un paquet de statut | **83** | Documentée |
| Sortie du mode diagnostic | **F6** | Documentée comme « reset/disconnect » |
| Réveil d'un mode d'édition EEPROM | Autre adresse, distincte du mode normal | Documenté, **délibérément exclu de ce chantier** |
| Commandes d'apprentissage de télécommande | Famille séparée, pas un service de lecture | Documenté, exclu du premier lot |

**Attention aux niveaux physiques :** la prise OBD 16 contacts documentée reçoit la commande sur **OBD 3** (entrée 5AS / C61-16) et répond sur **OBD 1** (sortie 5AS / C61-24), cette dernière **inversée** et de l'ordre de 0–12 V. Il faut le bon câble 5AS adapté, même lorsqu'une interface FT232R ou CH340 apparaît en COM dans Windows. Le fil de la prise moteur n'est pas interchangeable sans adaptation. Voir rapports 21 et 22 pour la fabrication.

### 2.1 Bits exacts à générer par BREAK pour l'adresse 0x1C

Le signal d'initialisation asynchrone à 5 bauds comprend **10 périodes de 200 ms** : un bit de démarrage **0**, les huit bits de `0x1C` **du bit faible vers le bit fort**, puis un bit d'arrêt **1**.

| Période (indice 0..9) | Type | Niveau logique | Durée |
|---|---|---|---|
| 0 | start | 0 | 200 ms |
| 1 | bit 0 | 0 | 200 ms |
| 2 | bit 1 | 0 | 200 ms |
| 3 | bit 2 | 1 | 200 ms |
| 4 | bit 3 | 1 | 200 ms |
| 5 | bit 4 | 1 | 200 ms |
| 6 | bit 5 | 0 | 200 ms |
| 7 | bit 6 | 0 | 200 ms |
| 8 | bit 7 | 0 | 200 ms |
| 9 | stop | 1 | 200 ms |

Ces bits sont calculés par le nouveau `Lucas5AsProtocol::slowBitHigh()`. Dans le KKL moteur, `BREAK activé` commande habituellement l'état logique bas. **Cette correspondance physique devra être vérifiée pour le câble 5AS modifié** ; ne pas conclure que le seul appel Windows `SetCommBreak` prouve le fonctionnement sur la ligne OBD 3. Les temps doivent être pris à partir d'une horloge unique : une succession naïve de 10 temporisations indépendantes cumule le temps d'appel USB et dérive.

### 2.2 Contrôle d'écho et confirmation : point encore ouvert

Le code MEMS 1.9 traite le dialogue ISO 9141 :
- attend `55` plus deux octets de clé,
- renvoie l'inverse de la deuxième clé,
- attend le complément de l'adresse initiale, avec gestion de l'écho local éventuel.

**Pour le 5AS, Portman confirme explicitement seulement `55 83 76` après l'adresse `1C`.** L'usage strictement analogue d'un ACK `~76 = 89` puis d'une confirmation `~1C = E3` est **une hypothèse de transposition ISO 9141**, pas une séquence Lucas 5AS démontrée par la source inspectée. Il faut une **trace d'échange réel** ou un code 5AS complet pour décider :
- si la confirmation par complément est requise ;
- ses délais et éventuels octets supplémentaires ;
- la présence ou non d'un écho, et sur quelle ligne K ou L ;
- si `0x83` attend une terminaison/ack supplémentaire ;
- le format exact (parité, contrôle de trame) des variantes.

**Décision de fiabilité :** la future interface ne devra afficher « 5AS connecté » **qu'après réception et interprétation d'une vraie réponse du boîtier** ; jamais sur la seule présence d'un COM ou sur une réponse synthétique. Le protocole 5AS ne doit pas partager les commandes RC5 ni la session moteur MEMS 1.9, même s'il réutilise les principes techniques du réveil.

## 3. Paquet de statut 5AS : extraction vérifiée contre les offsets Portman

Portman indique que l'**octet 0** représente la **longueur totale du paquet, octet 0 compris**. L'exemple documenté contient au moins 49 octets, car l'identification ASCII se termine à l'indice **48**.

| Indice(s) | Lecture retenue | Qualification |
|---|---|---|
| 0 | taille du paquet | Trame incomplète refusée |
| 1, bit 7 | entrée capot | Ne pas supposer que bit 7 = « ouvert » sans source |
| 6, bit 7 | contact/allumage | Valeur brute 0/1 |
| 9, bit 7 | portes | Valeur brute 0/1 |
| 11, bit 7 | coffre possible | Le qualificatif **« (?) »** est dans la source |
| 16–19 | 4 octets bruts de réception PLIP | Pas des compteurs permanents fiables |
| 20–22 | numéro série, ordre faible vers fort | Conserver les octets ; pas un EKA |
| 35–48 | texte ASCII de type de boîtier, exemple `AP3.005AS-98MY` | Uniquement si réellement présent |
| Tous autres indices | inconnus | Ne pas inventer de libellé ou de conversion |

La seule longueur ne permet **pas** de prouver un checksum, qui reste non établi. Il ne faut pas imposer celui du RC5 ou celui du MEMS 2J.

### 3.1 Modèle de décodage proposé

- Conserver le paquet reçu **exactement** en brut.
- Ne traiter que les champs dont les octets existent dans la trame complète déclarée.
- Ignorer au besoin l'écho **83** au début, pas à l'intérieur d'une trame valide.
- **Ne pas scanner arbitrairement** les octets du contenu tronqué en les prenant pour de petites longueurs : des compteurs PLIP situés à l'intérieur pourraient être faussement pris pour une réponse.
- Ne jamais interpréter les quatre compteurs PLIP comme des preuves de programmation de télécommandes ou du bon état de synchronisation moteur.
- Ne jamais lire EKA, emplacements mémoire sensibles ou informations destinées à contourner l'antidémarrage dans le premier module de diagnostic.

## 4. Travail de code déjà enregistré dans l'unique branche

### 4.1 `lucas5asprotocol.h` — créé

Décodeur **pur, sans accès série** :
- constantes `0x1C`, `55 83 76`, `9600`, `83`, `F6`;
- vecteur 5 bauds avec les dix bits de 200 ms ;
- identification du réveil normal **distinct** du réveil de mode mémoire `55 83 F7` ;
- décodage des entrées documentées, quatre octets PLIP, numéro de série brut, identification ASCII ;
- contrôle de longueur, conservation de données inconnues et gestion des trames partielles ;
- extraction d'une trame après écho ou zéro de préfixe, sans recherche dangereuse d'une fausse longueur dans la charge utile.

**Aucune commande EEPROM ou apprentissage n'est présente dans ce décodeur.**

### 4.2 `lucas5asprotocol_test.cpp` — créé

Tests unitaires avec **trames fabriquées**, indépendants d'un câble et d'un véhicule :
1. Les dix bits du réveil `1C` dans le bon ordre.
2. La réponse normale `55 83 76`, distincte d'une réponse de mode mémoire.
3. Un paquet synthétique de 49 octets avec les indices 1/6/9/11, 16–19, 20–22, 35–48.
4. Les réponses tronquées, vides, de longueur incohérente et au-delà de la limite de taille.
5. La présence d'un écho et le refus de décoder une trame encore incomplète.
6. La conservation des octets bruts et des champs non interprétés.

**Ces tests ne sont pas une preuve de communication physique réelle**, mais évitent de confondre ces trois familles de protocoles.

### 4.3 `CMakeLists.txt` — enregistrement de l'autotest

Le test `lucas5as_protocol_selftest` a été ajouté comme cible **Qt5::Core** et étape de non-régression pour la **prochaine compilation** de SESSION. Il reste indépendant du module RC5, de ROSCO, de MEMS 1.9, de l'IA et des bases documentaires.  
**Statut réel :** enregistré dans les sources ; **pas encore compilé/validé** sur le runner Windows lors de la rédaction de ce rapport.

## 5. Prochaine étape : passer du décodeur au vrai bouton 5AS

État de fonctionnement attendu dans la seconde colonne, une fois le pilote prêt :

1. **Sans câble USB** → « Aucun port série détecté » ; pas de tentative de connexion fictive.
2. **Avec câble USB non raccordé au 5AS** → essayer le port, puis « Aucune réponse Lucas 5AS valide » ; ne pas déclarer connecté.
3. **Avec câble 5AS matériellement compatible et boîtier réellement présent** → 5 bauds `1C`, dialogue de réveil confirmé, lecture `83`, identification et états affichés uniquement à partir de la réponse réelle.
4. **Déconnecter** → libérer le port, rétablir BREAK inactif et garder les autres sessions indépendantes.
5. **Rafraîchir les états** → nouvelle commande de lecture normale, sans apprentissage ni modification mémoire.

Implémentation à isoler dans une **session/worker Lucas 5AS**, avec QThread et annulation, à l'image de la structure RC5 **sans en recopier les octets de protocole**. La liste des ports COM vient de `SerialAdapterDetector`, mais **le VID/PID n'est pas une preuve que les lignes du KKL ont été inversées et raccordées comme il faut**.

**Point bloquant pour annoncer une prise en charge confirmée :** on ne possède pas encore de **capture complète d'un vrai réveil 5AS jusqu'à la réponse au service 83** avec le câble choisi. Le logiciel peut être préparé et testé sur données synthétiques, mais la réussite physique du dialogue doit être confirmée avec un module 5AS ou une référence publique comportant les échanges précis.

## 6. Ce qui n'a PAS été modifié

- Aucune nouvelle branche.
- Aucune compilation supplémentaire ni nouveau Setup pour les recherches 5AS de ce rapport.
- Aucune commande programmant les télécommandes, synchronisant le 5AS avec l'ECU moteur, changeant l'EKA, écrivant en EEPROM ou modifiant les alarmes.
- Aucune modification du protocole moteur 1.6/1.9/2J/3, de ROSCO, du RC5 ni de la structure en trois colonnes.
- Aucune modification des bases documentaires ni des Releases publiques de mise à jour.

**Conclusion :** le principe de réveil et le parseur de lecture normal sont identifiés et enregistrés, avec une base de tests à exécuter lors du prochain build. **L'étape suivante est le vrai transport série 5AS**, une fois son échange de confirmation recherché ou ses hypothèses explicitement soumises au test matériel.
