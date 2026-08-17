# Suivi permanent des recherches MEMS

Dernière mise à jour : 2026-08-17
Branche : `ui-rebuild`
Point de départ de la campagne : build #534 / commit `2f00bfdca2b7d960e1f45dd3e21ddb0346704ee6`
Checkpoint précédent : `RECHERCHE_MEMS_REPRISE_2026-08-17.md`

## But

Ce dossier est la mémoire permanente des recherches destinées à enrichir la base ECU MEMS Manager. Il ne faut plus dépendre du contenu d'un chat pour savoir où la recherche en est.

Générations autorisées : **MEMS 1.2 / 1.3 / 1.6 / 1.9 uniquement**.

## Méthode obligatoire

Pour chaque information candidate :

1. comparer au code actuel d'ECU MEMS Manager ;
2. comparer à la base de référence reconstruite à partir de tous les `mems_reference_seed_*.qz64` et `research_enrichment*.qz64` ;
3. comparer le sens technique et pas seulement le libellé ;
4. classer le résultat : `DEJA_PRESENT`, `PRECISION`, `NOUVEAU`, `CONFLIT`, `REJETE`, `A_VERIFIER` ;
5. ne jamais écraser silencieusement un conflit ;
6. conserver source, génération, contexte et niveau de confiance ;
7. ne préparer un lot d'enrichissement qu'après recoupement suffisant.

## État de référence vérifié avant reprise

Base reconstruite du package #533 :

- intégrité SQLite : `ok` ;
- ECU : **85** ;
- affectations : **140** ;
- commandes protocole : **91**.

Éléments déjà étudiés et à ne pas recommencer inutilement :

- Colin Bourassa / `librosco` : `src/protocol.c`, `src/rosco.h`, historique 0x7D/0x80 ;
- Andrew Jackson / `rosco` et MemsFCR : commandes, structures, réglages, actionneurs, IAC ;
- James Portman / `rover-mems-documentation` : MEMS 1.3, 1.6, diagnostics, câbles, 5AS ;
- Blackbox Solutions : valeurs de service et fonctions diagnostic MEMS 1.6/1.9.

## Résultats déjà retenus / conflits déjà connus

- `7D:02` TPS angle : brut × 0,6 ;
- `7D:04` AFR : brut / 10 ;
- `7D:06` lambda : brut × 5 mV, ancien conflit documentaire conservé ;
- `7D:1F` Jack Count : offset dépend encore des implémentations Andrew/MemsFCR ;
- `80:19` crankshaft position : ne pas réduire à un booléen sans preuve ;
- commandes à comparer précisément avant ajout : `0x0F`, `0xFA`, `0xCC`, `0xFB`, `79/7A`, `7B/7C`, `89/8A`, `91/92`, `93/94`, `FD/FE` ;
- actionneurs présents sous `#if 0` dans librosco = **non confirmés** ;
- masques DTC contradictoires entre implémentations = **ne pas importer aveuglément** ;
- `MNE10050` candidat MEMS 1.2 = deuxième source indépendante requise ;
- fiches XML 1.3 / 1.6 / 1.9 : brochages à revalider ligne par ligne.

## File de recherche active

1. Brochage MEMS 1.3 : reconstruction source A / source B / fiche actuelle.
2. Brochage MEMS 1.6 : connecteur principal / second connecteur / variantes.
3. Brochage MEMS 1.9 : vérification ligne par ligne, notamment pins 1, 5, 10, 13, 31, 32.
4. Commande `0x0F` : sens exact reset adjustments / service adjustments.
5. Confirmation indépendante `7D:1F` Jack Count.
6. Sens/plage de `80:19`.
7. Masques DTC contradictoires.
8. Modes diagnostic `F0`, `F4` et autres séquences.
9. Deuxième source pour `MNE10050`.
10. Seuils Blackbox vs code d'analyse MEMS Manager.

## Journal des nouvelles recherches

| Date | Sujet | Source | Comparaison base/code | Statut | Décision |
|---|---|---|---|---|---|
| 2026-08-17 | Reprise après build #534 | Checkpoint dépôt + règles permanentes | État recadré | DEJA_PRESENT | Continuer sans réimporter les résultats déjà consignés |

## Prochaine action exacte

Reprendre par les brochages MEMS 1.3 / 1.6 / 1.9 et construire un tableau comparatif avant toute modification XML ou base. Ensuite traiter les conflits protocole et DTC. Mettre ce fichier à jour à chaque série de recherches avant de quitter la session.
