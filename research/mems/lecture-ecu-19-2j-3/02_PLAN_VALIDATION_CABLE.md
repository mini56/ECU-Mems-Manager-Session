# Recherche 01 — Plan de validation des câbles KKL

**Date : 17/11/2026** — aucun essai matériel réalisé dans cette recherche.

## Principe

Effectuer des essais **non destructifs**, d'abord côté port USB-série puis, lorsque le calculateur et son branchement sont disponibles, côté K-Line. Documenter les trois familles **séparément**, sans lancer de balayage automatique de protocoles.

**Un test réussi sur un port COM n'est pas un test ECU réussi.**

## Fiche à remplir pour chaque matériel

| Champ | Valeur |
|---|---|
| Identifiant du câble / numéro d'essai | À relever |
| Fabricant et puce (FT232R, CH340G, autre) | À relever |
| Vrai transceiver K-Line et type de connecteur | À confirmer |
| Windows (version, 32/64 bits) | À relever |
| Nom du port (COMx) | À relever |
| Version du pilote | À relever |
| Génération ECU et référence exacte | À relever |
| Véhicule / banc de test | À relever |
| État contact / moteur | À relever |
| Date et référence du programme | À relever |
| Trace de lecture et de réponses | À enregistrer |

## Étape A : vérification sans calculateur

1. **Identifier le câble** dans le Gestionnaire de périphériques Windows (menus/souris), noter port COM et fabricant/pilote.
2. Vérifier l'ouverture exclusive du bon port par `QSerialPort`.
3. Demander **9600 8N1**, puis **10400 8N1**, puis **9600 8E1**, avec fermeture et réouverture entre les tests ; vérifier les codes d'erreur et valeurs retournées par le pilote.
4. Vérifier que `setBreakEnabled(true)` puis `setBreakEnabled(false)` ne sont pas rejetés.
5. Ne PAS conclure que la vitesse physique réelle est exacte ni que la largeur d'impulsion K-Line est correcte à partir de cette étape seule.
6. Rétablir la configuration et libérer le port à la fin, sans changement permanent de pilote et sans écriture sur un ECU.

**Résultat attendu :** trois configurations acceptées par Windows et contrôle BREAK disponible, ou erreur consignée.

## Étape B : validation avec le calculateur correspondant

Avant tout essai, confirmer **interface K-Line et câblage adaptés au véhicule/ECU**. Interrompre si la référence ou le brochage est incertain.

| ECU | Préparation et première requête | Validation minimale |
|---|---|---|
| MEMS 1.9 | 5 bauds, adresse `0x16`, puis reprise 9600 8N1 et dialogue ROSCO existant | Synchronisation/confirmation, identification ECU, réponse de mesure cohérente |
| MEMS 2J | Fast Init 25/25 ms, `81 13 F7 81 0C`, 10400 8N1 | Réponse `C1 D5 8F`, démarrage diagnostic, trame `21 09` avec réponse `61 09` |
| MEMS 3 | 9600 8E1, `1A 9A`, trame format Rover/BMW | Réponse `5A 9A`, session, `21 06` et données de régime/MAP |

Les paramètres énumérés sont documentés par RoverMEMS ; ils **ne constituent pas encore des résultats de notre matériel**.

## Qualité des trames et délais

Le lecteur doit obligatoirement tester et journaliser :

- **Echo USB/K-Line** vs réponse de l'ECU.
- **Longueur, en-tête, contrôle de trame** (somme modulo 256 du 2J, XOR dans la voie 9600 du MEMS 3).
- **Accusés de réception, `7F` refus, timeouts, trames partielles, contrôle BREAK**.
- **Identifiant ECU/variante** et décision de compatibilité ; ne pas supposer que toutes les commandes `21 xx` sont disponibles.
- **Unités** : pas d'unité injecteur inventée pour une valeur brute.
- **Séparation des états de confiance** : requête documentée, réponse lue, conversion validée.
- Déconnexion propre et réouverture de port sans interférer avec les autres générations.

**Interdit pendant cette étape :** effacement des défauts, écritures mémoire, adaptation, immobiliseur, flash ou déclenchement d'actionneurs. Les fonctions d'identification/sessions/accès demandées pour obtenir des valeurs sont distinctes d'une reprogrammation.

## Fichiers de validation envisagés

Noms indicatifs pour une future instrumentation du programme (aucune fonctionnalité créée à ce stade) :

- `MEMS_<generation>_<date>_COM.txt` : câble, pilote, configuration, ouverture et erreurs.
- `MEMS_<generation>_<date>_TRAMES.csv` : horodatage, direction, requête/réponse hexadécimale, écho, checksum, résultat.
- `MEMS_<generation>_<date>_MESURES.csv` : valeurs brutes, calcul, unité, statut (validé/à vérifier/indisponible).
- `MEMS_<generation>_<date>_BILAN.md` : étapes PASS / PARTIEL / ÉCHEC, problèmes et références ECU.

Pour le MEMS 1.9, **réutiliser et respecter** autant que possible la journalisation déjà fournie par l'onglet `Test ECU 1.9`, plutôt que créer des doublons.

## Cas d'échec à ne pas masquer

- Le pilote accepte `10400` mais l'ECU ne répond pas : **vitesse réelle / BREAK / câblage / ECU à départager**, pas « câble compatible ».
- Le port s'ouvre mais BREAK échoue : arrêt du réveil 1.9/2J sur ce câble.
- Réponse négative sur commande optionnelle : valeur marquée indisponible, aucune reconnexion en boucle.
- Réponse d'un autre calculateur ou non-identification : ne jamais envoyer de commande adaptée à une famille supposée.
- Échec sans ECU disponible : **non testé**, pas « validé ».
- Tests SPI 1.2/1.3/1.6 : les valider séparément pour établir la non-régression.

## Critères pour clôturer la recherche n° 1

- [x] Documentation constructeur/éditeurs examinée.
- [x] Paramètres 1.9, 2J, MEMS 3 documentés.
- [x] Possibilités logicielles Qt 5 identifiées.
- [ ] Identification du câble de test réel et du pilote Windows.
- [ ] Vérification du comportement réel 10400 bauds et BREAK 25 ms.
- [ ] Vérification de la réponse K-Line de chaque ECU disponible.
- [ ] Rapport de validation signé par les logs ; non-régression ROSCO.

Les cases non cochées sont **des travaux futurs**, pas des tests exécutés.
