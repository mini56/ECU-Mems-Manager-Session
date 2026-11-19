# MEMS 2J — audit documentaire des variantes, mesures et défauts

Date : 19 novembre 2026. **Étude documentaire achevée au niveau des sources disponibles ; aucune mesure réalisée sur ECU réel.**
Dépôt de travail : ECU-Mems-Manager-Session, branche existante fix-documentary-additive-session-161-20261118.
Objet : préparer l'intégration en lecture des valeurs/erreurs sans modifier les sources applicatives. Pas de patch, de pilote ajouté ni de run pour cette étude.

## 1. Échelle de confiance utilisée

- **Établi documentaire** : information confirmée directement dans les sources et parfois par une seconde documentation technique.
- **Décodage constaté dans la source** : reproduit fidèlement le comportement du code Go de référence ; ne signifie pas qu'une unité ou une variante a été validée expérimentalement.
- **À confirmer sur ECU** : spécification incomplète, divergence de sources, valeur brute ou comportement spécifique à une variante.
- **Non disponible / non prouvé** : ne pas afficher de valeur inventée, ni interpréter automatiquement un code de défaut incertain.

## 2. Familles et variantes réelles

| Application | Famille / équipement démontré par documents | Particularités pour les mesures |
| --- | --- | --- |
| Mini MPi, fin des années 1990 | MEMS 2J, injection multipoint à **deux injecteurs** sur cette Mini | Commande 21 25 refusée d'après le commentaire du décodeur Go ; 21 10 parfois refusée. Ne pas fabriquer quatre mesures d'injecteurs utilisables parce qu'une réponse 21 0C possède quatre octets. |
| MG F / Lotus Elise moteur VVC, Rover 200 Vi / BRM et autres K-Series VVC EU2 | MEMS 2J avec distribution VVC | Gestion de période d'arbres à cames, capteur de température d'huile et éléments spécifiques VVC ; certains jeux de données peuvent être différents. Aucune correspondance universelle 21 25 = angle réel d'arbre à cames démontrée. |
| MG F 1.8 non-VVC / Elise standard d'origine | **Souvent MEMS 1.9**, et non systématiquement 2J | Ne pas classer une voiture comme « 2J » sur son seul modèle. L'identification passe par une réponse de diagnostic réussie. |
| Autres Rover à motorisations particulières | Liste de compatibilité tierce plus large, avec variantes et calibrations différentes | Pas de matrice exhaustive des identifiants ECU par version logicielle. La présence d'une fonction dépend du calculateur et de sa réponse, pas d'un modèle supposé. |

Informations d'identification documentées dans la fiche Blackbox SM072 : codes de calibrations (5 lettres + 3 chiffres), ex. préfixes ADHM (Mini), KSH3 (MGF), KSHB (Rover 200 Vi), KSH8 (Coupe Vi), KNH7/KPH7 (KV6). **Ces préfixes ne sont pas des identifiants protocolaires universels, et leur accès par une commande 2J donnée n'a pas été prouvé ici.**

Le matériel MEMS 2J typique a deux connecteurs ECU 36 broches (rouge et noir) et une prise diagnostic K-Line 16 broches. La prise de forme OBD ne signifie pas que l'ECU répond aux PID OBD-II universels.

Les capteurs non montés peuvent produire des valeurs incohérentes ou des défauts « présents » : confirmation indépendante pscan, notamment pour les capteurs température de carburant et ambiance. Ne pas traduire automatiquement toute réponse non nulle en panne avérée.

**Conclusion variantes :** Mini MPi et VVC sont des cibles confirmées ; la liste de véhicules de référence est indicative, **pas** une table des commandes garanties. Les options et logiciels ECU restent à découvrir via réponses réelles, sans verrou préalable ajouté.

## 3. Matrice des lectures 0x21/0x61 — source Go figée

La commande se présente, hors initialisation, comme payload 21 XX. Une réponse positive commence par payload 61 XX. Offsets ci-dessous : les positions [2], [3], etc. concernent la **charge utile après retrait du premier octet longueur et du checksum**, donc payload [0]=61 et payload [1]=XX. Les nombres u16 sont BIG-ENDIAN.

