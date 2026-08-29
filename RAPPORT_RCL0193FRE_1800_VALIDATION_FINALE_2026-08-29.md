# RCL0193FRE — LOT 1800 MOTEUR + DEPOLLUTION — VALIDATION FINALE

Date : 2026-08-29

## Résultat

Lot 1800 validé et poussé sur `tmp-rave-visual-backfill`.

- Commit final : `4dcba0d7e8f26ff2fae772ef734ef6521d0abc5e`
- Message : `Add RCL0193FRE motor emissions batch 1800`
- GitHub Actions : run `33270387415` — `TEMP RCL0193FRE 1800 FINAL INSTALL`
- Conclusion : **success**
- Production `MEMSX64` vérifiée pendant le run : BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, inchangée.

## Périmètre intégré

- Moteur : PDF 50-95
- Contrôle de dépollution : PDF 96-103
- 52 pages utiles ; pages blanches 51 et 97 exclues.

## Contenu final validé

- +88 connaissances : 52 pages complètes, 32 opérations constructeur, 4 sous-thèmes dépollution ;
- +71 spécifications et +71 valeurs ;
- +60 phases de procédures ;
- +699 étapes ;
- +81 exigences ;
- +36 relations ;
- +22 alias effectifs ;
- 52 captures constructeur 150 dpi avec dessins/légendes/texte conservés.

## Contrôles GitHub Actions

Tous verts :
- transport SHA exact ;
- QZ64 exact ;
- rendu 52/52 pages ;
- manifest mis à jour ;
- audit produit ;
- `PRAGMA integrity_check=ok` ;
- `PRAGMA user_version=20` ;
- 93 faits RAVE historiques préservés ;
- 105 faits experts externes préservés ;
- 88/88 connaissances 1800 avec portée ;
- aucune famille MEMS 1.6/1.9 inventée ;
- vérification distante après commit réussie ;
- workflow, installateur et transport temporaires supprimés de l'état final.

## Valeurs et informations notables conservées

- jeu axial pignon primaire : 0,089 à 0,165 mm ;
- rondelles de butée : 2,79 à 3,04 mm par incréments de 0,05 mm ;
- jeu culbuteurs : 0.30 mm ;
- plaque de butée arbre à cames : 11 N.m ;
- poulie vilebrequin : 150 N.m ;
- culasse : 34 N.m puis serrage final 68 N.m dans l'ordre illustré ;
- dépollution : ECM, sonde à oxygène chauffée, correction de richesse et convertisseur catalytique décrits par le constructeur ;
- EVAP : boîte charbon actif, soupape de purge commandée par ECM, purge inhibée jusqu'à température moteur supérieure à 70°C ;
- avertissement carburant plombé / détérioration catalyseur conservé.

## Prochaine action exacte

Continuer l'aspiration de `RCL0193FRE` sur les sections suivantes, toujours par lots additifs et sans toucher à `MEMSX64` : circuit de refroidissement PDF 146-159, collecteur/échappement PDF 160-175, puis embrayage et transmission.
