# Inventaire diagnostic MEMS 1.6, 1.9, 2J, 3 et fonctions antidémarrage

Date : **17 novembre 2026** — Recherche documentaire, aucun pilote ECU changé ni testé sur véhicule.  
Statuts : **SOURCE_CONFIRMEE** = fonction connue dans des outils tiers ; **TRAME_ETUDIEE** = commande/format trouvé ; **A_VALIDER** = disponible selon variante, unité ou décodage inconnu ; **NON_IMPLEMENTE_SESSION** = pas encore livré dans SESSION.

## Sources principales et recoupement
- RoverMEMS, tables ROSCO : https://rovermems.com/diagnostics/technical/
- RoverMEMS, 2J : https://rovermems.com/mems-2j/index.html
- RoverMEMS, 3 : https://rovermems.com/mems-3/index.html
- Andrew Revill, familles et fonctions : https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
- Andrew Revill, remapping 1.9 : https://andrewrevill.co.uk/MEMSMapperMems19.htm
- Andrew Revill, EU2/2J : https://andrewrevill.co.uk/MEMSMapperEU2.htm
- Blackbox 1.6/1.9 : https://www.blackbox-solutions.com/site/product?sku=SM002
- Blackbox 2J : https://www.blackbox-solutions.com/site/product?sku=SM072
- Blackbox 3 : https://www.blackbox-solutions.com/site/product?sku=SM003
- 5AS indépendant : https://rovermems.com/5as/
- Source protocolaire : https://github.com/james-portman/rover-mems-agent

## MEMS 1.6 — ROSCO

**Liaison** : UART 9600 8N1, généralement connecteur ROSCO trois broches, adaptation électrique correcte indispensable. Initialisation de la famille 1.3/1.6 documentée dans RoverMEMS. Ne pas confondre 1.6 avec une K-Line 2J.

**Lecture confirmée dans la famille ROSCO** :
- `0x80` : trame de mesures incluant régime, température eau, température air, température carburant si équipée, MAP, batterie, papillon, ralenti, avance et temps de charge bobine.
- `0x7D` : états, boucle lambda, correction carburant, commande ventilateurs, régulation ralenti, états et bits de défaut, selon firmware.
- `0xD0` identification ECU, `0xD1` identification étendue/logiciel, `0xD2` statut sécurité, `0xF0` mode de session (consulter source et code SESSION).
- Trames `0x7D` / `0x80` de longueur variable selon générations ; décoder selon la taille *réellement annoncée*, jamais supposer un format unique.
- Informations supplémentaires selon équipement : fonctions prises en charge (lambda, ventilateurs, purge, climatisation, capteur cliquetis, vitesse moteur, injection), adaptations et certains actionneurs, mais **la lecture seule est prioritaire**.
- `D2` n'est pas une preuve à elle seule que l'ECU est autorisé à démarrer : le statut de communication anti-démarrage mérite une lecture et une interprétation séparées.

**À valider** : variation des champs de 0x7D/0x80 entre SPI et MPI, conventions d'échelle lambda/TPS et masques DTC déjà signalés en conflit dans `research/mems/ETAT_RECHERCHE.md`.

## MEMS 1.9 — réveil K-Line puis ROSCO

**Liaison** : 9600 8N1 après slow-init 5 bauds à adresse 0x16 ; la phase de réveil et la phase ROSCO sont séparées. SESSION possède déjà `memsinterface_dispatch.cpp` et un test guidé `mems19testtab.h`.

**Lectures** : identification D0/D1/D2, session F0, mêmes familles de trames normales `0x7D` / `0x80` que ROSCO avec variantes, données moteur, défauts, mesures d'injection et lecture RAM uniquement sur firmware exact et cartographie corrélée. Un logiciel externe signale aussi des fonctions de surveillance du lien antidémarrage.

**Contrôle nécessaire** : ne pas remplacer l'initialisation déjà codée en copiant l'exemple Go `ecu-19.go` ; préserver les ECU SPI. L'essai matériel complet du parcours 1.9 demeure à effectuer.

## MEMS 2J — KWP2000

**Liaison** : K-Line, fast-init (25 ms BREAK / 25 ms relâchement) ou slow-init, puis 10400 bauds 8N1 ; écho local possible. Trame d'entrée spécifique `81 13 F7 81 0C`, réponse d'amorçage attendue `03 C1 D5 8F 28`. Trames normales length + payload + somme modulo 256 (voir explication RoverMEMS sur champs exacts).

**Commandes de lecture étudiées dans rover-mems-agent** : `21 01` température eau, `21 02` huile, `21 03` air, `21 07` MAP, `21 08` papillon, `21 09` régime, `21 0A` lambda/correction, `21 0C` injecteurs (valeurs brutes), `21 10` batterie, `21 12` ralenti, `21 19` défauts. Réponses positives `61 xx`, négatives possibles `7F`. Certaines requêtes refusées selon véhicule (dont Mini MPI), sans interrompre toute la session.

**Autres données attestées dans outils de diagnostic** : température ambiante, température carburant (si capteur équipé), angle et état papillon, corrections de richesse, identifiant de cartographie, identification véhicule, température huile, pression admission, régime et actionneurs. Ne pas assimiler l'existence dans un outil de diagnostic à une commande identifiée pour SESSION.

