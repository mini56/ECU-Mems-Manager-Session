# Audit des réglages moteur : ancien MEMS-ROSCO vs Manager

**Date : 21/11/2026 — dépôt privé ECU-Mems-Manager-Session.**  
**Portée de cet audit :** vérifier l'ensemble des réglages additionnels possibles à partir des captures de l'utilisateur, du code de Manager, des sources publiques MEMS-ROSCO et des descriptions protocolaires. Aucun envoi de commande et aucune modification du runtime par cet audit.

## 1. Diagnostic précis du cinquième réglage visible sur l'ancien programme

L'écran de l'ancien MEMS-ROSCO comporte **quatre adaptations classiques** (Fuel trim, Idle decay, Idle speed, Ignition advance) et une cinquième rangée de boutons `idle air minus / idle air plus`, **déplacement de l'actionneur IAC**. L'indicateur `hot idle` est une **mesure affichée**, et non une sixième commande prouvée.

Dans Manager :
- les 4 adaptations existent déjà dans la page Réglages (`settingspage.cpp` — `rebuildAdjustArea` et `compactAdjustForm`) ;
- `MEMS_OpenIAC = 0xFD` et `MEMS_CloseIAC = 0xFE` existent dans `librosco-x64/rosco.h` ;
- les commandes sont déjà reliées aux boutons de la page **Actionneurs** (`mainwindow.cpp`, `on_m_IACPlusButton_clicked` et `on_m_IACMinusButton_clicked` via `MEMSInterface`) ;
- `hot idle` apparaît déjà parmi les valeurs de diagnostic (`mainwindow.cpp` : `idle_base_pos`, `idle_hot`).

**Recommandation :** ajouter sur la page Réglages une cinquième ligne **« Air de ralenti — déplacer IAC »** en réutilisant les deux commandes `0xFD/0xFE` déjà fonctionnelles, et clairement la qualifier de **commande d'actionneur**, pas d'adaptation persistante. Selon la place disponible, afficher à côté la **position IAC** actuelle, en lecture seule, et/ou la valeur **ralenti chaud**. Ne pas dupliquer la couche de communication.

**Attention au code ancien :** `mainwindow.cpp` de LeopoldG fait `emit moveIAC(+1)` / `moveIAC(-1)`, alors que `mems_move_iac(info,desired_pos)` de la DLL moderne attend une **position ABSOLUE `uint8_t`** et peut répéter une commande jusqu'à 300 fois pour atteindre sa cible. Ne **pas** transposer ces deux valeurs comme des positions absolues ; pour la cinquième ligne, utiliser les commandes **un pas** `0xFD/0xFE`, avec le retour du calculateur.

## 2. Bouton « Sauvegarder les réglages » de LeopoldG

- Code d'origine public `LeopoldG/sources-mems-rosco` : bouton `m_saveadjustments` relié à `MEMSInterface::on_m_saveadjustments_clicked()`, qui appelle `mems_Save()` et émet `EmiSaveComplete` en cas de réussite déclarée.
- Ce dépôt expose la **déclaration** `mems_Save()` dans `rosco.h`, mais pas la définition de bas niveau ni les octets effectivement émis, qui ne sont donc **pas établis** par cette seule source.
- Dans la cartographie source de James Portman, `0x81` est indiqué comme **peut-être utilisé à la fin d'ajustements** : ce n'est ni une preuve de sauvegarde EEPROM ni une garantie de persistance après mise hors contact.
- L'implémentation actuelle de Manager/ROSCO ne possède **pas** d'API « Save » confirmée : ne pas substituer un reset (`0x0F` ou `0xFA`) et ne pas envoyer `0x81` simplement sur la foi du nom du bouton.
- **Étude à terminer avant activation** : retrouver le corps réel de `mems_Save` d'une bibliothèque ancienne autorisée, ou une trace TX/RX documentée, puis identifier famille/mode, réponse positive, effet de l'opération et persistance après un cycle d'allumage. Prévoir un avertissement et une confirmation avant toute sauvegarde permanente.

## 3. Inventaire des autres commandes

