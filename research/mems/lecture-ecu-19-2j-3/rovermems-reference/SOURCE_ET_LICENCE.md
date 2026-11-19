# Référence de développement — RoverMEMS, calculateurs MEMS 1.9 et 2J

Ce dossier **documentaire** conserve six fichiers source originaux du projet libre [RoverMEMS Agent](https://github.com/james-portman/rover-mems-agent), auteur du dépôt : **James Portman**.

- `ecu-2j.go` — communication et commandes MEMS 2J.
- `ecu-2j-parse.go` — interprétation des réponses et valeurs moteur.
- `ecu-2j-faults.go` — interprétation des défauts.
- `ecu-auth.go` — algorithme de réponse au défi (seed/key) pour la session 2J.
- `serialReadWrite.go` — lecture série employée par le programme Go de référence.
- `ecu-19.go` — réveil et échanges initiaux MEMS 1.9.

## Provenance vérifiable

- Dépôt original : https://github.com/james-portman/rover-mems-agent
- Révision de référence : `b381af39b2031c80303c5262055aa0c0121dbdd4`
- Sources d'origine, conservées sans modification :
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j-parse.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j-faults.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-auth.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/serialReadWrite.go
  - https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-19.go

## Analyses complémentaires

- [Audit 2J : variantes, mesures et défauts](../10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md)
- [Trames, conversions et prévalidation 1.9/2J](../11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md)

## Licence

Les sources du projet RoverMEMS sont sous **GNU GPL version 3**. Texte original : https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/LICENSE ; le dépôt ECU-Mems-Manager-Session contient aussi un fichier `LICENSE` GNU GPLv3.

Conserver l'attribution et les indications de licence. En cas de redistribution d'un logiciel utilisant ces sources, respecter les conditions de la GPLv3 (notamment mise à disposition du code source correspondant et signalement des modifications).

**État d'intégration :** références sauvegardées pour l'étude et le portage C++/Qt ; ces fichiers Go ne sont **pas** compilés, ni intégrés dans le pilote applicatif. Aucun changement des méthodes de connexion MEMS ou des écrans n'est réalisé ici.

**Suivi :** intégration sur une seule branche de développement, pas de patchs accumulés, aucun verrou ajouté sans accord.
