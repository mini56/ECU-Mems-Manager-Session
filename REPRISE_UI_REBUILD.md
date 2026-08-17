# Reprise de travail — ECU MEMS Manager

## Référence de travail VALIDÉE

- Branche unique de travail : `ui-rebuild`.
- **Dernier build validé fonctionnellement par l’utilisateur : #518.**
- Commit applicatif validé : `9fba41d125030616c9eb35eef07a10a00a90e138`.
- Le build #518 reste le **point de non-régression fonctionnel** pour l’explorateur et le moteur de recherche MEMS.
- Ce fichier sert de point de reprise pour éviter toute perte de contexte lors d’un changement de discussion.
- **Ne jamais repartir de #483 pour remplacer l’état actuel.** #483 reste seulement un ancien repère historique ; #518 est la référence fonctionnelle validée.

## Moteur de recherche MEMS — comportement validé à conserver

Le moteur de recherche de #518 fonctionne et son comportement doit être conservé dans toutes les modifications futures.

### Indexation obligatoire

- Indexer **tout le contenu technique réel de la base SQLite**.
- Indexer **chaque ligne XML** avec son contexte technique.
- Indexer le contenu **mot par mot** dans `search_terms`.
- Un mot seul doit pouvoir retrouver toutes les lignes techniques concernées.
- Une recherche multi-mots doit combiner les termes sans exiger une phrase exacte.
- Recherche insensible à la casse et aux accents.
- Ne jamais ajouter de faux synonymes ou mots artificiels dans toutes les lignes d’une catégorie.
- Ne jamais ajouter d’exception codée en dur pour un mot particulier (`vert`, `rouge`, `IAT`, etc.).

### Relations techniques à préserver

Une ligne trouvée doit conserver son contexte réel :

- génération MEMS ;
- section XML ;
- broche / pin ;
- fonction / composant ;
- couleur de fil ;
- commande ;
- valeur ;
- paramètre ;
- DTC ;
- protocole ;
- actionneur ;
- mesure / donnée ;
- documentation / référence / note.

### Filtres

- Les filtres de catégorie et de génération doivent uniquement **affiner** la recherche.
- Le texte saisi reste toujours du **contenu à rechercher** ; il ne doit jamais être interprété automatiquement comme un changement de catégorie.
- Le filtre Câblage ne doit pas lancer de requête cachée dans Documentation.
- Le classement des tables doit rester cohérent : par exemple `protocol_profiles` appartient à **Protocole**, pas à Fichier.

### Pertinence et affichage

- Conserver l’ordre de pertinence fourni par le moteur de recherche ; ne pas retrier ensuite les résultats avec une priorité fixe de catégorie.
- Utiliser des titres techniques utiles lorsqu’ils existent, jamais des identifiants numériques comme titre principal si un nom technique est disponible.
- Les fiches XML doivent conserver le défilement vertical.
- Les rectangles / pastilles de couleur des fils doivent rester visibles dans les fiches techniques, y compris lorsque la couleur provient d’une cellule XML générique.

## Architecture de recherche validée

- `MemsGlobalSearchIndex.cpp` est le **constructeur central de l’index**.
- L’index doit être construit de manière déterministe avant utilisation de l’explorateur.
- **Ne pas réintroduire d’indexation différée par `QTimer`.**
- `MemsXmlRowIndexer` et `MemsSearchCompletenessPatch` ne doivent pas réécrire ou compléter tardivement l’index global.
- Les modules secondaires peuvent conserver uniquement les fonctions d’affichage nécessaires (scroll, rendu XML, couleurs), sans mutation tardive de l’index.
- La base de référence enrichie doit être conservée.
- À chaque nouveau numéro de build, le cache généré de base/XML/index doit être renouvelé afin qu’un nouveau build ne réutilise pas silencieusement un ancien cache.

## Self-test de non-régression — OBLIGATOIRE

Le test `mems_search_selftest` fait partie du garde-fou fonctionnel et ne doit pas être supprimé ou rendu non bloquant.

Le build #518 a validé automatiquement :

- index : **950 documents** ;
- `vert rouge` + catégorie `wiring` → **4 résultats** ;
- `IAT` + `wiring` → **4 résultats** ;
- `temperature air` + `wiring` → **2 résultats**, preuve de recherche sans accent ;
- `D0` + `command` → **2 résultats** ;
- `P0115` + `dtc` → **1 résultat** ;
- `tension batterie` + `data` → **2 résultats** ;
- `9600` + `protocol` → **8 résultats** ;
- test de pertinence `IAT` : premier résultat validé = **`[wiring] 6 — IAT — Vert / Rouge`** ;
- résultat final : **`PASS MEMS search semantic self-test`**.

