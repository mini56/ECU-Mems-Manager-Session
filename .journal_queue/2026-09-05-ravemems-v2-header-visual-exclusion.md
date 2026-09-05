## RAVEMEMS V2 — décision d’extraction visuelle : exclure uniquement l’en-tête pour les images

Clarification utilisateur confirmée pendant l’audit de refondation RAVEMEMS V2.

Règle à appliquer :
- l’en-tête de page reste intégralement disponible pour l’extraction du texte, des pages, opérations, références et autres données documentaires ;
- l’exclusion concerne uniquement la détection/capture des visuels ;
- les images/objets graphiques situés dans la zone d’en-tête sont ignorés avant création des assets visuels, car cette zone contient les icônes/pictogrammes répétitifs inutiles ;
- les vraies illustrations techniques commencent sous l’en-tête selon le corpus RAVE observé ;
- la marge de capture doit être appliquée après cette exclusion et ne doit pas réintroduire la zone d’en-tête dans le crop final ;
- cette correction doit être réalisée dans RAVEMEMS V2 avant toute nouvelle extraction complète du corpus, puis validée sur les images extraites avant assemblage SQLite et reconstruction DLL.

Aucun changement MEMSX64 / aucun nouveau build production dans cette étape.
