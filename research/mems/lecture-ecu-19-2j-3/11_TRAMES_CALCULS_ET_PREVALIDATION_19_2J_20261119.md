# MEMS 1.9 / 2J — vérification documentaire des trames, calculs et prérequis matériels

Date : 19 novembre 2026. **Vérification statique et arithmétique terminée, pas d'essai sur ECU réel.** Travail sur documentation uniquement.
Source RoverMEMS figée : b381af39b2031c80303c5262055aa0c0121dbdd4, licence GPLv3.
**Ne pas modifier ECU MEMS Manager SESSION, sa sélection USB, ses connexions, ses pages ou ses garde-fous dans cette étude.**

## A. MEMS 1.9 : chemin documenté et comparaison au code Manager

- Matériel : câble KKL K-Line, 9600 bauds, 8 bits, sans parité, 1 bit d'arrêt.
- Réveil ISO9141 à 5 bauds : adresse **0x16** transmise LSB d'abord, avec bit de début 0 et bit d'arrêt 1. Durée de chaque bit **200 ms**. Une trame = 10 bits soit environ **2000 ms**, sans compter le repos initial.
- Réponse classique **55 76 83** : 55 octet de synchronisation, 76 et 83 octets-clés.
- Le complément bit à bit du second octet-clé 83 est **7C**. Après 7C, confirmation **E9** (= complément 0x16).
- Puis protocole ROSCO de lecture (identification et mesures). Il ne faut pas confondre le réveil ISO9141 avec la structure des réponses ROSCO.

**Manager, code existant vérifié (memsinterface_dispatch.cpp) :**
- performMems19Wakeup() émet le start/8 bits/stop avec QSerialPort::setBreakEnabled(), et attend le 0x55 + 2 clés ; il utilise le complément réel du second octet (et non 7C codé en dur), puis attend E9.
- Après confirmation, il ferme le port, puis connectToECU() rouvre via tryRoscoConnect() ; il reste **à mesurer** si la transition de port respecte le délai du calculateur réel.
- La connexion tente aujourd'hui ROSCO sur **tous** les adaptateurs avant la branche 1.9 : non conforme à la sélection par câble physique déjà décidée, mais **aucune correction n'est effectuée maintenant**.
- Il existe un onglet Test ECU 1.9. Aucune trace prouvant la réponse d'un véritable 1.9 avec ce câble précis n'a été retrouvée dans ce dossier.
- Le code RoverMEMS ecu-19.go contient un réveil comparable et utilise le complément 7C comme constante, mais il ne constitue pas une preuve matérielle.

**Point à contrôler au test matériel :** le signal BREAK du CH340 KKL, la synchronisation 55/clé, l'octet E9, la réouverture du port, une identification confirmée et au moins une valeur moteur 0x80 cohérente.

## B. MEMS 2J : séquence, transport, session

**Électrique :** K-Line via câble KKL, **10400 bauds, 8N1**, réveil Fast Init : BREAK actif bas **25 ms**, repos haut **25 ms**, puis commande initiale spécifique. La référence Go effectue 200 ms repos avant le BREAK, 50 ms supplémentaires après le 25/25 avant d'envoyer la commande ; **ce timing additionnel est celui du code étudié**, pas une garantie universelle.

**Initialisation spéciale** : envoi 81 13 F7 81 0C ; réponse attendue **03 C1 D5 8F 28** (hors échos). La structure d'initialisation est spécifique et ne doit pas être interprétée comme une requête 21 xx.

**Trames opérationnelles telles que construites par le Go de James Portman :**
- octet longueur de la charge utile ;
- charge utile (ex. 21 09) ;
- checksum = somme modulo 256 de longueur + tous les octets de charge utile.
- Trames complètes *hors initialisation* : [LONGUEUR][PAYLOAD][SOMME].
- Les réponses ont le même encadrement ; distinguer **écho de la trame émise** et réponse ECU. Un découpage tronqué/invalide ne prouve aucune réponse.
- RoverMEMS documente également une forme d'encadrement général KWP2000 avec format/adresses. **Ne pas fusionner naïvement l'enveloppe générique ISO 14230 avec le sous-format effectivement transmis par le Go après initialisation.** Si une variante émet un format différent, la constater sur trace avant de changer le parseur.

