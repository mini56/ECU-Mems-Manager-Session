
## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — avant diagnostic pixel des 737 fidélités visuelles

Le run 33794551649 confirme que la suppression de l'arrondi des BBOX ne change pas le résultat : 1/738 fidélités visuelles vérifiées, 737/738 refusées, tandis que 401/401 liens visuels sont vérifiés et tous les gardes texte/sémantiques restent à zéro défaut.

Avant toute nouvelle correction, une passe diagnostique va mesurer sur les 737 écarts : différence absolue maximale, différence moyenne, nombre/proportion d'octets pixels différents, origine du Pixmap et espace couleur. Le validateur restera strict et continuera d'échouer tant que la cause n'est pas démontrée. Aucun changement sur MEMSX64, protégé sur BUILD #103 1d6316bd1746d6f2b4cfb751cab88d18e27ef730.
