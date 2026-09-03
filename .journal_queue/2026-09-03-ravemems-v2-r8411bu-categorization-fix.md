
## RAVEMEMS V2 — correction générique révélée par le test aléatoire R8411BU

État avant pousse corrective :
- PDF test : `rave/Mini Tech Bulletins/R8411BU.PDF`, blob `4948dca6152b13a0e19f8acc25362b33547276a3`, 2 pages.
- Run aléatoire `33796851792` : SUCCESS technique, audit final 0 après replay visuel.
- Défaut sémantique constaté : la date `23.12.98` a été interprétée comme une opération constructeur, avec le titre `Date:`.
- Défaut de catégorisation constaté : le document est enregistré `workshop_manual` alors que son chemin source est dans `Mini Tech Bulletins`.

Correction autorisée et limitée :
1. Rejeter génériquement comme numéro d’opération tout identifiant nu `dd.mm.yy` qui constitue une date calendrier plausible ; conserver les numéros présents dans un libellé explicite de type `Service Repair No` / `Repair Operation`.
2. Déduire génériquement `document_kind='technical_bulletin'` pour les sources dont le chemin appartient au dossier `Mini Tech Bulletins`; conserver `workshop_manual` par défaut.
3. Ne modifier ni la logique de lecture, ni les phases/étapes, ni le rendu visuel, ni les seuils d’audit.
4. Relancer R8411BU avec le même pipeline et exiger : 0 opération fantôme `23.12.98`, `document_kind=technical_bulletin`, intégrité SQLite OK, 0 FK, replay visuel et audit final à 0.

Production protégée : `MEMSX64` doit rester strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104.
