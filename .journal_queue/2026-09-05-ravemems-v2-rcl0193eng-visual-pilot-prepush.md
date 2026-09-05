## RAVEMEMS V2 — PRE-POUSSE PILOTE VISUEL RCL0193ENG

Décision avant toute modification technique sur `tmp-ravemems-v2-source-rebuild`.

### Périmètre
- Pilote uniquement : `RCL0193ENG` / `rave/xn/wmxn990e.pdf`.
- Aucun changement MEMSX64, protocole, acquisition, RAM, écriture ECU, SQLite runtime ou DLL.
- L'extraction du texte et la lecture de l'en-tête restent inchangées. L'exclusion d'en-tête concerne uniquement les candidats visuels.

### Constats directs sur le PDF source
- Page physique 53 : icône ENGINE raster dans l'en-tête, bbox env. `[471.8,17.7,552.4,58.0]`.
- Page physique 53 : vraie illustration mécanique raster, bbox env. `[63.9,210.6,252.0,502.5]`.
- Le rectangle raster source exact contient déjà l'illustration complète et son repère `12M3440` ; aucune marge de crop n'est nécessaire pour ce candidat.
- Les petits pictogrammes de sections (ENGINE, batterie, freins, SRS, etc.) sont des ressources raster répétées de nombreuses fois dans le document et expliquent une part importante des faux visuels historiques.

### Correctif pilote à pousser
1. Ajouter une politique visuelle séparée du parseur de texte.
2. Pour le profil Workshop Manual RCL0193ENG, rejeter un candidat raster entièrement contenu dans la zone d'en-tête utilisée par la géométrie de lecture (`7.5 %` de la hauteur de page).
3. Rejeter les petits raster décoratifs/pictogrammes réutilisés au moins 5 fois dans le même document, avec garde de petite taille/aire afin de ne pas supprimer une grande illustration technique réutilisée.
4. Pour les raster retenus, utiliser le `source_bbox` exact comme `crop_bbox` : marge raster = 0. La marge historique 24 pt / 32.4 pt ne doit plus être appliquée à ces images.
5. Conserver séparément `source_bbox` et `crop_bbox` dans les données, même s'ils sont égaux pour le raster exact.
6. Ne pas appliquer cette exclusion d'en-tête globalement aux bulletins techniques : certains bulletins possèdent de vraies illustrations près du haut de page.
7. Le fallback vectoriel n'est pas validé par ce pilote RCL0193ENG (les pages visuelles du manuel sont raster). Il reste à traiter/tester séparément avant le corpus complet.

### Tests obligatoires du pilote
- extraction complète RCL0193ENG seulement ;
- intégrité SQLite/FK du résultat d'extraction ;
- p.53 : l'icône ENGINE ne doit pas être créée comme visuel ;
- p.53 : exactement la vraie illustration mécanique doit rester ;
- son `source_bbox` doit être égal au `crop_bbox` ;
- le crop p.53 ne doit contenir ni texte voisin ni icône d'en-tête ;
- les pictogrammes répétitifs connus doivent être absents des visuels retenus ;
- les procédures/étapes texte de p.53 doivent rester inchangées, notamment opération `12.21.28`, contrôle aux cales et valeur `0.089 to 0.165 mm`.

Après le run, rapport immédiat avant toute autre pousse.
