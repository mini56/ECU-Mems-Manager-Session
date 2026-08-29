# RCL0193FRE — LOT 1790 — VALIDATION FINALE

Date : 2026-08-29

## Etat final

Le lot documentaire `research_enrichment_1790.qz64` est validé et poussé sur `tmp-rave-visual-backfill`.

- Commit final : `a47aa37ebf5ca22bb577f6e79b584ff196884568`
- Message : `Add RCL0193FRE maximum extraction batch 1790`
- GitHub Actions final : run `33269999194` — `TEMP RCL0193FRE 1790 FINAL INSTALL`
- Conclusion : **success**
- Production `MEMSX64` vérifiée après pousse : BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, inchangée.

## Source

Manuel atelier Mini français `RCL0193FRE`, fichier utilisateur `Manuel Rover MPI.pdf`.

- 371 pages
- 67 009 217 octets
- SHA-256 source : `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`
- aucune extraction OCR approximative ; couche texte décodée déterministement puis contrôlée contre les pages constructeur.

## Périmètre 1790

52 pages constructeur retenues :
`34,35,36,37,38,39,40,42,43,44,45,46,48,49,104,106,107,108,109,110,111,112,113,114,115,116,117,118,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,138,140,141,142,143,144,145`.

Le lot couvre :
- réglages, caractéristiques, couples, contenances et lubrifiants ;
- système de gestion moteur MEMS ;
- capteurs/actionneurs associés ;
- alimentation carburant ;
- procédures constructeur, étapes, avertissements, outils et exigences ;
- captures originales des tableaux, dessins et pages de procédure.

Les portées SPi boîte manuelle, SPi boîte automatique et MPi sont séparées. Aucune génération MEMS n'a été inférée lorsque le constructeur ne l'indique pas.

## Comptages finaux contrôlés par GitHub Actions

- connaissances : 429
- spécifications : 350
- valeurs : 351
- procédures/phases : 51 pour 27 opérations constructeur
- étapes de procédures : 344
- exigences : 50
- relations : 379
- alias : 51
- captures constructeur : 52

## Validation base

Résultat final reproduit dans GitHub Actions :

- `PRAGMA integrity_check = ok`
- `PRAGMA user_version = 20`
- `mems_rave_fact = 93` préservés
- `mems_expert_fact_external = 105` préservés
- 52/52 assets présents et hashés
- QZ64 et SQL décodé conformes aux SHA attendus
- absence de connaissances RCL0193FRE sans portée
- portées PDF 38 SPi manuel / 39 SPi auto / 40 MPi contrôlées
- pages MEMS/alimentation 104-145 portées MPi dans ce lot lorsque la section le prouve

## Cas constructeur conservés sans arbitrage

- culasse : séquence `34 N.m` puis `34 N.m de plus` conservée telle qu'imprimée ;
- PDF 45 : le même libellé `Boulon d'ancrage supérieur de ceinture avant` apparaît avec 32 N.m et 30 N.m ; les deux valeurs restent enregistrées comme conflit constructeur à vérifier, aucune valeur n'est inventée ou supprimée.

## Nettoyage

Le workflow final a supprimé :
- le workflow temporaire d'installation ;
- le validateur temporaire ;
- les fichiers de transport temporaires.

L'état final de `tmp-rave-visual-backfill` ne contient donc que les données documentaires permanentes du lot 1790 : QZ64, manifest, audit et 52 captures RCL0193FRE.

## Prochaine action exacte

Continuer l'extraction maximale de `RCL0193FRE` avec le prochain lot additif, sans toucher à `MEMSX64`, en commençant par les sections **Moteur PDF 50-95** et **Contrôle de dépollution PDF 96-103**, avec conservation maximale des informations, tableaux, illustrations, procédures, valeurs, couples, outils, avertissements et portées exactes constructeur.
