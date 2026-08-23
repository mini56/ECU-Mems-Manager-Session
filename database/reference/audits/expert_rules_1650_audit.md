# Audit — règles expertes initiales 1650

## Objet

Le lot `research_enrichment_1650.qz64` alimente pour la première fois le schéma expert créé en 1640. Il ne transforme pas les critères historiques du programme en spécifications constructeur.

## Source fonctionnelle

Les règles reprennent uniquement des contrôles déjà présents dans `diagnosticpanel.cpp` de MEMS Manager et des décodages déjà utilisés par le projet.

## Règles ajoutées

| Règle | Niveau | Raison |
|---|---|---|
| Défaut ECU actif | `decoded_by_project` | présence des octets/bits DTC déjà décodés par le projet |
| Tension batterie hors 11,5–15,2 V | `plausible` | seuil indicatif du Diagnostic automatique, pas une spécification constructeur |
| Correction court terme > ±20 | `plausible` | critère indicatif existant |
| Correction long terme > ±20 | `plausible` | critère indicatif existant après recentrage du champ |
| Dwell hors 1,9–3,1 ms près de 14 V | `plausible` | contrôle déjà appliqué par MEMS Manager, limité à 13,5–14,5 V |
| Écart ralenti chaud > ±15 | `plausible` | décodage projet + seuil indicatif existant |
| IAC en butée + erreur de ralenti | `plausible` | combinaison déjà signalée par le Diagnostic automatique |
| Défaut lambda signalé | `decoded_by_project` | bits de défaut déjà décodés par MEMS Manager |
| Défaut TPS/alimentation TPS signalé | `decoded_by_project` | bits de défaut déjà décodés par MEMS Manager |

## Garde-fous

- Les règles `plausible` ne peuvent pas produire une conclusion forte dans `ExpertEngine`.
- Les recommandations demandent des contrôles et recoupements ; aucune règle n'ordonne de remplacer une pièce.
- Le moteur expert reste multi-paramètres lorsqu'une règle l'exige, notamment dwell, ralenti chaud et IAC.
- Les données `conflit_a_verifier` restent pondérées à zéro et ne peuvent pas déclencher une hypothèse.
- Le lot 1650 ne modifie aucun décodage protocolaire ni aucun mode de lecture ECU.

## Portée

Ces règles constituent un noyau initial pour rendre le dialogue IA explicable. Elles ne représentent pas encore une couverture exhaustive du diagnostic MEMS 1.2/1.3/1.6/1.9.