| Requête | Valeur selon ecu-2j-parse.go | Conversion/source Go | Unité / confiance |
| --- | --- | --- | --- |
| 21 00 | Paquet non exploité | aucune | signification non documentée |
| 21 01 | Température liquide moteur | (u16[2:3] - 2732)/10 | °C, calcul documenté |
| 21 02 | Température huile | (u16[2:3] - 2732)/10 | °C, peut être non montée selon véhicule |
| 21 03 | Température air admission | (u16[2:3] - 2732)/10 | °C |
| 21 05 | Température carburant | u16[2:3], **sans décalage** dans le Go | unité NON déterminée ; ne pas afficher °C sans preuve |
| 21 06 | Paquet ignoré | aucune | non déterminé |
| 21 07 | MAP | u16[2:3]/100 | kPa, cohérent avec documentation Blackbox |
| 21 08 | Angle papillon | u16[2:3]/100 | degrés selon le Go ; plage à vérifier sur ECU |
| 21 09 | Régime | u16[2:3] | tr/min |
| 21 0A | Correction lambda + indication sonde | feedback=u16[2:3]/100 ; O2=u16[4:5] ; estimation AFR=(O2/1000)*2+10 | feedback en % selon le Go ; O2 nommé mV dans le Go, calibrage non recoupé ; AFR **estimation logicielle**, pas une mesure AFR ECU certifiée |
| 21 0B | Temps de charge bobines 1/2 | octets [2] et [3] /1000 | unité exacte et échelle **non confirmées** |
| 21 0C | Quatre champs injection | octets [2] à [5] sans transformation | valeurs brutes ; temps en ms **non établi** ; ne pas déduire 4 injecteurs physiques |
| 21 0D | Vitesse véhicule | octet [2] | unité non spécifiée par le Go (ne pas supposer km/h sans recoupement) |
| 21 0F | États | bit0 [2] papillon fermé, bit1 contact, bit3 demande climatisation | booléens d'après le Go ; états à recouper |
| 21 10 | Tension batterie | u16[4:5]/1000 | V ; refus parfois observé sur Mini MPi selon commentaire Go |
| 21 11 | Synchronisation déclencheurs primaire/secondaire | 1 - bit0, 1 - bit1 du champ [2] | états booléens **inversés** par le Go |
| 21 12 | Position moteur de ralenti | u16[2:3]/2 | demi-pas/pas selon le décodeur, résolution à valider |
| 21 13 | Boucle fermée | bit0 [2] | booléen |
| 21 19 | Défauts | masques détaillés ci-dessous | cartographie limitée à la réponse observée par l'auteur |
| 21 21 | Écart régime | u16[2:3] prétendument signé | **anomalie de conversion Go** identifiée |
| 21 25 | Valeur associée « cam_percent » | u16[2:3], aucune division par 100 | unité et sémantique NON prouvées ; Mini MPi refuse cette commande selon Go |
| 21 3A | Correction ralenti + régime de ralenti cible | u16[2:3]/10, puis u16[4:5] | première unité non établie ; seconde semble tr/min, à tester |

Il n'y a pas de requête 21 04, 21 0E, 21 14 etc. dans le balayage Go **étudié** : cela ne prouve pas leur inexistence dans tous les ECU 2J.

### Contrôles arithmétiques indépendants, non matériels

- 21 01 avec octets 0B 74 : brut 2932, température = (2932-2732)/10 = **20,0 °C**.
- 21 07 avec octets 27 10 : brut 10000, pression = **100,00 kPa**.
- 21 08 avec octets 0E 74 : brut 3700, angle = **37,00°**.
- 21 09 avec octets 0B B8 : brut 3000, **3000 tr/min**.
- 21 0A avec feedback=0x2710 (10000) : **100,00 %**.
- 21 10 avec deux octets utiles 30 39 (situés en [4:5]) : brut 12345, **12,345 V** selon la formule du Go.
- 21 12 avec 00 34 : brut 52, valeur = **26 unités du moteur pas-à-pas** selon la formule Go.
- 21 21 : pour convertir correctement un entier signé 16 bits en complément à deux, utiliser raw >= 32768 ? raw - 65536 : raw. Le Go utilise raw > 32768 et raw - 65535 : **deux erreurs de frontière/décalage** ; raw FFFF doit donner -1 (le Go donnerait 0) et raw 8000 doit donner -32768 (le Go donnerait +32768). Cette erreur arithmétique est documentaire, sans correction de code à ce stade.

## 4. Décodage des défauts 21 19

La réponse positive a pour payload 61 19, suivie de différents octets de données/masques. Les index ci-dessous sont **les positions de la charge utile Go**, pas des adresses RAM directes et pas des numéros DTC standard OBD-II.

