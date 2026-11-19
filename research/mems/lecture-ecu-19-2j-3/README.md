# ECU MEMS Manager — chantier de lecture MEMS 1.9 / 2J / 3

**Dernière mise à jour documentaire : 19 novembre 2026**  
**Responsable du suivi : dossier GitHub ; les discussions ne font pas office d'historique.**  
**Branche de recherche :** `research-mems2j-rovermems-protocol-20261117`  
**Portée : lecture seule, sans effacement des défauts ni programmation.**

## Objectif du chantier

Permettre à **ECU MEMS Manager SESSION** de communiquer avec les calculateurs **MEMS 1.9, 2J et 3**, de recevoir leurs mesures moteur et défauts, et de restituer ces résultats dans les pages existantes du programme, **sans régression** pour MEMS 1.2/1.3/1.6.

La présence d'un protocole dans une documentation, l'existence d'un code C++ et la validation sur un calculateur sont trois choses distinctes : ne jamais les déclarer équivalentes.

## Tableau d'avancement (constaté sur les sources, pas sur véhicule)

| Fonction / jalon | MEMS 1.9 | MEMS 2J | MEMS 3 |
|---|---|---|---|
| Principes du protocole documentés | **OUI** | **OUI** | **OUI** |
| Exemples de trames et décodages retrouvés | **OUI** | **OUI** | **OUI** |
| Recherche compatibilité KKL / Windows | **ÉTUDE 1 OUVERTE** | **ÉTUDE 1 OUVERTE** | **ÉTUDE 1 OUVERTE** |
| Lecteur dédié implémenté en C++/Qt | **OUI** (réveil 5 bauds + chemin ROSCO existant) | **NON** | **NON** |
| Tests hors calculateur du lecteur complet | **NON DÉMONTRÉ** | **NON COMMENCÉ** | **NON COMMENCÉ** |
| Première identification réussie sur ECU réel | **NON DOCUMENTÉE** | **NON DOCUMENTÉE** | **NON DOCUMENTÉE** |
| Valeurs moteur / défauts validés sur ECU réel | **NON** | **NON** | **NON** |
| Disponible à l'utilisateur avec validation complète | **NON** | **NON** | **NON** |

**Source actuelle du 1.9 :** `memsinterface_dispatch.cpp` (`performMems19Wakeup()` et `connectToECU()`) ; onglet de tests `mems19testtab.h`. Le test matériel réel reste à consigner.

**Source du travail 2J/3 :** étude du dépôt RoverMEMS de James Portman ; **aucun pilote 2J ou 3** n'a été ajouté au programme.

## Documents vivants dans ce dossier

1. **[01 — Recherche sur les interfaces KKL et Windows](01_COMPATIBILITE_KKL_WINDOWS.md)** : premier thème demandé ; faits vérifiés, limites et sources.
2. **[02 — Plan de tests des câbles et du transport](02_PLAN_VALIDATION_CABLE.md)** : critères de validation, journaux attendus, limites de sécurité.
3. **[03 — Journal et étapes du développement](03_JOURNAL_ET_ETAPES.md)** : décisions, avancement et prochaines actions.
4. **[09 — Décisions du 19/11/2026 : câbles USB et connexions MEMS 1.9/2J](09_DECISIONS_CABLES_USB_CONNEXION_19_2J_20261119.md)** : captures USB réelles (CH340 COM6, FTDI COM4), distinction câble/puce, initialisations, principe d'identification à la connexion, préservation des 15 pages et de ROSCO, travaux restant à valider.

Étude préalable des trames : [PROTOCOLES_MEMS_19_2J_3_ROVERMEMS_2026-11-17.md](../PROTOCOLES_MEMS_19_2J_3_ROVERMEMS_2026-11-17.md).  
État de recherche historique 1.2 à 1.9 : [ETAT_RECHERCHE.md](../ETAT_RECHERCHE.md) (daté août 2026 : ne pas changer rétroactivement ses décisions d'alors).

## Ordre de travail

- **Étude 1 — Compatibilité interface KKL / Windows :** documentation terminée pour le principe ; compatibilité du câble physique **à tester**.
- **Étude 2 — Variantes de calculateurs et commandes acceptées :** à documenter.
- **Étude 3 — Fiabilisation des trames et délais :** à documenter puis à implémenter.
- **Étude 4 — Unités et décodage des défauts :** à documenter puis à tester.
- **Développement indépendant 2J et 3 :** non démarré.
- **Intégration dans les pages ECU, mesures et erreurs :** non démarrée.
- **Campagne de tests sur vrais ECU :** non démarrée.

## Règles permanentes

- Zéro patch accumulé : modification de sources propre, un module par protocole.
- Aucune commande d'effacement, d'écriture, de recodage d'antidémarrage ni de flash dans les premiers lecteurs.
- N'envoyer aucun réveil 2J/3 à un ECU non identifié par simple essai automatique.
- Préserver la liaison 3 broches ROSCO pour 1.2/1.3/1.6, et le réveil 1.9 existant.
- Enregistrer **type ECU, identifiant appareil, vitesse, parité, version du pilote, requêtes et réponses**, distinguer données brutes et données interprétées.
- Un exemple Go sous GPL-3.0 est une source d'étude : **pas de copier-coller de code** sans examen des obligations de licence.
- Une compilation réussie ne signifie jamais une lecture ECU validée ; noter séparément le résultat de chaque test.
- **Aucun changement du code applicatif ni aucun Setup** pour la seule ouverture de ce dossier de recherches.

## Référence d'origine

[RoverMEMS Agent (James Portman)](https://github.com/james-portman/rover-mems-agent) — commit `b381af39b2031c80303c5262055aa0c0121dbdd4`, licence GPL-3.0 ;  
[Andrew Revill — familles de calculateurs et câbles](https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm) ;  
[Qt 5 — QSerialPort](https://qthub.com/static/doc/qt5/qtserialport/qserialport.html).