**Session/lectures (charge utile) :**
1. 10 A0 → 50 : lancement diagnostic tel qu'observé par l'auteur.
2. 27 01 → 67 01 + seed 16 bits ; génération de clé via ecu-auth.go, puis 27 02 + clé → 67 02. C'est un **échange faisant partie du protocole ECU**, pas un nouveau « verrou » de Manager. L'utilité/acceptation peut varier selon ECU réel.
3. 3E → 7E : entretien de session observé.
4. 21 19 → 61 19 + masques (défauts) ; 21 09 → 61 09 + régime ; autres 21 XX selon variante.
5. Réponse négative typique : **7F [service refusé] [code de réponse]**, qui doit être reconnue et rattachée à la requête, pas confondue avec une mesure.

Les commandes d'effacement 31 CB, d'antidémarrage 31 D0, et les autres services de modification existent dans le projet de référence, mais **ne sont pas nécessaires à la première intégration de lecture**. Leur existence documentaire ne signifie pas qu'elles doivent être exécutées.

## C. Vecteurs arithmétiques contrôlés hors calculateur

Les valeurs ci-dessous ont été recalculées indépendamment à partir de la définition [LEN][PAYLOAD][SUM]. Elles sont des **exemples synthétiques de vérification**, absolument pas des captures de communication réelle :

| Exemple | Octets complets en hexadécimal | Calcul de checksum / signification |
| --- | --- | --- |
| Réponse réveil 2J | 03 C1 D5 8F 28 | 03+C1+D5+8F=0x228 ; & FF = **28** |
| Requête régime | 02 21 09 2C | 02+21+09 = **2C** |
| Requête défauts | 02 21 19 3C | 02+21+19 = **3C** |
| Requête température eau | 02 21 01 24 | 02+21+01 = **24** |
| Exemple réponse régime 3000 tr/min | 04 61 09 0B B8 31 | 04+61+09+0B+B8=0x131 ; & FF = **31** ; brut 0B B8 = 3000 |
| Exemple réponse eau 20°C | 04 61 01 0B 74 E5 | checksum **E5** ; (0x0B74 - 2732)/10 = 20 |
| Exemple négatif pour service 21 | 03 7F 21 31 D4 | checksum **D4**, structure 7F 21 31. **Exemple fabriqué, pas un refus observé sur notre 2J** |
| Requête présence session | 01 3E 3F | 01+3E = **3F** |

**Attention :** un « checksum correct » sur un exemple fabriqué prouve seulement l'arithmétique, pas la conformité des délais, du câble, du format réel d'une variante ou de la signification des valeurs.

### Réponses à analyser, sans boucles d'échec

- Réponse positive **61 XX** : contrôler cohérence de l'identifiant XX et taille requise avant conversion ; conserver les octets bruts pour la traçabilité.
- Réponse **7F 21 NRC** : ne pas bloquer toute la session parce qu'une mesure optionnelle n'existe pas (en particulier 21 10, 21 25). La noter « indisponible pour cette session/variante » ; traitement à définir lors du développement, **sans verrou ajouté au programme**.
- Codes standard KWP2000 documentés : NRC 11 service inconnu, 12 sous-fonction non prise en charge, 13 format incorrect, 22 conditions/séquence, 31 hors plage, 78 réponse en attente. **Ces codes sont des définitions standard ; leur occurrence sur MEMS 2J reste à observer.**
- Paquets partiels, longueur nulle, longueur invraisemblable, écho, checksum erroné, perte du port, réponse hors séquence : **cas d'analyse technique** indispensables pour ne pas produire de fausses mesures. Le Go contient des TODO à ces endroits (checksum, longueurs, refus, reprise après échec).
- Une indisponibilité de champ ne doit pas devenir « calculateur inconnu » ni imposer de confirmer sa référence dans une base documentaire pour continuer la lecture des autres données.

