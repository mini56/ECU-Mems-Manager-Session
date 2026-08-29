# RCL0193FRE - lot 1850 - PRE-POUSSE

Branche documentaire cible : `tmp-rave-visual-backfill`.
Production interdite : `MEMSX64` doit rester sur BUILD #101 / `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Périmètre exact

- Manuel constructeur français `RCL0193FRE`.
- PDF 280 a 323.
- Sections : carrosserie + chauffage/aération + essuie-glaces/lave-glaces.
- 39 pages utiles retenues ; pages blanches/intercalaires exclues.
- 37 opérations de réparation constructeur.
- Captures = vraies pages constructeur rendues à 150 dpi, aucune image générée ni redessinée.

## Validation locale candidate

- `PRAGMA integrity_check = ok`.
- `PRAGMA user_version = 20`.
- Historiques préservés : 93 `mems_rave_fact`, 105 `mems_expert_fact_external`.
- 79 connaissances.
- 15 spécifications / valeurs de couple utiles.
- 71 phases de procédure.
- 444 étapes.
- 5 exigences structurées.
- 41 relations.
- 14 alias français tentés.
- 39 captures constructeur.

## Points importants conservés

- Carrosserie : accessoires extérieurs, garnitures intérieures, pare-brise, sièges et ceintures.
- Tableau de bord : dépose/repose avec autoradio, aérateurs, câble d'indicateur de vitesse, réglage de niveau de phare, témoin d'alarme, faisceau principal, groupe d'instruments et compte-tours.
- Opération constructeur `76.73.31` : la section Carrosserie indique explicitement que la ceinture avant et le prétensionneur doivent être remplacés ensemble et renvoie à la section SRS. Relation `related_to` ajoutée vers le lot 1840 portant le même numéro d'opération.
- Chauffage/aération : câble et vanne de chauffage, commutateur, bloc chauffage, moteur/ventilateur, résistances, radiateur.
- Essuie-glaces/lave-glaces : tube, bras, moteur, boîtiers à engrenage et câble à crémaillère, commutateur, temporisateur.
- Les références au système MEMS dans les procédures d'essuie-glace (filtre à air, module de relais de gestion moteur, purge canister) sont conservées exactement dans le texte et les pages source, sans les transformer artificiellement en données protocole.
- Les dimensions de foret/rivet ne sont pas promues en spécifications véhicule ; elles restent dans les étapes de procédure.

## Hashes candidats

- SQL SHA-256 : `33d31d20a8693d6d35ae8b6c2bbe34cf11f75c175c899c529b5f1f20db02a3a6`.
- QZ64 SHA-256 : `f6995365e3d28de5d9c693d87d1399408edaa20c4f95bf839a557a17115b8662`.
- TIFF G4 150 dpi SHA-256 : `45281ba49806df725d4db980ed291461c6779dada7a153a3bed0a0b7559f21b4`.
- Archive TIFF XZ SHA-256 : `db9eddacab7a60d1241b3af7771490f406b5de4ea43f27600f574944e2848478`.

PROCHAINE ACTION EXACTE : pousser uniquement le lot documentaire 1850 sur `tmp-rave-visual-backfill`, valider via GitHub Actions, vérifier l'état distant et nettoyer les fichiers temporaires. Ensuite traiter le dernier bloc RCL0193FRE : équipement électrique + instruments PDF 324-371.
