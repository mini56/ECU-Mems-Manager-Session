# AUDIT TESTBOOK / MEMS — CODES DÉFAUT, DIAGNOSTIC ET BROCHAGES

> Branche : `RAPPORT`  
> Statut : **audit documentaire uniquement — aucune modification runtime**  
> Complément de `AUDIT_RAVE_MINI_SPI_MPI.md`.

## Principe de preuve

Les sources sont volontairement séparées en niveaux :

- **constructeur Rover/TestBook** : documents Rover/MG/RAVE identifiables ;
- **diagnostic professionnel secondaire** : documentation d'outils spécialisés (ex. Blackbox/Faultmate) ;
- **reverse engineering protocole** : travaux MEMSFCR/RoverMEMS/ROSCO ;
- **non confirmé** : information actuellement présente dans MEMS Manager mais non retrouvée dans les sources précédentes.

Aucune correspondance ne doit être promue au niveau constructeur simplement parce qu'elle est cohérente avec le protocole.

---

# LOT T1 — TESTBOOK / IDENTIFICATION DES SYSTÈMES MINI

## TB-SYS-001 — Diagnose-Handbuch Rover Deutschland

- Document : **Diagnose-Handbuch, ROVER Deutschland GmbH, 3. Auflage Februar 1999**.
- Le document contient une section Mini dédiée avec données systèmes, outils de diagnostic et connecteurs.
- Il distingue explicitement plusieurs générations Mini à gestion Rover MEMS : **MEMS 1.3 (SPi)**, **MEMS 1.6 (SPi)** et, pour les Mini MPi tardives, **MEMS 2J (MPi)**.
- Il décrit également le passage du connecteur diagnostic rond 3 broches près de l'ECU aux connecteurs de diagnostic plus récents, dont le connecteur 16 broches sur les modèles tardifs.
- Niveau : `verifie_constructeur_copie_numerisee`.

### Règle IA

Ne pas répondre « Mini MPi = MEMS 1.9 » comme une vérité générale. La documentation Rover/TestBook consultée identifie la Mini MPi tardive comme **MEMS 2J**. Si une autre source classe une Mini/TBI sous MEMS 1.9, conserver le contexte de cette source et demander véhicule/ECU/VIN avant de conclure.

## TB-SYS-002 — conflit de couverture Blackbox

- La documentation professionnelle Blackbox SM002 classe **Rover Mini 94–97 SPI** sous MEMS 1.6 et mentionne aussi **Rover Mini 1.3 TBI 97>** dans la couverture MEMS 1.9.
- Une autre liste Blackbox réserve explicitement le module **MEMS 2J (SM072)** à la **Mini 1300 MPi**.
- Niveau : `diagnostic_professionnel_secondaire`.
- Décision : ce n'est pas une raison pour fusionner MEMS 1.9 et MEMS 2J. Le type exact doit rester déterminé par l'ECU/variant identifié.

---

# LOT T2 — STRUCTURE DES CODES DÉFAUT DU PAQUET 0x80

Sources : documentation protocole RoverMEMS/MEMSFCR/ROSCO. Niveau : `reverse_engineering_protocole`.

## TB-DTC-001 — byte 0x0D du paquet 0x80

Le byte `0x0D` est un bitfield de défauts. La numérotation naturelle des bits correspond aux positions **1 à 8** :

- bit 0 → **Code 1 : température liquide** — explicitement confirmé Mini SPi ;
- bit 1 → **Code 2 : température air admission** — explicitement confirmé Mini SPi ;
- bit 2 → **Code 3 : non documenté** ;
- bit 3 → position Code 4 : sur documentation protocole générique, `turbo overboosted` ;
- bit 4 → position Code 5 : température ambiante ;
- bit 5 → position Code 6 : température rail/carburant ;
- bit 6 → position Code 7 : cliquetis ;
- bit 7 → **Code 8 : non documenté**.

### Prudence Mini SPi

Les sources protocole précisent que, sur le Mini SPi étudié, seuls les défauts **1 et 2** étaient effectivement vérifiés/utilisés dans ce premier byte. Les libellés 4–7 sont donc des capacités MEMS plus générales et ne doivent pas être présentés comme des équipements nécessairement présents sur une Mini SPi.

## TB-DTC-002 — byte 0x0E du paquet 0x80

Le byte `0x0E` correspond aux positions **9 à 16** :

- bit 0 → position Code 9 : circuit/jauge température, libellé encore incertain ;
- bit 1 → **Code 10 : circuit pompe carburant** — explicitement confirmé Mini SPi ;
- bit 2 → **Code 11 : non documenté** ;
- bit 3 → position Code 12 : embrayage/relais climatisation ;
- bit 4 → position Code 13 : purge canister ;
- bit 5 → position Code 14 : MAP ;
- bit 6 → position Code 15 : commande boost/wastegate ;
- bit 7 → **Code 16 : circuit potentiomètre papillon** — explicitement confirmé Mini SPi.

