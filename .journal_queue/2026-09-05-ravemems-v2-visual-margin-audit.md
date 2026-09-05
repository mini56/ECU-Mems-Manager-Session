## 2026-09-05 — AUDIT RAVEMEMS V2 — MARGE DE CAPTURE VISUELLE CONFIRMÉE TROP LARGE

Observation utilisateur : lors de la capture des images RAVE, une marge est ajoutée autour de l'image et elle paraît trop importante.

Vérification dans le moteur neutre RAVEMEMS V2 `5232215f5853241572f580cf1696c9f881b254f4`, fichier `ravemems/v2/core_extract.py` :
- `expanded_clip(..., margin: float = 24.0)` ajoute 24 points PDF à gauche, en haut et à droite ;
- la marge basse est encore plus grande : `margin * 1.35`, soit 32.4 points PDF ;
- cette expansion est appliquée à chaque `source_rect` avant le rendu PNG.

Le filtre des candidats est également insuffisant pour les petits éléments décoratifs :
- image acceptée si largeur/hauteur >= 18 points ;
- image acceptée si sa surface représente >= 0.003 de la page ;
- aucune classification explicite ne rejette actuellement icônes, pictogrammes, logos ou décorations ;
- lorsque le PDF n'expose pas d'image raster candidate, le fallback `page.get_drawings()` peut unir de nombreux tracés de la page en une seule grande zone.

Conclusion : l'observation utilisateur est confirmée. La marge de 24/32.4 points peut capturer du texte ou des éléments voisins, mais la correction ne doit pas se limiter à réduire une constante : il faut également corriger la sélection des candidats visuels pour empêcher les icônes/pictogrammes de devenir des preuves techniques.

Correction à intégrer dans la refonte RAVEMEMS V2 AVANT toute nouvelle extraction des 47 PDF :
1. marge de crop réduite et/ou adaptative, symétrique sauf justification documentée ;
2. conservation séparée de `source_bbox` et `crop_bbox` ;
3. règles structurelles d'exclusion des visuels décoratifs/répétitifs de page ;
4. contrôle du fallback vectoriel pour ne pas unir arbitrairement toute la page ;
5. test visuel de non-régression sur vrais schémas mécaniques, électriques et vues éclatées ;
6. audit du cas RCL0193ENG p.53 pour garantir que le schéma du contrôle du jeu axial est capturé proprement sans éléments parasites.
