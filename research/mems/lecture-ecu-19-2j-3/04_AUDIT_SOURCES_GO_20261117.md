# Audit complémentaire des sources Go RoverMEMS — 17 novembre 2026

**Nature : recherche documentaire seulement. Aucun code ECU modifié, aucune commande exécutée sur un véhicule.**

Sources examinées directement : dépôt [james-portman/rover-mems-agent](https://github.com/james-portman/rover-mems-agent), fichiers `ecu-19.go`, `ecu-2j.go`, `ecu-2j-parse.go`, `ecu-2j-faults.go`, `ecu-3.go` sur branche `master`. Voir également `PROTOCOLES_MEMS_19_2J_3_ROVERMEMS_2026-11-17.md` (révision fixée et paramètres de liaison).

## MEMS 1.9

- Ne pas recréer un second pilote : le projet possède `memsinterface_dispatch.cpp`, une initialisation lente 5 bauds à adresse 0x16 et le test guidé `mems19testtab.h`.
- Le code Go `ecu-19.go` contient une commande supplémentaire 0x7C après son réveil et attend une réponse avec écho et 0xE9. **À comparer** avec le code 1.9 existant avant toute modification : les étapes détaillées des deux implémentations ne doivent pas être fusionnées aveuglément.
- Le code Go lui-même marque le timing d'une pause et la réponse à un octet-clé comme `TODO` : ce n'est pas une preuve de validation universelle.

## MEMS 2J

- `ecu-2j.go` définit l'initialisation `81 13 F7 81 0C` après fast-init et la lecture des paramètres par requêtes `21 xx`; `21 19` est la lecture des défauts.
- Une boucle de lecture comporte explicitement `TODO: check checksum ?`, un `TODO` sur les tailles de paquets et un commentaire sur la reprise intempestive du polling lorsqu'une lecture `0x21` est refusée.
- `ecu-2j-parse.go` indique expressément `todo: cope with 7F (fail/no)` : la future implémentation SESSION doit traiter la réponse négative, sans effacer ni quitter toute la session.
- **Conflit précis confirmé dans la table des défauts** `ecu-2j-faults.go` : deux libellés distincts « Coolant temp (present) » et « System voltage (present) » utilisent *le même bit* `buffer[12] >> 2`, avec un commentaire `one of these two is a typo`. Statut **CONFLIT_A_VERIFIER** : ne pas importer ces deux définitions comme certitudes.
- La fonction d'effacement et les autres fonctions de modification présentes dans l'application d'origine sont **hors périmètre** du pilote lecture seule.

## MEMS 3

- `ecu-3.go` définit l'en-tête `B8 13 F7`, l'initialisation `1A 9A`, les lectures `21 00`, `21 06`, `21 0A`, `21 0B`, `21 21` et la requête de défauts `18 00 00 00`.
- La boucle de réception inclut un `TODO: check checksum ?` : l'implémentation ne doit pas accepter une réponse invalide sans contrôle.
- Le code reconnaît des réponses négatives `7F` incomplètement (`todo: cope with 7F`). Les paquets partiels exigent des contrôles de bornes avant chaque accès.
- Les défauts sont décodés en groupes de trois octets (identifiant 16 bits + statut). Le dictionnaire d'interprétation doit rester distinct des trames brutes, avec mention de son niveau de confiance.
- Pas d'activation de l'effacement `14 00 00` au premier déploiement.

## Décision de conception avant code

1. Conserver **trois pilotes séparés** derrière une interface de lecture commune : 1.9 existant, 2J 10400/8N1 et 3 9600/8E1.
2. Construire d'abord des **parseurs purs** testés sur trames enregistrées : longueur, header, checksum, négatifs `7F`, timeout, écho, réponse tronquée.
3. Ne pas ouvrir simultanément le même port série depuis deux pilotes. L'onglet « Test câble USB » reste facultatif.
4. Lecture seule initiale : identification, températures, régime, MAP, papillon, tensions, lambda, défauts, lorsque prouvés par la source.
5. Valider le matériel sur ECU réel séparément de la compilation Windows : un Setup vert n'est pas une validation de protocole.

## Travail restant

- Recouper les paramètres d'initialisation avec `rovermems.com/mems-2j/` et `rovermems.com/mems-3/`.
- Construire un inventaire **par commande** : requête, réponse attendue, longueur, conversion, référence source, présence éventuelle dans SESSION.
- Séparer les éléments `DEJA_PRESENT`, `PRECISION`, `NOUVEAU`, `CONFLIT_A_VERIFIER`.
- Préparer captures et tests de conformité, puis seulement démarrer le pilote 2J de lecture seule.
- Examiner les obligations GPL-3.0 avant toute reprise de code source tiers ; privilégier une réimplémentation C++ documentée sur faits de protocole.

**Aucun Setup ou run n'est nécessaire pour enregistrer cette recherche.**
