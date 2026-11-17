# Journal de chantier — lecture des ECU MEMS 1.9 / 2J / 3

Règle : **distinguer recherche, présence du code, compilation, installation, essais matériels et validation utilisateur**. Un élément ne passe à « terminé » que lorsque son critère est vérifié et sa preuve conservée.

## Point de départ

- Branche principale de travail consultée : `fix-original-mems-xml-fiches-20261117` (SESSION 159 pour les fiches documentaires).
- Branche **séparée** pour la recherche K-Line : `research-mems2j-rovermems-protocol-20261117`.
- Protocole historique ROSCO 1.2/1.3/1.6 : chemin existant à ne pas altérer.
- MEMS 1.9 : réveil ISO9141 à 5 bauds et continuité ROSCO présents dans `memsinterface_dispatch.cpp`, onglet `mems19testtab.h`. **Essais réels à requalifier.**
- MEMS 2J, MEMS 3 : références de trames et de décodage externes retrouvées, **aucun lecteur C++/Qt intégré**.
- Source d'étude : [rover-mems-agent](https://github.com/james-portman/rover-mems-agent), SHA `b381af39b2031c80303c5262055aa0c0121dbdd4`, GPL-3.0.
- Étude préliminaire : [dossier protocoles](../PROTOCOLES_MEMS_19_2J_3_ROVERMEMS_2026-11-17.md).

## Journal des opérations

| Date | Travail | Constats / preuve | Statut |
|---|---|---|---|
| 17/11/2026 | Consultation des sources RoverMEMS Go | Protocoles 1.9, 2J et 3 distincts ; requêtes mesures/défauts présentes | **RECHERCHE EFFECTUÉE** |
| 17/11/2026 | Vérification de notre `memsinterface_dispatch.cpp` | 1.9 9600 8N1, BREAK et réveil 5 bauds déjà implémentés ; 2J/3 absents | **CODE CONTRÔLÉ** |
| 17/11/2026 | Recherche 01 KKL / FT232 / CH340 / Qt 5 / Windows | Câble KKL adapté en principe ; paramètres documentés ; compatibilité du périphérique utilisateur non prouvée | **DOCUMENTATION TERMINÉE / TEST MATÉRIEL EN ATTENTE** |
| 17/11/2026 | Ouverture du dossier et du plan de validation | `README.md`, recherche 01, plan de tests, présent journal | **ENREGISTRÉ SUR GITHUB** |
| — | Essais de vitesse et BREAK sur câble réel | Aucun essai réalisé | **NON COMMENCÉ** |
| — | Implémentation des pilotes C++ 2J/3 | Aucun changement des pilotes du programme | **NON COMMENCÉ** |
| — | Compilation / Setup spécifique lecture 2J/3 | Aucune compilation lancée | **NON COMMENCÉ** |
| — | Validation de valeurs/défauts sur ECU réel | Aucune capture | **NON COMMENCÉ** |

## Jalons de construction (ne pas les confondre)

| Jalon | Livrable concret | État |
|---|---|---|
| **R1** | Références protocoles et source de chaque commande | **Établi pour l'essentiel** ; variabilité à analyser |
| **R2** | Étude des câbles et pilotes Windows / procédure d'essai | **Documentée** ; matériel non testé |
| **R3** | Matrice ECU/variante/commandes acceptées | **À faire** |
| **R4** | Spécifications validées du transport, checksums, délais et trames | **À faire** |
| **R5** | Table d'unités et de décodage des défauts contrôlée | **À faire** |
| **D1** | Module indépendant 2J, tests sur trames enregistrées | **À faire** |
| **D2** | Module indépendant MEMS 3, tests sur trames enregistrées | **À faire** |
| **D3** | Liaison à l'interface ECU, mesures, erreurs et identification, sans régression 1.9/ROSCO | **À faire** |
| **T1** | Tests sans véhicule (relecture de trames, erreurs, déconnexions) | **À faire** |
| **T2** | Tests avec calculateurs réels et rapport de validation | **À faire** |
| **P1** | Setup compilé, installé et testé pour ces deux nouveaux lecteurs | **À faire** |

## Prochaine action exacte

**Poursuivre les autres points de recherche ciblés** (variantes 2J/3, traitement des refus `0x7F`, checksum, unités). Ensuite **préparer des tests hors ECU**, sans reprendre tout le programme et sans déclencher un Setup avant d'avoir un lot de sources cohérent.

Pour la recherche 01, l'action de terrain future sera de relever le modèle du câble KKL disponible et son pilote COM, puis tester séparément les trois modes : 9600 8N1 + BREAK lent ; 10400 8N1 + BREAK rapide ; 9600 8E1.

## Réserves et sécurité

- **Aucune preuve à ce stade de réponse réelle 2J/3 depuis ECU MEMS Manager.**
- Les erreurs ou TODO du code Go de référence ne sont pas des fonctionnalités à reproduire ; vérifier les longueurs et contrôles en C++.
- Ne pas confondre code libre GPL-3.0 et code réutilisable sans obligations.
- Ne pas changer silencieusement la compatibilité établie avec les MEMS 1.2, 1.3, 1.6 et 1.9.
- Ne jamais déclarer « vert » sur la simple présence d'un fichier source : seul un run exécuté le prouve et le véhicule reste à tester.