### Prudence Mini SPi

Les travaux MEMSFCR/ROSCO indiquent que les quatre codes clairement observés/contrôlés sur Mini SPi sont **1, 2, 10 et 16**. Les autres positions existent dans le bitfield/gamme MEMS mais doivent rester liées à la configuration réellement supportée par l'ECU.

---

# LOT T3 — STRUCTURE DES CODES 17–24 DU PAQUET 0x7D

Source : RoverMEMS technical / reverse engineering du paquet `0x7D`. Niveau : `reverse_engineering_protocole`.

Le byte `0x05` du paquet `0x7D` est documenté comme byte DTC. Si on poursuit la numérotation des bits à partir de 17 :

- bit 0 → **Code 17 : non documenté** ;
- bit 1 → **Code 18 : non documenté** ;
- bit 2 → **Code 19 : non documenté** ;
- bit 3 → **Code 20 : chauffage lambda / relais** ;
- bit 4 → **Code 21 : synchronisation vilebrequin** ;
- bit 5 → **Code 22 : commande ventilateur 1** ;
- bit 6 → **Code 23 : NON DOCUMENTÉ dans la source protocole consultée** ;
- bit 7 → **Code 24 : commande ventilateur 2**.

### Conclusion importante sur MEMS Manager

Les libellés actuels **20, 21, 22 et 24** de MEMS Manager sont cohérents avec la structure `0x7D` retrouvée.

En revanche, **`Code 23 = commande antidémarrage` n'est pas confirmé** par cette source : le bit 6 n'y possède pas de signification documentée. Les recherches dans MEMSFCR public n'ont pas retrouvé de `faultCode23`/immobiliser correspondant.

Décision audit :
- conserver le code actuel inchangé pendant l'audit ;
- marquer **Code 23 / antidémarrage = `preuve_insuffisante`** ;
- rechercher une source TestBook/MEMS/firmware supplémentaire avant toute correction du logiciel ou de la base.

---

# LOT T4 — NATURE DES DÉFAUTS MEMS SELON DOCUMENTATION PROFESSIONNELLE

Source : Blackbox Solutions, module MEMS 1.6/1.9 SM002. Niveau : `diagnostic_professionnel_secondaire`.

## TB-DIAG-001 — défaut mémorisé ≠ toujours défaut actif

La mémoire défaut MEMS mélange :
- des événements historiques qui restent mémorisés jusqu'à effacement ;
- certains états internes dynamiques qui apparaissent lorsque l'entrée manque puis peuvent disparaître automatiquement lorsque le signal revient.

Exemple documenté : un défaut lié au signal vilebrequin peut apparaître lorsque le moteur est arrêté parce que ce signal n'existe qu'en rotation.

### Règle IA

Quand l'utilisateur demande « j'ai cette erreur, qu'est-ce que ça veut dire ? », la réponse immédiate doit distinguer :
1. défaut actuellement actif ;
2. défaut mémorisé/intermittent ;
3. état pouvant être normal dans les conditions de test (ex. moteur arrêté) ;
4. fonction non montée/non supportée par la variante.

## TB-DIAG-002 — fonctions optionnelles/non montées

MEMS peut être configuré pour supporter des fonctions qui ne sont pas nécessairement montées sur le véhicule : sonde O2, purge, ventilateur, cliquetis, climatisation, CAM, groupes d'injecteurs, ventilateur compartiment, température carburant/ambiante, etc.

Une erreur relative à une fonction absente ne doit donc pas être interprétée automatiquement comme une panne matérielle du véhicule sans vérifier la configuration ECU.

---

# LOT T5 — VALEURS DIAGNOSTIQUES PROFESSIONNELLES UTILES AUX RÉPONSES IMMÉDIATES

Source : Blackbox SM002. Niveau : `diagnostic_professionnel_secondaire`, à recouper avec RAVE lorsque possible.

## TB-VAL-001 — Hot idle / position IAC apprise
- Valeur attendue indiquée : **10 à 50 pas**.
- C'est une valeur **adaptative/apprise**.
- Une valeur hors plage peut signaler défaut ou mauvais réglage.
- Conflit/écart à conserver : RCL0193 donne une plage plus resserrée **20–40 pas** dans son contexte Mini.

## TB-VAL-002 — coil charge / dwell
- À environ **14 V**, la documentation diagnostique indique environ **2–3 ms**.
- Une valeur élevée peut indiquer un problème dans le circuit primaire de bobine.
- Cohérent avec la plage de contrôle projet ~1,9–3,1 ms ; conserver toutefois l'origine de chaque plage.

## TB-VAL-003 — coolant open circuit
- Un circuit ouvert du capteur température liquide peut conduire à une valeur de substitution affichée d'environ **60 °C**.
- Symptômes associés indiqués : démarrage difficile, ralenti accéléré, consommation élevée, ventilateurs pouvant fonctionner en continu.