Le self-test contient désormais aussi un contrôle MEMS 1.2 : une recherche `connecteur 36 voies` dans `wiring` doit retrouver la fiche MEMS 1.2 avec son connecteur 36 voies.

Une modification future du moteur de recherche qui casse l’un de ces contrôles doit faire échouer le build et être considérée comme une régression jusqu’à correction.

Ces exemples sont des **tests de contrôle**, pas une liste exhaustive des mots que le moteur doit connaître. Le principe reste : **tout mot réel contenu dans la base SQLite ou les XML doit être indexé et retrouvable dans son contexte réel.**

## Fiche MEMS 1.2 — VALIDÉE VISUELLEMENT

- La fiche `database/reference/fiches/mems_1_2.xml.qz64` a été créée à partir de la fiche XML présentée visuellement à l’utilisateur puis validée avant intégration.
- Elle comporte le connecteur ECU **36 voies** et les 36 lignes de brochage.
- Les couleurs et affectations non universelles ne doivent jamais être présentées comme certaines : elles portent exactement **« À vérifier suivant modèle du véhicule »**.
- Ne jamais utiliser le statut « probable » dans cette fiche.
- La fiche contient aussi les informations ROSCO, la trame 0x80, les défauts documentés et les sources utilisées.

### Connecteurs réels à conserver dans les schémas

- **MEMS 1.2 : 1 connecteur ECU 36 voies.**
- **MEMS 1.3 : 2 connecteurs ECU, 36 + 18 voies.**
- **MEMS 1.6 : connecteur principal 36 voies ; variante 36 + 18 voies = « À vérifier suivant modèle du véhicule ».**
- **MEMS 1.9 : 1 connecteur ECU 36 voies.**
- **MEMS 1.2 / 1.3 / 1.6 : prise diagnostic Rover / ROSCO 3 broches.**
- Le SVG ROSCO principal est **noir**, en **vue de face uniquement** ; pas de vue de côté.
- La famille de prise 3 broches existe aussi en variante verte pour l’immobilisateur ; ne pas transformer la fiche générale en vert et ne pas confondre cette variante avec le connecteur ECU.
- **MEMS 1.9 : prise diagnostic 16 broches type J1962 / OBD**, utilisée pour la communication MEMS/K-Line et à ne pas présenter comme une simple interface OBD-II générique.

Les SVG intégrés sont stockés dans `database/reference/images/` et doivent conserver le style dark harmonisé noir/gris, texte blanc et accents orange.

## Base de référence

- Conserver la base enrichie/réparée actuelle.
- Ne pas supprimer les enrichissements de recherche déjà intégrés.
- Générations concernées : MEMS 1.2, 1.3, 1.6 et 1.9.
- Les fiches XML présentes concernent désormais **1.2, 1.3, 1.6 et 1.9**.
- Les informations MEMS 1.2 incertaines restent dans la fiche mais doivent être explicitement marquées **« À vérifier suivant modèle du véhicule »**.

## Interdictions de modification

- **Ne pas changer de branche : rester exclusivement sur `ui-rebuild`.**
- **Ne pas revenir sur `ui-modernisation` ni travailler sur une autre branche.**
- **Ne pas revenir en arrière sous #518 sur le moteur de recherche.**
- **Ne pas supprimer l’indexation mot par mot.**
- **Ne pas réintroduire les anciens indexeurs temporisés.**
- **Ne pas remettre de faux mots/synonymes génériques dans les lignes indexées.**
- **Ne pas remettre de tri fixe par catégorie après la recherche.**
- **Ne pas supprimer ou contourner le self-test sémantique bloquant.**
- **Ne pas modifier les éléments déjà validés sans demande explicite.**

## Point de reprise actuel

À toute reprise de travail :

1. Partir de `ui-rebuild` et considérer **#518 / `9fba41d125030616c9eb35eef07a10a00a90e138`** comme référence fonctionnelle validée du moteur de recherche.
2. Préserver toutes les fonctionnalités demandées avant et après #483 ; ne jamais faire un reset qui en supprimerait une partie.
3. Conserver la fiche MEMS 1.2 et les schémas de connecteurs décrits ci-dessus.
4. Vérifier que le self-test sémantique reste bloquant et vert après toute modification du moteur/base/XML.
5. Ne modifier l’architecture de recherche qu’en conservant au minimum tous les comportements validés dans #518.
6. Continuer les travaux futurs depuis ce socle sans régression.