| Index payload Go | Bits testés (numérotation 0..7) | Sens déclaré dans ecu-2j-faults.go |
| --- | --- | --- |
| [4] | 6 ambiance ; 5 alimentation ; 4 huile ; 2 liquide ; 0 système | tension trop basse (« low voltage »), groupe RAM 0x594 |
| [5] | 7 batterie ; 4 lambda banc 1 ; 2 TPS ; 1 air admission ; 0 MAP | tension trop basse, RAM 0x594 |
| [8] | 6 ambiance ; 5 alimentation ; 4 huile ; 2 liquide ; 0 système | tension trop haute (« high voltage »), RAM 0x590 |
| [9] | 7 batterie ; 4 lambda banc 1 ; 2 TPS ; 1 air admission ; 0 MAP | tension trop haute, RAM 0x590 |
| [12] | 6 ambiance ; 5 alimentation ; 4 huile ; 2 liquide **ET** 2 système | défaut présent, mais **collision** du même bit 2 pour deux significations : source Go explicitement ambiguë |
| [13] | 7 batterie ; 4 lambda ; 2 TPS ; 1 air admission ; 0 MAP | défaut présent |
| [23] | 3 MAP ; 2 huile ; 1 admission ; 0 liquide | deuxième ensemble « present 2 » |
| [25] | 3 MAP ; 2 huile ; 1 admission ; 0 liquide | défaut historique |
| [26] | 0 vitesse route ; 1 communication boîte auto ; 4 feedback banc 1 ; 5 feedback banc 2 | défaut présent |
| [28] | 0 vitesse route ; 1 communication boîte auto ; 4 feedback | défaut historique |

Les positions [6], [7], [10], [11], [14..22], [24], [27], [29] ne sont pas interprétées par ce fichier : **absence d'interprétation ne veut pas dire absence de défaut**.

### Réserves confirmées

1. Le tableau provient explicitement de la mémoire des **Mini MPi** dans le Go. Il n'est **pas prouvé universel** pour un 2J VVC ou KV6.
2. Le doublon à [12].bit2 est une **ambiguïté non résolue** : aucune source indépendante fiable examinée ne permet de trancher entre liquide et tension système. Ne pas annoncer deux défauts distincts comme certains sur ce bit.
3. Le code distingue défaut actuel (« present »), historique (« historic ») et constat haute/basse tension : ce ne sont pas nécessairement des DTC OBD-II en Pxxxx ; aucune correspondance Pxxxx n'a été justifiée.
4. Les défauts de capteurs absents sont possibles même sans panne. Source indépendante pscan : température carburant / ambiance souvent non montées.
5. Une réponse 21 19 plus courte que la longueur requise pour un champ ne doit pas être extrapolée ; même si les accès Go sont souvent bornés individuellement, la structure complète/variante n'est pas garantie.

## 5. Ce que l'étude tranche / ce qu'elle ne peut pas trancher

**Tranché :** familles principales Mini MPi et VVC, existence des requêtes, position des champs et formules du code Go, statut non justifié de certaines unités, structure et ambiguïtés de la cartographie Mini, erreur de conversion signée. La matrice documentaire est complète pour **les requêtes présentes dans la version Go auditée**.

**Non tranché sans traces d'ECU :** couverture exhaustive des variantes, unité des champs de carburant/injecteurs/bobines, sémantique de 21 25, collisions de bits défaut et cohérence des réponses différentes suivant firmware. Ces éléments doivent rester bruts/à vérifier, pas être maquillés comme validés.

## Sources contrôlées

- James Portman, code source original, commit b381af39b2031c80303c5262055aa0c0121dbdd4 :
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j-parse.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j-faults.go
- RoverMEMS, communication 2J : https://rovermems.com/mems-2j/index.html
- Blackbox Solutions, SM072 — applications, cartographies, données réelles : https://tmsmini.com/old_content/cooper/BlackBox/LUCAS%20MEMS%202J%20(SM072).htm
- Andrew Revill, familles Rover et câbles : https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
- pscan, compatibilité MEMS2J et capteurs non montés : https://pscan.uk/features/mems2j.html
- MGF VVC, documentation descriptive : https://mgfcar.de/library/ENGINE_MANAGEMENT_SYSTEM_VVC_MEMS_2J.htm
- MiniSpares (avril 2026), identification versions et variantes : https://www.minispares.com/blog/2026/04/03/mini-spi-and-mpi-fuel-injectionelectronic-management-systems-and-emissions/
- RoverMEMS documentation — brochage 2J (documentation indique que certains libellés peuvent être inexacts) : https://github.com/james-portman/rover-mems-documentation/blob/master/ECUs/MEMS%202J/ecu-pinout

**Aucun code applicatif modifié. Aucun essai matériel réalisé.**
