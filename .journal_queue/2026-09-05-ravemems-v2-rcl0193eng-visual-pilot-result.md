## RAVEMEMS V2 — RESULTAT PILOTE VISUEL RCL0193ENG

### Pousses techniques testées
- `6d6204d88a7a6085488e3601b24ebb05340ad233` — ajout de `ravemems/v2/source_rebuild_visual_policy.py`.
- `d9cbf9c1e5e2adae8c2edfc44d3d992f9fc87181` — ajout du workflow pilote `.github/workflows/tmp-ravemems-v2-source-rebuild-rcl0193eng.yml`.
- Branche : `tmp-ravemems-v2-source-rebuild`.
- Base moteur : `4942f1e7bd11bddf3c0f4cf9bcc5cbe0b6e11c4d`.

### CI vérifiée
- Run : `33963572597`.
- Job : `101299594059`.
- Etat vérifié via GitHub : `completed` / `success`.
- Tous les steps, extraction, gate et upload artefact : SUCCESS.
- MEMSX64 protégé et inchangé sur #105 : `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`.

### Résultat sémantique
Le pilote visuel n'a pas modifié l'extraction des procédures :
- opérations : 201 ;
- phases : 397 ;
- étapes : 3104 ;
- défauts de séquence numérique : 0.

Régression p.53 validée :
- opération `12.21.28` conservée ;
- titre `CRANKSHAFT PRIMARY GEAR END-FLOAT` conservé ;
- méthode `Using feeler gauges...` conservée ;
- valeur `0.089 to 0.165 mm` conservée.

### Résultat visuel
- Ancienne extraction RCL0193ENG : 738 candidats visuels.
- Pilote source-rebuild : 527 candidats visuels.
- 211 petits pictogrammes raster répétitifs ont donc été écartés par la politique pilote.
- p.53 : le faux pictogramme ENGINE d'en-tête est rejeté.
- p.53 : la vraie illustration mécanique est désormais l'unique visuel de la page et porte `VIS_P0053_001`.
- p.53 : `source_bbox == crop_bbox` ; marge raster supplémentaire = 0 point.
- Tous les visuels raster conservés par ce pilote ont `source_bbox == crop_bbox` et aucun visuel entièrement dans la bande d'en-tête 7,5 % n'est conservé.
- Le fallback vectoriel historique page-wide est volontairement désactivé dans ce pilote ; son remplacement doit être conçu/testé séparément avant corpus complet.

### Intégrité
- SQLite d'extraction : `integrity_check=ok`.
- foreign_key_check : 0 problème.

IMPORTANT : le manifest générique produit encore `audit_issue_count=846`. Ce nombre ne doit pas être caché ni présenté comme zéro. Les gates structurels et la régression p.53 passent, mais le corpus RCL0193ENG n'est PAS encore déclaré sémantiquement final.

Les 527 visuels sont des **candidats réduits**, pas les 527 visuels finaux approuvés. Il reste une revue sémantique directe pour retirer les éléments décoratifs uniques résiduels et vérifier les associations visuel <-> procédure avant toute construction de SQLite finale/DLL.

### Artefact
- `RAVEMEMS-V2-SOURCE-REBUILD-RCL0193ENG-PILOT`
- Artifact ID : `9968709165`
- taille ZIP : 21,787,010 octets
- SHA256 ZIP : `d2c9d1a8ab2b29398a7c46438c1df9b1b1f8b882a3257f3c9f7679fb84b67200`
- contient notamment `pilot_review/RCL0193ENG-p53-primary-gear-end-float.png`.

### PROCHAINE ACTION EXACTE
Avant toute nouvelle pousse technique : télécharger/inspecter l'artefact exact du run et effectuer la revue sémantique directe des 527 candidats RCL0193ENG (contact sheets + métadonnées), afin de produire une couche de décisions accept/reject et d'association aux opérations/phases. Ne pas passer aux 40 autres PDF, ne pas construire la SQLite finale, ne pas construire la DLL, et ne pas lancer de BUILD #106 avant cette validation.
