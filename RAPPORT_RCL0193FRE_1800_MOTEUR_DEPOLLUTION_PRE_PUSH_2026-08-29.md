# RCL0193FRE — LOT 1800 MOTEUR + DEPOLLUTION — PRE-POUSSE

Date : 2026-08-29

## Etat à préserver

- Production `MEMSX64` : BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48` — ne pas modifier.
- Branche documentaire : `tmp-rave-visual-backfill`.
- Dernier lot validé : 1790, commit `a47aa37ebf5ca22bb577f6e79b584ff196884568`.
- Schéma additif : 11 tables, `PRAGMA user_version=20`.
- Historiques à préserver : 93 faits RAVE / 105 faits experts externes.

## Source

Même PDF utilisateur `RCL0193FRE` validé pour 1790 :
- 371 pages ;
- 67 009 217 octets ;
- SHA-256 `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.

Aucun OCR approximatif : texte issu de la couche texte du PDF décodée déterministement et pages contrôlées visuellement.

## Périmètre 1800

Sections :
- Moteur : PDF 50-95 ;
- Contrôle de dépollution : PDF 96-103.

Pages utiles conservées : 52. Les pages PDF 51 et 97 sont blanches et exclues.

Liste : `50,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,98,99,100,101,102,103`.

## Extraction structurée candidate validée localement

- 52 connaissances page complètes avec texte source intégral et lien vers capture constructeur ;
- 32 opérations de réparation constructeur ;
- 60 phases de procédures ;
- 699 étapes constructeur conservées dans leur ordre ;
- 81 exigences : avertissements, attentions, remarques, outils spéciaux et remplacements explicites ;
- 71 spécifications/valeurs structurées ;
- 36 relations de provenance/cross-check ;
- 22 nouveaux alias effectifs (24 proposés, deux déjà présents historiquement et donc ignorés sans duplication) ;
- 4 sous-thèmes dépollution supplémentaires pour recherche : émissions du carter, EVAP, convertisseur catalytique, implantation EVAP.

Les énoncés numériques génériques sont stockés avec le texte constructeur exact sans inventer un composant ou une sémantique qui ne seraient pas prouvés.

Exemples directement prouvés dans le bloc :
- jeu axial pignon primaire : `0,089 à 0,165 mm` ;
- rondelles de butée : `2,79 à 3,04 mm`, incréments `0,05 mm` ;
- jeu culbuteurs à froid : `0.30 mm` ;
- plaque butée arbre à cames : `11 N.m` ;
- poulie vilebrequin : `150 N.m` ;
- culasse : serrage progressif `34 N.m` puis final `68 N.m` dans l'ordre illustré ;
- le chapitre dépollution décrit la boucle de correction par sonde à oxygène chauffée / ECM / convertisseur catalytique ;
- soupape de purge EVAP commandée par ECM et maintenue fermée jusqu'à ce que la température moteur dépasse `70°C` ;
- avertissement constructeur contre le carburant plombé pour le convertisseur catalytique.

## Opérations constructeur détectées

32 opérations, dont : réglage jeu axial pignon primaire, réglage culbuteurs, plaque butée arbre à cames, poulie et joint avant vilebrequin, joint de culasse, couvre-culbuteurs, rampe culbuteurs, soupapes, moteur/boîte, silentblocs, barres d'appui, joints/carter volant, volant, filtre et pompe à huile, joint moteur/boîte, manocontact/soupape pression huile, distribution/tendeur, reniflard-séparateur, EVAP, soupape purge, convertisseur catalytique et bouclier thermique.

## Portée

Par prudence, ce lot est relié à la portée générale constructeur `SCOPE-RCL0193FRE-MINI-VIN134455` et **aucun numéro MEMS 1.6/1.9 n'est inventé**. Les pages peuvent mentionner ECM, ECT, CKP, EVAP etc., mais cela ne suffit pas à réétiqueter automatiquement toutes les opérations moteur en une famille MEMS précise.

## Captures

Les 52 pages sont préparées en rendu constructeur monochrome CCITT Group 4 **150 dpi**, 1240 x 1754 px, afin de mieux conserver dessins, légendes, tableaux et texte que le transport 120 dpi du lot précédent.

Transport visuel local :
- TIFF 52 pages SHA-256 `93aba01da598a2d9a610b8eee736baccbef0b0849ed5eb11dc5a64808cc2bf0a` ;
- archive XZ SHA-256 `7dd7ae07169fe969e0877de871cc701fdd5f580fedd528527433602986610035`.

QZ64 candidat :
- SHA-256 `743d27910f60760e35f5efb81232a67a8488fc919f998a86e8d0d521128da0af` ;
- SQL décodé SHA-256 `9168045f39f7e53d91b4eed079acc3fb5f79476bde0ecfe2f37ce98b2d0e9a53`.

## Validation locale complète

Reconstruction sur l'état final 1790 :
- `integrity_check=ok` ;
- `user_version=20` ;
- historiques 93/105 inchangés ;
- delta : +88 connaissances, +88 portées de connaissances, +71 specs, +71 valeurs, +60 procédures, +699 étapes, +81 exigences, +36 relations, +22 alias effectifs ;
- aucune connaissance 1800 sans portée ;
- aucune portée MEMS spécifique inventée.

## Prochaine action exacte

Pousser le lot additif `research_enrichment_1800.qz64`, les 52 captures constructeur, le manifest et l'audit sur `tmp-rave-visual-backfill`, valider la base reconstruite dans GitHub Actions, nettoyer tous les transports/workflows temporaires, puis consigner immédiatement la validation finale dans `RAPPORT`.
