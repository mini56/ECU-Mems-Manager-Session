# RAPPORT RCL0193FRE — EXTRACTION MAXIMALE

Date : 2026-08-29

Ce rapport complète `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` pour le chantier documentaire RCL0193FRE. Il est écrit avant toute nouvelle pousse de données/captures.

## Etat projet à préserver

- Production `MEMSX64` : BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48` — NE PAS MODIFIER.
- Branche documentaire active : `tmp-rave-visual-backfill`.
- HEAD vérifié avant ce chantier : `dfb5b8525b1d5685d1070914eb78c64f30aa7ff6`.
- Dernier lot documentaire présent : `research_enrichment_1780.qz64` (AKM6348 CH86).
- Tables historiques RAVE/expert : 93 / 105, à préserver strictement.
- Schéma additif 1730 : 11 tables connaissance/portée/spécification/procédure/alias, `PRAGMA user_version=20`.

## Source utilisateur

Fichier fourni directement par l'utilisateur : `Manuel Rover MPI.pdf`.

Identité vérifiée dans le document :
- publication `RCL0193FRE` ;
- manuel d'atelier Mini en français ;
- amendements visibles 3e/4e édition, avec page d'amendement indiquant le passage à la 5e édition ;
- 371 pages PDF ;
- couvre les modifications des Mini à partir du NIV `SAXXNNAZEBD 134455` ;
- doit être utilisé avec `AKM7169` et `RCL0194` ;
- Rover Technical Communication / Rover Group Limited 1999.

Le PDF possède une couche texte encodée avec des glyphes privés. Une conversion déterministe a été trouvée : les caractères privés U+F020..U+F0FF se décodent par `chr(0xF120-codepoint)`. La reconstruction des espaces par positions de glyphes permet de récupérer le texte français sans OCR. Aucun OCR approximatif n'est nécessaire.

## Cartographie complète des 371 pages

- Introduction : PDF 8-13
- Renseignements généraux : 14-27
- Information : 28-49
- Moteur : 50-95
- Contrôle de dépollution : 96-103
- Système de gestion moteur - MEMS : 104-135
- Système d'alimentation : 136-145
- Circuit de refroidissement : 146-159
- Collecteur et échappement : 160-175
- Embrayage : 176-191
- Boîte de vitesses manuelle : 192-201
- Arbres de transmission : 202-209
- Direction : 210-225
- Suspension : 226-233
- Freins : 234-255
- SRS : 256-279
- Carrosserie : 280-303
- Chauffage et aération : 304-313
- Essuie-glaces / lave-glaces : 314-323
- Equipement électrique : 324-357
- Instruments : 358-371

## Mesure documentaire complète

Extraction déterministe du texte des 371 pages :
- opérations de réparation constructeur détectées : **201** ;
- pages/occurrences d'avertissements, attentions et remarques réparties dans toutes les grandes sections ;
- le manuel contient également caractéristiques, couples de serrage, contenances/fluides, tableaux, dessins techniques, implantations, procédures de dépose/repose, outils spéciaux, consommables et contrôles.

Nombre d'opérations par grande section :
- Moteur 27
- Contrôle dépollution 5
- Gestion moteur MEMS 20
- Alimentation 7
- Refroidissement 8
- Collecteur/échappement 7
- Embrayage 7
- Boîte manuelle 6
- Arbres de transmission 3
- Direction 6
- Suspension 3
- Freins 14
- SRS 4
- Carrosserie 24
- Chauffage/aération 7
- Essuie/lave 6
- Equipement électrique 33
- Instruments 14

Total = 201.

## Point de portée essentiel — ne pas tout classer MPi

Le manuel RCL0193FRE contient explicitement dans la section Information :
- PDF 38 : `Modèle: SPi avec boîte de vitesses manuelle` ;
- PDF 39 : `Modèle: SPi avec boîte automatique` ;
- PDF 40 : `Modèle: MPi`.

Ces trois portées doivent rester séparées. Le fait que le manuel soit central pour la Mini MPi ne permet pas de réétiqueter les pages SPi en MEMS 1.9.

## Premier gros lot retenu — 1790

Objectif : commencer par le bloc le plus utile au projet ECU/MEMS tout en conservant toutes les preuves visuelles utiles.

Pages candidates :
- Information / réglages / couples / fluides : PDF 34-49 ;
- Système de gestion moteur MEMS : PDF 104-135 ;
- Système d'alimentation : PDF 136-145.

58 pages physiques dans la plage. Les pages réellement blanches/intercalaires seront exclues des assets. Les autres pages sont conservées comme captures constructeur françaises, y compris tableaux et illustrations.

Le lot doit intégrer :
- texte source français décodé page par page ;
- capture originale de chaque page utile ;
- portées exactes SPi manuel / SPi automatique / MPi / Mini à partir du NIV lorsque prouvées ;
- spécifications et valeurs structurées ;
- couples de serrage structurés ;
- procédures de réparation avec numéro constructeur et étapes ordonnées ;
- avertissements, attentions, remarques, outils spéciaux, pièces/joints à remplacer et contrôles ;
- relations connaissance -> page/capture ;
- alias français des composants ;
- aucune réécriture des 93/105 lignes historiques.

Valeurs déjà vérifiées visuellement/textuellement dans ce premier bloc incluent notamment :
- MPi PDF 40 : moteur 12A2LK70, 1275 cm3, ordre 1-3-4-2, rapport 10.0:1, ralenti ECM 900 ±50 tr/min, CO ralenti <0,4 %, avance nominale 12° avant PMH, jeu soupapes à froid 0,27-0,33 mm ;
- couples gestion moteur PDF 43 : ECT 15 N.m, MAP 6 N.m, IAT 7 N.m, TP 1,5 N.m, corps papillon 8 N.m, pédale accélérateur 25 N.m ;
- contenances PDF 48 : carburant 34,0 L, moteur/boîte avec filtre 4,8 L, boîte auto 5,1 L, refroidissement 4,0 L, lave-glace 2,3 L ; carburant sans plomb 95 RON minimum ;
- PDF 112 : deux injecteurs entre rampe pressurisée et collecteur d'admission ;
- PDF 115 : pompe électrique dans réservoir, commande ECM via relais, régulateur mécanique référencé à la dépression ;
- PDF 116 : interrupteur inertiel IFS et précaution de contrôle de fuite avant réarmement ;
- PDF 125 : écartement bougie 0,85 mm, serrage 25 N.m ;
- PDF 126 : bobine 10 N.m ;
- PDF 127 : ECM de remplacement nécessitant TestBook pour programmer le code antivol avant démarrage ;
- PDF 128 : IAT 7 N.m ;
- PDF 129 : ECT 15 N.m ;
- PDF 130 : CKP 6 N.m, fixation connecteur 3 N.m ;
- PDF 133 : HO2S, clé 22 mm, rondelle neuve, serrage 55 N.m ;
- PDF 138 : essai pression alimentation, outils 18G 1500-A / 18G 1500-5 / 18G 1500 ;
- PDF 140 : vidange réservoir avec avertissements inflammabilité/explosion/toxicité ;
- PDF 142 : pompe alimentation, joint neuf, écrous réservoir 9 N.m ;
- PDF 144 : jaugeur avec outil 18G 1467 et joints neufs ;
- PDF 145 : clapet deux voies/support 9 N.m.

Ces éléments seront intégrés uniquement avec leur page/capture correspondante ; les pages restantes seront ensuite traitées par lots successifs jusqu'à couverture complète du manuel.

## Prochaine action technique autorisée

Construire et valider `research_enrichment_1790.qz64` + captures RCL0193FRE du premier bloc sur `tmp-rave-visual-backfill`, mettre à jour `manifest.json`, produire un audit reproductible, reconstruire la SQLite complète et vérifier : `integrity_check=ok`, `user_version=20`, 93 faits RAVE historiques, 105 faits experts historiques, absence de clés orphelines, images présentes et portées SPi/MPi non mélangées.

Aucun #102, aucun changement protocole/ECU/RAM/UI/Qwen/ONNX/32 bits.
