# Rover MEMS 1.6 SPi — analyse ciblée de la source Colin Bourassa

**Date de vérification :** 23 novembre 2026  
**Source primaire :** [Rover MEMS 1.6 diagnostic protocol — Colin Bourassa](https://colinbourassa.github.io/car_stuff/mems_interface/#table-1-initialization-sequence) (article publié en 2014, actualisations en 2015).  
**Portée annoncée par l'auteur :** recherches sur quelques calculateurs MEMS 1.6 SPi, principalement une voiture. **Ne pas extrapoler automatiquement** aux MEMS 1.2, 1.3, 1.9, 2J ou 3.

**Comparaison interne :** `librosco-x64/ROSCO_COMMAND_CARTOGRAPHY.md`, `librosco-x64/protocol.c`, `librosco-x64/rosco.h`, `research/mems/ETAT_RECHERCHE.md`. Statuts conformes à ce dernier document : `DEJA_PRESENT`, `PRECISION`, `CONFLIT`, `A_VERIFIER`.

## Résultats à conserver

| Sujet | Observation provenant de la source | État par rapport à Manager | Statut / décision |
|---|---|---|---|
| Liaison 1.6 | 9600 bauds, 8 bits, sans parité, un bit stop ; interface USB série **TTL 0–5 V**, et non RS-232 classique | 9600/8N1 déjà codé en x64 | **DEJA_PRESENT / PRECISION** ; documenter les niveaux électriques pour éviter les mauvaises interfaces |
| Prise diagnostic 3 broches C549 | C549-1 masse (fil adaptateur noir), C549-2 réception ECU→PC (jaune), C549-3 transmission PC→ECU (orange) ; connecteur TE 172201 | Références câblage existantes à comparer selon variante | **PRECISION / A_VERIFIER** ; orientation et couleurs du véhicule non universelles |
| Initialisation | Exemple source : `CA → 75 → D0` avec échos ; notre DLL fait `CA → 75 → F4 → D0` et lit quatre octets après l'écho D0 | Différence documentée dans notre cartographie | **CONFLIT DE SÉQUENCE / VARIANTE** ; ne pas supprimer F4 sans trace matérielle |
| Identification D0 | MNE101170 : `D0 99 00 03 03` ; MNE101070 : `D0 99 00 02 03` (premier D0 = écho) | D0 et lecture quatre octets déjà présents | **PRECISION** ; exemples pour tests de parsing, pas table universelle d'identification |
| Trames normales | `0x7D` : 32 octets ; `0x80` : 28 octets ; champs multioctets dans l'ordre fort d'abord | Structures déjà présentes dans `rosco.h` | **DEJA_PRESENT** ; aucune réécriture nécessaire |
| Tension lambda | `7D:06` annoncé à **0,5 mV par unité brute** | `ETAT_RECHERCHE.md` consigne **5 mV** et un conflit antérieur | **CONFLIT PRIORITAIRE** : facteur ×10 ; ne changer ni calcul ni diagnostic sans recoupement |
| Temps bobine | `80:17–18` : entier 16 bits, **0,002 ms par unité brute** | `rosco.h` distingue bien les deux octets | **PRECISION / A_VERIFIER** : comparer le décodage effectif et les seuils du diagnostic automatique ; cette source ne fournit **aucun seuil de panne** |
| Avance allumage | `80:16` : pas de 0,5°, plage indiquée −24° à 103,5° | Champ brut existant | **PRECISION / A_VERIFIER** suivant génération |
| IAC pas à pas | `80:12` : position de référence rapportée de 0 à 180 pour Mini SPi ; `FB` lit la position, `FD/FE` la font varier par pas | Commandes et champ IAC recensés | **PRECISION** ; ne pas assimiler la plage brute ECU au nombre de pas de l'interface |
| DTC | Pour Mini SPi, `80:0D` bits 0/1 : capteurs liquide de refroidissement / air d'admission ; `80:0E` bits 1/7 : circuit pompe / potentiomètre papillon | Divergences de masques déjà connues | **A_VERIFIER** par ECU et référence, sans généralisation |
| Commande F6 | Sur un banc d'essai de l'auteur, `F6` a laissé les commandes en simple écho, sans trame ; une nouvelle initialisation a rétabli les réponses | F6 déjà identifié comme fin/réinitialisation possible de session | **PRECISION / A_VERIFIER** ; observation non prouvée sur tous les calculateurs |
| Actionneurs et réglages | La source recense notamment commandes de relais, pompe, IAC et ajustements de ralenti, mélange et avance | L'essentiel figure déjà dans la cartographie ROSCO | **DEJA_PRESENT** ; la commande dépend de la famille et du mode, surtout F7/F8/D1/D3 |

## Points de vérification proposés

1. **Lambda** : comparer l'unité et l'offset `7D:06` sur deux références indépendantes et sur une trace ECU réelle. Garder visibles les deux hypothèses 0,5 / 5 mV jusqu'à résolution.
2. **Bobine** : vérifier la conversion exacte du temps bobine 16 bits dans Manager et son interprétation dans *Diagnostic automatique*. Un pas de conversion n'est pas un seuil de défaillance.
3. **Initialisation** : confronter `CA/75/D0` et `CA/75/F4/D0` à la famille, au firmware et aux réponses réelles, sans changer la routine qui fonctionne.
4. **Câble / brochage** : comparer les références et orientation de la prise C549 aux fiches de câblage MEMS 1.6 du projet, sans assimiler prise ECU et prise antidémarrage.
5. **Masques DTC et F6** : documenter les limites Mini SPi avant d'envisager une règle logicielle générale.

## Décision de conservation

**Archivage documentaire uniquement.** Ce relevé ne valide pas le fonctionnement sur calculateur réel et n'autorise **aucune modification** de la DLL, du protocole ROSCO, des seuils de diagnostic, des bases chiffrées ou du Setup. Les points déjà présents ne sont pas réimportés comme nouveautés. Les conflits restent explicitement non résolus.

**Provenance :** article Colin Bourassa, tableaux 0 à 5 et notes de protocole, consulté le 23/11/2026 ; correspondances faites sur le code source et les documents existants de la branche unique.
