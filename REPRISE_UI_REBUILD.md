# Reprise de travail — ECU MEMS Manager

## Référence de travail

- Branche unique de travail : `ui-rebuild`.
- Dernier build connu : **#483**.
- Ce fichier sert de point de reprise pour éviter toute perte de contexte lors d’un changement de discussion.

## Sujet actuel — recherche détaillée dans les XML

Le travail en cours concerne l’explorateur / moteur de recherche de la base MEMS.

La recherche doit être **détaillée à l’intérieur du contenu des fichiers XML** : elle ne doit pas se limiter à retrouver le nom d’un fichier ou d’une fiche. Elle doit pouvoir retrouver une information technique présente dans une ligne XML, par exemple une commande, un paramètre, une valeur, une broche, une couleur de fil ou une donnée associée à un ECU MEMS.

L’objectif est que l’utilisateur obtienne directement les résultats techniques pertinents contenus dans les XML.

## Défilement vertical des résultats

La zone qui affiche les résultats de recherche doit disposer d’un **défilement vertical fonctionnel** lorsque le nombre de résultats dépasse la hauteur disponible.

Le défilement doit permettre d’accéder à l’ensemble des résultats sans les tronquer.

## MEMS 1.2 — couleurs de fils

La vérification des **couleurs de fils du connecteur MEMS 1.2** reste en cours.

Les couleurs doivent continuer à être recherchées, recoupées et vérifiées.

**Ne pas intégrer ces couleurs dans le XML MEMS 1.2 tant qu’elles ne sont pas suffisamment confirmées.**

## Interdictions de modification

- **Ne pas changer de branche : rester exclusivement sur `ui-rebuild`.**
- **Ne pas revenir sur `ui-modernisation` ni travailler sur une autre branche.**
- **Ne pas intégrer les couleurs MEMS 1.2 tant que leur vérification n’est pas jugée suffisamment fiable.**
- **Ne pas modifier d’autres fichiers ou fonctions qui ne sont pas directement concernés par la demande en cours sans instruction explicite.**
- **Ne pas modifier les éléments déjà validés sans demande explicite.**

## Point de reprise

À la reprise du travail :

1. Partir de la branche `ui-rebuild` au dernier état connu, build **#483**.
2. Continuer la recherche détaillée dans le contenu des XML.
3. Vérifier que la liste des résultats peut défiler verticalement correctement.
4. Poursuivre la vérification documentaire des couleurs MEMS 1.2.
5. Ne rien intégrer concernant les couleurs MEMS 1.2 avant confirmation suffisante.