## D. Analyse vérifiée des limites du code Go de référence

- **ecu-2j.go** : ne contrôle pas la somme de contrôle des réponses (TODO), accepte une taille de trame sans borne maximale définie et redémarre ou saute imparfaitement après refus de mesure ; ouvre une routine de lecture pouvant rester active sur reconnexion.
- **ecu-2j-parse.go** : plusieurs accès par index sont effectués sans vérifier la longueur de charge utile avant l'accès ; les unités de carburant/injecteurs/bobines ne sont pas établies ; la conversion signée de 21 21 a une erreur démontrable ; 21 25 est potentiellement absent sur Mini.
- **ecu-2j-faults.go** : ambiguïté explicite à [12].bit2 et matrice dérivée de RAM Mini MPi, pas preuve d'exhaustivité VVC.
- **ecu-19.go** : référence de réveil 1.9, pas remplacement du code C++ existant.
- **ecu-auth.go** : algorithme seed/key effectivement nécessaire au déroulement décrit par la référence Go ; portage C++ futur uniquement après décision de développer.
- **serialReadWrite.go** : routine de lecture spécifique Go, pas composant Qt ; aucune nécessité de reprendre ses goroutines dans Manager.

## E. Prérequis de validation physique : critères clairs

| Contrôle | 1.9 | 2J | Statut ici |
| --- | --- | --- | --- |
| Détection de l'adaptateur USB connu (COM et VID/PID) | CH340 KKL identifié côté PC | CH340 KKL identifié côté PC | **Constaté**, ne prouve pas le dialogue ECU |
| Ouverture port | 9600 8N1 | 10400 8N1 | QSerialPort sait demander ces réglages ; câble réel **non testé** |
| Réveil actif K-Line | 0x16 à 5 bauds | 25/25 ms BREAK | **Non validé électriquement** |
| Première réponse ECU | 55 + 2 clés, confirmation E9 | 03 C1 D5 8F 28 | **Non capturée sur ECU réel** |
| Mesure réelle | 0x80 / ROSCO | 21 09 / 61 09 | **Non capturée** |
| Défauts et valeurs selon variante | trames 0x80 / 0x7D | 21 19 / 61 19 et 21 XX | **Non comparés à un instrument de référence** |
| Conservation du chemin ROSCO 3 broches | 1.2 / 1.3 / 1.6 | sans impact | **Non réévalué**, aucun code modifié |

**Ce qui est démontré :** formats d'exemples, checksums, opérations arithmétiques, séquences documentaires et écarts de sources.
**Ce qui reste impossible à conclure ici :** capacité réelle du câble CH340 à produire 25 ms de BREAK sur la K-Line, transition 1.9, variantes non représentées et mesures réelles. Ces tests appartiennent à une étape matériel ultérieure.

## F. Sources

- https://rovermems.com/mems-2j/index.html
- https://rovermems.com/diagnostics/technical/
- https://github.com/james-portman/rover-mems-agent/tree/b381af39b2031c80303c5262055aa0c0121dbdd4
- https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
- https://pscan.uk/features/mems2j.html
- https://pscan.uk/features/mems19.html
- https://standards.iteh.ai/catalog/standards/iso/20c9a21d-a095-41d7-81fa-423494aa28e7/iso-14230-3-1999
- Source Manager lue sans modification : memsinterface_dispatch.cpp, memsinterface.cpp, serialadapterdetector.h, protocolcontext.h sur branche fix-documentary-additive-session-161-20261118.

Aucun fichier programme modifié, aucun Setup déclenché, aucune liaison réelle ECU mesurée.