**Défauts** : bits « présent », « historique », niveaux haut/bas. Conflit source confirmé : `ecu-2j-faults.go` attribue deux libellés au même bit `buffer[12] >> 2`. **CONFLIT_A_VERIFIER**, ne pas livrer un diagnostic trompeur.

## MEMS 3 — KWP/Rover BMW

**Liaison de la variante étudiée** : K-Line 9600 8E1 ; envoi `B8 13 F7`, réception `13 B8 F7` ; longueur explicite, vérifications de contrôle par trame. L'auteur RoverMEMS signale une variabilité des contrôles de réponse ; **ne pas imposer par hypothèse une seule forme**.

**Commandes étudiées** : `21 00` températures, `21 06` MAP / papillon / régime, `21 0A` correction carburant, `21 0B` charge bobines, `21 21` écart régime, `18 00 00 00` lecture défauts (réponse `58` suivie de codes/statuts). Réponse négative `7F` possible ; toujours vérifier la longueur reçue.

**Enrichissements de diagnostic confirmés chez Blackbox** : nombreuses mesures **brutes du capteur vs valeurs calculées/retenues par l'ECU** (températures eau, huile, air ; pression admission ; tension batterie ; sondes lambda). Autres rubriques : adaptation carburant, papillon, allumage, régulation ralenti et statut immobiliseur, selon version. Les variantes 5AS/Pektron et EWS ne partagent pas toutes les mêmes conditions de sécurité.

**Lecture seule** : identifier version, mesures, défauts, paramètres d'antidémarrage. Pas d'effacement, écriture, apprentissage, changement de cartographie ou fonction de neutralisation dans un premier pilote.

## Antidémarrage — inventaire distinct de la communication ECU

**Important** : « lire l'état », « synchroniser/appairer » et « neutraliser » sont trois actions différentes. La dernière modifie la sécurité du véhicule ; ce dossier ne contient ni instructions de contournement ni séquence d'écriture.

| Famille | Lecture d'état | Réappairage / apprentissage documenté | Neutralisation permanente documentée | Décision SESSION |
|---|---|---|---|---|
| MEMS 1.6 | Oui, système de sécurité et liaison | Oui sur outillage spécialisé | Pas de méthode universelle confirmée pour firmware stock | Étudier les états / erreurs en lecture seule |
| MEMS 1.9 | Oui, dont statut sécurité D2 et diagnostic de liaison | Oui pour remplacement de module | Modification firmware décrite par Andrew Revill ; une simple option générique de cartographie stock n'existe pas | Aucune écriture sécurité |
| MEMS 2J | Oui, selon diagnostic et véhicule | Oui via outils adaptés | Capacité publiée pour ECU préparé/modifié et certains projets de firmware/cartographie | Pas de neutralisation dans le pilote |
| MEMS 3 | Oui, états détaillés et synchronisation | Oui, procédure variante-dépendante | Capacité publiée dans MEMS Mapper, distinction 5AS/Pektron/BMW EWS | Afficher seulement état et conditions connues |

**5AS** : module distinct de l'ECU moteur. L'appairage d'une télécommande au 5AS ne relie pas nécessairement le 5AS au calculateur moteur. Un câble 5AS peut être spécifique : ne pas faire croire que la connexion ECU 1.9/2J vérifie le 5AS sans adaptation.

**Données de diagnostic pertinentes** : signal présent/absent, code reconnu ou non, état autorisé/non autorisé, demande de synchronisation possible, statut de liaison, codes et erreurs ; les champs réels et la signification des statuts doivent être corrélés au firmware et au module. Ne jamais considérer un simple signal électrique présent comme la preuve d'une authentification réussie.

**Limites de sécurité** : les opérations de neutralisation et de programmation doivent faire l'objet d'un chantier séparé, avec identification exacte de l'ECU, preuve d'autorisation du propriétaire, procédures et outillage constructeur/spécialiste et sauvegardes. Aucun bouton « supprimer antidémarrage » ne doit être ajouté au module de diagnostic.

## Recommandations pour développement futur

1. Figer le lecteur existant 1.9 et obtenir ses journaux de test réel.
2. Créer un décodeur 2J autonome sur paquets enregistrés : écho, longueur, réponse négative, checksum, coupures et conversions.
3. Créer séparément un décodeur MEMS3 ; gérer variantes d'entête/contrôle sans contaminer ROSCO.
4. Ajouter un catalogue de mesures par variante : **documenté**, **trame identifiée**, **décodage vérifié**, **matériel testé**.
5. Préparer un panneau **État antidémarrage** exclusivement informatif et indépendant de la connexion obligatoire.
6. Ne jamais supposer qu'un FT232/CH340 détecté et ouvrable est une interface K-Line électriquement conforme.
7. Conserver les divergences de sources et les commandes à effets comme hors périmètre.

## Points ouverts à recouper

- Identification exacte des paramètres d'antidémarrage et de leurs réponses ROSCO selon firmware 1.6/1.9.
- Nomenclature complète des statuts et DTC de 2J (le doublon bit 12 est confirmé).
- Prise en charge de multiples variantes MEMS3 (5AS/Pektron/EWS) et des réponses de checksum.
- Identification des unités et offsets des valeurs injecteurs 2J.
- Variations de longueur/offset des trames ROSCO entre 1.6 SPI, 1.6 MPI et 1.9 MPI.
- Statut du projet de remappage spécifique MEMS 1.6 : les informations disponibles ne confirment pas un procédé universel sans modification matérielle.
