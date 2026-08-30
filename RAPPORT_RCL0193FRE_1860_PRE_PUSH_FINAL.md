# RCL0193FRE — lot 1860 — PRE-POUSSE FINAL

Date de reprise : 2026-08-30.

## Production gelée
- `MEMSX64` doit rester exactement sur BUILD #101 : `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Aucune modification production, protocole, UI, IA ou 32 bits dans ce lot.

## Branche documentaire
- Branche autorisée : `tmp-rave-visual-backfill`.
- Base avant 1860 : lot 1850 validé, HEAD `dfcab839a7c66c132f95559e524f3765326217b2`.

## Source
- Document : Rover Mini Workshop Manual `RCL0193FRE`, 5e édition, 1999.
- PDF source exact : 371 pages.
- SHA-256 source : `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.

## Périmètre 1860
- Equipement électrique + instruments, PDF 324–371.
- 45 pages constructeur utiles conservées.
- Pages blanches/intercalaires exclues : 325, 327, 359.
- Captures réelles constructeur, monochromes 150 dpi ; aucune image générée/redessinée.

## Contenu validé localement
- 112 connaissances + 112 portées.
- 18 spécifications + 18 valeurs.
- 50 opérations constructeur, structurées en 100 phases de procédure.
- 516 étapes ordonnées.
- 29 exigences / outils / avertissements.
- 79 relations.
- 75 tentatives d'alias dans le SQL ; 64 alias effectifs après `INSERT OR IGNORE`.
- `PRAGMA integrity_check = ok`.
- `PRAGMA user_version = 20`.
- Historiques attendus inchangés : 93 RAVE / 105 experts.

## Hashes candidats
- `research_enrichment_1860.sql` : `1cf7cd58e71d436fc701401d841f0a4b521c9f386ff23a5720d8fb7a5b86fd54`.
- `research_enrichment_1860.qz64` : `18e859c236a7e865b3416e0083f024e872bebda540c2dbf15ce233c0900c18be`.
- transport TIFF G4 xz : `fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08`.
- TIFF G4 décompressé : `d25fd347ccf9fd795c635fffff5a0dcc2800bbe33e185416c9a188d034b8f2c4`.

## Contrôles importants
- Courroie auxiliaire : 10 kg, flèche 6–8 mm, écrou poulie de tension 25 N·m.
- Réglage de phare : commutateur position 0, 1,4 % sous l’horizontale.
- Alternateur : poulie 25 N·m ; outil Rover 18G 1653.
- Démarreur : 37 N·m ; borne solénoïde 4 N·m.
- Accouplement tournant SRS : déconnexion batterie masse en premier, attente 10 min, ensemble non démontable.
- Sonde température d’huile : 60 N·m.
- Révisions constructeur 06/99 et 11/98 conservées avec leurs pages.

## Prochaine action exacte
1. Pousser uniquement le transport temporaire + workflow 1860 sur `tmp-rave-visual-backfill`.
2. GitHub Actions doit vérifier `MEMSX64` #101, les SHA du transport, reconstruire la SQLite complète, conserver 93/105, générer les 45 captures, valider les compteurs et l'intégrité.
3. Le workflow ne committe 1860 que si tous les contrôles passent puis supprime transport/workflow temporaire.
4. Après succès distant : rapport post-pousse immédiat.
5. Ensuite : audit final de couverture des 371 pages ; si trou réel, backfill 1870, sinon clôture RCL0193FRE.