| Groupe | Octets / API | Statut / recommandation |
| --- | --- | --- |
| Richesse principale | `79/7A` | Déjà implanté dans Manager |
| Ralenti chaud / retour au ralenti (« idle decay ») | `89/8A` | Déjà implanté ; examiner l'intitulé traduit, garder le compteur propre à ce réglage |
| Consigne de vitesse de ralenti | `91/92` | Déjà implanté |
| Décalage d'avance à l'allumage | `93/94` | Déjà implanté |
| Déplacement du moteur IAC | `FD/FE` ; `FB` lit la position | Disponible dans Actionneurs. Bon candidat à exposer aussi dans Réglages comme **commande transitoire** |
| Richesse secondaire (« fuel trim 2 ») | `7B/7C` | Commandes documentées par RoverMEMS ; Colin Bourassa les décrit sur certaines variantes comme modifiant le même compteur que 79/7A. Sur MEMS 1.9, compte rendu technique signalant une fonction identique au trim 1 ; **pas de cinquième adaptation indépendante prouvée** |
| Réglage automatique ralenti / avance ? | `7E/7F` | Sens **incertain** dans la source principale ; aucune UI active sans analyse de trames et matériel |
| Fin / enregistrement d'ajustements ? | `81` | Fonction **incertaine** ; aucune preuve directe de persistance ; voir §2 |
| Reset / remise aux valeurs précédentes | `0F/FA/AF` | Conflits de signification suivant famille/mode. Les wrappers actuels préservent leur contrat historique ; ne pas confondre avec une sauvegarde |
| Calibration / programmation mode 4 | `80–BF`, `D1`, `D3`, `F8` en mode 4 | **Pas de commandes de réglage ordinaires** : écriture en RAM/ROM ; exclure de la page Réglages, sauf projet séparé validé |
| MEMS 2J / MEMS 3 | sessions et commandes spécifiques | Leurs commandes de mesure/effacement de défauts existent, mais **ne réutilisent pas les octets de réglage ROSCO**. Il faut une analyse distincte par calculateur |

**Important :** `7C` est également utilisé dans la négociation K-Line MEMS 1.9 ; `F8` a d'autres fonctions selon le mode. La compatibilité ne peut pas être déduite d'un seul octet.

## 4. Vérifications / intégration recommandées, par ordre

1. Compléter la page Réglages par les commandes IAC déjà existantes **un pas à la fois**, sans utiliser `moveIAC(-1)` comme une position absolue.
2. Afficher la position IAC / ralenti chaud uniquement à partir de données réellement remontées, sans inventer de nouvelles valeurs. Revoir les libellés « position ralenti chaud » vs « idle decay » avant renommage.
3. Rechercher la **véritable** implémentation de `mems_Save()` ou une trace de transaction démontrant la sauvegarde. N'ajouter un bouton actif qu'après preuve ; vérifier le résultat après coupure/redémarrage du boîtier.
4. Ne présenter **fuel trim 2** comme un réglage distinct que si la famille ECU et les retours de commande le démontrent. Ne pas annoncer un nouveau paramètre si deux boutons modifient la même valeur.
5. Continuer l'étude documentaire des commandes incertaines `7E/7F/81`, avec une interdiction de les déclencher implicitement au branchement.
6. Ajouter au test de non-régression l'existence et la mise en page de la nouvelle ligne si celle-ci est décidée, sans toucher à ROSCO/2J/3 ni aux fonctions Lucas.
7. Pour toute commande pouvant mémoriser ou effacer des adaptations, demander une confirmation **expliquant la conséquence**. Les changements d'actionneur moteur nécessitent également un avertissement adapté quand le moteur tourne.

## Références

- Code source original : https://github.com/LeopoldG/sources-mems-rosco (fichiers `mainwindow.cpp`, `mainwindow.ui`, `memsinterface.cpp`, `rosco.h`).
- James Portman, cartographie détaillée : https://rovermems.com/diagnostics/technical/
- Colin Bourassa, essais MEMS 1.6 : https://colinbourassa.github.io/car_stuff/mems_interface/
- Tests sur MEMS 1.9 : https://memsdiag.blogspot.com/ (étude des deux trims de richesse).
- Blackbox Solutions, module MEMS 1.6/1.9 : https://www.blackbox-solutions.com/site/product?sku=SM002
- Cartographie interne déjà présente : `librosco-x64/ROSCO_COMMAND_CARTOGRAPHY.md`.

**Statut :** audit documenté. Aucune commande de modification ajoutée au binaire dans ce commit ; pas de compilation GitHub Actions déclenchée pour cet inventaire.
