# Règles permanentes — recherche et enrichissement MEMS

Ce fichier définit la méthode obligatoire pour toute recherche future destinée à enrichir la base de données d’ECU MEMS Manager.

## Portée

- Générations concernées : MEMS 1.2, 1.3, 1.6 et 1.9 uniquement.
- Branche de travail : `ui-rebuild`.
- La recherche documentaire ne doit pas modifier le comportement applicatif validé sans demande explicite.

## Méthode obligatoire avant tout ajout

Pour chaque information trouvée sur Internet, dans un logiciel tiers, dans un dépôt source, dans une documentation constructeur ou dans une capture de communication :

1. Vérifier d’abord si l’information existe déjà dans le **code actuel d’ECU MEMS Manager**.
2. Vérifier ensuite si elle existe déjà dans la **base de référence réellement reconstruite** à partir des `mems_reference_seed_*.qz64` et de tous les lots `research_enrichment*.qz64` du build courant.
3. Comparer le **sens technique**, pas seulement le texte exact : une même commande, fonction, trame ou donnée peut déjà être présente sous un autre nom ou une autre traduction.
4. Si l’information est déjà présente avec le même sens et le même niveau de précision : **ne rien ajouter**.
5. Si la nouvelle source apporte une précision supplémentaire : enrichir ou compléter l’enregistrement existant au lieu de créer un doublon.
6. Si une nouvelle source contredit une information existante : ne jamais écraser silencieusement l’ancienne. Enregistrer le désaccord comme **conflit à vérifier**, avec les deux sources.
7. Si l’information est réellement nouvelle : l’ajouter avec sa source, la génération concernée, son contexte technique et son niveau de confiance.

## Détection des doublons

La comparaison doit au minimum prendre en compte :

- génération MEMS ;
- octet / commande hexadécimale ;
- catégorie de commande ;
- séquence d’initialisation ;
- format et longueur de réponse ;
- trame 0x7D / 0x80 et offsets ;
- fonction / actionneur ;
- paramètre ou réglage ;
- DTC / défaut ;
- broche / connecteur / couleur de fil ;
- ECU / référence / véhicule / marché ;
- source et observation matérielle.

Deux lignes différentes en texte mais techniquement équivalentes ne doivent pas être comptées comme deux découvertes.

## Niveaux de confiance et sources

- Une donnée constructeur ou une donnée explicitement vérifiée par le projet doit rester distinguée d’une source externe.
- Une observation réellement capturée doit être enregistrée comme observation, pas transformée automatiquement en règle universelle.
- Une information non vérifiée sur toutes les variantes doit être marquée clairement comme telle.
- Les contradictions doivent utiliser un statut de type `conflit_a_verifier` plutôt que choisir arbitrairement une version.
- Ne jamais inventer une fonction, une commande, un brochage ou une couleur pour compléter une fiche.

## Enrichissement de la base

- Ne pas créer un nouveau lot pour chaque petite découverte.
- Regrouper les vraies nouveautés dans un lot d’enrichissement cohérent.
- Avant le push, reconstruire la base complète et vérifier les doublons une seconde fois.
- Après ajout, contrôler l’intégrité SQLite, les compteurs attendus et le self-test de recherche.
- Toute nouvelle information doit rester retrouvable par le moteur de recherche global avec son contexte réel.

## Règle de non-régression

**Recherche → comparaison avec MEMS Manager réel → suppression des doublons → ajout uniquement des informations nouvelles ou plus précises → source + niveau de confiance + conflit explicite si nécessaire.**

Cette règle est obligatoire pour toutes les futures campagnes de recherche MEMS.
