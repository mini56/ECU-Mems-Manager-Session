# Réparation base MEMS — build #501 / v1.5.0

## Incident

Le fichier `database/reference/research_enrichment.qz64` ajouté au build #499 contenait un flux zlib invalide. Le build #500 compilait malgré cela car le workflow principal ne reconstruisait que les quatre seeds historiques et ne testait pas les lots d'enrichissement.

## Correction

Le lot #499 a été reconstruit à partir des données/provenances déjà archivées et des données de référence ECU MEMS Manager. Les fonctions déjà présentes dans MEMS Manager restent attribuées en priorité à ECU MEMS Manager ; les sources externes servent de recoupement.

Le lot réparé fournit exactement :

- 13 faits techniques/protocole ;
- 103 observations de commandes ;
- 60 observations de champs/trames 7D/80 ;
- 11 entrées de pannes connues/plausibles ;
- 30 références firmware/ECU.

Total lot #499 : **217 lignes actives**.

Le lot #500 reste inchangé et valide : **117 lignes actives**.

Après reconstruction complète des quatre seeds + #499 + #500 :

- 85 ECU de base ;
- 140 affectations de base ;
- 91 commandes de base ;
- 813 lignes structurées au total ;
- `PRAGMA integrity_check = ok`.

## Prévention

Le workflow `Validate MEMS Reference Database` reconstruit désormais réellement la SQLite à chaque push, décompresse chaque qz64 actif, valide les XML, vérifie les comptes des lots #499/#500 et exécute `PRAGMA integrity_check`.

## Règles de provenance conservées

- `decoded_by_project` : décodage/propriété ECU MEMS Manager ;
- `verifie_constructeur` : documentation constructeur identifiable ;
- `recoupee` : plusieurs éléments techniques cohérents ;
- `source_externe` : source identifiable mais non constructeur/non suffisamment recoupée ;
- `plausible` : hypothèse technique ;
- `non_verifie` : information incertaine ;
- `conflit_a_verifier` : sources contradictoires, aucune valeur forcée.

MEMS FCR n'est pas utilisé comme source finale d'une valeur sans source originale vérifiable.
