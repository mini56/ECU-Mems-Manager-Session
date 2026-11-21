# Archive Lucas 5AS — mémoire EEPROM, références Andrew Revill

**21 novembre 2026 — ECU-Mems-Manager-Session.** Étude documentaire indépendante de la compilation SESSION 192.

## Source

https://andrewrevill.co.uk/MapFirmwareLibrary/Lucas%205AS/

Répertoire directement consulté : **18 fichiers .bin**, avec taille affichée **256 octets** par fichier.

| Groupe dans les noms de fichiers | Nombre |
|---|---:|
| Lucas_5AS sans variante précisée | 3 |
| P2.005AS-97MY | 3 |
| P3.005AS-98MY | 10 |
| P4.005AS-02MY | 2 |
| TOTAL | 18 |

La mention 97MY/98MY/02MY provient des noms de fichiers. Ne pas déduire la compatibilité exacte avec le matériel sur ce seul critère.

## Signification technique des .bin

La documentation originale d'Andrew Revill explique qu'il s'agit d'images **EEPROM 256 octets** pour des boîtiers Lucas 5AS, distinctes des binaires de firmware moteur MEMS.

Références :
- https://andrewrevill.co.uk/5ASCloningPart1.htm
- https://andrewrevill.co.uk/5ASCloningPart2.htm
- https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm

Le format peut regrouper des informations sensibles : identifiants uniques, données de télécommandes, codes d'accès de secours et éléments d'association avec le moteur. Les documents originaux signalent également qu'une écriture incorrecte peut empêcher le bon fonctionnement du module.

## Intérêt concret pour Manager

1. Inventorier les générations et les références connues du 5AS.
2. Étudier, localement et en lecture seule, les différences de structure de sauvegarde entre P2 / P3 / P4.
3. Préparer une future fonction d'ouverture et d'identification d'un fichier EEPROM .bin, distincte du diagnostic série et de l'apprentissage PLIP.
4. Lors de l'analyse, **masquer par défaut les données de sécurité** ; ne pas les publier dans des logs, Release notes, captures ou packs de base.
5. Ne pas en déduire un algorithme de reprogrammation, un code d'identité ou une compatibilité physique sans démonstration documentée et test adapté.

## Périmètre de cette livraison documentaire

- Le lien et l'inventaire sont conservés en **référence externe** dans le dépôt privé.
- Aucun des 18 fichiers .bin n'est ajouté au code, au Setup, aux Releases ou à la base documentaire.
- Aucun changement au runtime Manager, aux protocoles 5AS / 10AS / Pektron / RC5 / MEMS.
- **Aucune écriture sur calculateur n'est déclenchée**.
- La publication et les tests du RUN SESSION 192 restent indépendants.

**Conclusion :** données de référence potentiellement précieuses pour le futur volet « sauvegardes 5AS » ; pas une condition préalable au diagnostic et à l'apprentissage PLIP actuellement développés.