## TB-VAL-004 — IAT
- Un défaut IAT peut dégrader légèrement les performances et la correction à chaud ; une valeur fixe peut apparaître en circuit ouvert.
- Ne pas inventer la valeur fixe sans source propre au variant.

## TB-VAL-005 — idle speed error
- Une erreur de ralenti supérieure à **100 tr/min** est présentée par cette source comme indice que l'ECU ne contrôle plus correctement le ralenti.
- À conserver comme seuil diagnostic professionnel, pas comme valeur constructeur Rover universelle.

---

# LOT T6 — BROCHAGE MEMS 1.6 : PREMIER INVENTAIRE

Source : Blackbox SM002, niveau `diagnostic_professionnel_secondaire`. Ce lot sert à préparer les contrôles « quoi mesurer et où », mais ne sera pas injecté comme brochage constructeur tant qu'il n'est pas recoupé avec schémas Rover.

Broches explicitement listées dans la source consultée :

- 1 : injecteur n°1 ;
- 2 : IACV ;
- 4 : masse ;
- 5 : module coding plug, entrée ;
- 6 : vitesse véhicule, entrée ;
- 7 : capteur température liquide, entrée ;
- 8 : pare-brise chauffant, entrée ;
- 9 : ligne diagnostic ;
- 10 : MIL, commande masse ;
- 11 : injecteurs pairs 2/4/6/8, masse/commande ;
- 12 : relais moteur, commande masse ;
- 13 : injecteurs impairs 1/3/5/7, masse/commande ;
- 14 : masse ;
- 15 : tension batterie, entrée ;
- 16 : relais pompe carburant, commande masse ;
- 17 : vanne purge canister ;
- 18 : diagnostic ;
- 19 : contact d'allumage, entrée ;
- 20 : capteur position papillon, entrée ;
- 21 : pressostat climatisation, entrée ;
- 22 : débitmètre MAF, entrée selon application ;
- 23 : sonde oxygène gauche, entrée ;
- 24 : sonde oxygène droite, entrée ;
- 25 : température carburant, retour/masse selon source ;
- 26/28/29 : voies IACV selon source ;
- 27 : coding plug, masse sous contact.

### Règle de sécurité

Ce brochage est **générique MEMS 1.6 multi-application**. Il ne doit jamais être donné comme brochage exact d'une Mini sans recoupement par ECU/référence/variant et schéma Rover correspondant.

---

# LOT T7 — INFORMATIONS RAVE ÉLECTRIQUES DÉJÀ RECOUPÉES

Source constructeur : `RCL0213ENG` / Mini Electrical Reference Library.

- MAP : monté directement sur collecteur, information de charge utilisée pour carburant + avance ;
- ECT : thermistance au circuit liquide, signal inverse de température ;
- TP : potentiomètre 5 V, rôle ralenti/enrichissement/décélération ;
- IAT : NTC, utilisée avec MAP pour correction densité d'air ;
- IACV : quatre phases commandées par ECM, dérivation d'air autour du papillon ;
- alimentation ECM/relais principal et alimentation des injecteurs, purge, stepper, bobine documentées par couleurs de fils dans RCL0213.

Les schémas RCL0194 restent la source à privilégier pour transformer ces informations en **pinouts exacts Mini**, plutôt que de recopier un brochage MEMS 1.6 générique.

---

# CONSÉQUENCES POUR L'AUDIT DES QUESTIONS IA

Les réponses immédiates doivent maintenant pouvoir distinguer plusieurs niveaux :

- **« Que signifie Code 1/2/10/16 sur Mini SPi ? »** → correspondance protocole fortement établie ;
- **« Que signifie Code 20/21/22/24 ? »** → correspondance `0x7D` documentée en reverse engineering, avec niveau de preuve affichable dans la base ;
- **« Code 23 ? »** → ne pas affirmer antidémarrage comme vérité tant que la source manque ;
- **« J'ai un défaut vilebrequin moteur arrêté »** → expliquer qu'un état dynamique sans signal peut être attendu selon outil/condition et demander si le défaut persiste pendant le lancement ;
- **« J'ai une erreur d'un équipement absent »** → vérifier d'abord la configuration/support ECU ;
- **« Quelle broche contrôler ? »** → ne fournir un numéro que si ECU/variant identifié et source correspondante disponible.

# PROCHAINE RECHERCHE

1. rechercher dans RCL0194/AKM7169 le **brochage Mini exact** pour CTS, IAT, TPS, MAP, lambda, IACV, pompe et purge ;
2. rechercher une source supplémentaire pour **Code 23 / bit 6 du byte DTC 0x7D:0x05** ;
3. recouper les codes 4–7 et 9,12–15 avec les variantes qui les supportent réellement ;
4. rechercher les procédures de contrôle constructeur permettant d'alimenter `défaut → symptômes → contrôles` ;
5. ne modifier ni base runtime ni code IA avant validation de l'audit.
